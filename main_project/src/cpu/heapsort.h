////////////////////////////////////////////////////////////////////////////////
// cpu/heapsort.h — реализация Heap Sort для CPU
//
// КЛАСС: HeapSort<T>
//
//   void sort(T* data, int size, cb, stop):
//     — Фаза 1: buildMaxHeap(data, size) — heapify снизу вверх.
//       cb(COMPARE, parent, child, ...) при каждом siftDown.
//     — Фаза 2: n-1 итераций:
//       swap(data[0], data[i]); siftDown(data, 0, i-1);
//       cb(SWAP, 0, i, ...) — корень на место.
//       cb(SORTED, i, -1, ...) — элемент занял финальную позицию.
//
//   void siftDown(T* data, int root, int end, cb, stop):
//     — Стандартный sift-down. cb(COMPARE/SWAP) при каждом шаге.
//
//   ОСОБЕННОСТИ АНИМАЦИИ:
//     — Фаза buildHeap хаотична на вид, фаза извлечения упорядочивает с конца.
//     — Хорошо заметна структура дерева при использовании ColorScheme::Heatmap.
////////////////////////////////////////////////////////////////////////////////
