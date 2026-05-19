#include "arraygenerator.h"
#include <execution>

namespace SortBench {

// Helper function for Fisher-Yates shuffle on subset (used in NearlySorted)
template<typename T, typename URBG>
void partialFisherYates(std::vector<T>& vec, size_t swapCount, URBG& gen) {
    std::uniform_int_distribution<size_t> dist(0, vec.size() - 1);
    for (size_t i = 0; i < swapCount; ++i) {
        size_t idx1 = dist(gen);
        size_t idx2 = dist(gen);
        std::swap(vec[idx1], vec[idx2]);
    }
}

// Optimized equals using memcmp for trivially copyable types
template<typename T>
bool equalsOptimized(const std::vector<T>& v1, const std::vector<T>& v2) {
    if (v1.size() != v2.size()) return false;
    if constexpr (std::is_trivially_copyable_v<T>) {
        return std::memcmp(v1.data(), v2.data(), v1.size() * sizeof(T)) == 0;
    } else {
        return v1 == v2;
    }
}

} // namespace SortBench
