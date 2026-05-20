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
template void thrustRadixSortWrapper<int>(int*, int, cudaStream_t);
template void thrustRadixSortWrapper<long>(long*, int, cudaStream_t);
template void thrustRadixSortWrapper<float>(float*, int, cudaStream_t);
template void thrustRadixSortWrapper<double>(double*, int, cudaStream_t);
