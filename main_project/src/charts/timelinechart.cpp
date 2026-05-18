#include "timelinechart.h"
#include <QtCharts/QChart>
#include <cmath>

namespace SortBench {

TimelineChart::TimelineChart(QWidget *parent)
    : BenchmarkChartView(parent)
{
    auto *chart = new QChart();
    chart->setTitle(tr("Масштабируемость: Время vs Размер массива"));
    
    m_theoreticalNlogN = new QLineSeries();
    m_theoreticalNlogN->setName(tr("O(n log n)"));
    m_theoreticalNlogN->setPen(QPen(Qt::gray, 2, Qt::DashLine));
    
    m_theoreticalN2 = new QLineSeries();
    m_theoreticalN2->setName(tr("O(n²)"));
    m_theoreticalN2->setPen(QPen(Qt::lightGray, 2, Qt::DashDotLine));
    
    m_axisX = new QLogValueAxis();
    m_axisX->setTitleText(tr("Размер массива (лог)"));
    m_axisX->setRange(1000.0, 100000000.0);
    
    m_axisY = new QLogValueAxis();
    m_axisY->setTitleText(tr("Время (мс, лог)"));
    m_axisY->setRange(0.1, 10000.0);
    
    chart->addSeries(m_theoreticalNlogN);
    chart->addSeries(m_theoreticalN2);
    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    
    m_theoreticalNlogN->attachAxis(m_axisX);
    m_theoreticalNlogN->attachAxis(m_axisY);
    m_theoreticalN2->attachAxis(m_axisX);
    m_theoreticalN2->attachAxis(m_axisY);
    
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    
    setChart(chart);
    
    rebuildTheoreticalCurves();
}

void TimelineChart::addDataPoint(const QString &algoName, int arraySize, double timeMs)
{
    if (!m_algoSeries.contains(algoName)) {
        auto *series = new QLineSeries();
        series->setName(algoName);
        
        // Назначаем цвета алгоритмам
        static QMap<QString, QColor> colorMap;
        if (colorMap.isEmpty()) {
            colorMap["Quick Sort"] = QColor(52, 152, 219);
            colorMap["Merge Sort"] = QColor(39, 174, 96);
            colorMap["Heap Sort"] = QColor(155, 89, 182);
            colorMap["Radix Sort"] = QColor(241, 196, 15);
            colorMap["std::sort"] = QColor(230, 126, 34);
            colorMap["Bitonic Sort"] = QColor(231, 76, 60);
            colorMap["Thrust Radix Sort"] = QColor(149, 165, 166);
        }
        
        QColor color = colorMap.value(algoName, QColor(127, 127, 127));
        series->setColor(color);
        series->setPen(QPen(color, 2));
        
        chart()->addSeries(series);
        series->attachAxis(m_axisX);
        series->attachAxis(m_axisY);
        
        m_algoSeries[algoName] = series;
    }
    
    m_algoSeries[algoName]->append(static_cast<double>(arraySize), timeMs);
    
    rebuildTheoreticalCurves();
}

void TimelineChart::clear()
{
    for (auto *series : m_algoSeries) {
        series->clear();
        chart()->removeSeries(series);
        delete series;
    }
    m_algoSeries.clear();
}

void TimelineChart::setShowTheoretical(bool show)
{
    m_showTheoretical = show;
    m_theoreticalNlogN->setVisible(show);
    m_theoreticalN2->setVisible(show);
}

void TimelineChart::setAlgorithmVisible(const QString &algoName, bool visible)
{
    if (m_algoSeries.contains(algoName)) {
        m_algoSeries[algoName]->setVisible(visible);
    }
}

void TimelineChart::rebuildTheoreticalCurves()
{
    if (!m_showTheoretical) return;
    
    // Получаем диапазон данных
    double minX = m_axisX->min();
    double maxX = m_axisX->max();
    
    // Генерируем 20 точек на логарифмическом диапазоне
    m_theoreticalNlogN->clear();
    m_theoreticalN2->clear();
    
    double logMin = std::log10(minX);
    double logMax = std::log10(maxX);
    double step = (logMax - logMin) / 20.0;
    
    // Нормировка кривых по первой точке данных
    double normFactor = 1.0;
    for (auto *series : m_algoSeries) {
        if (series->count() > 0) {
            QPointF firstPoint = series->at(0);
            double n = firstPoint.x();
            double t = firstPoint.y();
            double theoretical = n * std::log2(n);
            if (theoretical > 0) {
                normFactor = t / theoretical;
                break;
            }
        }
    }
    
    for (int i = 0; i <= 20; ++i) {
        double logN = logMin + i * step;
        double n = std::pow(10.0, logN);
        
        // O(n log n)
        double nlogn = n * std::log2(n) * normFactor;
        m_theoreticalNlogN->append(n, nlogn);
        
        // O(n²) - нормируем иначе
        double n2_factor = 1e-10; // Подбираем для визуализации
        double n2 = n * n * n2_factor;
        m_theoreticalN2->append(n, n2);
    }
}

} // namespace SortBench
