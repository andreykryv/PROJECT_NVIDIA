/**
 * @file mainwindow.h
 * @brief Главное окно интерфейса SortBench.
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
#include <QLabel>
#include <QToolButton>
#include <QLineEdit>
#include <QStackedWidget>
#include <QScrollArea>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <vector>

// Заголовочные файлы Qt Charts для компиляции объявлений типов
#include <QChartView>
#include <QChart>
#include <QAbstractAxis>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>

#include "sorting_visualizer.h"
#include "benchmark_runner.h"

// Интерактивная плитка выбора алгоритма
class AlgTile : public QFrame {
    Q_OBJECT
public:
    QCheckBox* checkbox = nullptr;
    QLabel* titleLabel = nullptr;
    QLabel* descLabel = nullptr;
    QString algId;
    bool isGPU;

    AlgTile(const QString& name, const QString& shortDesc, const QString& id, bool gpu, QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
};

// Структура метаданных описания алгоритма
struct AlgDescData {
    QString name;
    QString best;
    QString avg;
    QString worst;
    QString space;
    QString description;
};

// Плитка справочника теории алгоритмов
class DescCard : public QFrame {
    Q_OBJECT
public:
    QLabel* titleLabel = nullptr;
    QLabel* descLabel = nullptr;
    DescCard(const AlgDescData& data, QWidget* parent = nullptr);
};

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
    void onOpenSettings(); // Слот для открытия настроек

    void switchToBenchmarkPage();
    void switchToVisualizerPage();
    void switchToDescriptionPage();

private:
    void setupUI();
    void setupLeftSidebar(QHBoxLayout* root);
    void setupTopBar(QVBoxLayout* parent);
    void setupBenchmarkPage();
    void setupVisualizerPage();
    void setupDescriptionPage();
    void setupRightSidebar(QHBoxLayout* parent);
    void setupCharts();
    void loadAvailableAlgorithms();
    void applyMasterStylesheet();

    void updateCharts();
    void updateMetricCards();
    void updateSystemTelemetry();
    std::vector<QString> getSelectedAlgorithms();

    // Навигация
    QToolButton* m_navBenchBtn   = nullptr;
    QToolButton* m_navVisualBtn  = nullptr;
    QToolButton* m_navDescBtn    = nullptr;
    QToolButton* m_navExportBtn  = nullptr;
    QToolButton* m_settBtn       = nullptr; // Кнопка настроек
    QLabel*      m_pageTitle     = nullptr;
    QLabel*      m_pageSubtitle  = nullptr;

    QStackedWidget* m_stackedWidget = nullptr;

    // Вкладка Аналитики
    QChartView*         m_chartView   = nullptr;
    QChart*             m_chart       = nullptr;
    QBarSeries*         m_barSeries   = nullptr;
    QAbstractAxis*      m_axisX       = nullptr; // Базовый класс для гибкой подмены осей на лету
    QValueAxis*         m_axisY       = nullptr;
    QTableWidget*       m_statsTable  = nullptr;

    QLabel* m_metricAlgCount  = nullptr;
    QLabel* m_metricBestCpu   = nullptr;
    QLabel* m_metricBestGpu   = nullptr;
    QLabel* m_metricSpeedup   = nullptr;

    // Вкладка Визуализации
    SortingVisualizer* m_visualizer      = nullptr;
    QComboBox*         m_visualAlgCombo  = nullptr;
    QSpinBox*          m_visualSizeSpin  = nullptr;
    QPushButton*       m_visualGenBtn    = nullptr;
    QPushButton*       m_visualStartBtn  = nullptr;
    QPushButton*       m_visualPauseBtn  = nullptr;
    QPushButton*       m_visualStopBtn   = nullptr;
    QSlider*           m_visualSpeedSlider = nullptr;
    QLabel*            m_visualStatusLabel = nullptr;

    // Элементы конфигурации правого сайдбара
    QLineEdit*     m_sidebarSearch  = nullptr;
    QSpinBox*      m_arraySizeSpin  = nullptr;
    QComboBox*     m_distCombo      = nullptr;
    QComboBox*     m_dataTypeCombo  = nullptr;
    QSpinBox*      m_runsSpin       = nullptr;
    QPushButton*   m_runBenchBtn    = nullptr;
    QPushButton*   m_stopBenchBtn   = nullptr;
    QPushButton*   m_exportCsvBtn   = nullptr;
    QPushButton*   m_exportPngBtn   = nullptr;
    QProgressBar*  m_benchProgress  = nullptr;

    // Новые элементы управления и таймеры
    QCheckBox*     m_sweepModeCheck = nullptr;
    QTimer*        m_telemetryTimer = nullptr;

    // Верхний поиск
    QLineEdit*     m_topSearchEdit  = nullptr;

    // Системная телеметрия
    QLabel*      m_topCpuLabel      = nullptr;
    QLabel*      m_topGpuLabel      = nullptr;
    QLabel*      m_sidebarGpuLabel  = nullptr;
    QPushButton* m_toggleGpuBtn     = nullptr;
    QWidget*     m_gpuLedIndicator  = nullptr;
    QProgressBar* m_gpuVramBar      = nullptr;
    QLabel*      m_gpuVramLabel     = nullptr;

    // Данные рантайма
    BenchmarkRunner*              m_benchRunner    = nullptr;
    std::vector<double>           m_visualData;
    std::atomic<bool>             m_stopVisualRequested;
    std::atomic<bool>             m_isVisualPaused;
    std::atomic<int>              m_visualDelayMs;
    QThread*                      m_visualSortThread = nullptr;
    std::vector<Benchmark::StatResults> m_accumulatedResults;
    bool                          m_gpuConnected = true;

    // Контейнеры динамических карточек
    std::vector<AlgTile*>   m_tiles;
    std::vector<DescCard*>  m_descCards;
};