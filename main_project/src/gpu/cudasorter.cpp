////////////////////////////////////////////////////////////////////////////////
// gpu/cudasorter.cpp — реализация фасада GPU-алгоритмов (хост-сторона)
//
// СОДЕРЖИМОЕ:
//   Конструктор: cudaSetDevice, cudaStreamCreate x numStreams,
//               cudaMallocHost для pinned buffers.
//   sort<T>(): switch(algo) вызывает соответствующий .cuh файл.
//     — BitonicSort:    BitonicSortWrapper::sort(d_data, size, stream)
//     — ThrustRadix:    ThrustRadixSortWrapper::sort(d_data, size)
//     — GpuQuickSort:   GpuQuickSortWrapper::sort(d_data, size, blockSize)
//     — CubDeviceSort:  CubDeviceSortWrapper::sort(d_data, size)
//   Замер времени через cudaEvent_t для каждой фазы.
//   Явные инстанции шаблона: int32_t, int64_t, float, double.
////////////////////////////////////////////////////////////////////////////////
