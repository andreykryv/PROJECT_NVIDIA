#include "heapsort.h"

namespace SortBench {

// HeapSort полностью реализован в заголовочном файле как template class
// Этот файл оставлен для явных инстанциаций шаблона

template class HeapSort<int32_t>;
template class HeapSort<int64_t>;
template class HeapSort<float>;
template class HeapSort<double>;

} // namespace SortBench
