#include <cuda_runtime.h>
#include <thrust/device_ptr.h>
#include <thrust/sort.h>
#include <thrust/execution_policy.h>

// Простая реализация GPU Quick Sort через Thrust для совместимости
template<typename T>
void gpuQuickSortWrapper(T* d_data, int size, int blockSize, cudaStream_t stream) {
    (void)blockSize;
    thrust::device_ptr<T> dev_ptr(d_data);
    thrust::sort(thrust::cuda::par.on(stream), dev_ptr, dev_ptr + size);
}

// Явные инстанции для уникальных типов
// Примечание: int и int32_t могут быть одним типом, то же для long и int64_t
#if defined(__linux__) && defined(__x86_64__)
// На Linux x86_64: int = int32_t, long = int64_t
template void gpuQuickSortWrapper<int>(int*, int, int, cudaStream_t);
template void gpuQuickSortWrapper<long>(long*, int, int, cudaStream_t);
#else
// На других платформах могут отличаться
template void gpuQuickSortWrapper<int32_t>(int32_t*, int, int, cudaStream_t);
template void gpuQuickSortWrapper<int64_t>(int64_t*, int, int, cudaStream_t);
#endif
template void gpuQuickSortWrapper<float>(float*, int, int, cudaStream_t);
template void gpuQuickSortWrapper<double>(double*, int, int, cudaStream_t);
