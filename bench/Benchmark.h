#ifndef STEAMSEARCH_BENCHMARK_H
#define STEAMSEARCH_BENCHMARK_H

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

namespace bench {

struct Stats {
    double minNs, p50Ns, p99Ns, maxNs, meanNs;
};

inline Stats computeStats(std::vector<double>& samplesNs) {
    std::sort(samplesNs.begin(), samplesNs.end());
    const size_t n = samplesNs.size();
    double sum = 0;
    for (double v : samplesNs) sum += v;

    Stats s{};
    s.minNs = samplesNs.front();
    s.maxNs = samplesNs.back();
    s.meanNs = sum / (double)n;
    s.p50Ns = samplesNs[(size_t)(0.50 * (double)(n - 1))];
    s.p99Ns = samplesNs[(size_t)(0.99 * (double)(n - 1))];
    return s;
}

inline const char* unitFor(double ns) { return ns >= 1000.0 ? "us" : "ns"; }
inline double scaled(double ns) { return ns >= 1000.0 ? ns / 1000.0 : ns; }

inline void printRow(const std::string& name, const Stats& s, const char* extra = "") {
    std::printf("%-28s  p50 %8.2f %-2s   p99 %8.2f %-2s   mean %8.2f %-2s   min %8.2f %-2s   max %8.2f %-2s  %s\n",
        name.c_str(),
        scaled(s.p50Ns), unitFor(s.p50Ns),
        scaled(s.p99Ns), unitFor(s.p99Ns),
        scaled(s.meanNs), unitFor(s.meanNs),
        scaled(s.minNs), unitFor(s.minNs),
        scaled(s.maxNs), unitFor(s.maxNs),
        extra);
}

// Runs `fn(i)` `batchSize` times per sample and records elapsed/batchSize,
// for `samples` samples. Batching keeps steady_clock::now() overhead from
// swamping single sub-100ns calls while still letting outliers (cache
// misses, scheduler jitter) show up in the percentiles.
template <typename Fn>
Stats measure(size_t samples, size_t batchSize, Fn&& fn) {
    std::vector<double> ns;
    ns.reserve(samples);
    size_t counter = 0;

    for (size_t s = 0; s < samples; s++) {
        auto start = std::chrono::steady_clock::now();
        for (size_t b = 0; b < batchSize; b++) {
            fn(counter++);
        }
        auto end = std::chrono::steady_clock::now();
        double elapsedNs = std::chrono::duration<double, std::nano>(end - start).count();
        ns.push_back(elapsedNs / (double)batchSize);
    }
    return computeStats(ns);
}

} // namespace bench

#endif //STEAMSEARCH_BENCHMARK_H
