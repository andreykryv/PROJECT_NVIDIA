#include "comparisonbarchart.h"
#include <QtCharts/QChart>

namespace SortBench {

ComparisonBarChart::ComparisonBarChart(QWidget *parent)
    : BenchmarkChartView(parent)
{
    auto *chart = new QChart();
    chart->setTitle(tr("Сравнение CPU vs GPU"));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    m_series = new QBarSeries();
    m_cpuSet = new QBarSet(tr("CPU"));
    m_gpuKernelSet = new QBarSet(tr("GPU (kernel)"));
    m_gpuTransferSet = new QBarSet(tr("GPU (transfer)"));
    
    m_cpuSet->setColor(QColor(52, 152, 219));       // синий
    m_gpuKernelSet->setColor(QColor(39, 174, 96));  // зелёный
    m_gpuTransferSet->setColor(QColor(127, 140, 141)); // серый
    
    m_series->append(m_cpuSet);
    m_series->append(m_gpuKernelSet);
    m_series->append(m_gpuTransferSet);
    
    m_axisX = new QBarCategoryAxis();
    m_axisY = new QValueAxis();
    m_axisY->setTitleText(tr("Время (мс)"));
    m_axisY->setRange(0.0, 100.0);
    
    chart->addSeries(m_series);
    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);
    
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    
    setChart(chart);
}

void ComparisonBarChart::addResult(const BenchmarkResult &result)
{
    m_results.append(result);
    rebuildChart();
}

void ComparisonBarChart::setResults(const QList<BenchmarkResult> &results)
{
    m_results = results;
    rebuildChart();
}

void ComparisonBarChart::clear()
{
    m_results.clear();
    m_categories.clear();
    
    // Очищаем наборы данных путём удаления и повторного добавления
    m_series->remove(m_cpuSet);
    m_series->remove(m_gpuKernelSet);
    m_series->remove(m_gpuTransferSet);
    
    delete m_cpuSet;
    delete m_gpuKernelSet;
    delete m_gpuTransferSet;
    
    m_cpuSet = new QBarSet(tr("CPU"));
    m_gpuKernelSet = new QBarSet(tr("GPU (kernel)"));
    m_gpuTransferSet = new QBarSet(tr("GPU (transfer)"));
    
    m_cpuSet->setColor(QColor(52, 152, 219));       // синий
    m_gpuKernelSet->setColor(QColor(39, 174, 96));  // зелёный
    m_gpuTransferSet->setColor(QColor(127, 140, 141)); // серый
    
    m_series->append(m_cpuSet);
    m_series->append(m_gpuKernelSet);
    m_series->append(m_gpuTransferSet);
    
    m_axisX->clear();
}

void ComparisonBarChart::rebuildChart()
{
    clear();
    
    double maxValue = 0.0;
    
    for (const auto &result : m_results) {
        QString category = QString("%1\n%2")
            .arg(toString(result.params.cpuAlgorithm))
            .arg(QString::number(result.params.arraySize));
        
        m_categories << category;
        m_cpuSet->append(result.cpuTimeMs);
        m_gpuKernelSet->append(result.gpuKernelTimeMs);
        m_gpuTransferSet->append(result.gpuH2DTimeMs + result.gpuD2HTimeMs);
        
        maxValue = std::max(maxValue, result.cpuTimeMs);
        maxValue = std::max(maxValue, result.gpuTotalTimeMs);
    }
    
    m_axisX->append(m_categories);
    m_axisY->setRange(0.0, maxValue * 1.2);
}

} // namespace SortBench
