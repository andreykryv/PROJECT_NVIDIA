#include "quicksort.h"

namespace SortBench {

// QuickSort полностью реализован в заголовочном файле как template class
// Этот файл оставлен для явных инстанциаций шаблона

template class QuickSort<int32_t>;
template class QuickSort<int64_t>;
template class QuickSort<float>;
template class QuickSort<double>;

} // namespace SortBench
