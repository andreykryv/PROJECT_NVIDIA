////////////////////////////////////////////////////////////////////////////////
// cpu/cpusorter.h — фасад всех CPU-алгоритмов сортировки
//
// НАЗНАЧЕНИЕ:
//   CpuSorter — единая точка вызова для всех CPU-алгоритмов. Принимает
//   параметры теста и callback для анимации, делегирует конкретному алгоритму.
//
// КЛАСС: CpuSorter
//
//   using SortCallback = std::function<void(
//       const void* data,          // указатель на массив (для копирования кадра)
//       int size,                  // число элементов
//       int idx1, int idx2,        // индексы "активных" элементов
//       HighlightType type,        // тип события
//       long long comparisons,     // накопленных сравнений
//       long long swaps            // накопленных перестановок
//   )>
//
//   МЕТОД:
//   template<typename T>
//   void sort(T* data, int size, CpuAlgorithm algo,
//             SortCallback callback, std::atomic<bool>& stop);
//     — Диспатч через switch(algo) на конкретную реализацию.
//     — Callback вызывается изнутри алгоритма после каждого события.
//     — stop проверяется в горячем цикле; при stop==true алгоритм прерывается.
//
//   ВСПОМОГАТЕЛЬНЫЙ МЕТОД:
//   bool shouldEmitFrame(std::chrono::steady_clock::time_point& lastEmit,
//                        int targetFPS);
//     — Возвращает true не чаще чем 1000/targetFPS мс. Используется всеми
//       алгоритмами для ограничения частоты вызова callback.
////////////////////////////////////////////////////////////////////////////////
