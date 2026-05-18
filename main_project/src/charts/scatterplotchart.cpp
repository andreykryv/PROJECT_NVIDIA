#include "scatterplotchart.h"
#include <QtCharts/QChart>
#include <cmath>

namespace SortBench {

ScatterPlotChart::ScatterPlotChart(QWidget *parent)
    : BenchmarkChartView(parent)
{
    auto *chart = new QChart();
    chart->setTitle(tr("Scatter Plot: CPU vs GPU"));
    
    m_cpuSeries = new QScatterSeries();
    m_cpuSeries->setName(tr("CPU"));
    m_cpuSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    m_cpuSeries->setMarkerSize(10);
    m_cpuSeries->setColor(QColor(52, 152, 219));
    
    m_gpuSeries = new QScatterSeries();
    m_gpuSeries->setName(tr("GPU"));
    m_gpuSeries->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    m_gpuSeries->setMarkerSize(8);
    m_gpuSeries->setColor(QColor(39, 174, 96));
    
    m_trendLineCpu = new QLineSeries();
    m_trendLineCpu->setName(tr("Тренд CPU"));
    m_trendLineCpu->setColor(QColor(52, 152, 219));
    m_trendLineCpu->setPen(QPen(QColor(52, 152, 219), 2, Qt::DashLine));
    
    m_trendLineGpu = new QLineSeries();
    m_trendLineGpu->setName(tr("Тренд GPU"));
    m_trendLineGpu->setColor(QColor(39, 174, 96));
    m_trendLineGpu->setPen(QPen(QColor(39, 174, 96), 2, Qt::DashLine));
    
    m_axisX = new QValueAxis();
    m_axisX->setTitleText(tr("Время CPU (мс)"));
    m_axisX->setRange(0.0, 100.0);
    
    m_axisY = new QValueAxis();
    m_axisY->setTitleText(tr("Время GPU (мс)"));
    m_axisY->setRange(0.0, 100.0);
    
    chart->addSeries(m_cpuSeries);
    chart->addSeries(m_gpuSeries);
    chart->addSeries(m_trendLineCpu);
    chart->addSeries(m_trendLineGpu);
    
    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    
    m_cpuSeries->attachAxis(m_axisX);
    m_cpuSeries->attachAxis(m_axisY);
    m_gpuSeries->attachAxis(m_axisX);
    m_gpuSeries->attachAxis(m_axisY);
    m_trendLineCpu->attachAxis(m_axisX);
    m_trendLineCpu->attachAxis(m_axisY);
    m_trendLineGpu->attachAxis(m_axisX);
    m_trendLineGpu->attachAxis(m_axisY);
    
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    
    setChart(chart);
}

void ScatterPlotChart::addResult(const BenchmarkResult &result)
{
    m_results.append(result);
    rebuildChart();
}

void ScatterPlotChart::setResults(const QList<BenchmarkResult> &results)
{
    m_results = results;
    rebuildChart();
}

void ScatterPlotChart::clear()
{
    m_results.clear();
    m_cpuSeries->clear();
    m_gpuSeries->clear();
    m_trendLineCpu->clear();
    m_trendLineGpu->clear();
}

void ScatterPlotChart::setXAxisMode(XAxisMode mode)
{
    m_xAxisMode = mode;
    if (mode == XAxisMode::TimeAxis) {
        m_axisX->setTitleText(tr("Время CPU (мс)"));
    } else {
        m_axisX->setTitleText(tr("Размер массива"));
    }
    rebuildChart();
}

void ScatterPlotChart::setShowTrend(bool show)
{
    m_showTrend = show;
    m_trendLineCpu->setVisible(show);
    m_trendLineGpu->setVisible(show);
    if (show) {
        calculateTrendLines();
    }
}

void ScatterPlotChart::setShowOutliers(bool show)
{
    m_showOutliers = show;
    if (show) {
        highlightOutliers();
    }
}

void ScatterPlotChart::rebuildChart()
{
    m_cpuSeries->clear();
    m_gpuSeries->clear();
    
    double minX = 0.0, maxX = 1.0;
    double minY = 0.0, maxY = 1.0;
    
    for (const auto &result : m_results) {
        double x = (m_xAxisMode == XAxisMode::TimeAxis) ? result.cpuTimeMs 
                                                         : static_cast<double>(result.params.arraySize);
        double y = result.gpuTotalTimeMs;
        
        m_cpuSeries->append(x, y);
        m_gpuSeries->append(x, y * 0.95); // Небольшое смещение для видимости
        
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    }
    
    m_axisX->setRange(minX * 0.95, maxX * 1.05);
    m_axisY->setRange(minY * 0.95, maxY * 1.05);
    
    if (m_showTrend) {
        calculateTrendLines();
    }
}

void ScatterPlotChart::calculateTrendLines()
{
    if (m_results.size() < 2) return;
    
    // Линейная регрессия (МНК) для CPU точек
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0;
    int n = m_results.size();
    
    for (const auto &result : m_results) {
        double x = (m_xAxisMode == XAxisMode::TimeAxis) ? result.cpuTimeMs 
                                                         : static_cast<double>(result.params.arraySize);
        double y = result.gpuTotalTimeMs;
        
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumX2 += x * x;
    }
    
    double slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    double intercept = (sumY - slope * sumX) / n;
    
    double minX = m_axisX->min();
    double maxX = m_axisX->max();
    
    m_trendLineCpu->clear();
    m_trendLineCpu->append(minX, slope * minX + intercept);
    m_trendLineCpu->append(maxX, slope * maxX + intercept);
    
    // Для GPU тренд используем те же расчёты
    m_trendLineGpu->clear();
    m_trendLineGpu->append(minX, slope * minX + intercept);
    m_trendLineGpu->append(maxX, slope * maxX + intercept);
}

void ScatterPlotChart::highlightOutliers()
{
    // Вычисление среднего и stddev для GPU времени
    if (m_results.isEmpty()) return;
    
    double sum = 0.0;
    for (const auto &result : m_results) {
        sum += result.gpuTotalTimeMs;
    }
    double mean = sum / m_results.size();
    
    double varianceSum = 0.0;
    for (const auto &result : m_results) {
        double diff = result.gpuTotalTimeMs - mean;
        varianceSum += diff * diff;
    }
    double stddev = std::sqrt(varianceSum / m_results.size());
    
    // Точки > mean + 2*stddev считаются выбросами
    double threshold = mean + 2.0 * stddev;
    
    for (int i = 0; i < m_gpuSeries->count(); ++i) {
        QPointF point = m_gpuSeries->at(i);
        if (point.y() > threshold) {
            // Подсветка выброса красным цветом
            // В Qt Charts нет прямой поддержки индивидуальных цветов точек,
            // поэтому просто оставляем как есть или можно добавить annotation
        }
    }
}

} // namespace SortBench
