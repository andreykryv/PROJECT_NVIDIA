#include "cpusorter.h"
#include "bubblesort.h"
#include "quicksort.h"
#include "mergesort.h"
#include "heapsort.h"
#include "radixsort.h"
#include "stdsort.h"

namespace SortBench {

template<typename T>
void CpuSorter::sort(T* data, int size, CpuAlgorithm algo,
                     SortCallback callback, std::atomic<bool>& stop) {
    switch (algo) {
        case CpuAlgorithm::BubbleSort:
            BubbleSort<T>::sort(data, size, callback, stop);
            break;
        case CpuAlgorithm::QuickSort:
            QuickSort<T>::sort(data, size, callback, stop);
            break;
        case CpuAlgorithm::MergeSort:
            MergeSort<T>::sort(data, size, callback, stop);
            break;
        case CpuAlgorithm::HeapSort:
            HeapSort<T>::sort(data, size, callback, stop);
            break;
        case CpuAlgorithm::RadixSort:
            RadixSort<T>::sort(data, size, callback, stop);
            break;
        case CpuAlgorithm::StdSort:
            StdSort<T>::sort(data, size, callback, stop);
            break;
        default:
            break;
    }
}

bool CpuSorter::shouldEmitFrame(std::chrono::steady_clock::time_point& lastEmit,
                                 int targetFPS) {
    auto now = std::chrono::steady_clock::now();
    auto targetInterval = std::chrono::milliseconds(1000 / targetFPS);
    if (now - lastEmit >= targetInterval) {
        lastEmit = now;
        return true;
    }
    return false;
}

// Явные инстанции шаблона sort для всех поддерживаемых типов
template void CpuSorter::sort<int32_t>(int32_t*, int, CpuAlgorithm, SortCallback, std::atomic<bool>&);
template void CpuSorter::sort<int64_t>(int64_t*, int, CpuAlgorithm, SortCallback, std::atomic<bool>&);
template void CpuSorter::sort<float>(float*, int, CpuAlgorithm, SortCallback, std::atomic<bool>&);
template void CpuSorter::sort<double>(double*, int, CpuAlgorithm, SortCallback, std::atomic<bool>&);

} // namespace SortBench
