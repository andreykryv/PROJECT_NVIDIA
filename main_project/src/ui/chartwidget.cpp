////////////////////////////////////////////////////////////////////////////////
// ui/chartwidget.cpp — реализация виджета графиков
//
// СОДЕРЖИМОЕ:
//   Реализует построение и обновление всех QtCharts-графиков.
//
// ПОСТРОЕНИЕ barChart:
//   — QBarSeries с тремя наборами QBarSet: "CPU", "GPU compute", "GPU transfer".
//   — QBarCategoryAxis для оси X: имена алгоритмов.
//   — QValueAxis для оси Y: диапазон 0..maxTime автоматически.
//   — QChart::setAnimationOptions(SeriesAnimations) — плавное появление баров.
//   — QChartView с политикой масштабирования: RubberBandZoom.
//
// ПОСТРОЕНИЕ scatterChart:
//   — QLineSeries для каждого алгоритма + каждого типа (CPU/GPU).
//   — QLogValueAxis по обоим осям для охвата 5 порядков величины.
//   — Маркеры точек: круги диаметром 8px.
//   — При наведении на точку: tooltip с точным значением.
//   — Теоретические кривые добавляются как QLineSeries с setDashDotLine.
//
// ПОСТРОЕНИЕ speedupChart:
//   — QAreaSeries поверх QLineSeries для закрашивания области.
//   — Заливка цветом: зелёный (ускорение > 1) или красный (< 1).
//   — QValueAxis Y с setRange(0, maxSpeedup * 1.1).
//
// ПОСТРОЕНИЕ gpuDetailChart:
//   — QStackedBarSeries с 4 наборами.
//   — Легенда снизу (QLegend::Alignment::Bottom).
//   — Цвета: H2D=#4A90D9, Kernel=#27AE60, D2H=#E67E22, Sync=#E74C3C.
//
// ЭКСПОРТ:
//   — grab() на QChartView даёт QPixmap, который сохраняется через QPixmap::save().
//   — Для SVG: QSvgGenerator + QChart::paint().
//   — Для PDF: QPrinter + QChart::paint().
//
// ТЕМА:
//   — isDark=true: QChart::ChartThemeDark + настройка шрифтов и цвета фона.
//   — isDark=false: QChart::ChartThemeLight.
//   — Кастомные цвета серий задаются вручную через QPen/QBrush независимо от темы.
////////////////////////////////////////////////////////////////////////////////
