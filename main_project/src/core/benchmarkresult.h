////////////////////////////////////////////////////////////////////////////////
// core/benchmarkresult.h — структура результата одного завершённого теста
//
// НАЗНАЧЕНИЕ:
//   Хранит все метрики и метаданные, полученные после завершения теста.
//   Используется: UI (StatsPanel, ChartWidget, ResultsTableModel),
//   экспорт (CsvExporter), история (QList<BenchmarkResult>).
//
// СТРУКТУРА BenchmarkResult:
//
//   Идентификация:
//     QString    id                    — QUuid::createUuid().toString()
//     QDateTime  timestamp             — момент завершения
//     SortParams params                — параметры запуска
//
//   Временны́е метрики (миллисекунды):
//     double cpuTimeMs                 — полное время CPU сортировки
//     double gpuTotalTimeMs            — полное время GPU (с transfer)
//     double gpuKernelTimeMs           — только execution ядра
//     double gpuH2DTimeMs              — Host→Device копирование
//     double gpuD2HTimeMs              — Device→Host копирование
//     double gpuSyncOverheadMs         — cudaDeviceSynchronize overhead
//     double arrayGenerationTimeMs     — время генерации массива
//
//   Операционные счётчики:
//     long long cpuComparisons
//     long long cpuSwaps
//     long long cpuArrayAccesses
//     long long gpuKernelLaunches
//
//   Корректность:
//     bool isSorted                    — массив отсортирован (is_sorted == true)
//     bool cpuGpuMatch                 — CPU и GPU дали одинаковый результат
//
//   Системная информация:
//     QString cpuName                  — "Intel Core i9-13900K"
//     QString gpuName                  — "NVIDIA GeForce RTX 4090"
//     int     gpuComputeCapMajor/Minor
//     size_t  gpuVramUsedBytes
//     QString cudaVersion              — "12.3"
//     QString qtVersion                — QT_VERSION_STR
//
//   Производные методы:
//     double  speedup() const          — cpuTimeMs / gpuTotalTimeMs
//     double  gpuOnlySpeedup() const   — cpuTimeMs / gpuKernelTimeMs
//     double  throughputCPU() const    — arraySize / cpuTimeMs * 1000 (Mэлем/с)
//     double  throughputGPU() const    — аналогично для GPU
//     QString summary() const          — однострочное описание
//
//   Сериализация:
//     QJsonObject          toJson() const
//     static BenchmarkResult fromJson(const QJsonObject &)
//     QStringList          toCsvRow() const
//     static QStringList   csvHeaders()
////////////////////////////////////////////////////////////////////////////////
