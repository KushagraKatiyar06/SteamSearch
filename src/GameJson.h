#ifndef STEAMSEARCH_GAMEJSON_H
#define STEAMSEARCH_GAMEJSON_H

#include <nlohmann/json.hpp>
#include "CompactGame.h"
#include "GameStore.h"
#include "Similarity.h"

// Shared shape for a game entry in a /recommend response.
inline nlohmann::json gameToJson(const CompactGame& g, float score, const char* algorithm = nullptr) {
    using json = nlohmann::json;

    json minHash = json::array();
    for (int j = 0; j < 150; j++) minHash.push_back(g.minHashSignature[j]);

    json out = {
        {"id", g.id},
        {"name", GameStore::getString(g.nameOffset)},
        {"imageURL", GameStore::getString(g.imageUrlOffset)},
        {"score", roundToTwo(score)},
        {"price", roundToTwo(g.price)},
        {"tagBits", {g.tagBits[0], g.tagBits[1], g.tagBits[2], g.tagBits[3], g.tagBits[4], g.tagBits[5], g.tagBits[6], g.tagBits[7]}},
        {"minHash", minHash},
        {"developer", GameStore::getString(g.developerOffset)},
        {"publisher", GameStore::getString(g.publisherOffset)},
        {"genres", GameStore::getString(g.genresOffset)},
        {"reviewScore", g.reviewScore < 0 ? nullptr : json(roundToTwo(g.reviewScore * 100.0f))},
        {"metacriticScore", g.metacriticScore < 0 ? nullptr : json(g.metacriticScore)}
    };

    if (algorithm) out["algorithm"] = algorithm;
    return out;
}

#endif //STEAMSEARCH_GAMEJSON_H
