#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <crow.h>
#include <nlohmann/json.hpp>
#include "CompactGame.h"

using json = nlohmann::json;

std::vector<CompactGame> globalGames;
std::vector<char> globalStringPool;

float roundToTwo(float val) {
    return std::round(val * 100.0f) / 100.0f;
}

const char* getString(uint32_t offset) {
    if (offset >= globalStringPool.size()) return "";
    return &globalStringPool[offset];
}

void loadData() {
    std::cout << "Attempting to load games.bin..." << std::endl;

    std::ifstream gFile("data/games.bin", std::ios::binary | std::ios::ate);

    // Ensure file opened
    if (!gFile.is_open()) {
        std::cerr << "CRITICAL ERROR: Could not open data/games.bin! Check if the file is in the 'data' folder." << std::endl;
        return;
    }

    std::streamsize gSize = gFile.tellg();

    // Ensure file is not empty or corrupted
    if (gSize <= 0) {
        std::cerr << "CRITICAL ERROR: games.bin is empty or tellg() failed." << std::endl;
        return;
    }

    gFile.seekg(0, std::ios::beg);

    // Explicitly print the size for the logs
    std::cout << "Found games.bin, size: " << gSize << " bytes. Allocating memory..." << std::endl;

    try {
        globalGames.resize(gSize / sizeof(CompactGame));
        gFile.read((char*)globalGames.data(), gSize);
    } catch (const std::bad_alloc& e) {
        std::cerr << "OUT OF MEMORY ERROR: Allocation failed during games.bin load." << std::endl;
        return;
    }

    // Repeat for strings.bin
    std::ifstream sFile("data/strings.bin", std::ios::binary | std::ios::ate);
    if (!sFile.is_open()) {
        std::cerr << "CRITICAL ERROR: data/strings.bin not found!" << std::endl;
        return;
    }

    std::cout << "Successfully loaded " << globalGames.size() << " games into RAM." << std::endl;
}

// Jaccard's Tag Similarity
float getJaccard(const CompactGame& a, const CompactGame& b) {
    int intersect = 0, unionSize = 0;
    for (int i = 0; i < 8; i++) {
        intersect += __builtin_popcount(a.tagBits[i] & b.tagBits[i]);
        unionSize += __builtin_popcount(a.tagBits[i] | b.tagBits[i]);
    }
    return unionSize == 0 ? 0 : (float)intersect / unionSize;
}

// MinHash
float getMinHash(const CompactGame& a, const CompactGame& b) {
    int matches = 0;
    for (int i = 0; i < 150; i++) {
        if (a.minHashSignature[i] == b.minHashSignature[i]) matches++;
    }
    return (float)matches / 150.0f;
}

// Weighted Cosine
float getCosine(const CompactGame& a, const CompactGame& b) {
    float dot = 0;
    for (int i = 0; i < 128; i++) dot += a.cosineSignature[i] * b.cosineSignature[i];
    return dot;
}


std::string urlDecode(std::string str) {
    std::string res;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            res += (char)std::strtol(str.substr(i + 1, 2).c_str(), nullptr, 16);
            i += 2;
        } else if (str[i] == '+') {
            res += ' ';
        } else {
            res += str[i];
        }
    }
    return res;
}

int main() {
    loadData();
    crow::SimpleApp app;

    // Search Route
    CROW_ROUTE(app, "/search/<path>")
    ([&](std::string query) {
        query = urlDecode(query);

        std::transform(query.begin(), query.end(), query.begin(), ::tolower);

        json results = json::array();
        int count = 0;

        for (const auto& g : globalGames) {
            std::string name = getString(g.nameOffset);
            std::string nameLower = name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

            if (nameLower.find(query) != std::string::npos) {
                // Fixed minHash array construction
                json mHash = json::array();
                for(int i = 0; i < 150; i++) mHash.push_back(g.minHashSignature[i]);

                results.push_back({
                    {"id", g.id},
                    {"name", name},
                    {"imageURL", getString(g.imageUrlOffset)},
                    {"tagBits", {g.tagBits[0], g.tagBits[1], g.tagBits[2], g.tagBits[3], g.tagBits[4], g.tagBits[5], g.tagBits[6], g.tagBits[7]}},
                    {"minHash", mHash}
                });
                if (++count >= 15) break;
            }
        }
        auto response = crow::response(results.dump());
        response.add_header("Access-Control-Allow-Origin", "*");
        response.add_header("Content-Type", "application/json; charset=utf-8");
        return response;
    });

    // Balanced Recommendation
    CROW_ROUTE(app, "/recommend/global/<int>")
    ([&](int targetId) {
        auto it = std::find_if(globalGames.begin(), globalGames.end(), [targetId](const CompactGame& g) {
            return g.id == targetId;
        });

        if (it == globalGames.end()) return crow::response(404, "Game not found");
        const CompactGame& target = *it;

        std::vector<std::pair<float, int>> results;
        for (int i = 0; i < (int)globalGames.size(); i++) {
            if (globalGames[i].id == targetId) continue;

            float s_jac = getJaccard(target, globalGames[i]);
            float s_min = getMinHash(target, globalGames[i]);
            float s_cos = getCosine(target, globalGames[i]);

            float globalScore = (s_cos * 0.5f) + (s_min * 0.3f) + (s_jac * 0.2f);
            if (globalScore > 0.15f) results.push_back({globalScore, i});
        }

        std::sort(results.rbegin(), results.rend());

        json res = json::array();
        for (int i = 0; i < std::min((int)results.size(), 90); i++) {
            const auto& g = globalGames[results[i].second];

            json mHash = json::array();
            for(int j = 0; j < 150; j++) mHash.push_back(g.minHashSignature[j]);

            res.push_back({
                {"id", g.id},
                {"name", getString(g.nameOffset)},
                {"score", roundToTwo(results[i].first)},
                {"imageURL", getString(g.imageUrlOffset)},
                {"price", roundToTwo(g.price)},
                {"algorithm", "global_weighted"},
                {"tagBits", {g.tagBits[0], g.tagBits[1], g.tagBits[2], g.tagBits[3], g.tagBits[4], g.tagBits[5], g.tagBits[6], g.tagBits[7]}},
                {"minHash", mHash}
            });
        }
        auto response = crow::response(res.dump());
        response.add_header("Access-Control-Allow-Origin", "*");
        response.add_header("Content-Type", "application/json; charset=utf-8");
        return response;
    });

    // Specific Algorithms
    CROW_ROUTE(app, "/recommend/<string>/<int>")
    ([&](std::string type, int id) {
        auto it = std::find_if(globalGames.begin(), globalGames.end(), [id](const CompactGame& g) {
            return g.id == id;
        });

        if (it == globalGames.end()) return crow::response(404, "Game not found");
        const CompactGame& target = *it;

        std::vector<std::pair<float, int>> results;
        for (int i = 0; i < (int)globalGames.size(); i++) {
            if (globalGames[i].id == id) continue;

            float score = 0;
            if (type == "jaccard") score = getJaccard(target, globalGames[i]);
            else if (type == "minhash") score = getMinHash(target, globalGames[i]);
            else if (type == "cosine") score = getCosine(target, globalGames[i]);

            if (score > 0.1) results.push_back({score, i});
        }

        std::sort(results.rbegin(), results.rend());

        json res = json::array();
        for (int i = 0; i < std::min((int)results.size(), 90); i++) {
            const auto& g = globalGames[results[i].second];

            json mHash = json::array();
            for(int j = 0; j < 150; j++) mHash.push_back(g.minHashSignature[j]);

            res.push_back({
                {"id", g.id},
                {"name", getString(g.nameOffset)},
                {"imageURL", getString(g.imageUrlOffset)},
                {"score", roundToTwo(results[i].first)},
                {"price", roundToTwo(g.price)},
                {"tagBits", {g.tagBits[0], g.tagBits[1], g.tagBits[2], g.tagBits[3], g.tagBits[4], g.tagBits[5], g.tagBits[6], g.tagBits[7]}},
                {"minHash", mHash}
            });
        }
        auto response = crow::response(res.dump());
        response.add_header("Access-Control-Allow-Origin", "*");
        response.add_header("Content-Type", "application/json; charset=utf-8");
        return response;
    });

    CROW_CATCHALL_ROUTE(app)
    ([](const crow::request& req, crow::response& res) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");

        if (req.method == crow::HTTPMethod::Options) {
            res.code = 200;
            res.end();
        } else {
            res.code = 404;
            res.end();
        }
    });

    const char* port = std::getenv("PORT");
    uint16_t portNum = port ? (uint16_t)std::stoi(port) : 8080;

    std::cout << "C++ Server online on port " << portNum << std::endl;
    app.port(portNum).multithreaded().run();
    return 0;
}