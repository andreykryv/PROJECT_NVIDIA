////////////////////////////////////////////////////////////////////////////////
// cpu/quicksort.h — реализация Quick Sort (3-way partition) для CPU
//
// КЛАСС: QuickSort<T>
//
//   void sort(T* data, int size, cb, stop)
//     — Запускает рекурсивный quicksort3Way(data, 0, size-1, cb, stop).
//
//   void quicksort3Way(T* data, int lo, int hi, cb, stop):
//     — Pivot = data[lo + (hi-lo)/2] (медиана из трёх для лучшего выбора).
//     — Трёхпутевое разделение: lt, gt, i указатели.
//     — Элементы < pivot: [lo..lt-1]; == pivot: [lt..gt]; > pivot: [gt+1..hi].
//     — cb(PIVOT, pivotIdx, ...) — подсветка опорного элемента.
//     — cb(COMPARE, i, pivotIdx, ...) и cb(SWAP, ...) при каждом действии.
//     — Для коротких подмассивов (hi - lo < 10): insertionSort (оптимизация).
//     — Рекурсия: сначала меньший подмассив (stack depth O(log n)).
//     — stop проверяется перед рекурсией.
//
//   void insertionSort(T* data, int lo, int hi, cb, stop):
//     — Линейная вставка для малых подмассивов ≤ 10 элементов.
////////////////////////////////////////////////////////////////////////////////
