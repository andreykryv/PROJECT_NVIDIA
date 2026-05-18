////////////////////////////////////////////////////////////////////////////////
// gpu/cudaquicksort.cu — реализация GPU Quick Sort (компилируется nvcc)
//
// СОДЕРЖИМОЕ:
//   Реализует gpuPartitionKernel с shared-memory prefix scan.
//   Реализует gpuInsertionSortSmall для подмассивов < 32 элементов.
//   GpuQuickSortWrapper::sort<T>: итеративный движок уровней.
//
// ДЕТАЛИ PARTITION KERNEL:
//   — Pivot выбирается нитью 0 и broadcast через shared memory.
//   — Нити параллельно вычисляют predicate[i] = (data[i] < pivot) ? 1 : 0.
//   — Exclusive scan на predicate[] даёт позиции элементов < pivot.
//   — Scatter: каждая нить записывает элемент в вычисленную позицию.
//   — Элементы > pivot аналогично через обратный scan.
//   — Результат: data[lo..pivotPos-1] < pivot, data[pivotPos+1..hi] > pivot.
//
// ЯВНЫЕ ИНСТАНЦИИ: int32_t, float.
// int64_t и double: заглушка → ThrustRadixSort (они эффективнее для 8-байтовых).
////////////////////////////////////////////////////////////////////////////////
