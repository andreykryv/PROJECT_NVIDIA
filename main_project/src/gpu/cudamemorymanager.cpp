#include "cudamemorymanager.h"

// DeviceBuffer, PinnedBuffer, CudaStream, CudaEvent полностью реализованы
// inline в cudamemorymanager.h как шаблонные классы.
// Здесь только явные инстанции шаблонов для сокращения времени линковки.

template class DeviceBuffer<int32_t>;
template class DeviceBuffer<int64_t>;
template class DeviceBuffer<float>;
template class DeviceBuffer<double>;

template class PinnedBuffer<int32_t>;
template class PinnedBuffer<int64_t>;
template class PinnedBuffer<float>;
template class PinnedBuffer<double>;
