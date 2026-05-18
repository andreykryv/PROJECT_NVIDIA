#ifndef QUICKSORT_H
#define QUICKSORT_H

#include <algorithm>
#include "cpusorter.h"

namespace SortBench {

template<typename T>
class QuickSort {
private:
    static void insertionSort(T* data, int lo, int hi, CpuSorter::SortCallback callback, 
                              std::atomic<bool>& stop, long long& comparisons, long long& swaps) {
        for (int i = lo + 1; i <= hi && !stop.load(); ++i) {
            T key = data[i];
            int j = i - 1;
            
            while (j >= lo && !stop.load()) {
                ++comparisons;
                if (callback) {
                    callback(data, hi - lo + 1, j, j + 1, HighlightType::Compare, comparisons, swaps);
                }
                
                if (data[j] > key) {
                    data[j + 1] = data[j];
                    ++swaps;
                    --j;
                    
                    if (callback) {
                        callback(data, hi - lo + 1, j + 1, j + 2, HighlightType::Swap, comparisons, swaps);
                    }
                } else {
                    break;
                }
            }
            data[j + 1] = key;
        }
    }

    static void quicksort3Way(T* data, int lo, int hi, CpuSorter::SortCallback callback,
                              std::atomic<bool>& stop, long long& comparisons, long long& swaps) {
        if (lo >= hi || stop.load()) return;
        
        // Для малых подмассивов используем insertion sort
        if (hi - lo < 10) {
            insertionSort(data, lo, hi, callback, stop, comparisons, swaps);
            return;
        }
        
        // Выбор опорного элемента (медиана из трёх)
        int mid = lo + (hi - lo) / 2;
        int pivotIdx = lo;
        if (data[lo] > data[mid]) {
            if (data[hi] < data[mid]) pivotIdx = mid;
            else if (data[hi] < data[lo]) pivotIdx = hi;
        } else {
            if (data[hi] > data[lo]) pivotIdx = mid;
            else if (data[hi] > data[mid]) pivotIdx = hi;
        }
        
        T pivot = data[pivotIdx];
        
        // Сообщаем о выборе опорного элемента
        if (callback) {
            callback(data, hi - lo + 1, pivotIdx, -1, HighlightType::Pivot, comparisons, swaps);
        }
        
        // Перемещаем опорный элемент в начало
        if (pivotIdx != lo) {
            std::swap(data[lo], data[pivotIdx]);
            ++swaps;
            if (callback) {
                callback(data, hi - lo + 1, lo, pivotIdx, HighlightType::Swap, comparisons, swaps);
            }
        }
        
        // Трёхпутевое разделение (Dutch National Flag)
        int lt = lo, gt = hi;
        int i = lo + 1;
        
        while (i <= gt && !stop.load()) {
            ++comparisons;
            if (callback) {
                callback(data, hi - lo + 1, i, pivotIdx, HighlightType::Compare, comparisons, swaps);
            }
            
            if (data[i] < pivot) {
                if (i != lt) {
                    std::swap(data[i], data[lt]);
                    ++swaps;
                    if (callback) {
                        callback(data, hi - lo + 1, i, lt, HighlightType::Swap, comparisons, swaps);
                    }
                }
                ++lt;
                ++i;
            } else if (data[i] > pivot) {
                std::swap(data[i], data[gt]);
                ++swaps;
                --gt;
                if (callback) {
                    callback(data, hi - lo + 1, i, gt, HighlightType::Swap, comparisons, swaps);
                }
            } else {
                ++i;
            }
        }
        
        // Рекурсия на меньших подмассивах сначала (для оптимизации стека)
        if (lt - lo - 1 < hi - gt - 1) {
            quicksort3Way(data, lo, lt - 1, callback, stop, comparisons, swaps);
            quicksort3Way(data, gt + 1, hi, callback, stop, comparisons, swaps);
        } else {
            quicksort3Way(data, gt + 1, hi, callback, stop, comparisons, swaps);
            quicksort3Way(data, lo, lt - 1, callback, stop, comparisons, swaps);
        }
    }

public:
    static void sort(T* data, int size, CpuSorter::SortCallback callback, std::atomic<bool>& stop) {
        long long comparisons = 0;
        long long swaps = 0;
        
        if (size <= 1) return;
        
        quicksort3Way(data, 0, size - 1, callback, stop, comparisons, swaps);
        
        // Помечаем весь массив как отсортированный
        if (!stop.load() && callback) {
            for (int i = 0; i < size; ++i) {
                callback(data, size, i, -1, HighlightType::Sorted, comparisons, swaps);
            }
        }
    }
};

} // namespace SortBench

#endif // QUICKSORT_H
