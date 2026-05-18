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

// Явные инстанции
template void thrustRadixSortWrapper<int32_t>(int32_t*, int, cudaStream_t);
template void thrustRadixSortWrapper<int64_t>(int64_t*, int, cudaStream_t);
template void thrustRadixSortWrapper<float>(float*, int, cudaStream_t);
template void thrustRadixSortWrapper<double>(double*, int, cudaStream_t);
