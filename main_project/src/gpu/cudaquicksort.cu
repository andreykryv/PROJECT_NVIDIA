#include <cuda_runtime.h>
#include <cub/cub.cuh>

// Wrapper функция для CUB Device Radix Sort
template<typename T>
void cubDeviceSortWrapper(T* d_data, int size, cudaStream_t stream) {
    // Определяем тип ключа для CUB
    using KeyT = T;
    
    // Выделяем временную память для сортировки
    void* d_temp_storage = nullptr;
    size_t temp_storage_bytes = 0;
    
    // Первый вызов для определения размера временной памяти
    cub::DeviceRadixSort::SortKeys(
        d_temp_storage, temp_storage_bytes,
        d_data, d_data, size,
        0, sizeof(KeyT) * 8,
        stream);
    
    // Выделяем память
    cudaMalloc(&d_temp_storage, temp_storage_bytes);
    
    // Второй вызов для выполнения сортировки
    cub::DeviceRadixSort::SortKeys(
        d_temp_storage, temp_storage_bytes,
        d_data, d_data, size,
        0, sizeof(KeyT) * 8,
        stream);
    
    // Освобождаем временную память
    cudaFree(d_temp_storage);
}

// Явные инстанции
template void cubDeviceSortWrapper<int32_t>(int32_t*, int, cudaStream_t);
template void cubDeviceSortWrapper<int64_t>(int64_t*, int, cudaStream_t);
template void cubDeviceSortWrapper<float>(float*, int, cudaStream_t);
template void cubDeviceSortWrapper<double>(double*, int, cudaStream_t);
