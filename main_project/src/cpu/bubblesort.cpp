#include "bubblesort.h"

namespace SortBench {

// BubbleSort полностью реализован в заголовочном файле как template class
// Этот файл оставлен для явных инстанциаций шаблона

template class BubbleSort<int32_t>;
template class BubbleSort<int64_t>;
template class BubbleSort<float>;
template class BubbleSort<double>;

} // namespace SortBench
