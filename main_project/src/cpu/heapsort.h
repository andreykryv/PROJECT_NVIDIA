#ifndef HEAPSORT_H
#define HEAPSORT_H

#include <algorithm>
#include "cpusorter.h"

namespace SortBench {

template<typename T>
class HeapSort {
private:
    static void siftDown(T* data, int root, int end, 
                         CpuSorter::SortCallback callback, std::atomic<bool>& stop,
                         long long& comparisons, long long& swaps) {
        while (!stop.load()) {
            int left = 2 * root + 1;
            int right = 2 * root + 2;
            int largest = root;
            
            if (left <= end) {
                ++comparisons;
                if (callback) {
                    callback(data, end + 1, root, left, HighlightType::Compare, comparisons, swaps);
                }
                
                if (data[left] > data[largest]) {
                    largest = left;
                }
            }
            
            if (right <= end) {
                ++comparisons;
                if (callback) {
                    callback(data, end + 1, root, right, HighlightType::Compare, comparisons, swaps);
                }
                
                if (data[right] > data[largest]) {
                    largest = right;
                }
            }
            
            if (largest != root) {
                std::swap(data[root], data[largest]);
                ++swaps;
                
                if (callback) {
                    callback(data, end + 1, root, largest, HighlightType::Swap, comparisons, swaps);
                }
                
                root = largest;
            } else {
                break;
            }
        }
    }

public:
    static void sort(T* data, int size, CpuSorter::SortCallback callback, std::atomic<bool>& stop) {
        long long comparisons = 0;
        long long swaps = 0;
        
        if (size <= 1) return;
        
        // Фаза 1: построение max-heap (buildMaxHeap)
        for (int i = size / 2 - 1; i >= 0 && !stop.load(); --i) {
            siftDown(data, i, size - 1, callback, stop, comparisons, swaps);
        }
        
        // Фаза 2: извлечение элементов из heap
        for (int i = size - 1; i > 0 && !stop.load(); --i) {
            // Перемещаем корень (максимум) в конец
            std::swap(data[0], data[i]);
            ++swaps;
            
            if (callback) {
                callback(data, size, 0, i, HighlightType::Swap, comparisons, swaps);
                callback(data, size, i, -1, HighlightType::Sorted, comparisons, swaps);
            }
            
            // Восстанавливаем heap для оставшихся элементов
            siftDown(data, 0, i - 1, callback, stop, comparisons, swaps);
        }
        
        // Помечаем первый элемент как отсортированный
        if (!stop.load() && callback) {
            callback(data, size, 0, -1, HighlightType::Sorted, comparisons, swaps);
        }
    }
};

} // namespace SortBench

#endif // HEAPSORT_H
