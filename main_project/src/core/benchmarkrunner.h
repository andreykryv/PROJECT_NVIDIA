////////////////////////////////////////////////////////////////////////////////
// core/benchmarkrunner.h — заголовок менеджера серийного тестирования
//
// НАЗНАЧЕНИЕ:
//   Управляет авто-серией: перебирает комбинации алгоритмов и размеров,
//   запускает их последовательно через SortBenchEngine, собирает результаты.
//
// СТРУКТУРА BatchConfig:
//   QList<CpuAlgorithm>  cpuAlgorithms
//   QList<GpuAlgorithm>  gpuAlgorithms
//   QList<int>           arraySizes     — напр. {1000, 10000, 100000, 1000000}
//   QList<DataType>      dataTypes
//   QList<Distribution>  distributions
//   int                  repeatCount    = 3
//   bool                 randomizeOrder = false
//
// КЛАСС BenchmarkRunner : public QObject
//   void startBatch(BatchConfig, SortBenchEngine*)
//   void stopBatch()
//   int  totalTests() const
//   int  completedTests() const
//   signals: batchStarted(int), testStarted(int,SortParams),
//            testFinished(int,BenchmarkResult), batchFinished(QList<BenchmarkResult>),
//            progressUpdated(int,int)
////////////////////////////////////////////////////////////////////////////////
