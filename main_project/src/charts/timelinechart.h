#ifndef TIMELINECHART_H
#define TIMELINECHART_H

#include "charts/benchmarkchartview.h"
#include <QtCharts/QLineSeries>
#include <QtCharts/QLogValueAxis>
#include <QMap>

namespace SortBench {

class TimelineChart : public BenchmarkChartView
{
    Q_OBJECT

public:
    explicit TimelineChart(QWidget *parent = nullptr);
    
    void addDataPoint(const QString &algoName, int arraySize, double timeMs);
    void clear();
    
    void setShowTheoretical(bool show);
    void setAlgorithmVisible(const QString &algoName, bool visible);

private:
    void rebuildTheoreticalCurves();
    
    QMap<QString, QLineSeries*> m_algoSeries;
    QLineSeries *m_theoreticalNlogN;
    QLineSeries *m_theoreticalN2;
    QLogValueAxis *m_axisX;
    QLogValueAxis *m_axisY;
    
    bool m_showTheoretical = true;
};

} // namespace SortBench

#endif // TIMELINECHART_H
