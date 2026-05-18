#include "chartwidget.h"
#include "charts/comparisonbarchart.h"
#include "charts/scatterplotchart.h"
#include <QVBoxLayout>
#include <QChartView>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QLineSeries>
#include <QScatterSeries>
#include <QAreaSeries>
#include <QStackedBarSeries>
#include <QDateTime>
#include <QFile>
#include <QSvgGenerator>
#include <QPrinter>
#include <QPainter>

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
    barChart = new ComparisonBarChart();
    barChart->setTitle("Время выполнения (мс)");
    
    auto *chartView = new QChartView(barChart->chart());
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setRubberBand(QChartView::RectangleRubberBand);
    
    tabs->addTab(chartView, "Время выполнения");
}

void ChartWidget::createScatterChart() {
    scatterChart = new ScatterPlotChart();
    scatterChart->setTitle("Масштабируемость");
    
    auto *chartView = new QChartView(scatterChart->chart());
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setRubberBand(QChartView::RectangleRubberBand);
    
    tabs->addTab(chartView, "Масштабируемость");
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
    
    auto *axisX = new QValueAxis();
    axisX->setTitleText("Размер массива");
    axisX->setType(QValueAxis::AxisTypeLog);
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

void ChartWidget::addResult(const BenchmarkResult& result) {
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
    barChart->updateData(results);
}

void ChartWidget::updateScatterChart() {
    if (!scatterChart) return;
    scatterChart->updateData(results);
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
    QMap<int, QList<const BenchmarkResult*>> bySize;
    for (const auto& res : results) {
        bySize[res.arraySize].append(&res);
    }
    
    // Создаём серию для каждого алгоритма
    QMap<QString, QLineSeries*> algoSeries;
    
    for (auto it = bySize.begin(); it != bySize.end(); ++it) {
        int size = it.key();
        for (const auto *res : it.value()) {
            QString algoName = toString(res->params.cpuAlgorithm);
            
            if (!algoSeries.contains(algoName)) {
                algoSeries[algoName] = new QLineSeries();
                algoSeries[algoName]->setName(algoName);
                
                // Цвет в зависимости от CPU/GPU
                if (res->isGpu) {
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
            if (!res->isGpu && cpuTime < 0) {
                cpuTime = res->totalTimeMs;
            }
            if (res->isGpu && gpuTime < 0) {
                gpuTime = res->totalTimeMs;
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
    for (auto *barSet : sets) {
        barSet->clear();
    }
    
    // Фильтруем только GPU результаты
    QList<const BenchmarkResult*> gpuResults;
    for (const auto& res : results) {
        if (res.isGpu) {
            gpuResults.append(&res);
        }
    }
    
    if (gpuResults.isEmpty()) return;
    
    // Добавляем данные для каждого результата
    for (const auto *res : gpuResults) {
        sets[0]->append(res->h2dTimeMs);      // H2D
        sets[1]->append(res->kernelTimeMs);   // Kernel
        sets[2]->append(res->d2hTimeMs);      // D2H
        sets[3]->append(res->syncTimeMs);     // Sync
    }
    
    // Обновляем категории оси X
    auto *axisX = qobject_cast<QBarCategoryAxis*>(gpuDetailChart->axes(Qt::Horizontal).first());
    if (axisX) {
        axisX->clear();
        for (const auto *res : gpuResults) {
            axisX->append(toString(res->params.cpuAlgorithm).left(15));
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
    // Подсветка серии с данным именем
    if (barChart) {
        barChart->highlightSeries(name);
    }
    if (scatterChart) {
        scatterChart->highlightSeries(name);
    }
}

void ChartWidget::setupChartTheme(bool isDark) {
    QChart::ChartTheme theme = isDark ? QChart::ChartThemeDark : QChart::ChartThemeLight;
    
    if (barChart) barChart->chart()->setTheme(theme);
    if (scatterChart) scatterChart->chart()->setTheme(theme);
    if (speedupChart) speedupChart->setTheme(theme);
    if (gpuDetailChart) gpuDetailChart->setTheme(theme);
}
