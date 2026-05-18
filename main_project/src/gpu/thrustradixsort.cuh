////////////////////////////////////////////////////////////////////////////////
// gpu/thrustradixsort.cuh — обёртка над Thrust/CUB Radix Sort
//
// НАЗНАЧЕНИЕ:
//   Использует высокооптимизированную сортировку из библиотеки NVIDIA Thrust,
//   которая внутри использует CUB (CUDA UnBound) DeviceRadixSort.
//
// ВКЛЮЧАЕМЫЕ ЗАГОЛОВКИ (только в .cu):
//   #include <thrust/device_vector.h>
//   #include <thrust/sort.h>
//   #include <cub/device/device_radix_sort.cuh>
//
// ХОСТ-ОБЁРТКА:
//   ThrustRadixSortWrapper::sort<T>(T* d_data, int size, cudaStream_t stream):
//     — Вариант 1 (thrust):
//         thrust::device_ptr<T> d_ptr = thrust::device_pointer_cast(d_data);
//         thrust::sort(thrust::cuda::par.on(stream), d_ptr, d_ptr + size);
//     — Вариант 2 (CUB, предпочтительный, больше контроля):
//         cub::DeviceRadixSort::SortKeys(d_temp, temp_bytes, d_in, d_out, size,
//                                        0, sizeof(T)*8, stream);
//     — Для float/double: обработка знакового бита через CUB-трансформацию.
//     — Возвращает kernelTimeMs через cudaEvent.
//
// ПРИМЕЧАНИЕ:
//   Thrust — header-only при использовании thrust::sort. CUB требует .cu.
//   Оба варианта поддерживаются через компиляционный флаг USE_CUB_SORT.
////////////////////////////////////////////////////////////////////////////////
