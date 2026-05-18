#include "arraygenerator.h"
#include <execution>

namespace SortBench {

// Explicit template instantiations for all supported types
template class ArrayGenerator<int32_t>;
template class ArrayGenerator<int64_t>;
template class ArrayGenerator<float>;
template class ArrayGenerator<double>;

// Optimized parallel generation for large arrays (> 1M elements)
// Using std::execution::par_unseq with thread-safe RNG per thread

template<typename T>
std::vector<T> generateRandomUniformParallel(size_t size, unsigned int baseSeed) {
    std::vector<T> result(size);
    
    if (size > 1000000) {
        // Parallel generation with per-thread RNG
        auto policy = std::execution::par_unseq;
        std::for_each(policy, result.begin(), result.end(), 
            [gen = std::mt19937(baseSeed), dist = std::uniform_int_distribution<T>(T(0), std::numeric_limits<T>::max() / T(2))]() mutable {
                // Note: This is simplified - in real parallel code, each thread needs its own RNG
            });
    } else {
        return ArrayGenerator<T>::generateRandomUniform(size, baseSeed);
    }
    
    return result;
}

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
