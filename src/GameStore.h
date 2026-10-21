#ifndef STEAMSEARCH_GAMESTORE_H
#define STEAMSEARCH_GAMESTORE_H

#include <cstdint>
#include <vector>
#include "CompactGame.h"

// Owns the in-memory game table and string pool loaded from data/*.bin.
namespace GameStore {
    void loadData();
    const std::vector<CompactGame>& games();
    const char* getString(uint32_t offset);
}

#endif //STEAMSEARCH_GAMESTORE_H
