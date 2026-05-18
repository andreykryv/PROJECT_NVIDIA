#include "stdsort.h"

namespace SortBench {

// StdSort полностью реализован в заголовочном файле как template class
// Этот файл оставлен для явных инстанциаций шаблона

template class StdSort<int32_t>;
template class StdSort<int64_t>;
template class StdSort<float>;
template class StdSort<double>;

} // namespace SortBench
