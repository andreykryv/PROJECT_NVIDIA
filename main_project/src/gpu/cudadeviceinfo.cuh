////////////////////////////////////////////////////////////////////////////////
// gpu/cudadeviceinfo.cuh — CUDA-заголовок (включается только в .cu файлах)
//
// НАЗНАЧЕНИЕ:
//   Объявляет device-side утилиты и расширенные CUDA-специфические
//   функции, которые нельзя объявить в .h без cuda_runtime.h.
//
// СОДЕРЖИМОЕ:
//   — Объявление функции cudaQueryDeviceInfo() реализованной в .cu
//   — Вспомогательные макросы CUDA_CHECK(err) для обработки ошибок:
//       #define CUDA_CHECK(call) { cudaError_t err = (call); \
//           if(err != cudaSuccess) { /* emit error signal или throw */ } }
//   — CUDA_CHECK_LAST() — проверяет cudaGetLastError() после kernel launch
//   — Макрос DIVUP(n, d) — деление с округлением вверх: (n + d - 1) / d
//   — Макрос ALIGN_UP(n, alignment) — выравнивание вверх
//   — Константы: WARP_SIZE = 32, MAX_BLOCK_SIZE = 1024
////////////////////////////////////////////////////////////////////////////////

#ifndef CUADEVICEINFO_CUH
#define CUADEVICEINFO_CUH

#include <cuda_runtime.h>

// QString используется только в host-коде, не в CUDA device коде
#ifdef __CUDACC__
    // В CUDA device коде не используем Qt
#else
    #include <QString>
#endif

// Макросы для обработки ошибок CUDA
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err__ = (call); \
        if (err__ != cudaSuccess) { \
            fprintf(stderr, "CUDA error %s:%d: %s\n", __FILE__, __LINE__, \
                    cudaGetErrorString(err__)); \
        } \
    } while(0)

#define CUDA_CHECK_LAST() \
    do { \
        cudaError_t err__ = cudaGetLastError(); \
        if (err__ != cudaSuccess) { \
            fprintf(stderr, "CUDA last error %s:%d: %s\n", __FILE__, __LINE__, \
                    cudaGetErrorString(err__)); \
        } \
    } while(0)

// Макросы для вычислений
#define DIVUP(n, d) (((n) + (d) - 1) / (d))
#define ALIGN_UP(n, alignment) (((n) + (alignment) - 1) & ~((alignment) - 1))

// Константы
constexpr int WARP_SIZE = 32;
constexpr int MAX_BLOCK_SIZE = 1024;

// Device-side функции объявляются в .cu файле
#ifdef __CUDACC__
__device__ inline int getLaneId() {
    int laneId;
    asm("mov.s32 %0, %%laneid;" : "=r"(laneId));
    return laneId;
}

__device__ inline int getWarpId() {
    return threadIdx.x / WARP_SIZE;
}
#endif

#endif // CUADEVICEINFO_CUH
