////////////////////////////////////////////////////////////////////////////////
// core/arraygenerator.h — шаблонный генератор тестовых массивов
//
// НАЗНАЧЕНИЕ:
//   Создаёт std::vector<T> заданного распределения для тестирования.
//   Шаблонизирован по типу T: int32_t, int64_t, float, double.
//
// МЕТОДЫ (все статические, шаблонные):
//
//   generate<T>(size, dist, seed) → std::vector<T>
//     Диспатчит на один из специализированных методов ниже.
//
//   generateRandomUniform<T>(size, seed)
//     std::uniform_int_distribution / uniform_real_distribution
//     Диапазон: [0, max_value_for_type / 2] для равномерного покрытия.
//
//   generateNearlySorted<T>(size, seed)
//     Отсортированный массив + ~2% случайных swap (size/50 инверсий).
//
//   generateReverseSorted<T>(size)
//     std::iota([0..size-1]) затем std::reverse.
//
//   generateManyDuplicates<T>(size, seed)
//     Пул из √size уникальных значений, массив заполняется случайно из пула.
//
//   generateSawtooth<T>(size)
//     Период = √size. Значение[i] = (i % period) * (maxVal / period).
//
//   generateSteppedNoise<T>(size, seed)
//     10 ступеней; внутри каждой — равномерный шум ±10% от шага ступени.
//
//   generateRandomNormal<T>(size, seed, mean=0.5, stddev=0.15)
//     Box-Muller через std::normal_distribution<double>, clamp+cast к T.
//
//   Утилиты:
//     isSorted<T>(vector)  → bool
//     equals<T>(v1, v2)    → bool
//     minMax<T>(vector)    → std::pair<T,T>
//     describeDistribution(Distribution, size) → QString  (для лога)
////////////////////////////////////////////////////////////////////////////////
