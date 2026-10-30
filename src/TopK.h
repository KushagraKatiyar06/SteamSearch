#ifndef STEAMSEARCH_TOPK_H
#define STEAMSEARCH_TOPK_H

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

// Keeps the k highest-scoring (score, index) pairs seen via offer(), without
// ever holding more than k elements at once. Equivalent to collecting every
// candidate that clears the inclusion threshold into a vector and sorting
// the whole thing, but avoids that vector (and the sort) growing far past k
// when the threshold isn't very selective.
class TopKScores {
public:
    explicit TopKScores(size_t k) : k_(k) { heap_.reserve(k); }

    void offer(float score, int index) {
        std::pair<float, int> candidate{score, index};
        if (heap_.size() < k_) {
            heap_.push_back(candidate);
            std::push_heap(heap_.begin(), heap_.end(), std::greater<>{});
        } else if (candidate > heap_.front()) {
            std::pop_heap(heap_.begin(), heap_.end(), std::greater<>{});
            heap_.back() = candidate;
            std::push_heap(heap_.begin(), heap_.end(), std::greater<>{});
        }
    }

    // Held elements, best (highest score, then highest index) first - same
    // ordering as sorting every candidate with std::sort(rbegin(), rend()).
    std::vector<std::pair<float, int>> extractSorted() {
        std::sort(heap_.begin(), heap_.end(), std::greater<>{});
        return std::move(heap_);
    }

private:
    size_t k_;
    std::vector<std::pair<float, int>> heap_;
};

#endif //STEAMSEARCH_TOPK_H
