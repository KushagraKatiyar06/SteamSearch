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

// string interning technique
std::vector<char> stringPool;
std::unordered_map<std::string, uint32_t> stringCache;

// setupMinHash
std::unordered_map<std::string, int> tagToIndex;
std::vector<std::vector<int>> hashCombinations;

uint32_t addToPool(const std::string& str) {
    if (str.empty()) {
        return 0;
    }

    auto it = stringCache.find(str);
    if (it != stringCache.end()) {
        return it->second;
    }

    uint32_t offset = static_cast<uint32_t>(stringPool.size());
    for (char c: str) {
        stringPool.push_back(c);
    }
    stringPool.push_back('\0');

    stringCache[str] = offset;
    return offset;
}

void setupMinHash() {
    std::ifstream tagFile("data/tags.txt");
    std::string line;
    int tagCount = 0;

    while (std::getline(tagFile, line)) {
        if (!line.empty()) {
            tagToIndex[line] = tagCount++;
        }
    }

    std::vector<int> baseIndices(tagCount);
    for (int i = 0; i < tagCount; i++) {
        baseIndices[i] = i;
    }

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
    
    // Open two separate binary files for games
    std::ofstream out1("data/games_1.bin", std::ios::binary);
    std::ofstream out2("data/games_2.bin", std::ios::binary);
    
    int totalProcessed = 0;

    if (stringPool.empty()) stringPool.push_back('\0');

    for (auto& [id_str, info] : data.items()) {
        CompactGame cg = {};
        cg.id = std::stoul(id_str);

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

        auto gameTags = info.value("tags", json::object());
        std::vector<int> currentTagIndices;

        for (auto& [tagName, count] : gameTags.items()) {
            if (tagToIndex.count(tagName)) {
                int idx = tagToIndex[tagName];
                currentTagIndices.push_back(idx);

                if (idx < 256) {
                    cg.tagBits[idx / 32] |= (1U << (idx % 32));
                }

                uint32_t bucket = std::hash<int>{}(idx) % 128;
                cg.cosineSignature[bucket] += static_cast<float>(count.get<int>());
            }
        }

        for (int i = 0; i < 150; i++) {
            int minVal = 999999;
            for (int tagIdx : currentTagIndices) {
                if (hashCombinations[i][tagIdx] < minVal) minVal = hashCombinations[i][tagIdx];
            }
            cg.minHashSignature[i] = (currentTagIndices.empty()) ? 0 : minVal;
        }

        float sumSq = 0;
        for (int i = 0; i < 128; i++) sumSq += cg.cosineSignature[i] * cg.cosineSignature[i];
        if (sumSq > 0) {
            float invRoot = 1.0f / std::sqrt(sumSq);
            for (int i = 0; i < 128; i++) cg.cosineSignature[i] *= invRoot;
        }

        // 55,000 games
        if (totalProcessed < 55000) {
            out1.write(reinterpret_cast<const char*>(&cg), sizeof(CompactGame));
        } else {
            out2.write(reinterpret_cast<const char*>(&cg), sizeof(CompactGame));
        }

        totalProcessed++;
    }

    out1.close();
    out2.close();

    std::ofstream outStrings("data/strings.bin", std::ios::binary);
    outStrings.write(stringPool.data(), stringPool.size());
    outStrings.close();

    std::cout << "Successfully converted " << totalProcessed << " games." << std::endl;
    std::cout << "Data split into games_1.bin and games_2.bin" << std::endl;
}

void verifyConversion() {
    auto checkFile = [](std::string path) {
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if (!in.is_open()) {
            std::cout << "Failed to open " << path << std::endl;
            return;
        }
        std::streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);
        
        CompactGame test;
        in.read(reinterpret_cast<char*>(&test), sizeof(CompactGame));
        
        std::cout << "File: " << path << " | Size: " << size << " bytes | First Game ID: " << test.id <<  std::endl;
    };

    std::cout << "\n--- Verification ---" << std::endl;
    checkFile("data/games_1.bin");
    checkFile("data/games_2.bin");
}

int main() {
    //runConversion();
    verifyConversion();
    return 0;
}