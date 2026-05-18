#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QList>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QAreaSeries>
#include "../core/benchmarkresult.h"

class ComparisonBarChart;
class ScatterPlotChart;

class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChartWidget(QWidget *parent = nullptr);
    ~ChartWidget() override = default;

    void addResult(const SortBench::BenchmarkResult& result);
    void clearResults();
    void setMaxResults(int max);
    bool exportChart(int tabIndex, const QString& path);
    void highlightAlgorithm(const QString& name);
    void setupChartTheme(bool isDark);

public slots:
    void rebuildCharts();
    void updateBarChart();
    void updateScatterChart();
    void updateSpeedupChart();
    void updateGPUDetailChart();

private:
    void setupTabs();
    void createBarChart();
    void createScatterChart();
    void createSpeedupChart();
    void createGPUDetailChart();

    QTabWidget *tabs;
    
    // Charts
    ComparisonBarChart *barChart;
    ScatterPlotChart *scatterChart;
    QChartView *speedupView;
    QChartView *gpuDetailView;
    
    QChart *speedupChart;
    QChart *gpuDetailChart;
    
    // Data
    QList<SortBench::BenchmarkResult> results;
    int maxStoredResults;
};

#endif // CHARTWIDGET_H
