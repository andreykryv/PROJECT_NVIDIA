#ifndef COMPARISONBARCHART_H
#define COMPARISONBARCHART_H

#include "charts/benchmarkchartview.h"
#include "core/benchmarkresult.h"
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QList>

namespace SortBench {

class ComparisonBarChart : public BenchmarkChartView
{
    Q_OBJECT

public:
    explicit ComparisonBarChart(QWidget *parent = nullptr);
    
    void addResult(const BenchmarkResult &result);
    void setResults(const QList<BenchmarkResult> &results);
    void clear();

private:
    void rebuildChart();
    
    QBarSeries *m_series;
    QBarSet *m_cpuSet;
    QBarSet *m_gpuKernelSet;
    QBarSet *m_gpuTransferSet;
    QBarCategoryAxis *m_axisX;
    QValueAxis *m_axisY;
    
    QList<BenchmarkResult> m_results;
    QStringList m_categories;
};

} // namespace SortBench

#endif // COMPARISONBARCHART_H
