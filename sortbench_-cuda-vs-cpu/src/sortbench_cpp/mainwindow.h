/**
 * @file mainwindow.h
 * @brief Главное окно интерфейса SortBench.
 * Сочетает панели настроек, бенчмарк-аналитик, визуализатор и графики QtCharts.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include <QTableWidget>
#include <QProgressBar>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTabWidget>
#include <QLabel>
#include <QChartView>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>

#include "sorting_visualizer.h"
#include "benchmark_runner.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Слоты для бенчмарков
    void onStartBenchmark();
    void onStopBenchmark();
    void onBenchmarkProgress(int percent);
    void onAlgorithmCompleted(const Benchmark::StatResults& results);
    void onBenchmarkFinished();

    // Слоты для интерактивной визуализации
    void onStartVisualSort();
    void onPauseResumeVisual();
    void onStopVisual();
    void onVisualSpeedChanged(int value);
    void onVisualStep(const std::vector<double>& arr, int act1, int act2, int piv);

    // Дополнительные операции
    void onExportCSV();
    void onExportPNG();
    void onGenerateVisualArray();
    void onToggleGpu();

private:
    void setupUI();
    void setupCharts();
    void updateCharts();
    void loadAvailableAlgorithms();

    // Виджеты управления бенчмарками
    QListWidget* m_algListWidget = nullptr;
    QSpinBox* m_arraySizeSpin = nullptr;
    QComboBox* m_distCombo = nullptr;
    QComboBox* m_dataTypeCombo = nullptr;
    QSpinBox* m_runsSpin = nullptr;
    QPushButton* m_runBenchBtn = nullptr;
    QPushButton* m_stopBenchBtn = nullptr;
    QPushButton* m_exportCsvBtn = nullptr;
    QPushButton* m_exportPngBtn = nullptr;
    QProgressBar* m_benchProgress = nullptr;
    QTableWidget* m_statsTable = nullptr;
    
    // Элементы вкладок
    QTabWidget* m_mainTabs = nullptr;
    
    // Вкладка Графика (QtCharts)
    QChartView* m_chartView = nullptr;
    QChart* m_chart = nullptr;
    QBarSeries* m_barSeries = nullptr;
    QBarCategoryAxis* m_axisX = nullptr;
    QValueAxis* m_axisY = nullptr;

    // Вкладка Визуализатора
    SortingVisualizer* m_visualizer = nullptr;
    QComboBox* m_visualAlgCombo = nullptr;
    QSpinBox* m_visualSizeSpin = nullptr;
    QPushButton* m_visualGenBtn = nullptr;
    QPushButton* m_visualStartBtn = nullptr;
    QPushButton* m_visualPauseBtn = nullptr;
    QPushButton* m_visualStopBtn = nullptr;
    QSlider* m_visualSpeedSlider = nullptr;
    QLabel* m_visualStatusLabel = nullptr;

    // Архитектурные элементы
    BenchmarkRunner* m_benchRunner = nullptr;
    
    // Состояние визуализации
    std::vector<double> m_visualData;
    std::atomic<bool> m_stopVisualRequested;
    std::atomic<bool> m_isVisualPaused;
    std::atomic<int> m_visualDelayMs; // Задержка в мс (регулируется слайдером)
    QThread* m_visualSortThread = nullptr;

    // Сохраненные данные результатов для графиков и CSV
    std::vector<Benchmark::StatResults> m_accumulatedResults;

    // Состояние CUDA телеметрии
    bool m_gpuConnected = true;
    QLabel* m_telemetryTextLabel = nullptr;
    QPushButton* m_toggleGpuBtn = nullptr;
    QWidget* m_ledIndicator = nullptr;
};
