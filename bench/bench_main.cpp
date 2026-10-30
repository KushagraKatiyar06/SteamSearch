// Standalone benchmark harness for the recommendation query path.
// Build with -DBUILD_BENCHMARKS=ON, then run bench_runner from the repo root
// (it looks for data/ the same way GameStore::loadData() does).
#include <algorithm>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "Benchmark.h"
#include "CompactGame.h"
#include "GameStore.h"
#include "ParallelTopK.h"
#include "Similarity.h"
#include "TopK.h"

namespace {

// Pre-optimization /recommend/global scan: push every candidate that clears
// the threshold into a vector, then fully sort it. Kept here as the
// reference implementation the TopK version is checked against.
std::vector<std::pair<float, int>> recommendGlobalScanFullSort(const std::vector<CompactGame>& games, const CompactGame& target) {
    std::vector<std::pair<float, int>> results;
    results.reserve(256);

    for (int i = 0; i < (int)games.size(); i++) {
        if (games[i].id == target.id) continue;
        float globalScore = getGlobalWeighted(target, games[i]);
        if (globalScore > 0.15f) results.push_back({globalScore, i});
    }

    std::sort(results.rbegin(), results.rend());
    if (results.size() > 90) results.resize(90);
    return results;
}

// Current /recommend/global scan: bounded top-k heap, same as mainServer.cpp.
std::vector<std::pair<float, int>> recommendGlobalScanTopK(const std::vector<CompactGame>& games, const CompactGame& target) {
    TopKScores topK(90);
    for (int i = 0; i < (int)games.size(); i++) {
        if (games[i].id == target.id) continue;
        float globalScore = getGlobalWeighted(target, games[i]);
        if (globalScore > 0.15f) topK.offer(globalScore, i);
    }
    return topK.extractSorted();
}

// Same as recommendGlobalScanTopK, scan split across numThreads workers.
std::vector<std::pair<float, int>> recommendGlobalScanParallel(const std::vector<CompactGame>& games, const CompactGame& target, unsigned numThreads) {
    return parallelTopK(games, target, target.id, 0.15f, 90, getGlobalWeighted, numThreads);
}

} // namespace

int main() {
    GameStore::loadData();
    const auto& games = GameStore::games();
    if (games.empty()) {
        std::fprintf(stderr, "No games loaded - run this from the repo root (needs data/games_*.bin).\n");
        return 1;
    }

    std::printf("\nLoaded %zu games. Running benchmarks...\n\n", games.size());

    // Pairwise micro-benchmarks: hold one target, sweep the other operand
    // across the dataset so cache behavior looks like the real scan.
    const CompactGame& target = games[games.size() / 2];
    volatile float sink = 0.0f;

    auto jac = bench::measure(2000, 64, [&](size_t i) {
        sink += getJaccard(target, games[i % games.size()]);
    });
    bench::printRow("getJaccard", jac);

    auto minh = bench::measure(2000, 64, [&](size_t i) {
        sink += getMinHash(target, games[i % games.size()]);
    });
    bench::printRow("getMinHash", minh);

    auto cos = bench::measure(2000, 64, [&](size_t i) {
        sink += getCosine(target, games[i % games.size()]);
    });
    bench::printRow("getCosine", cos);

    std::printf("\n");

    // End-to-end: one full /recommend/global scan over the whole dataset.
    // Sample against several different target games spread through the
    // array so results aren't an artifact of one game's tag/score density.
    std::vector<size_t> targets;
    for (int k = 0; k < 10; k++) targets.push_back((games.size() / 10) * k);

    size_t resultSink = 0;
    auto fullSort = bench::measure(/*samples=*/30, /*batchSize=*/1, [&](size_t i) {
        resultSink += recommendGlobalScanFullSort(games, games[targets[i % targets.size()]]).size();
    });
    bench::printRow("recommend/global - full sort (old)", fullSort, "(one request's compute time)");

    auto topK = bench::measure(/*samples=*/30, /*batchSize=*/1, [&](size_t i) {
        resultSink += recommendGlobalScanTopK(games, games[targets[i % targets.size()]]).size();
    });
    bench::printRow("recommend/global - top-k heap (new)", topK, "(one request's compute time)");

    unsigned hwThreads = std::max(1u, std::thread::hardware_concurrency());
    for (unsigned n : {2u, 4u, std::min(8u, hwThreads), hwThreads}) {
        auto parallel = bench::measure(/*samples=*/30, /*batchSize=*/1, [&](size_t i) {
            resultSink += recommendGlobalScanParallel(games, games[targets[i % targets.size()]], n).size();
        });
        char label[64];
        std::snprintf(label, sizeof(label), "recommend/global - top-k, %u threads", n);
        bench::printRow(label, parallel, "(one request's compute time)");
    }

    std::printf("\nCorrectness check: top-k heap and parallel top-k match full-sort reference ");
    bool allMatch = true;
    for (size_t t : targets) {
        auto reference = recommendGlobalScanFullSort(games, games[t]);
        auto optimized = recommendGlobalScanTopK(games, games[t]);
        auto parallel = recommendGlobalScanParallel(games, games[t], 4);
        if (reference != optimized) {
            allMatch = false;
            std::printf("\n  TopK MISMATCH for target index %zu: reference has %zu results, optimized has %zu\n",
                t, reference.size(), optimized.size());
        }
        if (reference != parallel) {
            allMatch = false;
            std::printf("\n  Parallel MISMATCH for target index %zu: reference has %zu results, parallel has %zu\n",
                t, reference.size(), parallel.size());
            for (size_t i = 0; i < std::min(reference.size(), parallel.size()); i++) {
                if (reference[i] != parallel[i]) {
                    std::printf("    [%zu] ref=(%.9g,%d) parallel=(%.9g,%d)\n",
                        i, reference[i].first, reference[i].second, parallel[i].first, parallel[i].second);
                }
            }
        }
    }
    std::printf("%s\n", allMatch ? "[OK]" : "[FAILED]");

    std::printf("\n(sinks: %f %zu - ignore, here to stop the optimizer deleting the loops)\n",
        sink, resultSink);
    return allMatch ? 0 : 1;
}
