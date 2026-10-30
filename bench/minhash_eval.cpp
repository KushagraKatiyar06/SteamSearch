// Evaluates getMinHash() against getJaccard() as a recall@k / ranking
// agreement check, plus the per-call speed of each.
//
// Important caveat (see Similarity.h / converter.cpp): getJaccard() is
// exact Jaccard, but only over the ~256 of 453 total tags that fit in
// CompactGame::tagBits (first 256 by tags.txt order) - it is NOT exact
// Jaccard over a game's full tag set. getMinHash() has no such cap; its
// signature is built from every tag the game has. So this is not "MinHash
// vs. true exact Jaccard" - it's "MinHash's full-tag-set estimate vs. the
// existing 256-tag-truncated Jaccard used in production". The original
// games.json this was converted from is no longer on disk, so a true
// full-set Jaccard isn't reconstructable from the shipped data; this is
// the best comparison available from it.
#include <algorithm>
#include <cstdio>
#include <unordered_set>
#include <vector>

#include "Benchmark.h"
#include "CompactGame.h"
#include "GameStore.h"
#include "Similarity.h"
#include "TopK.h"

namespace {

std::vector<std::pair<float, int>> topKBy(const std::vector<CompactGame>& games, int queryIdx,
    float (*scoreFn)(const CompactGame&, const CompactGame&), size_t k) {
    TopKScores topK(k);
    const CompactGame& query = games[queryIdx];
    for (int i = 0; i < (int)games.size(); i++) {
        if (i == queryIdx) continue;
        topK.offer(scoreFn(query, games[i]), i);
    }
    return topK.extractSorted();
}

double recallAtK(const std::vector<std::pair<float, int>>& reference, const std::vector<std::pair<float, int>>& candidate) {
    std::unordered_set<int> refIds;
    for (const auto& [score, idx] : reference) refIds.insert(idx);
    size_t hits = 0;
    for (const auto& [score, idx] : candidate) {
        if (refIds.count(idx)) hits++;
    }
    return reference.empty() ? 0.0 : (double)hits / (double)reference.size();
}

} // namespace

int main() {
    GameStore::loadData();
    const auto& games = GameStore::games();
    if (games.empty()) {
        std::fprintf(stderr, "No games loaded - run this from the repo root (needs data/games_*.bin).\n");
        return 1;
    }

    // tagBits-blind games: every one of their tags (if any) landed past the
    // 256-tag cutoff, so getJaccard() sees them as tagless against everyone.
    size_t tagBitsBlind = 0, trulyTagless = 0;
    for (const auto& g : games) {
        bool noTagBits = true;
        for (uint32_t w : g.tagBits) if (w != 0) { noTagBits = false; break; }
        if (noTagBits) {
            tagBitsBlind++;
            bool noMinHashTags = true;
            for (uint32_t m : g.minHashSignature) if (m != 0) { noMinHashTags = false; break; }
            if (noMinHashTags) trulyTagless++;
        }
    }
    std::printf("Games with zero tagBits (getJaccard sees them as tagless): %zu / %zu\n", tagBitsBlind, games.size());
    std::printf("  of those, truly tagless (zero MinHash tags too): %zu\n", trulyTagless);
    std::printf("  i.e. %zu games have real tags that are entirely invisible to getJaccard()\n\n", tagBitsBlind - trulyTagless);

    // Sample queries spread through the dataset, skipping tagBits-blind
    // games (getJaccard has no meaningful ranking for them - everything
    // scores 0).
    std::vector<int> queries;
    size_t stride = std::max((size_t)1, games.size() / 300);
    for (size_t i = 0; i < games.size() && queries.size() < 200; i += stride) {
        bool blind = true;
        for (uint32_t w : games[i].tagBits) if (w != 0) { blind = false; break; }
        if (!blind) queries.push_back((int)i);
    }
    std::printf("Evaluating recall@k over %zu query games\n\n", queries.size());

    for (size_t k : {10, 50, 90}) {
        double totalRecall = 0;
        for (int q : queries) {
            auto jaccardTop = topKBy(games, q, getJaccard, k);
            auto minHashTop = topKBy(games, q, getMinHash, k);
            totalRecall += recallAtK(jaccardTop, minHashTop);
        }
        std::printf("recall@%-3zu (MinHash top-%zu vs getJaccard top-%zu): %.1f%%\n",
            k, k, k, 100.0 * totalRecall / (double)queries.size());
    }

    std::printf("\nPer-call speed (same dataset, same machine):\n");
    const CompactGame& probe = games[games.size() / 2];
    volatile float sink = 0;
    auto jacStats = bench::measure(2000, 64, [&](size_t i) { sink += getJaccard(probe, games[i % games.size()]); });
    bench::printRow("getJaccard", jacStats);
    auto minStats = bench::measure(2000, 64, [&](size_t i) { sink += getMinHash(probe, games[i % games.size()]); });
    bench::printRow("getMinHash", minStats);
    std::printf("\n(sink: %f - ignore, here to stop the optimizer deleting the loop)\n", sink);

    return 0;
}
