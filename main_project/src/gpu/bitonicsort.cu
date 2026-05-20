#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <limits>
#include <algorithm>
#include <stdint.h>
#include <cfloat>

// CUDA-совместимые функции для получения максимального значения
template<typename T>
__device__ __forceinline__ T getMaxValue();

// Специализации для различных типов
template<>
__device__ __forceinline__ int32_t getMaxValue<int32_t>() {
    return INT32_MAX;
}

template<>
__device__ __forceinline__ int64_t getMaxValue<int64_t>() {
    return INT64_MAX;
}

template<>
__device__ __forceinline__ float getMaxValue<float>() {
    return FLT_MAX;
}

template<>
__device__ __forceinline__ double getMaxValue<double>() {
    return DBL_MAX;
}

template<typename T>
__device__ void compareAndSwap(T* data, int i, int j, bool dir) {
    T vi = data[i];
    T vj = data[j];

    if (dir) {
        if (vi > vj) {
            data[i] = vj;
            data[j] = vi;
        }
    } else {
        if (vi < vj) {
            data[i] = vj;
            data[j] = vi;
        }
    }
}

// Ядро для одного шага битонической сортировки
template<typename T>
__global__ void bitonicSortStepKernel(T* data, int size, int k, int j, bool dir) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int ixj = i ^ j;

    if (i < size && ixj < size) {
        if (ixj > i) {
            compareAndSwap(data, i, ixj, dir);
        }
    }
}

// Битоническая сортировка в shared memory для малых размеров
template<typename T>
__global__ void bitonicSortSharedKernel(T* data, int size) {
     extern __shared__ unsigned char sdata_raw[];
    T* sdata = reinterpret_cast<T*>(sdata_raw);

    int tid = threadIdx.x;
    int offset = blockIdx.x * blockDim.x * 2;

    // Загрузка данных в shared memory
    if (offset + tid < size) {
        sdata[tid] = data[offset + tid];
    } else {
        sdata[tid] = getMaxValue<T>();
    }

    if (offset + blockDim.x + tid < size) {
        sdata[blockDim.x + tid] = data[offset + blockDim.x + tid];
    } else {
        sdata[blockDim.x + tid] = getMaxValue<T>();
    }

    __syncthreads();

    // Битоническая сортировка в shared memory
    for (int k = 2; k <= blockDim.x * 2; k *= 2) {
        for (int j = k / 2; j > 0; j /= 2) {
            int ixj = tid ^ j;
            if (ixj > tid) {
                bool dir = (tid & k) == 0;
                compareAndSwap(sdata, tid, ixj, dir);
            }
            __syncthreads();
        }
    }

    // Запись результата обратно
    if (offset + tid < size) {
        data[offset + tid] = sdata[tid];
    }
    if (offset + blockDim.x + tid < size) {
        data[offset + blockDim.x + tid] = sdata[blockDim.x + tid];
    }
}

// Wrapper функция для битонической сортировки
template<typename T>
void bitonicSortWrapper(T* d_data, int size, cudaStream_t stream) {
    // Вычисляем следующий размер степени двойки
    int paddedSize = 1;
    while (paddedSize < size) {
        paddedSize *= 2;
    }

    // Для малых размеров используем shared memory kernel
    if (paddedSize <= 512) {
        int blockSize = paddedSize / 2;
        int gridSize = (size + paddedSize - 1) / paddedSize;
        size_t sharedMemSize = paddedSize * sizeof(T);

        bitonicSortSharedKernel<T><<<gridSize, blockSize, sharedMemSize, stream>>>(d_data, size);
        return;
    }

    // Для больших размеров используем итеративный подход
    for (int k = 2; k <= paddedSize; k *= 2) {
        for (int j = k / 2; j > 0; j /= 2) {
            bool dir = (k == paddedSize);  // На последнем этапе сортируем по возрастанию
            int threadsPerBlock = 256;
            int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;

            bitonicSortStepKernel<T><<<blocksPerGrid, threadsPerBlock, 0, stream>>>(
                d_data, size, k, j, dir);
        }
    }
}

// Явные инстанции
template void bitonicSortWrapper<int32_t>(int32_t*, int, cudaStream_t);
template void bitonicSortWrapper<int64_t>(int64_t*, int, cudaStream_t);
template void bitonicSortWrapper<float>(float*, int, cudaStream_t);
template void bitonicSortWrapper<double>(double*, int, cudaStream_t);