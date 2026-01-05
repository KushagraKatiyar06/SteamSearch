//
// Created by kkati on 1/5/2026.
//

#ifndef STEAMSEARCH_COMPACTGAME_H
#define STEAMSEARCH_COMPACTGAME_H


struct CompactGame {
    uint32_t id;
    float reviewScore;
    int16_t metacriticScore;
    float price;

    uint32_t minHashSignature[150];
    float cosineSignature[128];

    uint32_t nameOffset;
    uint32_t imageUrlOffset;
    uint32_t developerOffset;
    uint32_t publisherOffset;
    uint32_t genresOffset;
};


#endif //STEAMSEARCH_COMPACTGAME_H