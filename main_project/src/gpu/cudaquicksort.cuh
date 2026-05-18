////////////////////////////////////////////////////////////////////////////////
// gpu/cudaquicksort.cuh — GPU Quick Sort через параллельный partition
//
// ПРИНЦИП РАБОТЫ:
//   Классический QuickSort плохо параллелится из-за рекурсии.
//   GPU-вариант использует итеративный подход с очередью подзадач:
//
//   Уровень 1: Весь массив разделяется одним блоком (или варпом).
//   Уровень 2: Два подмассива разделяются двумя блоками одновременно.
//   ...
//   Уровень log(n): n/threshold подмассивов, каждый < threshold элементов.
//   Финал: Insertion sort для малых подмассивов (threshold = 32).
//
// ЯДРА:
//
//   __global__ void gpuPartitionKernel<T>(T* data, int* tasks, int* taskCount,
//                                          int* newTasks, int* newTaskCount):
//     — Каждый блок берёт задачу (lo, hi) из tasks[].
//     — Pivot: медиана трёх (data[lo], data[mid], data[hi]).
//     — Параллельный partition через prefix scan в shared memory.
//     — Добавляет два подзадания в newTasks[].
//
//   __global__ void gpuInsertionSortSmall<T>(T* data, int* tasks, int taskCount):
//     — Финальный проход: каждый блок insertion-sort на малом подмассиве.
//
// ХОСТ-ОБЁРТКА:
//   GpuQuickSortWrapper::sort<T>(T* d_data, int size, int blockSize):
//     — Инициализирует tasks[0] = {0, size-1}.
//     — Итерационный цикл: запускает gpuPartitionKernel пока задачи > threshold.
//     — Финальный запуск gpuInsertionSortSmall.
//     — Управление памятью для tasks/newTasks через DeviceBuffer.
////////////////////////////////////////////////////////////////////////////////
