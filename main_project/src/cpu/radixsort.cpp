#include "radixsort.h"

namespace SortBench {

// RadixSort полностью реализован в заголовочном файле как template class
// Этот файл оставлен для явных инстанциаций шаблона

template class RadixSort<int32_t>;
template class RadixSort<int64_t>;
template class RadixSort<float>;
template class RadixSort<double>;

} // namespace SortBench
