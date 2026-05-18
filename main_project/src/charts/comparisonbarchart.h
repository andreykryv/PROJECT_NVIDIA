////////////////////////////////////////////////////////////////////////////////
// charts/comparisonbarchart.h — групповая столбчатая диаграмма сравнения
//
// НАЗНАЧЕНИЕ:
//   Показывает время выполнения CPU vs GPU для всех протестированных
//   алгоритмов. Основной график для сравнения производительности.
//
// КЛАСС: ComparisonBarChart : public BenchmarkChartView
//
//   ЧЛЕНЫ:
//     QBarSeries *series            — основная серия
//     QBarSet *cpuSet               — синие бары (CPU время)
//     QBarSet *gpuComputeSet        — зелёные бары (GPU kernel)
//     QBarSet *gpuTransferSet       — серые бары (GPU H2D+D2H)
//     QBarCategoryAxis *axisX
//     QValueAxis *axisY             — ось времени (мс)
//     QList<BenchmarkResult> data   — отображаемые результаты
//
//   МЕТОДЫ:
//     void addResult(BenchmarkResult)   — добавить бар и перестроить
//     void setResults(QList<BenchmarkResult>) — полная перерисовка
//     void setGroupBy(GroupBy gb)       — группировка по алгоритму / размеру / типу
//     void setLogScale(bool)            — логарифмическая ось Y
//     void showValueLabels(bool)        — подписи над барами
//     void highlightBest()              — подсветить лучший результат
//
//   ИНТЕРАКТИВНОСТЬ:
//     — Клик на бар: показывает детальный QDialog с полной BenchmarkResult.
//     — Hover: tooltip с точным значением и параметрами.
//     — DoubleClick: zoom на выбранную группу алгоритмов.
////////////////////////////////////////////////////////////////////////////////
