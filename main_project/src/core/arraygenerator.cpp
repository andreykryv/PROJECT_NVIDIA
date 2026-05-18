////////////////////////////////////////////////////////////////////////////////
// core/arraygenerator.cpp — реализация генератора массивов
//
// Явные инстанции шаблонов внизу файла:
//   template class ArrayGenerator<int32_t>;
//   template class ArrayGenerator<int64_t>;
//   template class ArrayGenerator<float>;
//   template class ArrayGenerator<double>;
//
// Оптимизация для больших массивов (> 1M элементов):
//   Используется std::execution::par_unseq для параллельного заполнения.
//   Для параллельного std::transform нужен потокобезопасный ГПСЧ:
//   каждый поток получает свой mt19937 с seed = baseSeed + threadIndex.
//
// generateNearlySorted(): использует Fisher-Yates shuffle на подмножестве.
// generateManyDuplicates(): std::uniform_int_distribution для индекса в пуле.
// equals(): std::memcmp для скорости (только для trivially copyable типов).
////////////////////////////////////////////////////////////////////////////////
