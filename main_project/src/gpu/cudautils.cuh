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
