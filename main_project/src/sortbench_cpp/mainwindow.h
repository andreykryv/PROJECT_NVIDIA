/**
 * @file mainwindow.h
 * @brief Главное окно интерфейса SortBench — Teamify Dashboard Style.
 * Трёхколоночный макет: левый сайдбар (иконки навигации),
 * центральный контент (график + карточки метрик + таблица / визуализатор),
 * правый сайдбар (конфигурация + алгоритмы + запуск).
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
#include <QToolButton>
#include <QLineEdit>
#include <QStackedWidget>
#include <QScrollArea>
#include <QChartView>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "sorting_visualizer.h"
#include "benchmark_runner.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartBenchmark();
    void onStopBenchmark();
    void onBenchmarkProgress(int percent);
    void onAlgorithmCompleted(const Benchmark::StatResults& results);
    void onBenchmarkFinished();

    void onStartVisualSort();
    void onPauseResumeVisual();
    void onStopVisual();
    void onVisualSpeedChanged(int value);
    void onVisualStep(const std::vector<double>& arr, int act1, int act2, int piv);

    void onExportCSV();
    void onExportPNG();
    void onGenerateVisualArray();
    void onToggleGpu();

    void switchToBenchmarkPage();
    void switchToVisualizerPage();

private:
    // ── UI Build Helpers ──────────────────────────────────────────────────────
    void setupUI();
    void setupLeftSidebar(QHBoxLayout* root);
    void setupTopBar(QVBoxLayout* parent);
    void setupBenchmarkPage();
    void setupVisualizerPage();
    void setupRightSidebar(QHBoxLayout* parent);
    void setupCharts();
    void loadAvailableAlgorithms();
    void applyMasterStylesheet();

    // ── Update helpers ────────────────────────────────────────────────────────
    void updateCharts();
    void updateMetricCards();

    // ═══════════════════════════════════════════════════════════════════════
    //  Navigation
    // ═══════════════════════════════════════════════════════════════════════
    QToolButton* m_navBenchBtn   = nullptr;
    QToolButton* m_navVisualBtn  = nullptr;
    QToolButton* m_navExportBtn  = nullptr;
    QLabel*      m_pageTitle     = nullptr;
    QLabel*      m_pageSubtitle  = nullptr;

    // ═══════════════════════════════════════════════════════════════════════
    //  Page switcher
    // ═══════════════════════════════════════════════════════════════════════
    QStackedWidget* m_stackedWidget = nullptr;

    // ═══════════════════════════════════════════════════════════════════════
    //  Benchmark page widgets
    // ═══════════════════════════════════════════════════════════════════════
    QChartView*         m_chartView   = nullptr;
    QChart*             m_chart       = nullptr;
    QBarSeries*         m_barSeries   = nullptr;
    QBarCategoryAxis*   m_axisX       = nullptr;
    QValueAxis*         m_axisY       = nullptr;
    QTableWidget*       m_statsTable  = nullptr;

    // Metric cards (live update)
    QLabel* m_metricAlgCount  = nullptr;   // Total algorithms tested
    QLabel* m_metricBestCpu   = nullptr;   // Best CPU time
    QLabel* m_metricBestGpu   = nullptr;   // Best GPU time
    QLabel* m_metricSpeedup   = nullptr;   // GPU speedup factor

    // ═══════════════════════════════════════════════════════════════════════
    //  Visualiser page widgets
    // ═══════════════════════════════════════════════════════════════════════
    SortingVisualizer* m_visualizer      = nullptr;
    QComboBox*         m_visualAlgCombo  = nullptr;
    QSpinBox*          m_visualSizeSpin  = nullptr;
    QPushButton*       m_visualGenBtn    = nullptr;
    QPushButton*       m_visualStartBtn  = nullptr;
    QPushButton*       m_visualPauseBtn  = nullptr;
    QPushButton*       m_visualStopBtn   = nullptr;
    QSlider*           m_visualSpeedSlider = nullptr;
    QLabel*            m_visualStatusLabel = nullptr;

    // ═══════════════════════════════════════════════════════════════════════
    //  Right sidebar — configuration
    // ═══════════════════════════════════════════════════════════════════════
    QListWidget*   m_algListWidget  = nullptr;
    QSpinBox*      m_arraySizeSpin  = nullptr;
    QComboBox*     m_distCombo      = nullptr;
    QComboBox*     m_dataTypeCombo  = nullptr;
    QSpinBox*      m_runsSpin       = nullptr;
    QPushButton*   m_runBenchBtn    = nullptr;
    QPushButton*   m_stopBenchBtn   = nullptr;
    QPushButton*   m_exportCsvBtn   = nullptr;
    QPushButton*   m_exportPngBtn   = nullptr;
    QProgressBar*  m_benchProgress  = nullptr;

    // ═══════════════════════════════════════════════════════════════════════
    //  Top bar / GPU telemetry
    // ═══════════════════════════════════════════════════════════════════════
    QLabel*      m_telemetryTextLabel = nullptr;
    QPushButton* m_toggleGpuBtn       = nullptr;
    QWidget*     m_ledIndicator       = nullptr;

    // ═══════════════════════════════════════════════════════════════════════
    //  Runtime state
    // ═══════════════════════════════════════════════════════════════════════
    BenchmarkRunner*              m_benchRunner    = nullptr;
    std::vector<double>           m_visualData;
    std::atomic<bool>             m_stopVisualRequested;
    std::atomic<bool>             m_isVisualPaused;
    std::atomic<int>              m_visualDelayMs;
    QThread*                      m_visualSortThread = nullptr;
    std::vector<Benchmark::StatResults> m_accumulatedResults;
    bool                          m_gpuConnected = true;
};