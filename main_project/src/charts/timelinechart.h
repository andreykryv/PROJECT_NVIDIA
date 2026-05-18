////////////////////////////////////////////////////////////////////////////////
// charts/timelinechart.h — график зависимости время vs размер массива
//
// НАЗНАЧЕНИЕ:
//   Scatter+Line Plot: X — размер массива (логарифм), Y — время (логарифм).
//   Позволяет видеть, как масштабируются алгоритмы.
//   Включает теоретические кривые O(n log n) и O(n²) для ориентира.
//
// КЛАСС: TimelineChart : public BenchmarkChartView
//
//   ЧЛЕНЫ:
//     QMap<QString, QLineSeries*> algoSeries  — по одной серии на алгоритм
//     QLineSeries *theoreticalNlogN
//     QLineSeries *theoreticalN2
//     QLogValueAxis *axisX, *axisY
//
//   МЕТОДЫ:
//     void addDataPoint(QString algoName, int arraySize, double timeMs)
//     void rebuildTheoreticalCurves()    — перестроить кривые под текущий диапазон
//     void setShowTheoretical(bool)      — показать/скрыть теоретические кривые
//     void setAlgorithmVisible(QString, bool)  — скрыть/показать отдельный алгоритм
//
//   ТЕОРЕТИЧЕСКИЕ КРИВЫЕ:
//     Нормируются по первой точке данных, чтобы проходить через неё.
//     Рисуются пунктиром (QPen::DashLine) для отличия от измеренных данных.
////////////////////////////////////////////////////////////////////////////////
