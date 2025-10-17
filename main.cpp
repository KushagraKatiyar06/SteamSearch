#include "crow.h"
#include "readJson.h"
#include "Game.h"
#include "jaccardsSimilarity.h"
#include "cosineSimilarity.h"
#include "minHash.h"
#include "algorithms_B.h"
#include "multiFeatureSimilarity.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <rapidfuzz/fuzz.hpp>
#include <algorithm>
#include <queue>
#include <unistd.h>
#include <chrono>
#include <stdexcept>
#include <set>

using json = nlohmann::json;
using namespace std;

unordered_map<string, Game> metaData;
unordered_map<string, int> indexedTags;
cosineSimilarity cosineSim(indexedTags);
minHash minHashObj(150, indexedTags);
algorithms_b decisionTree;
unordered_map<string, string> decoder;

void printLinks() {

    std::cout << "http://localhost:8081" << std::endl;

}

void setup_server_data() {

    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    std::cout << "Current working directory: " << cwd << std::endl;

    try {
        ifstream f("games.json");
        if (!f.is_open()) {
            throw runtime_error("Could not open games.json.");
        }
        json dataJSON = json::parse(f);
        readJson(dataJSON, metaData);

        string tagFile = "tags.txt";
        indexedTags = readTags(tagFile);

        cout << "Successfully loaded game data and tags." << endl;

        ifstream inFile("decoder.txt");
        string line;
        while (getline(inFile, line)) {
            istringstream iss(line);
            if (string key, value; getline(iss, key, '\t') && getline(iss, value)) {
                decoder[key] = value;
            }
        }

        cosineSim = cosineSimilarity(indexedTags);
        cosineSim.createGameSignatures(metaData);

        minHashObj = minHash(150, indexedTags);

    } catch (const exception& e) {
        cerr << "Error loading data: " << e.what() << endl;
    }
}

crow::response search_api(const crow::request& req) {
    string query = req.url_params.get("q");

    if (query.empty()) {
        return crow::response(200);
    }

    string query_lower = query;
    transform(query_lower.begin(), query_lower.end(), query_lower.begin(),
              [](unsigned char c){ return tolower(c); });

    vector<pair<double, string>> matches;

    for (const auto& pair : metaData) {
        string gameNameLower = pair.first;
        transform(gameNameLower.begin(), gameNameLower.end(), gameNameLower.begin(),
                  [](unsigned char c){ return tolower(c); });

        double score = rapidfuzz::fuzz::ratio(query_lower, gameNameLower);
        if (score > 60) {
            matches.push_back({score, pair.first});
        }
    }

    sort(matches.begin(), matches.end(), [](const auto& a, const auto& b) {
        return a.first > b.first;
    });

    crow::json::wvalue suggestionsJson;
    int count = 0;
    for (const auto& match : matches) {
        if (count >= 10) {
            break;
        }
        suggestionsJson[count] = match.second;
        count++;
    }

    return crow::response(suggestionsJson);
}

crow::json::wvalue getVisualizationData(const string& source, const string& compare, const string& algorithm) {
    crow::json::wvalue visData;
    if (algorithm == "jaccard" || algorithm == "weighted_jaccard") {
        const auto& tagsA = metaData[source].getTags();
        const auto& tagsB = metaData[compare].getTags();
        set<string> shared, unique1, unique2;
        for (const auto& tag_pair : tagsA) {
            if (tagsB.count(tag_pair.first)) {
                shared.insert(tag_pair.first);
            } else {
                unique1.insert(tag_pair.first);
            }
        }
        for (const auto& tag_pair : tagsB) {
            if (!tagsA.count(tag_pair.first)) {
                unique2.insert(tag_pair.first);
            }
        }

        crow::json::wvalue shared_arr, unique1_arr, unique2_arr;
        int count = 0;
        for (const auto& tag : shared) {
            shared_arr[count++] = tag;
        }
        count = 0;
        for (const auto& tag : unique1) {
            unique1_arr[count++] = tag;
        }
        count = 0;
        for (const auto& tag : unique2) {
            unique2_arr[count++] = tag;
        }

        visData["shared"] = std::move(shared_arr);
        visData["unique1"] = std::move(unique1_arr);
        visData["unique2"] = std::move(unique2_arr);
    } else if (algorithm == "cosine") {
        set<string> shared_tags;
        const auto& tagsA = metaData[source].getTags();
        const auto& tagsB = metaData[compare].getTags();
        for (const auto& tag_pair : tagsA) {
            if (tagsB.count(tag_pair.first)) {
                shared_tags.insert(tag_pair.first);
            }
        }
        crow::json::wvalue shared_tags_arr;
        int count = 0;
        for (const auto& tag : shared_tags) {
            shared_tags_arr[count++] = tag;
        }
        visData["shared_tags"] = std::move(shared_tags_arr);
    } else if (algorithm == "minhash") {
        minHash mh(150, indexedTags);
        vector<int> sourceSignature = mh.createSignature(metaData[source]);
        vector<int> compareSignature = mh.createSignature(metaData[compare]);
        vector<bool> matches_array;
        for (size_t i = 0; i < sourceSignature.size(); ++i) {
            matches_array.push_back(sourceSignature[i] == compareSignature[i]);
        }
        crow::json::wvalue matches_array_json;
        int count = 0;
        for (const auto& match : matches_array) {
            matches_array_json[count++] = match;
        }
        visData["matches_array"] = std::move(matches_array_json);
    } else if (algorithm == "multi_feature") {
        const Game& gameA = metaData[source];
        const Game& gameB = metaData[compare];
        visData["tags_score"] = calculateWeighted(gameA.getTags(), gameB.getTags());

        set<string> devsA, devsB;
        for(const auto& s : gameA.getDevelopers()) devsA.insert(s);
        for(const auto& s : gameB.getDevelopers()) devsB.insert(s);
        visData["developers_score"] = calculateJaccard(devsA, devsB);

        set<string> pubsA, pubsB;
        for(const auto& s : gameA.getPublisher()) pubsA.insert(s);
        for(const auto& s : gameB.getPublisher()) pubsB.insert(s);
        visData["publishers_score"] = calculateJaccard(pubsA, pubsB);

        visData["review_score"] = calculateReviewScoreSimilarity(gameA.getReviewScore(), gameB.getReviewScore());
    } else if (algorithm == "decision_tree") {
        visData["path"] = {"Shared Developer: Yes", "Shared Publisher: No", "Decision Tree Score: 0.85"};
    }
    return visData;
}

crow::response jaccard_api(const crow::request& req) {
    auto start_time = chrono::high_resolution_clock::now();
    string source = req.url_params.get("game");
    const int num_games_to_list = 51;

    if (source.empty() || metaData.find(source) == metaData.end()) {
        return crow::response(404, "Game not found.");
    }

    priority_queue<pair<double, string>> maxHeap;
    string compare;

    for (const auto& pair : metaData) {
        if (pair.first != source) {
            compare = pair.first;
            string non_const_source = source;
            string non_const_compare = compare;
            maxHeap.emplace(jaccardsSimilarity(non_const_source, non_const_compare, metaData), pair.first);
        }
    }

    crow::json::wvalue recommendationsJson;
    int count = 0;

    recommendationsJson["selectedGame"]["name"] = source;
    recommendationsJson["selectedGame"]["imageURL"] = metaData[source].getImageURL();

    while (!maxHeap.empty() && count < num_games_to_list) {
        auto top = maxHeap.top();
        maxHeap.pop();

        recommendationsJson["games"][count]["name"] = top.second;
        recommendationsJson["games"][count]["score"] = top.first;
        recommendationsJson["games"][count]["reviewScore"] = metaData[top.second].getReviewScore();
        recommendationsJson["games"][count]["imageURL"] = metaData[top.second].getImageURL();
        recommendationsJson["games"][count]["visualization_data"] = getVisualizationData(source, top.second, "jaccard");
        count++;
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    recommendationsJson["timing_ms"] = duration.count();

    return crow::response(recommendationsJson);
}

crow::response weighted_jaccard_api(const crow::request& req) {
    auto start_time = chrono::high_resolution_clock::now();
    string source = req.url_params.get("game");
    const int num_games_to_list = 50;

    if (source.empty() || metaData.find(source) == metaData.end()) {
        return crow::response(404, "Game not found.");
    }

    priority_queue<pair<double, string>> maxHeap;
    string compare;

    for (const auto& pair : metaData) {
        if (pair.first != source) {
            compare = pair.first;
            string non_const_source = source;
            string non_const_compare = compare;
            maxHeap.emplace(jaccardsSimilarityWeighted(non_const_source, non_const_compare, metaData), pair.first);
        }
    }

    crow::json::wvalue recommendationsJson;
    int count = 0;

    recommendationsJson["selectedGame"]["name"] = source;
    recommendationsJson["selectedGame"]["imageURL"] = metaData[source].getImageURL();

    while (!maxHeap.empty() && count < num_games_to_list) {
        auto top = maxHeap.top();
        maxHeap.pop();

        recommendationsJson["games"][count]["name"] = top.second;
        recommendationsJson["games"][count]["score"] = top.first;
        recommendationsJson["games"][count]["reviewScore"] = metaData[top.second].getReviewScore();
        recommendationsJson["games"][count]["imageURL"] = metaData[top.second].getImageURL();
        recommendationsJson["games"][count]["visualization_data"] = getVisualizationData(source, top.second, "weighted_jaccard");
        count++;
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    recommendationsJson["timing_ms"] = duration.count();

    return crow::response(recommendationsJson);
}

crow::response cosine_api(const crow::request& req) {
    auto start_time = chrono::high_resolution_clock::now();
    string source = req.url_params.get("game");
    const int num_games_to_list = 50;

    if (source.empty() || metaData.find(source) == metaData.end()) {
        return crow::response(404, "Game not found.");
    }

    priority_queue<pair<double, string>> maxHeap;
    string compare;

    for (const auto& pair : metaData) {
        if (pair.first != source) {
            compare = pair.first;
            maxHeap.emplace(cosineSim.similarity(source, compare), pair.first);
        }
    }

    crow::json::wvalue recommendationsJson;
    int count = 0;

    recommendationsJson["selectedGame"]["name"] = source;
    recommendationsJson["selectedGame"]["imageURL"] = metaData[source].getImageURL();

    while (!maxHeap.empty() && count < num_games_to_list) {
        auto top = maxHeap.top();
        maxHeap.pop();

        recommendationsJson["games"][count]["name"] = top.second;
        recommendationsJson["games"][count]["score"] = top.first;
        recommendationsJson["games"][count]["reviewScore"] = metaData[top.second].getReviewScore();
        recommendationsJson["games"][count]["imageURL"] = metaData[top.second].getImageURL();
        recommendationsJson["games"][count]["visualization_data"] = getVisualizationData(source, top.second, "cosine");
        count++;
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    recommendationsJson["timing_ms"] = duration.count();

    return crow::response(recommendationsJson);
}

crow::response minhash_api(const crow::request& req) {
    auto start_time = chrono::high_resolution_clock::now();
    string source = req.url_params.get("game");
    const int num_games_to_list = 50;

    if (source.empty() || metaData.find(source) == metaData.end()) {
        return crow::response(404, "Game not found.");
    }

    minHash mh(150, indexedTags);

    unordered_map<string, vector<int>> allSignatures;
    for (const auto& pair : metaData) {
        allSignatures[pair.first] = mh.createSignature(pair.second);
    }

    priority_queue<pair<double, string>> maxHeap;
    vector<int> sourceSignature = allSignatures[source];

    for (const auto& pair : allSignatures) {
        if (pair.first != source) {
            double score = mh.miniJaccards(sourceSignature, pair.second);
            maxHeap.emplace(score, pair.first);
        }
    }

    crow::json::wvalue recommendationsJson;
    int count = 0;

    recommendationsJson["selectedGame"]["name"] = source;
    recommendationsJson["selectedGame"]["imageURL"] = metaData[source].getImageURL();

    while (!maxHeap.empty() && count < num_games_to_list) {
        auto top = maxHeap.top();
        maxHeap.pop();

        recommendationsJson["games"][count]["name"] = top.second;
        recommendationsJson["games"][count]["score"] = top.first;
        recommendationsJson["games"][count]["reviewScore"] = metaData[top.second].getReviewScore();
        recommendationsJson["games"][count]["imageURL"] = metaData[top.second].getImageURL();
        recommendationsJson["games"][count]["visualization_data"] = getVisualizationData(source, top.second, "minhash");
        count++;
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    recommendationsJson["timing_ms"] = duration.count();

    return crow::response(recommendationsJson);
}

crow::response multi_feature_api(const crow::request& req) {
    auto start_time = chrono::high_resolution_clock::now();
    string source = req.url_params.get("game");
    const int num_games_to_list = 50;

    if (source.empty() || metaData.find(source) == metaData.end()) {
        return crow::response(404, "Game not found.");
    }

    priority_queue<pair<double, string>> topSimilarGames;
    double weightTags = 0.5;
    double weightPublishers = 0.1;
    double weightDevelopers = 0.1;
    double weightReviewScore = 0.3;

    Game& sourceGame = metaData[source];

    for (const auto& pair : metaData) {
        if (pair.first == source) { continue; }
        const Game& compareGame = pair.second;
        double similarity = calculateOverallWeightedSimilarity(sourceGame, compareGame, weightTags, weightPublishers, weightDevelopers, weightReviewScore);
        topSimilarGames.emplace(similarity, pair.first);
    }

    crow::json::wvalue recommendationsJson;
    int count = 0;

    recommendationsJson["selectedGame"]["name"] = source;
    recommendationsJson["selectedGame"]["imageURL"] = metaData[source].getImageURL();

    while (!topSimilarGames.empty() && count < num_games_to_list) {
        auto top = topSimilarGames.top();
        topSimilarGames.pop();

        recommendationsJson["games"][count]["name"] = top.second;
        recommendationsJson["games"][count]["score"] = top.first;
        recommendationsJson["games"][count]["reviewScore"] = metaData[top.second].getReviewScore();
        recommendationsJson["games"][count]["imageURL"] = metaData[top.second].getImageURL();
        recommendationsJson["games"][count]["visualization_data"] = getVisualizationData(source, top.second, "multi_feature");
        count++;
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    recommendationsJson["timing_ms"] = duration.count();

    return crow::response(recommendationsJson);
}

crow::response decision_tree_api(const crow::request& req) {
    auto start_time = chrono::high_resolution_clock::now();
    string source = req.url_params.get("game");
    const int num_games_to_list = 50;

    if (source.empty() || metaData.find(source) == metaData.end()) {
        return crow::response(404, "Game not found.");
    }

    vector<string> rankings = decisionTree.decisionTree(source, metaData, 500);

    crow::json::wvalue recommendationsJson;
    int count = 0;

    recommendationsJson["selectedGame"]["name"] = source;
    recommendationsJson["selectedGame"]["imageURL"] = metaData[source].getImageURL();

    for (const auto& gameName : rankings) {
        if (count >= num_games_to_list) {
            break;
        }
        if (metaData.find(gameName) == metaData.end()) {
            continue;
        }

        recommendationsJson["games"][count]["name"] = gameName;
        recommendationsJson["games"][count]["score"] = 0;
        recommendationsJson["games"][count]["reviewScore"] = metaData[gameName].getReviewScore();
        recommendationsJson["games"][count]["imageURL"] = metaData[gameName].getImageURL();
        recommendationsJson["games"][count]["visualization_data"] = getVisualizationData(source, gameName, "decision_tree");
        count++;
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time);
    recommendationsJson["timing_ms"] = duration.count();

    return crow::response(recommendationsJson);
}

int main() {
    setup_server_data();

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](){
        crow::response res;
        res.set_static_file_info("public/index.html");
        return res;
    });

    CROW_ROUTE(app, "/api/search")
      .methods("GET"_method)
      (search_api);

    CROW_ROUTE(app, "/api/jaccard")
      .methods("GET"_method)
      (jaccard_api);

    CROW_ROUTE(app, "/api/weighted_jaccard")
      .methods("GET"_method)
      (weighted_jaccard_api);

    CROW_ROUTE(app, "/api/cosine")
      .methods("GET"_method)
      (cosine_api);

    CROW_ROUTE(app, "/api/minhash")
      .methods("GET"_method)
      (minhash_api);

    CROW_ROUTE(app, "/api/multi_feature")
      .methods("GET"_method)
      (multi_feature_api);

    CROW_ROUTE(app, "/api/decision_tree")
      .methods("GET"_method)
      (decision_tree_api);

    printLinks();
    app.port(8081).multithreaded().run();
    return 0;
}