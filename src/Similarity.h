#ifndef STEAMSEARCH_SIMILARITY_H
#define STEAMSEARCH_SIMILARITY_H

#include <cmath>
#include "CompactGame.h"

inline float roundToTwo(float val) {
    return std::round(val * 100.0f) / 100.0f;
}

// Jaccard's Tag Similarity
inline float getJaccard(const CompactGame& a, const CompactGame& b) {
    int intersect = 0, unionSize = 0;
    for (int i = 0; i < 8; i++) {
        intersect += __builtin_popcount(a.tagBits[i] & b.tagBits[i]);
        unionSize += __builtin_popcount(a.tagBits[i] | b.tagBits[i]);
    }
    return unionSize == 0 ? 0 : (float)intersect / unionSize;
}

// MinHash
inline float getMinHash(const CompactGame& a, const CompactGame& b) {
    int matches = 0;
    for (int i = 0; i < 150; i++) {
        if (a.minHashSignature[i] == b.minHashSignature[i]) matches++;
    }
    return (float)matches / 150.0f;
}

// Weighted Cosine
inline float getCosine(const CompactGame& a, const CompactGame& b) {
    float dot = 0;
    for (int i = 0; i < 128; i++) dot += a.cosineSignature[i] * b.cosineSignature[i];
    return dot;
}

#endif //STEAMSEARCH_SIMILARITY_H
