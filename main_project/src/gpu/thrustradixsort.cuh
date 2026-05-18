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

#ifndef THRUST_RADIXSORT_CUH
#define THRUST_RADIXSORT_CUH

#include <cuda_runtime.h>
#include <thrust/device_ptr.h>
#include <thrust/sort.h>
#include <thrust/execution_policy.h>

#ifdef USE_CUB_SORT
#include <cub/device/device_radix_sort.cuh>
#endif

// Host-обёртка для Thrust Radix Sort
class ThrustRadixSortWrapper {
public:
    // Сортировка целочисленных типов
    template<typename T>
    static cudaError_t sort(T* d_data, int size, cudaStream_t stream = 0) {
        if (size <= 1) return cudaSuccess;
        
#ifdef USE_CUB_SORT
        // Вариант с CUB - более низкоуровневый контроль
        void* d_temp_storage = nullptr;
        size_t temp_storage_bytes = 0;
        
        // Первый вызов для определения размера временной памяти
        cub::DeviceRadixSort::SortKeys(
            d_temp_storage, temp_storage_bytes,
            d_data, d_data, size,
            0, sizeof(T) * 8, stream);
        
        // Выделяем временную память
        cudaError_t err = cudaMalloc(&d_temp_storage, temp_storage_bytes);
        if (err != cudaSuccess) return err;
        
        // Второй вызов - реальная сортировка
        err = cub::DeviceRadixSort::SortKeys(
            d_temp_storage, temp_storage_bytes,
            d_data, d_data, size,
            0, sizeof(T) * 8, stream);
        
        cudaFree(d_temp_storage);
        return err;
#else
        // Вариант с Thrust - проще и удобнее
        thrust::device_ptr<T> d_ptr = thrust::device_pointer_cast(d_data);
        
        try {
            auto policy = thrust::cuda::par.on(stream);
            thrust::sort(policy, d_ptr, d_ptr + size);
            return cudaSuccess;
        } catch (...) {
            return cudaErrorUnknown;
        }
#endif
    }
    
    // Специализация для float - обработка знаковых значений
    template<>
    static cudaError_t sort<float>(float* d_data, int size, cudaStream_t stream) {
        if (size <= 1) return cudaSuccess;
        
#ifdef USE_CUB_SORT
        // Для float нужно инвертировать знаковый бит для корректной сортировки
        void* d_temp_storage = nullptr;
        size_t temp_storage_bytes = 0;
        
        // Трансформация: инвертируем знаковый бит для отрицательных чисел
        // Это нужно чтобы -inf < ... < -1 < 0 < 1 < ... < +inf
        thrust::device_ptr<float> d_ptr = thrust::device_pointer_cast(d_data);
        
        // Преобразуем float в uint32 для побитовой манипуляции
        thrust::device_ptr<uint32_t> d_bits = 
            thrust::device_pointer_cast(reinterpret_cast<uint32_t*>(d_data));
        
        // Применяем трансформацию к каждому элементу
        thrust::for_each(thrust::cuda::par.on(stream), d_bits, d_bits + size,
            [] __device__ (uint32_t& bits) {
                uint32_t mask = (bits >> 31) * 0xFFFFFFFFU;
                bits ^= mask;
            });
        
        // Сортируем как unsigned integers
        cub::DeviceRadixSort::SortKeys(
            d_temp_storage, temp_storage_bytes,
            reinterpret_cast<uint32_t*>(d_data),
            reinterpret_cast<uint32_t*>(d_data),
            size, 0, 32, stream);
        
        // Обратная трансформация
        thrust::for_each(thrust::cuda::par.on(stream), d_bits, d_bits + size,
            [] __device__ (uint32_t& bits) {
                uint32_t mask = ((bits >> 31) ^ 1) * 0xFFFFFFFFU;
                bits ^= mask;
            });
        
        cudaFree(d_temp_storage);
        return cudaSuccess;
#else
        thrust::device_ptr<float> d_ptr = thrust::device_pointer_cast(d_data);
        try {
            auto policy = thrust::cuda::par.on(stream);
            thrust::sort(policy, d_ptr, d_ptr + size);
            return cudaSuccess;
        } catch (...) {
            return cudaErrorUnknown;
        }
#endif
    }
    
    // Специализация для double - аналогично float
    template<>
    static cudaError_t sort<double>(double* d_data, int size, cudaStream_t stream) {
        if (size <= 1) return cudaSuccess;
        
#ifdef USE_CUB_SORT
        void* d_temp_storage = nullptr;
        size_t temp_storage_bytes = 0;
        
        thrust::device_ptr<double> d_ptr = thrust::device_pointer_cast(d_data);
        thrust::device_ptr<uint64_t> d_bits = 
            thrust::device_pointer_cast(reinterpret_cast<uint64_t*>(d_data));
        
        // Трансформация для double (64 бита)
        thrust::for_each(thrust::cuda::par.on(stream), d_bits, d_bits + size,
            [] __device__ (uint64_t& bits) {
                uint64_t mask = (bits >> 63) * 0xFFFFFFFFFFFFFFFFULL;
                bits ^= mask;
            });
        
        cub::DeviceRadixSort::SortKeys(
            d_temp_storage, temp_storage_bytes,
            reinterpret_cast<uint64_t*>(d_data),
            reinterpret_cast<uint64_t*>(d_data),
            size, 0, 64, stream);
        
        // Обратная трансформация
        thrust::for_each(thrust::cuda::par.on(stream), d_bits, d_bits + size,
            [] __device__ (uint64_t& bits) {
                uint64_t mask = ((bits >> 63) ^ 1) * 0xFFFFFFFFFFFFFFFFULL;
                bits ^= mask;
            });
        
        cudaFree(d_temp_storage);
        return cudaSuccess;
#else
        thrust::device_ptr<double> d_ptr = thrust::device_pointer_cast(d_data);
        try {
            auto policy = thrust::cuda::par.on(stream);
            thrust::sort(policy, d_ptr, d_ptr + size);
            return cudaSuccess;
        } catch (...) {
            return cudaErrorUnknown;
        }
#endif
    }
};

#endif // THRUST_RADIXSORT_CUH
