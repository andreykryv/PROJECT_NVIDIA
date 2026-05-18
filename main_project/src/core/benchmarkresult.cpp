////////////////////////////////////////////////////////////////////////////////
// core/benchmarkresult.cpp — реализация вспомогательных методов BenchmarkResult
//
// speedup():     защита от деления на ноль; 0.0 если gpuTotalTimeMs == 0.
// gpuOnlySpeedup(): аналогично по gpuKernelTimeMs.
// summary():     "QuickSort(CPU) 12.3мс vs BitonicSort(GPU) 2.1мс → 5.86x @ 100K int32"
// toJson():      сериализует все 25+ полей; числа с 6 знаками после запятой.
// fromJson():    QJsonObject::value().toDouble(0.0) — устойчив к отсутствующим полям.
// toCsvRow():    25 полей в фиксированном порядке (совпадает с csvHeaders()).
// csvHeaders():  static const QStringList с заголовками колонок CSV.
////////////////////////////////////////////////////////////////////////////////
