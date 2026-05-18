////////////////////////////////////////////////////////////////////////////////
// core/sortbenchengine.cpp — реализация движка тестирования
//
// КОНСТРУКТОР:
//   Создаёт CpuSorter и CudaSorter. Запускает gpuMemPollTimer (500мс).
//   Подключает gpuMemPollTimer::timeout → pollGpuMemory().
//
// startBenchmark() — главный метод, вызывается из потока движка:
//   1.  setState(GeneratingArray)
//   2.  generateArray() — ArrayGenerator::generate<T>(size, dist, seed)
//   3.  emit progressUpdated(10, "CPU: " + name) → runCpuSort()
//       — Передаёт в CpuSorter лямбду-callback для генерации кадров анимации.
//       — Callback вызывается после каждого сравнения/перестановки.
//       — Ограничение частоты кадров: пропускает callback если прошло < 1000/fps мс.
//       — Записывает cpuTimeMs = stageTimer.elapsed().
//   4.  emit progressUpdated(55, "GPU H2D") → runGpuSort():
//       a. cudaEventCreate для 4 точек замера (H2D start/stop, kernel start/stop)
//       b. cudaMemcpy или cudaMemcpyAsync (pinned) Host→Device, запись gpuH2DTimeMs
//       c. Вызов CudaSorter::sort() с записью gpuKernelTimeMs
//       d. cudaMemcpy Device→Host, запись gpuD2HTimeMs
//       e. cudaDeviceSynchronize + замер gpuSyncOverheadMs
//   5.  emit progressUpdated(90, "Верификация") → verifyResults()
//       — std::is_sorted на результирующем массиве.
//       — Побайтовое std::equal между CPU и GPU результатами.
//   6.  collectSystemInfo() — cudaGetDeviceProperties, /proc/cpuinfo
//   7.  emit benchmarkFinished(partialResult)
//
// runCpuSort() — диспатч на нужный алгоритм по params.cpuAlgorithm:
//   Через std::visit или switch. Каждый алгоритм принимает:
//   T* data, int size, SortCallback callback, std::atomic<bool>& stopFlag.
//
// emitFrame():
//   Строит VisFrame: нормализует значения в [0,1] через min/max массива.
//   Заполняет highlightedIdx и highlightType. emit frameReady(frame).
//   Логгирует кадр если SORTBENCH_DEBUG_ANIMATION включён.
//
// checkPausePoint():
//   while (pauseRequested.load() && !stopRequested.load()) {
//       QThread::msleep(10); QCoreApplication::processEvents(); }
////////////////////////////////////////////////////////////////////////////////
