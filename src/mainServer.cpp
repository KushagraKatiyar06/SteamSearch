#include <algorithm>
#include <iostream>
#include <thread>
#include <crow.h>
#include <nlohmann/json.hpp>
#include "CompactGame.h"
#include "GameJson.h"
#include "GameStore.h"
#include "ParallelTopK.h"
#include "Similarity.h"
#include "UrlUtils.h"

constexpr size_t kRecommendationLimit = 90;

// Crow's own worker pool (.multithreaded(), below) already sizes itself to
// hardware_concurrency() to handle concurrent requests, so a per-request
// scan can't also claim all cores without the two pools fighting each other
// under concurrent load. Capping at 4 keeps that contention bounded on
// larger machines; std::min against hardware_concurrency() matters more on
// a small/constrained host (e.g. a 1-2 vCPU container), where spawning
// threads the box has no cores for would just add overhead - on a single
// core it correctly falls back to 1, which runs with no extra threads at
// all (see parallelTopK).
unsigned scanThreads() {
    static const unsigned n = std::min(4u, std::max(1u, std::thread::hardware_concurrency()));
    return n;
}

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

        auto top = parallelTopK(allGames, target, (uint32_t)targetId, 0.15f, kRecommendationLimit,
            getGlobalWeighted, scanThreads());

        json res = json::array();
        for (const auto& [score, idx] : top) {
            res.push_back(gameToJson(allGames[idx], score, "global_weighted"));
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

        std::vector<std::pair<float, int>> top;
        if (type == "jaccard") {
            top = parallelTopK(allGames, target, (uint32_t)id, 0.1f, kRecommendationLimit, getJaccard, scanThreads());
        } else if (type == "minhash") {
            top = parallelTopK(allGames, target, (uint32_t)id, 0.1f, kRecommendationLimit, getMinHash, scanThreads());
        } else if (type == "cosine") {
            top = parallelTopK(allGames, target, (uint32_t)id, 0.1f, kRecommendationLimit, getCosine, scanThreads());
        }

        json res = json::array();
        for (const auto& [score, idx] : top) {
            res.push_back(gameToJson(allGames[idx], score));
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
