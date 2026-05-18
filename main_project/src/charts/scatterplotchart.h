#ifndef SCATTERPLOTCHART_H
#define SCATTERPLOTCHART_H

#include "charts/benchmarkchartview.h"
#include <QtCharts/QScatterSeries>
#include <QtCharts/QLineSeries>
#include <QList>

namespace SortBench {

enum class XAxisMode {
    TimeAxis,
    ArraySizeAxis
};

class ScatterPlotChart : public BenchmarkChartView
{
    Q_OBJECT

public:
    explicit ScatterPlotChart(QWidget *parent = nullptr);
    
    void addResult(const BenchmarkResult &result);
    void setResults(const QList<BenchmarkResult> &results);
    void clear();
    
    void setXAxisMode(XAxisMode mode);
    void setShowTrend(bool show);
    void setShowOutliers(bool show);

private:
    void rebuildChart();
    void calculateTrendLines();
    void highlightOutliers();
    
    QScatterSeries *m_cpuSeries;
    QScatterSeries *m_gpuSeries;
    QLineSeries *m_trendLineCpu;
    QLineSeries *m_trendLineGpu;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    
    QList<BenchmarkResult> m_results;
    XAxisMode m_xAxisMode = XAxisMode::TimeAxis;
    bool m_showTrend = false;
    bool m_showOutliers = false;
};

} // namespace SortBench

#endif // SCATTERPLOTCHART_H
