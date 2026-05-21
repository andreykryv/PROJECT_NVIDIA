#include "chartwidget.h"
#include "charts/comparisonbarchart.h"
#include "charts/scatterplotchart.h"
#include "core/sortparams.h"
#include <QVBoxLayout>
#include <QChartView>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QLogValueAxis>
#include <QLineSeries>
#include <QScatterSeries>
#include <QAreaSeries>
#include <QStackedBarSeries>
#include <QDateTime>
#include <QFile>
#include <QSvgGenerator>
#include <QPainter>

namespace SortBench {

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
    , barChart(nullptr)
    , scatterChart(nullptr)
    , speedupView(nullptr)
    , gpuDetailView(nullptr)
    , speedupChart(nullptr)
    , gpuDetailChart(nullptr)
    , maxStoredResults(50)
{
    setupTabs();
    createBarChart();
    createScatterChart();
    createSpeedupChart();
    createGPUDetailChart();
    setupChartTheme(false);
}

void ChartWidget::setupTabs() {
    tabs = new QTabWidget(this);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    setLayout(layout);
}

void ChartWidget::createBarChart() {
    barChart = new SortBench::ComparisonBarChart(this);
    barChart->setTitle("Время выполнения (мс)");
    tabs->addTab(barChart, "Время выполнения");  // use barChart directly
}

void ChartWidget::createScatterChart() {
    scatterChart = new SortBench::ScatterPlotChart(this);
    scatterChart->setTitle("Масштабируемость");
    tabs->addTab(scatterChart, "Масштабируемость");  // use scatterChart directly
}

void ChartWidget::createSpeedupChart() {
    speedupChart = new QChart();
    speedupChart->setTitle("Ускорение GPU (T_cpu / T_gpu)");
    speedupChart->setAnimationOptions(QChart::SeriesAnimations);
    
    // Горизонтальная линия y=1
    auto *baseline = new QLineSeries();
    baseline->append(0, 1.0);
    baseline->append(100000000, 1.0);
    baseline->setName("Безубыточность");
    QPen baselinePen(Qt::gray);
    baselinePen.setStyle(Qt::DashLine);
    baseline->setPen(baselinePen);
    speedupChart->addSeries(baseline);
    
    auto *axisX = new QLogValueAxis();
    axisX->setTitleText("Размер массива");
    axisX->setMin(100);
    axisX->setMax(100000000);
    speedupChart->addAxis(axisX, Qt::AlignBottom);
    
    auto *axisY = new QValueAxis();
    axisY->setTitleText("Ускорение (раз)");
    axisY->setRange(0, 10);
    speedupChart->addAxis(axisY, Qt::AlignLeft);
    
    speedupView = new QChartView(speedupChart);
    speedupView->setRenderHint(QPainter::Antialiasing);
    tabs->addTab(speedupView, "Ускорение GPU");
}

void ChartWidget::createGPUDetailChart() {
    gpuDetailChart = new QChart();
    gpuDetailChart->setTitle("Детали GPU времени");
    gpuDetailChart->setAnimationOptions(QChart::SeriesAnimations);
    
    auto *stackedSeries = new QStackedBarSeries();
    stackedSeries->setName("GPU компоненты");
    
    // 4 набора: H2D, Kernel, D2H, Sync
    QVector<QBarSet*> sets;
    QStringList labels = {"H2D", "Kernel", "D2H", "Sync"};
    QVector<QColor> colors = {QColor("#4A90D9"), QColor("#27AE60"), 
                               QColor("#E67E22"), QColor("#E74C3C")};
    
    for (int i = 0; i < 4; ++i) {
        auto *barSet = new QBarSet(labels[i]);
        barSet->setColor(colors[i]);
        sets.append(barSet);
        stackedSeries->append(barSet);
    }
    
    gpuDetailChart->addSeries(stackedSeries);
    
    auto *axisX = new QBarCategoryAxis();
    axisX->append("Алгоритмы");
    gpuDetailChart->addAxis(axisX, Qt::AlignBottom);
    stackedSeries->attachAxis(axisX);
    
    auto *axisY = new QValueAxis();
    axisY->setTitleText("Время (мс)");
    gpuDetailChart->addAxis(axisY, Qt::AlignLeft);
    stackedSeries->attachAxis(axisY);
    
    gpuDetailChart->legend()->setAlignment(Qt::AlignBottom);
    
    gpuDetailView = new QChartView(gpuDetailChart);
    gpuDetailView->setRenderHint(QPainter::Antialiasing);
    tabs->addTab(gpuDetailView, "Детали GPU");
}

void ChartWidget::addResult(const SortBench::BenchmarkResult& result) {
    results.append(result);
    
    // Ограничиваем количество результатов
    if (results.size() > maxStoredResults) {
        results.removeFirst();
    }
    
    rebuildCharts();
}

void ChartWidget::clearResults() {
    results.clear();
    rebuildCharts();
}

void ChartWidget::setMaxResults(int max) {
    maxStoredResults = max;
    while (results.size() > maxStoredResults) {
        results.removeFirst();
    }
    rebuildCharts();
}

void ChartWidget::rebuildCharts() {
    updateBarChart();
    updateScatterChart();
    updateSpeedupChart();
    updateGPUDetailChart();
}

void ChartWidget::updateBarChart() {
    if (!barChart) return;
    barChart->setResults(results);
}

void ChartWidget::updateScatterChart() {
    if (!scatterChart) return;
    scatterChart->setResults(results);
}

void ChartWidget::updateSpeedupChart() {
    if (!speedupChart) return;
    
    // Удаляем старые серии (кроме baseline)
    auto seriesList = speedupChart->series();
    for (auto *series : seriesList) {
        if (qobject_cast<QLineSeries*>(series)) {
            auto *lineSeries = qobject_cast<QLineSeries*>(series);
            if (lineSeries->name() != "Безубыточность") {
                speedupChart->removeSeries(series);
                delete series;
            }
        } else if (qobject_cast<QAreaSeries*>(series)) {
            speedupChart->removeSeries(series);
            delete series;
        }
    }
    
    // Группируем результаты по размеру массива
    QMap<int, QList<const SortBench::BenchmarkResult*>> bySize;
    for (const auto& res : results) {
        bySize[res.params.arraySize].append(&res);
    }
    
    // Создаём серию для каждого алгоритма
    QMap<QString, QLineSeries*> algoSeries;
    
    for (auto it = bySize.begin(); it != bySize.end(); ++it) {
        int size = it.key();
        for (const auto *res : it.value()) {
            QString algoName = SortBench::toString(res->params.cpuAlgorithm);
            
            if (!algoSeries.contains(algoName)) {
                algoSeries[algoName] = new QLineSeries();
                algoSeries[algoName]->setName(algoName);
                
                // Цвет в зависимости от CPU/GPU - определяем по gpuKernelTimeMs
                bool isGpu = res->gpuKernelTimeMs > 0.0;
                if (isGpu) {
                    algoSeries[algoName]->setPen(QPen(QColor("#27AE60"), 2));
                } else {
                    algoSeries[algoName]->setPen(QPen(QColor("#3498DB"), 2));
                }
                
                speedupChart->addSeries(algoSeries[algoName]);
                algoSeries[algoName]->attachAxis(speedupChart->axes(Qt::Horizontal).first());
                algoSeries[algoName]->attachAxis(speedupChart->axes(Qt::Vertical).first());
            }
        }
    }
    
    // Заполняем точки
    for (auto it = bySize.begin(); it != bySize.end(); ++it) {
        int size = it.key();
        
        // Находим CPU и GPU времена для этого размера
        double cpuTime = -1, gpuTime = -1;
        for (const auto *res : it.value()) {
            bool isGpu = res->gpuKernelTimeMs > 0.0;
            if (!isGpu && cpuTime < 0) {
                cpuTime = res->cpuTimeMs;
            }
            if (isGpu && gpuTime < 0) {
                gpuTime = res->gpuTotalTimeMs;
            }
        }
        
        if (cpuTime > 0 && gpuTime > 0) {
            double speedup = cpuTime / gpuTime;
            for (auto *series : algoSeries) {
                series->append(size, speedup);
            }
        }
    }
}

void ChartWidget::updateGPUDetailChart() {
    if (!gpuDetailChart) return;
    
    auto *stackedSeries = qobject_cast<QStackedBarSeries*>(gpuDetailChart->series().first());
    if (!stackedSeries || stackedSeries->barSets().isEmpty()) return;
    
    auto sets = stackedSeries->barSets();
    // Очищаем наборы, создавая новые значения (в Qt6 нет clear())
    for (auto *barSet : sets) {
        // Удаляем все значения и добавляем заново
        while (barSet->count() > 0) {
            barSet->remove(0);
        }
    }
    
    // Фильтруем только GPU результаты (где gpuKernelTimeMs > 0)
    QList<const SortBench::BenchmarkResult*> gpuResults;
    for (const auto& res : results) {
        if (res.gpuKernelTimeMs > 0.0) {
            gpuResults.append(&res);
        }
    }
    
    if (gpuResults.isEmpty()) return;
    
    // Добавляем данные для каждого результата
    for (int i = 0; i < gpuResults.size(); ++i) {
        const auto *res = gpuResults[i];
        sets[0]->append(res->gpuH2DTimeMs);      // H2D
        sets[1]->append(res->gpuKernelTimeMs);   // Kernel
        sets[2]->append(res->gpuD2HTimeMs);      // D2H
        sets[3]->append(res->gpuSyncOverheadMs);     // Sync
    }
    
    // Обновляем категории оси X
    auto *axisX = qobject_cast<QBarCategoryAxis*>(gpuDetailChart->axes(Qt::Horizontal).first());
    if (axisX) {
        axisX->clear();
        for (int i = 0; i < gpuResults.size(); ++i) {
            const auto *res = gpuResults[i];
            axisX->append(SortBench::toString(res->params.cpuAlgorithm).left(15));
        }
    }
}

bool ChartWidget::exportChart(int tabIndex, const QString& path) {
    auto *chartView = qobject_cast<QChartView*>(tabs->widget(tabIndex));
    if (!chartView) return false;
    
    if (path.endsWith(".svg", Qt::CaseInsensitive)) {
        QSvgGenerator generator;
        generator.setFileName(path);
        generator.setSize(chartView->size());
        generator.setViewBox(chartView->rect());
        
        QPainter painter(&generator);
        chartView->scene()->render(&painter);
        return true;
    } else {
        QPixmap pixmap = chartView->grab();
        return pixmap.save(path);
    }
}

void ChartWidget::highlightAlgorithm(const QString& name) {
    // Подсветка серии с данным именем - заглушка, т.к. в Qt Charts нет прямой поддержки
    Q_UNUSED(name);
    // Можно реализовать через изменение цвета или маркеров серий при необходимости
}

void ChartWidget::setupChartTheme(bool isDark) {
    QChart::ChartTheme theme = isDark ? QChart::ChartThemeDark : QChart::ChartThemeLight;
    
    if (barChart) barChart->chart()->setTheme(theme);
    if (scatterChart) scatterChart->chart()->setTheme(theme);
    if (speedupChart) speedupChart->setTheme(theme);
    if (gpuDetailChart) gpuDetailChart->setTheme(theme);
}}
