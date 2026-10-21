#include "GameStore.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

namespace {
    std::vector<CompactGame> globalGames;
    std::vector<char> globalStringPool;
}

namespace GameStore {

const std::vector<CompactGame>& games() {
    return globalGames;
}

const char* getString(uint32_t offset) {
    if (offset >= globalStringPool.size()) return "";
    return &globalStringPool[offset];
}

void loadData() {
    std::vector<std::string> possiblePaths = {"data/", "src/data/", "../src/data/"};
    std::string foundPath = "data/";

    // Determine which directory actually contains our data
    for (const auto& p : possiblePaths) {
        std::ifstream check(p + "games_1.bin");
        if (check.good()) {
            foundPath = p;
            break;
        }
    }

    std::vector<std::string> gameFiles = {foundPath + "games_1.bin", foundPath + "games_2.bin"};
    globalGames.clear();

    for (const auto& path : gameFiles) {
        std::ifstream gFile(path, std::ios::binary | std::ios::ate);

        if (!gFile.is_open()) {
            std::cerr << "ERROR: Could not find " << path << "!" << std::endl;
            continue;
        }

        std::streamsize gSize = gFile.tellg();
        gFile.seekg(0, std::ios::beg);

        size_t numGamesInFile = gSize / sizeof(CompactGame);
        size_t currentSize = globalGames.size();

        globalGames.resize(currentSize + numGamesInFile);
        gFile.read((char*)&globalGames[currentSize], gSize);
        gFile.close();

        std::cout << "Loaded " << numGamesInFile << " games from " << path << std::endl;
    }

    std::string stringsPath = foundPath + "strings.bin";
    std::ifstream sFile(stringsPath, std::ios::binary | std::ios::ate);

    if (!sFile.is_open()) {
        std::cerr << "ERROR: Could not find " << stringsPath << "!" << std::endl;
        return;
    }

    std::streamsize sSize = sFile.tellg();
    sFile.seekg(0, std::ios::beg);
    globalStringPool.resize(sSize);
    sFile.read(globalStringPool.data(), sSize);
    sFile.close();

    std::cout << "Successfully loaded total of " << globalGames.size() << " games into RAM." << std::endl;
    std::cout << "Successfully loaded " << sSize << " bytes into String Pool." << std::endl;

    std::cout << "--- Data Verification (First 5 Games) ---" << std::endl;
    for (int i = 0; i < std::min((int)globalGames.size(), 5); i++) {
        const char* name = getString(globalGames[i].nameOffset);
        std::cout << "Index " << i << " | ID: " << globalGames[i].id
                  << " | Name: [" << (name ? name : "NULL") << "]" << std::endl;
    }
    std::cout << "-----------------------------------------" << std::endl;
}

}
