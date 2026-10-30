#ifndef STEAMSEARCH_PARALLELTOPK_H
#define STEAMSEARCH_PARALLELTOPK_H

#include <algorithm>
#include <thread>
#include <vector>

#include "CompactGame.h"
#include "TopK.h"

// Scores every game in `games` against `target` via scoreFn(target, candidate),
// skipping any game whose id == excludeId (the target itself), and returns
// the k highest results that clear `threshold` - same contract as building
// a vector of every candidate past the threshold and sorting it, just split
// across `numThreads` worker threads with a TopKScores(k) per thread,
// merged at the end (merge cost is O(numThreads * k), negligible next to
// the scan).
template <typename ScoreFn>
std::vector<std::pair<float, int>> parallelTopK(
    const std::vector<CompactGame>& games,
    const CompactGame& target,
    uint32_t excludeId,
    float threshold,
    size_t k,
    ScoreFn&& scoreFn,
    unsigned numThreads)
{
    numThreads = std::max(1u, numThreads);
    numThreads = (unsigned)std::min((size_t)numThreads, std::max((size_t)1, games.size()));

    std::vector<TopKScores> perThread(numThreads, TopKScores(k));
    std::vector<std::thread> workers;
    workers.reserve(numThreads - 1);

    size_t chunk = (games.size() + numThreads - 1) / numThreads;
    auto worker = [&](size_t begin, size_t end, size_t slot) {
        TopKScores& local = perThread[slot];
        for (size_t i = begin; i < end; i++) {
            if (games[i].id == excludeId) continue;
            float score = scoreFn(target, games[i]);
            if (score > threshold) local.offer(score, (int)i);
        }
    };

    for (unsigned t = 1; t < numThreads; t++) {
        size_t begin = (size_t)t * chunk;
        size_t end = std::min(begin + chunk, games.size());
        if (begin < end) workers.emplace_back(worker, begin, end, t);
    }
    // Run slot 0 on the calling thread instead of spawning numThreads workers.
    worker(0, std::min(chunk, games.size()), 0);

    for (auto& w : workers) w.join();

    TopKScores merged(k);
    for (auto& tk : perThread) {
        for (const auto& [score, idx] : tk.extractSorted()) merged.offer(score, idx);
    }
    return merged.extractSorted();
}

#endif //STEAMSEARCH_PARALLELTOPK_H
