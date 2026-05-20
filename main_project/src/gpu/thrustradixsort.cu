#include <cuda_runtime.h>
#include <thrust/device_ptr.h>
#include <thrust/sort.h>
#include <thrust/execution_policy.h>

// Wrapper функция для Thrust Radix Sort
template<typename T>
void thrustRadixSortWrapper(T* d_data, int size, cudaStream_t stream) {
    thrust::device_ptr<T> dev_ptr(d_data);
    
    // Используем thrust::sort с execution policy для стрима
    thrust::sort(thrust::cuda::par.on(stream), dev_ptr, dev_ptr + size);
}

// Явные инстанции для уникальных типов
// Примечание: int и int32_t могут быть одним типом, то же для long и int64_t
#if defined(__linux__) && defined(__x86_64__)
// На Linux x86_64: int = int32_t, long = int64_t
template void thrustRadixSortWrapper<int>(int*, int, cudaStream_t);
template void thrustRadixSortWrapper<long>(long*, int, cudaStream_t);
#else
// На других платформах могут отличаться
template void thrustRadixSortWrapper<int32_t>(int32_t*, int, cudaStream_t);
template void thrustRadixSortWrapper<int64_t>(int64_t*, int, cudaStream_t);
#endif
template void thrustRadixSortWrapper<float>(float*, int, cudaStream_t);
template void thrustRadixSortWrapper<double>(double*, int, cudaStream_t);
