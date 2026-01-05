//
// Created by kkati on 1/5/2026.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <random>
#include <nlohmann/json.hpp>
#include "CompactGame.h"

using json = nlohmann::json;

// string interning technique -> reduces ram overhead from 2gb to less than 50mb

// string pool contains unique strings from game data... stringCache converts it to a numerical value

//addToPool
std::vector<char> stringPool;
std::unordered_map<std::string, uint32_t> stringCache;

//setupMinHash
std::unordered_map<std::string, int> tagToIndex;
std::vector<std::vector<int>> hashCombinations;

uint32_t addToPool(const std::string& str) {
    if (str.empty()) {
        return 0;
    }

    // if a string is not already in the map, we take second element of the iterator (key -> value)
    auto it = stringCache.find(str);
    if (it != stringCache.end()) {
        return it->second;
    }

    // the offset is the current size to keep track of where the new strings start in the continuous memory
    uint32_t offset = static_cast<uint32_t>(stringPool.size());
    for (char c: str) {
        stringPool.push_back(c);
    }
    stringPool.push_back('\0');

    stringCache[str] = offset;
    return offset;
}

// creates signatures for minhash algorithm
void setupMinHash() {
    std::ifstream tagFile("data/tags.txt");
    std::string line;
    int tagCount = 0;

    while (std::getline(tagFile, line)) {
        if (!line.empty()) { // Fixed: process if NOT empty
            tagToIndex[line] = tagCount++;
        }
    }

    //  base indices
    std::vector<int> baseIndices(tagCount);
    for (int i = 0; i < tagCount; i++) {
        baseIndices[i] = i;
    }

    // Shuffle the base indices 150 times
    std::mt19937 g(42);
    for (int i = 0; i < 150; i++) {
        std::vector<int> shuffled = baseIndices;
        std::shuffle(shuffled.begin(), shuffled.end(), g);
        hashCombinations.push_back(shuffled);
    }
}

void runConversion() {
    setupMinHash();

    std::ifstream f("data/games.json");
    json data = json::parse(f);
    std::vector<CompactGame> games;

    if (stringPool.empty()) stringPool.push_back('\0');

    for (auto& [id_str, info] : data.items()) {
        CompactGame cg = {};
        cg.id = std::stoi(id_str);

        int pos = info.value("positive", 0);
        int neg = info.value("negative", 0);
        cg.reviewScore = (pos + neg == 0) ? -1.0f : (float)pos / (pos + neg);
        cg.price = info.value("price", 0.0f);
        cg.metacriticScore = info.value("metacritic_score", -1);

        cg.nameOffset = addToPool(info.value("name", ""));
        cg.imageUrlOffset = addToPool(info.value("header_image", ""));


        auto devs = info.value("developer", json::array());
        cg.developerOffset = addToPool(devs.empty() ? "" : devs[0].get<std::string>());

        auto pubs = info.value("publisher", json::array());
        cg.publisherOffset = addToPool(pubs.empty() ? "" : pubs[0].get<std::string>());

        auto genres = info.value("genres", json::array());
        std::string genreStr = "";
        for(size_t i = 0; i < genres.size(); ++i) {
            genreStr += genres[i].get<std::string>() + (i == genres.size() - 1 ? "" : ",");
        }
        cg.genresOffset = addToPool(genreStr);



        //  MinHash & Cosine Signature Generation
        auto gameTags = info.value("tags", json::object());
        std::vector<int> currentTagIndices;

        for (auto& [tagName, count] : gameTags.items()) {
            if (tagToIndex.count(tagName)) {
                int idx = tagToIndex[tagName];
                currentTagIndices.push_back(idx);

                uint32_t bucket = std::hash<int>{}(idx) % 128;
                cg.cosineSignature[bucket] += static_cast<float>(count.get<int>());
            }
        }

        for (int i = 0; i < 150; i++) {
            int minVal = 999999;
            for (int tagIdx : currentTagIndices) {
                if (hashCombinations[i][tagIdx] < minVal) {
                    minVal = hashCombinations[i][tagIdx];
                }
            }
            cg.minHashSignature[i] = (currentTagIndices.empty()) ? 0 : minVal;
        }

        float sumSq = 0;
        for (int i = 0; i < 128; i++) sumSq += cg.cosineSignature[i] * cg.cosineSignature[i];
        if (sumSq > 0) {
            float invRoot = 1.0f / std::sqrt(sumSq);
            for (int i = 0; i < 128; i++) cg.cosineSignature[i] *= invRoot;
        }

        games.push_back(cg);
    }

    //  Final Binary Export
    std::ofstream outGames("data/games.bin", std::ios::binary);
    outGames.write(reinterpret_cast<const char*>(games.data()), games.size() * sizeof(CompactGame));

    std::ofstream outStrings("/data/strings.bin", std::ios::binary);
    outStrings.write(stringPool.data(), stringPool.size());

    std::cout << "Successfully converted " << games.size() << " games." << std::endl;
}

void verifyConversion() {
    std::ifstream in("data/games.bin", std::ios::binary);
    CompactGame testGame;

    in.read(reinterpret_cast<char*>(&testGame), sizeof(CompactGame));

    std::cout << "\n--- Verification ---" << std::endl;
    std::cout << "ID: " << testGame.id << std::endl;
    std::cout << "Review Score: " << testGame.reviewScore << std::endl;
    std::cout << "Price: " << testGame.price << std::endl;

}

int main() {
    //runConversion();
    verifyConversion();
    return 0;
}



