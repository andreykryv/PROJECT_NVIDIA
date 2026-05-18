////////////////////////////////////////////////////////////////////////////////
// gpu/bitonicsort.cuh — CUDA Bitonic Sort (заголовок + device-объявления)
//
// НАЗНАЧЕНИЕ:
//   Реализует GPU Bitonic Sort — алгоритм сортировки сетями,
//   идеально подходящий для массового параллелизма GPU.
//
// ПРИНЦИП РАБОТЫ:
//   Bitonic sequence: последовательность, которая сначала возрастает,
//   затем убывает (или наоборот). Bitonic Sort строит и сортирует
//   такие последовательности за log²(n) фаз.
//   Каждая фаза полностью параллельна: n/2 независимых пар.
//
// ЯДРА:
//
//   __global__ void bitonicSortStep<T>(T* data, int j, int k):
//     — Одна ступень алгоритма. Блоки: DIVUP(n/2, blockSize).
//       threadIdx маппится на уникальную пару (i, ixj) = (i, i XOR j).
//       Вызывает compareAndSwap(data, i, ixj, direction).
//       Direction: (i & k) == 0 → ascending.
//     — Запускается из хоста в двойном цикле: k = 2..n, j = k/2..1.
//
//   __global__ void bitonicSortShared<T>(T* data, int n):
//     — Оптимизированная версия для подмассивов, умещающихся в shared memory.
//     — Все шаги для блока выполняются без глобальных чтений/записей.
//     — Блок: до 1024 нитей, shared: 2*blockDim.x * sizeof(T).
//
// ХОСТ-ОБЁРТКА:
//   BitonicSortWrapper::sort<T>(T* d_data, int size, cudaStream_t stream):
//     — Дополняет размер до ближайшей степени двойки (pad to power of 2).
//     — Лончит bitonicSortShared для блоков ≤ 2048, bitonicSortStep снаружи.
//     — Возвращает время kernel в миллисекундах (cudaEvent).
//
// ОГРАНИЧЕНИЯ:
//   — Оптимален для размеров = степень двойки.
//   — При других размерах: padding фиктивными MAX_VALUE элементами.
//   — Требует O(n) device памяти (in-place алгоритм).
////////////////////////////////////////////////////////////////////////////////

#ifndef BITONIC_SORT_CUH
#define BITONIC_SORT_CUH

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <limits>

// Устройство-функция: сравнение и обмен
template<typename T>
__device__ inline void compareAndSwap(T* data, int i, int j, bool ascending) {
    if ((ascending && data[i] > data[j]) || (!ascending && data[i] < data[j])) {
        T temp = data[i];
        data[i] = data[j];
        data[j] = temp;
    }
}

// Ядро одного шага битонической сортировки
template<typename T>
__global__ void bitonicSortStep(T* data, int j, int k, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int ixj = i ^ j;
    
    // Каждый элемент участвует только если i < ixj и ixj < n
    if (ixj > i && ixj < n) {
        bool ascending = (i & k) == 0;
        compareAndSwap(data, i, ixj, ascending);
    }
}

// Оптимизированное ядро с использованием shared memory
template<typename T>
__global__ void bitonicSortShared(T* data, int n) {
    extern __shared__ T shared[];
    
    int tid = threadIdx.x;
    int base = blockIdx.x * blockDim.x;
    
    // Загружаем данные в shared memory
    if (base + tid < n) {
        shared[tid] = data[base + tid];
    } else {
        shared[tid] = std::numeric_limits<T>::max();
    }
    __syncthreads();
    
    // Bitonic sort в shared memory
    for (int k = 2; k <= blockDim.x; k *= 2) {
        for (int j = k / 2; j > 0; j /= 2) {
            int ixj = tid ^ j;
            if (ixj > tid && ixj < blockDim.x) {
                bool ascending = (tid & k) == 0;
                compareAndSwap(shared, tid, ixj, ascending);
            }
            __syncthreads();
        }
    }
    
    // Записываем обратно в глобальную память
    if (base + tid < n) {
        data[base + tid] = shared[tid];
    }
}

// Вспомогательная функция для вычисления потолка деления
inline int divup(int a, int b) {
    return (a + b - 1) / b;
}

// Нахождение ближайшей степени двойки
inline int nextPowerOfTwo(int n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

// Host-обёртка для Bitonic Sort
class BitonicSortWrapper {
public:
    template<typename T>
    static cudaError_t sort(T* d_data, int size, int blockSize = 256,
                           cudaStream_t stream = 0) {
        if (size <= 1) return cudaSuccess;
        
        // Дополняем до степени двойки
        int n = nextPowerOfTwo(size);
        
        // Заполняем padding максимальными значениями
        if (n > size) {
            T fillValue = std::numeric_limits<T>::max();
            cudaMemsetAsync(d_data + size, 0xFF, (n - size) * sizeof(T), stream);
        }
        
        // Основной цикл битонической сортировки
        for (int k = 2; k <= n; k *= 2) {
            for (int j = k / 2; j > 0; j /= 2) {
                int numBlocks = divup(n / 2, blockSize);
                bitonicSortStep<T><<<numBlocks, blockSize, 0, stream>>>(
                    d_data, j, k, n);
                
                cudaError_t err = cudaGetLastError();
                if (err != cudaSuccess) return err;
            }
        }
        
        return cudaSuccess;
    }
};

#endif // BITONIC_SORT_CUH
