////////////////////////////////////////////////////////////////////////////////
// gpu/bitonicsort.cu — реализация CUDA Bitonic Sort (компилируется nvcc)
//
// СОДЕРЖИМОЕ:
//
//   bitonicSortStep<T>:
//     Каждая нить: int i = blockIdx.x * blockDim.x + threadIdx.x.
//     int ixj = i ^ j.
//     if (ixj > i) compareAndSwap(data, i, ixj, (i & k) == 0).
//
//   bitonicSortShared<T>:
//     Загружает 2*blockDim.x элементов в __shared__ T sdata[].
//     Выполняет все шаги для размера блока через барьеры __syncthreads().
//     Записывает результат обратно в global memory.
//
//   BitonicSortWrapper::sort<T>:
//     Вычисляет paddedSize = nextPowerOfTwo(size).
//     Если paddedSize > size: заполняет T* d_pad значением std::numeric_limits<T>::max().
//     Запускает двойной цикл: for k=2..paddedSize { for j=k/2..1 { launch step } }
//     При size ≤ 2048 и умещении в shared: один запуск bitonicSortShared.
//     cudaEventElapsedTime → возвращает kernelTimeMs.
//
// ЯВНЫЕ ИНСТАНЦИИ: int32_t, int64_t, float, double.
////////////////////////////////////////////////////////////////////////////////
