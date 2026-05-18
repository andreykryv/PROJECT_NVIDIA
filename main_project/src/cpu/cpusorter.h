#ifndef CPUSORTER_H
#define CPUSORTER_H

#include <functional>
#include <atomic>
#include <chrono>
#include "sortparams.h"

namespace SortBench {

enum class HighlightType {
    Compare,
    Swap,
    Sorted,
    Pivot,
    None
};

class CpuSorter {
public:
    using SortCallback = std::function<void(
        const void* data,          // указатель на массив (для копирования кадра)
        int size,                  // число элементов
        int idx1, int idx2,        // индексы "активных" элементов
        HighlightType type,        // тип события
        long long comparisons,     // накопленных сравнений
        long long swaps            // накопленных перестановок
    )>;

    template<typename T>
    void sort(T* data, int size, CpuAlgorithm algo,
              SortCallback callback, std::atomic<bool>& stop);

private:
    bool shouldEmitFrame(std::chrono::steady_clock::time_point& lastEmit,
                         int targetFPS = 60);

    // Объявления функций сортировки
    template<typename T>
    void bubbleSort(T* data, int size, SortCallback callback, std::atomic<bool>& stop);
    
    template<typename T>
    void quickSort(T* data, int size, SortCallback callback, std::atomic<bool>& stop);
    
    template<typename T>
    void mergeSort(T* data, int size, SortCallback callback, std::atomic<bool>& stop);
    
    template<typename T>
    void heapSort(T* data, int size, SortCallback callback, std::atomic<bool>& stop);
    
    template<typename T>
    void radixSort(T* data, int size, SortCallback callback, std::atomic<bool>& stop);
    
    template<typename T>
    void stdSort(T* data, int size, SortCallback callback, std::atomic<bool>& stop);
};

} // namespace SortBench

#endif // CPUSORTER_H
