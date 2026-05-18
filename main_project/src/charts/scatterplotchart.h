////////////////////////////////////////////////////////////////////////////////
// charts/scatterplotchart.h — scatter plot для детального сравнения
//
// НАЗНАЧЕНИЕ:
//   Отображает все измеренные точки как scatter plot с соединяющими линиями.
//   Каждая точка = один запуск теста. Позволяет видеть разброс при повторных
//   запусках и выбросы (outliers).
//
// КЛАСС: ScatterPlotChart : public BenchmarkChartView
//
//   Использует QScatterSeries для точек и QLineSeries для линий тренда.
//   Размер маркера = std::clamp(8, 4, 16): крупнее для CPU, меньше для GPU.
//   Форма маркера: CPU → круг, GPU → ромб (VMarker).
//   При hover на точку: popup с полными данными BenchmarkResult.
//
//   МЕТОДЫ:
//     void addResult(BenchmarkResult)
//     void setXAxis(XAxisMode) — TIME_AXIS / ARRAY_SIZE_AXIS
//     void setShowTrend(bool)  — линии тренда (линейная регрессия)
//     void setShowOutliers(bool) — подсветить выбросы (> 2σ от среднего)
////////////////////////////////////////////////////////////////////////////////
