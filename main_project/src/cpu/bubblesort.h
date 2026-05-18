#ifndef BUBBLESORT_H
#define BUBBLESORT_H

#include <algorithm>
#include "cpusorter.h"

namespace SortBench {

template<typename T>
class BubbleSort {
public:
    static void sort(T* data, int size, CpuSorter::SortCallback callback, std::atomic<bool>& stop) {
        long long comparisons = 0;
        long long swaps = 0;
        
        for (int i = 0; i < size - 1 && !stop.load(); ++i) {
            bool swapped = false;
            
            for (int j = 0; j < size - i - 1 && !stop.load(); ++j) {
                // Сообщаем о сравнении
                ++comparisons;
                if (callback) {
                    callback(data, size, j, j + 1, HighlightType::Compare, comparisons, swaps);
                }
                
                if (data[j] > data[j + 1]) {
                    std::swap(data[j], data[j + 1]);
                    ++swaps;
                    swapped = true;
                    
                    // Сообщаем о перестановке
                    if (callback) {
                        callback(data, size, j, j + 1, HighlightType::Swap, comparisons, swaps);
                    }
                }
            }
            
            // Помечаем последний элемент прохода как отсортированный
            if (!stop.load() && callback) {
                callback(data, size, size - 1 - i, -1, HighlightType::Sorted, comparisons, swaps);
            }
            
            // Оптимизация: если не было перестановок, массив уже отсортирован
            if (!swapped) {
                break;
            }
        }
        
        // Помечаем весь массив как отсортированный
        if (!stop.load() && callback) {
            for (int i = 0; i < size; ++i) {
                callback(data, size, i, -1, HighlightType::Sorted, comparisons, swaps);
            }
        }
    }
};

} // namespace SortBench

#endif // BUBBLESORT_H
