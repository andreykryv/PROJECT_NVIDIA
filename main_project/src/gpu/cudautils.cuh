////////////////////////////////////////////////////////////////////////////////
// gpu/cudautils.cuh — общие утилиты для CUDA-ядер (device-side)
//
// НАЗНАЧЕНИЕ:
//   Заголовок с inline __device__ функциями и шаблонами, используемыми
//   во всех GPU-алгоритмах. Включается только в .cu файлы.
//
// СОДЕРЖИМОЕ:
//
//   __device__ void compareAndSwap<T>(T* data, int i, int j, bool ascending):
//     — Сравнивает data[i] и data[j], меняет местами если нарушен порядок.
//     — Используется в Bitonic Sort.
//     — Атомарная версия не нужна: каждый поток работает с уникальными индексами.
//
//   __device__ T warpReduceMin<T>(T val):
//     — Редукция минимума внутри варпа через __shfl_down_sync.
//     — Используется для нахождения pivot в GpuQuickSort.
//
//   __device__ T warpReduceMax<T>(T val):
//     — Аналогично для максимума.
//
//   __device__ int exclusiveScanWarp(int val):
//     — Эксклюзивный scan внутри варпа через __shfl_up_sync.
//     — Используется для подсчёта prefix sum при partition.
//
//   __global__ void exclusiveScanBlock(int* data, int n):
//     — Параллельный scan в пределах одного блока через shared memory.
//     — Алгоритм Blelloch (up-sweep + down-sweep).
//
//   __device__ int laneId():
//     — return threadIdx.x & (WARP_SIZE - 1)  — ID нити в варпе.
//
//   __device__ int warpId():
//     — return threadIdx.x / WARP_SIZE
//
//   Шаблонные обёртки для атомарных операций:
//     __device__ T atomicMinT<T>(T* addr, T val) — для float через atomicCAS.
//     __device__ T atomicMaxT<T>(T* addr, T val)
////////////////////////////////////////////////////////////////////////////////

#ifndef CUDA_UTILS_CUH
#define CUDA_UTILS_CUH

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdint.h>

// Константы
constexpr int WARP_SIZE = 32;

// Получить ID нити в варпе
__device__ __forceinline__ int laneId() {
    return threadIdx.x & (WARP_SIZE - 1);
}

// Получить ID варпа в блоке
__device__ __forceinline__ int warpId() {
    return threadIdx.x / WARP_SIZE;
}

// Сравнение и обмен (для bitonic sort)
template<typename T>
__device__ inline void compareAndSwap(T* data, int i, int j, bool ascending) {
    if ((ascending && data[i] > data[j]) || (!ascending && data[i] < data[j])) {
        T temp = data[i];
        data[i] = data[j];
        data[j] = temp;
    }
}

// Warp reduction для минимума
template<typename T>
__device__ inline T warpReduceMin(T val) {
    for (int offset = WARP_SIZE / 2; offset > 0; offset /= 2) {
        T other = __shfl_down_sync(0xffffffff, val, offset);
        if (other < val) val = other;
    }
    return val;
}

// Warp reduction для максимума
template<typename T>
__device__ inline T warpReduceMax(T val) {
    for (int offset = WARP_SIZE / 2; offset > 0; offset /= 2) {
        T other = __shfl_down_sync(0xffffffff, val, offset);
        if (other > val) val = other;
    }
    return val;
}

// Warp reduction для суммы
__device__ inline int warpReduceSum(int val) {
    for (int offset = WARP_SIZE / 2; offset > 0; offset /= 2) {
        val += __shfl_down_sync(0xffffffff, val, offset);
    }
    return val;
}

// Эксклюзивный scan внутри варпа
__device__ inline int exclusiveScanWarp(int val) {
    int mask = 0xffffffff;
    int offset = 1;
    
    for (int d = 1; d < WARP_SIZE; d *= 2) {
        int t = __shfl_up_sync(mask, val, d);
        if (laneId() >= d) val += t;
    }
    
    return __shfl_up_sync(mask, val, 1, WARP_SIZE - 1);
}

// Атомарный минимум для float
__device__ inline float atomicMinF(float* addr, float value) {
    int* addr_as_int = reinterpret_cast<int*>(addr);
    int old = *addr_as_int;
    int assumed;
    
    do {
        assumed = old;
        float assumedFloat = __int_as_float(assumed);
        float minVal = fminf(assumedFloat, value);
        old = atomicCAS(addr_as_int, assumed, __float_as_int(minVal));
    } while (assumed != old);
    
    return __int_as_float(old);
}

// Атомарный максимум для float
__device__ inline float atomicMaxF(float* addr, float value) {
    int* addr_as_int = reinterpret_cast<int*>(addr);
    int old = *addr_as_int;
    int assumed;
    
    do {
        assumed = old;
        float assumedFloat = __int_as_float(assumed);
        float maxVal = fmaxf(assumedFloat, value);
        old = atomicCAS(addr_as_int, assumed, __float_as_int(maxVal));
    } while (assumed != old);
    
    return __int_as_float(old);
}

// Атомарный минимум для double (требует compute capability >= 6.0)
__device__ inline double atomicMinD(double* addr, double value) {
    unsigned long long int* addr_as_ull = 
        reinterpret_cast<unsigned long long int*>(addr);
    unsigned long long int old = *addr_as_ull;
    unsigned long long int assumed;
    
    do {
        assumed = old;
        double assumedDouble = __longlong_as_double(assumed);
        double minVal = fmin(assumedDouble, value);
        old = atomicCAS(addr_as_ull, assumed, __double_as_longlong(minVal));
    } while (assumed != old);
    
    return __longlong_as_double(old);
}

// Атомарный максимум для double
__device__ inline double atomicMaxD(double* addr, double value) {
    unsigned long long int* addr_as_ull = 
        reinterpret_cast<unsigned long long int*>(addr);
    unsigned long long int old = *addr_as_ull;
    unsigned long long int assumed;
    
    do {
        assumed = old;
        double assumedDouble = __longlong_as_double(assumed);
        double maxVal = fmax(assumedDouble, value);
        old = atomicCAS(addr_as_ull, assumed, __double_as_longlong(maxVal));
    } while (assumed != old);
    
    return __longlong_as_double(old);
}

// Универсальный atomicMin для любых типов
template<typename T>
__device__ inline T atomicMinT(T* addr, T val);

template<>
__device__ inline int atomicMinT<int>(int* addr, int val) {
    return atomicMin(addr, val);
}

template<>
__device__ inline float atomicMinT<float>(float* addr, float val) {
    return atomicMinF(addr, val);
}

template<>
__device__ inline double atomicMinT<double>(double* addr, double val) {
    return atomicMinD(addr, val);
}

// Универсальный atomicMax для любых типов
template<typename T>
__device__ inline T atomicMaxT(T* addr, T val);

template<>
__device__ inline int atomicMaxT<int>(int* addr, int val) {
    return atomicMax(addr, val);
}

template<>
__device__ inline float atomicMaxT<float>(float* addr, float val) {
    return atomicMaxF(addr, val);
}

template<>
__device__ inline double atomicMaxT<double>(double* addr, double val) {
    return atomicMaxD(addr, val);
}

// Ядро эксклюзивного scan для блока (Blelloch algorithm)
__global__ void exclusiveScanBlock(int* data, int n) {
    extern __shared__ int temp[];
    
    int tid = threadIdx.x;
    int offset = 1;
    
    // Загрузка данных
    if (blockIdx.x * blockDim.x + tid < n) {
        temp[tid] = data[blockIdx.x * blockDim.x + tid];
    } else {
        temp[tid] = 0;
    }
    __syncthreads();
    
    // Up-sweep
    for (int d = n / 2; d > 0; d /= 2) {
        __syncthreads();
        if (tid < d) {
            int ai = offset * (2 * tid + 1) - 1;
            int bi = offset * (2 * tid + 2) - 1;
            temp[bi] += temp[ai];
        }
        offset *= 2;
    }
    
    // Clear last element
    if (tid == 0) {
        temp[n - 1] = 0;
    }
    __syncthreads();
    
    // Down-sweep
    for (int d = 1; d < n; d *= 2) {
        offset >>= 1;
        __syncthreads();
        if (tid < d) {
            int ai = offset * (2 * tid + 1) - 1;
            int bi = offset * (2 * tid + 2) - 1;
            int t = temp[ai];
            temp[ai] = temp[bi];
            temp[bi] += t;
        }
    }
    __syncthreads();
    
    // Запись результата
    if (blockIdx.x * blockDim.x + tid < n) {
        data[blockIdx.x * blockDim.x + tid] = temp[tid];
    }
}

// Вычисление ceiling деления
__host__ __device__ inline int divUp(int a, int b) {
    return (a + b - 1) / b;
}

// Выравнивание по степени двойки
__host__ __device__ inline int alignUp(int a, int b) {
    return (a + b - 1) & ~(b - 1);
}

#endif // CUDA_UTILS_CUH
