#ifndef ARRAYGENERATOR_H
#define ARRAYGENERATOR_H

#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstring>
#include <QString>
#include "sortparams.h"

namespace SortBench {

class ArrayGenerator {
public:
    // Основной метод диспатча
    template<typename T>
    static std::vector<T> generate(size_t size, Distribution dist, unsigned int seed) {
        switch (dist) {
            case Distribution::RandomUniform:
                return generateRandomUniform<T>(size, seed);
            case Distribution::NearlySorted:
                return generateNearlySorted<T>(size, seed);
            case Distribution::ReverseSorted:
                return generateReverseSorted<T>(size);
            case Distribution::ManyDuplicates:
                return generateManyDuplicates<T>(size, seed);
            case Distribution::Sawtooth:
                return generateSawtooth<T>(size);
            case Distribution::SteppedNoise:
                return generateSteppedNoise<T>(size, seed);
            case Distribution::RandomNormal:
                return generateRandomNormal<T>(size, seed);
            default:
                return generateRandomUniform<T>(size, seed);
        }
    }

    // Random Uniform: [0, max_value/2]
    template<typename T>
    static std::vector<T> generateRandomUniform(size_t size, unsigned int seed) {
        std::mt19937 gen(seed);
        std::vector<T> result(size);
        
        if constexpr (std::is_floating_point_v<T>) {
            std::uniform_real_distribution<T> dist(T(0), T(1000));
            std::generate(result.begin(), result.end(), [&]() { return dist(gen); });
        } else {
            constexpr T maxVal = std::numeric_limits<T>::max();
            std::uniform_int_distribution<T> dist(T(0), maxVal / T(2));
            std::generate(result.begin(), result.end(), [&]() { return dist(gen); });
        }
        
        return result;
    }

    // Nearly Sorted: sorted + ~2% swaps
    template<typename T>
    static std::vector<T> generateNearlySorted(size_t size, unsigned int seed) {
        std::vector<T> result = generateReverseSorted<T>(size);
        std::mt19937 gen(seed);
        
        size_t swapCount = std::max(size_t(1), size / 50);
        std::uniform_int_distribution<size_t> dist(0, size - 1);
        
        for (size_t i = 0; i < swapCount; ++i) {
            size_t idx1 = dist(gen);
            size_t idx2 = dist(gen);
            std::swap(result[idx1], result[idx2]);
        }
        
        return result;
    }

    // Reverse Sorted
    template<typename T>
    static std::vector<T> generateReverseSorted(size_t size) {
        std::vector<T> result(size);
        std::iota(result.begin(), result.end(), T(0));
        std::reverse(result.begin(), result.end());
        return result;
    }

    // Many Duplicates: pool of sqrt(size) unique values
    template<typename T>
    static std::vector<T> generateManyDuplicates(size_t size, unsigned int seed) {
        std::mt19937 gen(seed);
        size_t poolSize = std::max(size_t(1), static_cast<size_t>(std::sqrt(size)));
        
        std::vector<T> pool(poolSize);
        if constexpr (std::is_floating_point_v<T>) {
            std::uniform_real_distribution<T> dist(T(0), T(1000));
            for (size_t i = 0; i < poolSize; ++i) {
                pool[i] = dist(gen);
            }
        } else {
            std::uniform_int_distribution<T> dist(T(0), T(1000));
            for (size_t i = 0; i < poolSize; ++i) {
                pool[i] = dist(gen);
            }
        }
        
        std::vector<T> result(size);
        std::uniform_int_distribution<size_t> poolDist(0, poolSize - 1);
        for (size_t i = 0; i < size; ++i) {
            result[i] = pool[poolDist(gen)];
        }
        
        return result;
    }

    // Sawtooth: periodic pattern
    template<typename T>
    static std::vector<T> generateSawtooth(size_t size) {
        std::vector<T> result(size);
        size_t period = std::max(size_t(1), static_cast<size_t>(std::sqrt(size)));
        
        T maxVal;
        if constexpr (std::is_floating_point_v<T>) {
            maxVal = T(1000);
        } else {
            maxVal = std::numeric_limits<T>::max() / T(2);
        }
        
        T step = maxVal / T(period);
        for (size_t i = 0; i < size; ++i) {
            result[i] = T(i % period) * step;
        }
        
        return result;
    }

    // Stepped Noise: 10 steps with ±10% noise
    template<typename T>
    static std::vector<T> generateSteppedNoise(size_t size, unsigned int seed) {
        std::vector<T> result(size);
        std::mt19937 gen(seed);
        
        const int numSteps = 10;
        T maxVal;
        if constexpr (std::is_floating_point_v<T>) {
            maxVal = T(1000);
        } else {
            maxVal = std::numeric_limits<T>::max() / T(2);
        }
        
        T stepSize = maxVal / T(numSteps);
        std::uniform_real_distribution<double> noiseDist(-0.1, 0.1);
        
        for (size_t i = 0; i < size; ++i) {
            int stepIdx = static_cast<int>((i * numSteps) / size);
            T baseValue = T(stepIdx) * stepSize;
            T noise = T(noiseDist(gen)) * stepSize;
            result[i] = baseValue + noise;
        }
        
        return result;
    }

    // Random Normal: Box-Muller transform
    template<typename T>
    static std::vector<T> generateRandomNormal(size_t size, unsigned int seed, 
                                                double mean = 0.5, double stddev = 0.15) {
        std::mt19937 gen(seed);
        std::normal_distribution<double> dist(mean, stddev);
        std::vector<T> result(size);
        
        T minVal, maxVal;
        if constexpr (std::is_floating_point_v<T>) {
            minVal = T(0);
            maxVal = T(1000);
        } else {
            minVal = T(0);
            maxVal = std::numeric_limits<T>::max() / T(2);
        }
        
        for (size_t i = 0; i < size; ++i) {
            double val = dist(gen);
            val = std::clamp(val, 0.0, 1.0);
            result[i] = static_cast<T>(val * (maxVal - minVal) + minVal);
        }
        
        return result;
    }

    // Utility: check if sorted
    template<typename T>
    static bool isSorted(const std::vector<T>& vec) {
        if (vec.size() <= 1) return true;
        for (size_t i = 1; i < vec.size(); ++i) {
            if (vec[i-1] > vec[i]) return false;
        }
        return true;
    }

    // Utility: compare two vectors
    template<typename T>
    static bool equals(const std::vector<T>& v1, const std::vector<T>& v2) {
        if (v1.size() != v2.size()) return false;
        if constexpr (std::is_trivially_copyable_v<T>) {
            return std::memcmp(v1.data(), v2.data(), v1.size() * sizeof(T)) == 0;
        } else {
            return v1 == v2;
        }
    }

    // Utility: get min/max
    template<typename T>
    static std::pair<T, T> minMax(const std::vector<T>& vec) {
        if (vec.empty()) return {T(0), T(0)};
        auto [minIt, maxIt] = std::minmax_element(vec.begin(), vec.end());
        return {*minIt, *maxIt};
    }

    // Describe distribution for logging
    static QString describeDistribution(Distribution dist, size_t size) {
        switch (dist) {
            case Distribution::RandomUniform:
                return QString("Случайное равномерное (%1 элементов)").arg(size);
            case Distribution::NearlySorted:
                return QString("Почти отсортированное (%1 элементов, ~%2 инверсий)")
                    .arg(size).arg(std::max(size_t(1), size / 50));
            case Distribution::ReverseSorted:
                return QString("Обратно отсортированное (%1 элементов)").arg(size);
            case Distribution::ManyDuplicates:
                return QString("Много повторений (%1 элементов, ~%2 уникальных)")
                    .arg(size).arg(static_cast<size_t>(std::sqrt(size)));
            case Distribution::Sawtooth:
                return QString("Пилообразное (%1 элементов, период ~%2)")
                    .arg(size).arg(static_cast<size_t>(std::sqrt(size)));
            case Distribution::SteppedNoise:
                return QString("Ступенчатый шум (%1 элементов, 10 ступеней)").arg(size);
            case Distribution::RandomNormal:
                return QString("Нормальное распределение (%1 элементов, μ=0.5, σ=0.15)").arg(size);
            default:
                return QString("Неизвестное распределение (%1 элементов)").arg(size);
        }
    }
};




} // namespace SortBench

#endif // ARRAYGENERATOR_H
