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

// Явные инстанции для уникальных типов
// Примечание: int и int32_t могут быть одним типом, то же для long и int64_t
#if defined(__linux__) && defined(__x86_64__)
// На Linux x86_64: int = int32_t, long = int64_t
template void cubDeviceSortWrapper<int>(int*, int, cudaStream_t);
template void cubDeviceSortWrapper<long>(long*, int, cudaStream_t);
#else
// На других платформах могут отличаться
template void cubDeviceSortWrapper<int32_t>(int32_t*, int, cudaStream_t);
template void cubDeviceSortWrapper<int64_t>(int64_t*, int, cudaStream_t);
#endif
template void cubDeviceSortWrapper<float>(float*, int, cudaStream_t);
template void cubDeviceSortWrapper<double>(double*, int, cudaStream_t);
