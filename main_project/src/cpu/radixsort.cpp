#include "radixsort.h"
#include <vector>
#include <cstring>
#include <limits>

namespace SortBench {

template<typename T>
void RadixSort<T>::sort(T* data, int size, CpuSorter::SortCallback callback, std::atomic<bool>& stop) {
    if (size <= 1 || stop.load()) return;
    
    long long comparisons = 0;
    long long swaps = 0;
    
    // LSD Radix Sort: 8 бит за проход (4 прохода для int32)
    constexpr int BITS_PER_PASS = 8;
    constexpr int NUM_PASSES = (sizeof(T) * 8) / BITS_PER_PASS;
    constexpr int BUCKET_SIZE = 1 << BITS_PER_PASS; // 256
    
    std::vector<T> buffer(size);
    int count[BUCKET_SIZE];
    
    for (int pass = 0; pass < NUM_PASSES && !stop.load(); ++pass) {
        // Очистка счётчиков
        std::memset(count, 0, sizeof(count));
        
        // Подсчёт частот для текущего разряда
        T shift = pass * BITS_PER_PASS;
        T mask = static_cast<T>(BUCKET_SIZE - 1);
        
        for (int i = 0; i < size && !stop.load(); ++i) {
            int bucket = static_cast<int>((data[i] >> shift) & mask);
            count[bucket]++;
            
            if (callback && (i % 100 == 0)) {
                callback(data, size, i, -1, HighlightType::Compare, ++comparisons, swaps);
            }
        }
        
        // Prefix sum: превращаем count в начальные позиции
        int total = 0;
        for (int i = 0; i < BUCKET_SIZE && !stop.load(); ++i) {
            int tmp = count[i];
            count[i] = total;
            total += tmp;
        }
        
        // Копирование в буфер с сортировкой по текущему разряду
        for (int i = 0; i < size && !stop.load(); ++i) {
            int bucket = static_cast<int>((data[i] >> shift) & mask);
            buffer[count[bucket]++] = data[i];
            
            if (callback && (i % 100 == 0)) {
                callback(buffer.data(), size, i, -1, HighlightType::Swap, comparisons, ++swaps);
            }
        }
        
        // Копирование обратно
        std::memcpy(data, buffer.data(), size * sizeof(T));
        
        if (callback) {
            callback(data, size, -1, -1, HighlightType::None, comparisons, swaps);
        }
    }
    
    // Помечаем весь массив как отсортированный
    if (!stop.load() && callback) {
        for (int i = 0; i < size; ++i) {
            callback(data, size, i, -1, HighlightType::Sorted, comparisons, swaps);
        }
    }
}

// Явные инстанции шаблона для целочисленных типов
template class RadixSort<int32_t>;
template class RadixSort<int64_t>;
template class RadixSort<uint32_t>;
template class RadixSort<uint64_t>;

} // namespace SortBench
