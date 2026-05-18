////////////////////////////////////////////////////////////////////////////////
// core/sortparams.h — структура параметров одного запуска теста
//
// НАЗНАЧЕНИЕ:
//   Единый POD-объект, описывающий полную конфигурацию теста.
//   Передаётся между UI (ControlPanel), движком (SortBenchEngine) и
//   алгоритмами (CpuSorter, CudaSorter).
//
// ПЕРЕЧИСЛЕНИЯ:
//   enum class CpuAlgorithm { BubbleSort, QuickSort, MergeSort, HeapSort,
//                              RadixSort, StdSort, None }
//   enum class GpuAlgorithm { BitonicSort, ThrustRadixSort, GpuQuickSort,
//                              CubDeviceSort, None }
//   enum class DataType     { Int32, Int64, Float, Double }
//   enum class Distribution { RandomUniform, NearlySorted, ReverseSorted,
//                              ManyDuplicates, Sawtooth, SteppedNoise, RandomNormal }
//   enum class ColorScheme  { Rainbow, Heatmap, Monochrome, StatusColors }
//
// СТРУКТУРА SortParams (все поля имеют разумные значения по умолчанию):
//   CpuAlgorithm cpuAlgorithm     = QuickSort
//   GpuAlgorithm gpuAlgorithm     = ThrustRadixSort
//   bool         enableCPU        = true
//   bool         enableGPU        = true
//   int          arraySize        = 100'000
//   DataType     dataType         = Int32
//   Distribution distribution     = RandomUniform
//   unsigned int randomSeed       = 42
//   bool         autoSeed         = false
//   int          animationFPS     = 60       // 0 = пошаговый режим
//   bool         showComparisons  = true
//   bool         showAccessCount  = false
//   ColorScheme  colorScheme      = Rainbow
//   int          maxVisElements   = 1000
//   int          repeatCount      = 1        // повторений для усреднения
//   bool         excludeOutliers  = false
//   int          cudaBlockSize    = 256
//   int          cudaStreams       = 4
//   bool         usePinnedMemory  = true
//
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (free functions в том же файле):
//   QString toString(CpuAlgorithm)     — "Quick Sort"
//   QString toString(GpuAlgorithm)     — "Thrust Radix Sort"
//   QString toString(DataType)         — "int32"
//   QString toString(Distribution)     — "Случайное равномерное"
//   size_t  elementSize(DataType)      — 4, 8, 4, 8 (байт)
//   size_t  arrayBytes(SortParams)     — arraySize * elementSize(dataType)
//   bool    operator==(SortParams,...)
//   QDebug  operator<<(QDebug, SortParams)  — удобный вывод в лог
////////////////////////////////////////////////////////////////////////////////
