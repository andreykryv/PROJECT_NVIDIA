#include "mergesort.h"

namespace SortBench {

// MergeSort полностью реализован в заголовочном файле как template class
// Этот файл оставлен для явных инстанциаций шаблона

template class MergeSort<int32_t>;
template class MergeSort<int64_t>;
template class MergeSort<float>;
template class MergeSort<double>;

} // namespace SortBench
