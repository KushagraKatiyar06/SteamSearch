#include <algorithm>
#include <iostream>
#include <crow.h>
#include <nlohmann/json.hpp>
#include "CompactGame.h"
#include "GameJson.h"
#include "GameStore.h"
#include "Similarity.h"
#include "UrlUtils.h"

using json = nlohmann::json;

static crow::response jsonResponse(const json& body) {
    auto response = crow::response(body.dump());
    response.add_header("Access-Control-Allow-Origin", "*");
    response.add_header("Content-Type", "application/json; charset=utf-8");
    return response;
}

int main() {
    GameStore::loadData();
    crow::SimpleApp app;

    // Search Route
    CROW_ROUTE(app, "/search/<path>")
    ([&](std::string query) {
        query = urlDecode(query);
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);

        std::cout << "Searching for: [" << query << "]" << std::endl;

        json results = json::array();
        int count = 0;

        for (const auto& g : GameStore::games()) {
            std::string name = GameStore::getString(g.nameOffset);
            std::string nameLower = name;
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

            if (nameLower.find(query) != std::string::npos) {
                results.push_back({
                    {"id", g.id},
                    {"name", name},
                    {"imageURL", GameStore::getString(g.imageUrlOffset)}
                });
                if (++count >= 15) break;
            }
        }
        return jsonResponse(results);
    });

    // Balanced Recommendation
    CROW_ROUTE(app, "/recommend/global/<int>")
    ([&](int targetId) {
        const auto& allGames = GameStore::games();
        auto it = std::find_if(allGames.begin(), allGames.end(), [targetId](const CompactGame& g) {
            return g.id == targetId;
        });

        if (it == allGames.end()) return crow::response(404, "Game not found");
        const CompactGame& target = *it;

        std::vector<std::pair<float, int>> results;
        for (int i = 0; i < (int)allGames.size(); i++) {
            if (allGames[i].id == targetId) continue;

            float s_jac = getJaccard(target, allGames[i]);
            float s_min = getMinHash(target, allGames[i]);
            float s_cos = getCosine(target, allGames[i]);

            float globalScore = (s_cos * 0.5f) + (s_min * 0.3f) + (s_jac * 0.2f);
            if (globalScore > 0.15f) results.push_back({globalScore, i});
        }

        std::sort(results.rbegin(), results.rend());

        json res = json::array();
        for (int i = 0; i < std::min((int)results.size(), 90); i++) {
            res.push_back(gameToJson(allGames[results[i].second], results[i].first, "global_weighted"));
        }
        return jsonResponse(res);
    });

    // Specific Algorithms
    CROW_ROUTE(app, "/recommend/<string>/<int>")
    ([&](std::string type, int id) {
        const auto& allGames = GameStore::games();
        auto it = std::find_if(allGames.begin(), allGames.end(), [id](const CompactGame& g) {
            return g.id == id;
        });

        if (it == allGames.end()) return crow::response(404, "Game not found");
        const CompactGame& target = *it;

        std::vector<std::pair<float, int>> results;
        for (int i = 0; i < (int)allGames.size(); i++) {
            if (allGames[i].id == id) continue;

            float score = 0;
            if (type == "jaccard") score = getJaccard(target, allGames[i]);
            else if (type == "minhash") score = getMinHash(target, allGames[i]);
            else if (type == "cosine") score = getCosine(target, allGames[i]);

            if (score > 0.1) results.push_back({score, i});
        }

        std::sort(results.rbegin(), results.rend());

        json res = json::array();
        for (int i = 0; i < std::min((int)results.size(), 90); i++) {
            res.push_back(gameToJson(allGames[results[i].second], results[i].first));
        }
        return jsonResponse(res);
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
