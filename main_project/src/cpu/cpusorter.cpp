////////////////////////////////////////////////////////////////////////////////
// cpu/cpusorter.cpp — реализация фасада CPU-алгоритмов
//
// sort<T>() через switch(algo) создаёт экземпляр нужного класса алгоритма
// и вызывает его метод sort(data, size, callback, stop).
// Явные инстанции шаблона: int32_t, int64_t, float, double.
//
// shouldEmitFrame():
//   auto now = std::chrono::steady_clock::now();
//   if (now - lastEmit >= targetInterval) { lastEmit = now; return true; }
//   return false;
////////////////////////////////////////////////////////////////////////////////
