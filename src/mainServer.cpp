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
    std::ifstream gFile("data/games.bin", std::ios::binary | std::ios::ate);
    std::streamsize gSize = gFile.tellg();
    gFile.seekg(0, std::ios::beg);
    globalGames.resize(gSize / sizeof(CompactGame));
    gFile.read((char*)globalGames.data(), gSize);

    std::ifstream sFile("data/strings.bin", std::ios::binary | std::ios::ate);
    std::streamsize sSize = sFile.tellg();
    sFile.seekg(0, std::ios::beg);
    globalStringPool.resize(sSize);
    sFile.read(globalStringPool.data(), sSize);

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

int main() {
    loadData();
    crow::SimpleApp app;

    // Search Route
    CROW_ROUTE(app, "/search/<string>")
    ([&](std::string query) {
        json results = json::array();
        int count = 0;
        for (const auto& g : globalGames) {
            std::string gameName = getString(g.nameOffset);
            auto it = std::search(gameName.begin(), gameName.end(), query.begin(), query.end(),
                [](char a, char b) { return std::tolower(a) == std::tolower(b); });

            if (it != gameName.end()) {
                results.push_back({
                    {"id", g.id},
                    {"name", gameName},
                    {"imageURL", getString(g.imageUrlOffset)}
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
        for (int i = 0; i < std::min((int)results.size(), 10); i++) {
            const auto& g = globalGames[results[i].second];
            res.push_back({
                {"id", g.id},
                {"name", getString(g.nameOffset)},
                {"score", roundToTwo(results[i].first)},
                {"imageURL", getString(g.imageUrlOffset)},
                {"price", roundToTwo(g.price)},
                {"algorithm", "global_weighted"}
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
        for (int i = 0; i < std::min((int)results.size(), 10); i++) {
            const auto& g = globalGames[results[i].second];
            res.push_back({
                {"id", g.id},
                {"name", getString(g.nameOffset)},
                {"imageURL", getString(g.imageUrlOffset)},
                {"score", roundToTwo(results[i].first)},
                {"price", roundToTwo(g.price)}
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

    std::cout << "C++ Server online at http://localhost:8080" << std::endl;
    app.port(8080).multithreaded().run();
    return 0;
}