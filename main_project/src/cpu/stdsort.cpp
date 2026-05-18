#include "stdsort.h"
#include <algorithm>

namespace SortBench {

template<typename T>
void StdSort<T>::sort(T* data, int size, CpuSorter::SortCallback callback, std::atomic<bool>& stop) {
    if (size <= 1 || stop.load()) return;
    
    // std::sort непрозрачен — не можем вызывать callback в процессе
    // Поэтому просто вызываем сортировку и потом один кадр
    std::sort(data, data + size);
    
    long long comparisons = 0;
    long long swaps = 0;
    
    // Один финальный кадр с помеченным отсортированным массивом
    if (!stop.load() && callback) {
        for (int i = 0; i < size; ++i) {
            callback(data, size, i, -1, HighlightType::Sorted, comparisons, swaps);
        }
    }
}

// Явные инстанции шаблона для всех типов
template class StdSort<int32_t>;
template class StdSort<int64_t>;
template class StdSort<float>;
template class StdSort<double>;

} // namespace SortBench
