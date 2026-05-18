////////////////////////////////////////////////////////////////////////////////
// core/sortbenchengine.h — заголовок центрального движка тестирования
//
// НАЗНАЧЕНИЕ:
//   SortBenchEngine — QObject, живущий в рабочем потоке (QThread).
//   Управляет запуском тестов, координирует CPU/GPU ветки, собирает метрики,
//   испускает сигналы для обновления всех частей UI.
//   Все вызовы методов из UI-потока — через Qt::QueuedConnection.
//
// КЛАСС: SortBenchEngine : public QObject
//
//   enum class State {
//     Idle, GeneratingArray, SortingCPU, SortingGPU,
//     TransferringH2D, TransferringD2H, Verifying, Paused, Finished, Stopped
//   }
//
//   ЧЛЕНЫ:
//     SortParams            currentParams
//     State                 state
//     std::atomic<bool>     stopRequested
//     std::atomic<bool>     pauseRequested
//     CpuSorter            *cpuSorter
//     CudaSorter           *gpuSorter
//     QElapsedTimer         stageTimer
//     BenchmarkResult       partialResult
//     QTimer               *gpuMemPollTimer   — опрос cudaMemGetInfo каждые 500мс
//
//   ПУБЛИЧНЫЕ СЛОТЫ:
//     startBenchmark(SortParams)
//     stopBenchmark()
//     pauseBenchmark()
//     resumeBenchmark()
//
//   СИГНАЛЫ:
//     progressUpdated(int percent, QString phase)
//     frameReady(VisFrame frame)
//     benchmarkFinished(BenchmarkResult)
//     benchmarkStopped()
//     logMessage(QString, int level)    // 0=info, 1=warn, 2=error
//     gpuMemoryUpdated(size_t used, size_t total)
//     stateChanged(State)
//
//   ПРИВАТНЫЕ МЕТОДЫ:
//     void runCpuSort()
//     void runGpuSort()
//     void generateArray()
//     void verifyResults()
//     void emitFrame(const void *data, int n, QList<int> highlighted,
//                    HighlightType type, long long cmp, long long swaps)
//     void checkPausePoint()           — while(paused && !stop) sleep(10ms)
//     void collectSystemInfo()
//     void pollGpuMemory()
////////////////////////////////////////////////////////////////////////////////
