#ifndef MERGESORT_H
#define MERGESORT_H

#include <vector>
#include "cpusorter.h"

namespace SortBench {

template<typename T>
class MergeSort {
private:
    static void merge(T* data, T* tmp, int lo, int mid, int hi, 
                      CpuSorter::SortCallback callback, std::atomic<bool>& stop,
                      long long& comparisons, long long& swaps) {
        if (stop.load()) return;
        
        int i = lo, j = mid + 1, k = lo;
        
        // Слияние двух отсортированных половин
        while (i <= mid && j <= hi && !stop.load()) {
            ++comparisons;
            if (callback) {
                callback(data, hi - lo + 1, i, j, HighlightType::Compare, comparisons, swaps);
            }
            
            if (data[i] <= data[j]) {
                tmp[k++] = data[i++];
            } else {
                tmp[k++] = data[j++];
                ++swaps;  // Инверсия обнаружена
            }
            
            if (callback) {
                callback(data, hi - lo + 1, k - 1, -1, HighlightType::Swap, comparisons, swaps);
            }
        }
        
        // Копирование оставшихся элементов левой части
        while (i <= mid && !stop.load()) {
            tmp[k++] = data[i++];
            if (callback) {
                callback(data, hi - lo + 1, k - 1, -1, HighlightType::Swap, comparisons, swaps);
            }
        }
        
        // Копирование оставшихся элементов правой части
        while (j <= hi && !stop.load()) {
            tmp[k++] = data[j++];
            if (callback) {
                callback(data, hi - lo + 1, k - 1, -1, HighlightType::Swap, comparisons, swaps);
            }
        }
        
        // Копирование результата обратно в исходный массив
        for (int idx = lo; idx <= hi && !stop.load(); ++idx) {
            data[idx] = tmp[idx];
            if (callback) {
                callback(data, hi - lo + 1, idx, -1, HighlightType::Compare, comparisons, swaps);
            }
        }
    }

public:
    static void sort(T* data, int size, CpuSorter::SortCallback callback, std::atomic<bool>& stop) {
        long long comparisons = 0;
        long long swaps = 0;
        
        if (size <= 1) return;
        
        // Выделяем временный буфер один раз
        std::vector<T> tmp(size);
        
        // Итеративный bottom-up merge sort
        for (int width = 1; width < size && !stop.load(); width *= 2) {
            for (int lo = 0; lo < size - 1 && !stop.load(); lo += 2 * width) {
                int mid = std::min(lo + width - 1, size - 1);
                int hi = std::min(lo + 2 * width - 1, size - 1);
                
                if (mid < hi) {
                    merge(data, tmp.data(), lo, mid, hi, callback, stop, comparisons, swaps);
                }
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

#endif // MERGESORT_H
