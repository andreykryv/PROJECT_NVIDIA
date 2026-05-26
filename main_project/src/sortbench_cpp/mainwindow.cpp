/**
 * @file mainwindow.cpp
 * @brief Описание поведения главного графического интерфейса SortBench.
 * Связывает селекторы параметров (размер, законы генерации массивов),
 * заполняет таблицы статистики, перестраивает QtCharts и дирижирует потоками.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QListWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QBarSet>
#include <QChart>
#include <QColor>
#include <QThread>
#include <QRandomGenerator>
#include <QListWidgetItem>      // добавлено
#include "cpu_algorithms.h" 
#include <QtGlobal>    // добавлено

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_stopVisualRequested(false)
    , m_isVisualPaused(false)
    , m_visualDelayMs(50) {
    
    setWindowTitle("SortBench: Тестирование производительности CUDA vs CPU");
    resize(1280, 800);

    // Архитектурные элементы
    m_benchRunner = new BenchmarkRunner(this);
    connect(m_benchRunner, &BenchmarkRunner::algorithmCompleted, this, &MainWindow::onAlgorithmCompleted);
    connect(m_benchRunner, &BenchmarkRunner::progressUpdated, this, &MainWindow::onBenchmarkProgress);
    connect(m_benchRunner, &BenchmarkRunner::finishedAll, this, &MainWindow::onBenchmarkFinished);

    setupUI();
    setupCharts();
    loadAvailableAlgorithms();
    onGenerateVisualArray(); // Инициализация первого массива для визуализатора
}

MainWindow::~MainWindow() {
    onStopVisual();
    if (m_benchRunner) {
        m_benchRunner->requestStop();
        m_benchRunner->wait(3000);
    }
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Главный вертикальный лейаут
    QVBoxLayout* mainVerticalLayout = new QVBoxLayout(centralWidget);
    mainVerticalLayout->setContentsMargins(0, 0, 0, 0);
    mainVerticalLayout->setSpacing(0);

    // --- PREMIUM HEADER BAR ---
    QWidget* headerWidget = new QWidget(this);
    headerWidget->setObjectName("headerWidget");
    
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 15, 20, 15);
    headerLayout->setSpacing(15);

    // Слева: Логотип и Название студии
    QWidget* logoContainer = new QWidget(this);
    QHBoxLayout* logoLayout = new QHBoxLayout(logoContainer);
    logoLayout->setContentsMargins(0, 0, 0, 0);
    logoLayout->setSpacing(12);

    QLabel* cpuEmojiIcon = new QLabel("⚡", this);
    cpuEmojiIcon->setStyleSheet("font-size: 16px; color: #60a5fa; background: #1c1c24; padding: 6px 10px; border-radius: 10px; border: 1px solid #2d2d34;");
    logoLayout->addWidget(cpuEmojiIcon);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);

    QWidget* titleRow = new QWidget(this);
    QHBoxLayout* titleRowLayout = new QHBoxLayout(titleRow);
    titleRowLayout->setContentsMargins(0, 0, 0, 0);
    titleRowLayout->setSpacing(8);

    QLabel* titleLabel = new QLabel("SortBench Studio", this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff; font-family: 'Segoe UI', Arial, sans-serif;");
    titleRowLayout->addWidget(titleLabel);

    QLabel* badgeLabel = new QLabel("CUDA VS CPU V1.3", this);
    badgeLabel->setStyleSheet("font-size: 8px; font-weight: bold; color: #60a5fa; background-color: rgba(37, 99, 235, 0.15); border: 1px solid rgba(59, 130, 246, 0.3); border-radius: 10px; padding: 2px 8px; text-transform: uppercase;");
    titleRowLayout->addWidget(badgeLabel);
    titleRowLayout->addStretch();
    titleLayout->addWidget(titleRow);

    QLabel* subtitleLabel = new QLabel("Высокоточная лаборатория анализа и моделирования алгоритмов сортировки", this);
    subtitleLabel->setStyleSheet("font-size: 11px; color: #a1a1aa;");
    titleLayout->addWidget(subtitleLabel);

    logoLayout->addLayout(titleLayout);
    headerLayout->addWidget(logoContainer);
    headerLayout->addStretch();

    // Справа: Статус-бар Телеметрии (как в React)
    QWidget* telemetryBar = new QWidget(this);
    telemetryBar->setStyleSheet("background-color: rgba(20, 20, 25, 0.6); border: 1px solid #1f1f23; border-radius: 12px; padding: 6px 15px;");
    QHBoxLayout* telemetryLayout = new QHBoxLayout(telemetryBar);
    telemetryLayout->setContentsMargins(10, 4, 10, 4);
    telemetryLayout->setSpacing(10);

    // Круглый LED индикатор
    m_ledIndicator = new QWidget(this);
    m_ledIndicator->setFixedSize(8, 8);
    m_ledIndicator->setStyleSheet("background-color: #10b981; border-radius: 4px;"); // Зеленый по умолчанию
    telemetryLayout->addWidget(m_ledIndicator);

    QLabel* prefixLabel = new QLabel("Система CUDA:", this);
    prefixLabel->setStyleSheet("font-weight: bold; color: #a1a1aa; font-size: 11px; border: none; background: transparent;");
    telemetryLayout->addWidget(prefixLabel);

    m_telemetryTextLabel = new QLabel("CPU: Intel Core i9-14900K | GPU: NVIDIA GeForce RTX 4090 24GB PCIe Gen4", this);
    m_telemetryTextLabel->setStyleSheet("color: #e4e4e7; font-size: 11px; font-family: 'Consolas', 'Courier New', monospace; border: none; background: transparent;");
    telemetryLayout->addWidget(m_telemetryTextLabel);

    m_toggleGpuBtn = new QPushButton("ОТКЛЮЧИТЬ", this);
    m_toggleGpuBtn->setCursor(Qt::PointingHandCursor);
    m_toggleGpuBtn->setStyleSheet(
        "QPushButton {"
        "  background-color: #1c1c24; border: 1px solid #2d2d34; border-radius: 6px; padding: 4px 10px; color: #e4e4e7; font-size: 9px; font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #272730; border-color: #3f3f46; color: #ffffff;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #121215;"
        "}"
    );
    connect(m_toggleGpuBtn, &QPushButton::clicked, this, &MainWindow::onToggleGpu);
    telemetryLayout->addWidget(m_toggleGpuBtn);

    headerLayout->addWidget(telemetryBar);
    mainVerticalLayout->addWidget(headerWidget);

    // Горизонтальный разделитель под хедером
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    separator->setStyleSheet("background-color: #1f1f23; max-height: 1px; border: none;");
    mainVerticalLayout->addWidget(separator);

    // Основное содержание стенда (боковая панель слева + табы справа)
    QWidget* contentWidget = new QWidget(this);
    mainVerticalLayout->addWidget(contentWidget, 1);

    QHBoxLayout* mainLayout = new QHBoxLayout(contentWidget);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    // Левая панель - Управление бенчмарками и глобальные настройки
    QGroupBox* sidebarGroupBox = new QGroupBox("Конфигурация стенда", this);
    sidebarGroupBox->setObjectName("sidebar");
    QVBoxLayout* sidebarLayout = new QVBoxLayout(sidebarGroupBox);
    sidebarLayout->setSpacing(10);

    // 1. Выбор алгоритма
    sidebarLayout->addWidget(new QLabel("Алгоритмы для сравнения:", this));
    m_algListWidget = new QListWidget(this);
    m_algListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    sidebarLayout->addWidget(m_algListWidget);

    // 2. Параметры массива
    QGridLayout* paramsGrid = new QGridLayout();
    paramsGrid->setSpacing(8);

    paramsGrid->addWidget(new QLabel("Размер массива (N):", this), 0, 0);
    m_arraySizeSpin = new QSpinBox(this);
    m_arraySizeSpin->setRange(10, 10000000);
    m_arraySizeSpin->setValue(10000); // 10 тысяч по умолчанию
    m_arraySizeSpin->setSingleStep(1000);
    paramsGrid->addWidget(m_arraySizeSpin, 0, 1);

    paramsGrid->addWidget(new QLabel("Тип распределения:", this), 1, 0);
    m_distCombo = new QComboBox(this);
    m_distCombo->addItem("Случайное равномерное", static_cast<int>(Benchmark::Distribution::Uniform));
    m_distCombo->addItem("Нормальное (Гаусс)", static_cast<int>(Benchmark::Distribution::Normal));
    m_distCombo->addItem("Обратно отсортированное", static_cast<int>(Benchmark::Distribution::ReverseSorted));
    m_distCombo->addItem("Почти упорядоченное (5%)", static_cast<int>(Benchmark::Distribution::AlmostSorted));
    m_distCombo->addItem("Одинаковые значения", static_cast<int>(Benchmark::Distribution::AllEqual));
    paramsGrid->addWidget(m_distCombo, 1, 1);

    paramsGrid->addWidget(new QLabel("Тип данных:", this), 2, 0);
    m_dataTypeCombo = new QComboBox(this);
    m_dataTypeCombo->addItem("float (одинарная точность)");
    m_dataTypeCombo->addItem("double (двойная точность)");
    m_dataTypeCombo->addItem("struct { double key; char data[64]; }");
    paramsGrid->addWidget(m_dataTypeCombo, 2, 1);

    paramsGrid->addWidget(new QLabel("Повторов для усреднения:", this), 3, 0);
    m_runsSpin = new QSpinBox(this);
    m_runsSpin->setRange(1, 20);
    m_runsSpin->setValue(5);
    paramsGrid->addWidget(m_runsSpin, 3, 1);

    sidebarLayout->addLayout(paramsGrid);

    // 3. Панель запуска бенчмарка
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_runBenchBtn = new QPushButton("Старт тест", this);
    m_runBenchBtn->setObjectName("startBtn");
    connect(m_runBenchBtn, &QPushButton::clicked, this, &MainWindow::onStartBenchmark);
    btnLayout->addWidget(m_runBenchBtn);

    m_stopBenchBtn = new QPushButton("Стоп", this);
    m_stopBenchBtn->setObjectName("stopBtn");
    m_stopBenchBtn->setEnabled(false);
    connect(m_stopBenchBtn, &QPushButton::clicked, this, &MainWindow::onStopBenchmark);
    btnLayout->addWidget(m_stopBenchBtn);

    sidebarLayout->addLayout(btnLayout);

    m_benchProgress = new QProgressBar(this);
    m_benchProgress->setValue(0);
    sidebarLayout->addWidget(m_benchProgress);

    // Экспорт результатов
    sidebarLayout->addWidget(new QLabel("Экспорт данных:", this));
    QHBoxLayout* exportLayout = new QHBoxLayout();
    m_exportCsvBtn = new QPushButton("Экспорт CSV", this);
    connect(m_exportCsvBtn, &QPushButton::clicked, this, &MainWindow::onExportCSV);
    exportLayout->addWidget(m_exportCsvBtn);

    m_exportPngBtn = new QPushButton("Сохранить график", this);
    connect(m_exportPngBtn, &QPushButton::clicked, this, &MainWindow::onExportPNG);
    exportLayout->addWidget(m_exportPngBtn);
    sidebarLayout->addLayout(exportLayout);

    sidebarLayout->addStretch();
    sidebarGroupBox->setFixedWidth(290);
    mainLayout->addWidget(sidebarGroupBox);

    // Правая панель - Вкладки (График & Таблица, Визуализатор)
    m_mainTabs = new QTabWidget(this);
    
    // Вкладка 1: Аналитика и Сравнительные графики
    QWidget* analyticsWidget = new QWidget(this);
    QVBoxLayout* analyticsLayout = new QVBoxLayout(analyticsWidget);
    analyticsLayout->setContentsMargins(5, 5, 5, 5);

    QSplitter* analyticsSplitter = new QSplitter(Qt::Vertical, this);

    // Место под графики QtCharts (создается в setupCharts)
    m_chartView = new QChartView(this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    analyticsSplitter->addWidget(m_chartView);

    // Таблица результатов
    m_statsTable = new QTableWidget(this);
    m_statsTable->setColumnCount(10);
    m_statsTable->setHorizontalHeaderLabels({
        "Алгоритм", "Аппарат", "N", "Min (ms)", "Max (ms)", 
        "Среднее (ms)", "Медиана (ms)", "H2D Upload", "GPU Core", "D2H Download"
    });
    m_statsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_statsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_statsTable->setAlternatingRowColors(true);
    analyticsSplitter->addWidget(m_statsTable);
    analyticsSplitter->setSizes({400, 250});

    analyticsLayout->addWidget(analyticsSplitter);
    m_mainTabs->addTab(analyticsWidget, "Сравнительные графики");

    // Вкладка 2: Динамический визуализатор
    QWidget* visualWidget = new QWidget(this);
    QVBoxLayout* visualLayout = new QVBoxLayout(visualWidget);

    m_visualizer = new SortingVisualizer(this);
    m_visualizer->setMinimumHeight(350);
    visualLayout->addWidget(m_visualizer, 1);

    // Панель управления анимацией
    QGroupBox* animControls = new QGroupBox("Управление анимацией", this);
    QHBoxLayout* animLayout = new QHBoxLayout(animControls);
    animLayout->setSpacing(10);

    animLayout->addWidget(new QLabel("Алгоритм:", this));
    m_visualAlgCombo = new QComboBox(this);
    animLayout->addWidget(m_visualAlgCombo);

    animLayout->addWidget(new QLabel("Элементов (N):", this));
    m_visualSizeSpin = new QSpinBox(this);
    m_visualSizeSpin->setRange(5, 1000); // Ограничено для читаемости
    m_visualSizeSpin->setValue(60);
    animLayout->addWidget(m_visualSizeSpin);

    m_visualGenBtn = new QPushButton("Генерировать", this);
    connect(m_visualGenBtn, &QPushButton::clicked, this, &MainWindow::onGenerateVisualArray);
    animLayout->addWidget(m_visualGenBtn);

    m_visualStartBtn = new QPushButton("Старт", this);
    m_visualStartBtn->setObjectName("startBtn");
    connect(m_visualStartBtn, &QPushButton::clicked, this, &MainWindow::onStartVisualSort);
    animLayout->addWidget(m_visualStartBtn);

    m_visualPauseBtn = new QPushButton("Пауза", this);
    m_visualPauseBtn->setEnabled(false);
    connect(m_visualPauseBtn, &QPushButton::clicked, this, &MainWindow::onPauseResumeVisual);
    animLayout->addWidget(m_visualPauseBtn);

    m_visualStopBtn = new QPushButton("Сброс", this);
    m_visualStopBtn->setEnabled(false);
    connect(m_visualStopBtn, &QPushButton::clicked, this, &MainWindow::onStopVisual);
    animLayout->addWidget(m_visualStopBtn);

    animLayout->addWidget(new QLabel("Скорость:", this));
    m_visualSpeedSlider = new QSlider(Qt::Horizontal, this);
    m_visualSpeedSlider->setRange(1, 100);
    m_visualSpeedSlider->setValue(50); // Средняя скорость
    m_visualSpeedSlider->setFixedWidth(120);
    connect(m_visualSpeedSlider, &QSlider::valueChanged, this, &MainWindow::onVisualSpeedChanged);
    animLayout->addWidget(m_visualSpeedSlider);
    
    // Первоначальный вызов для расчета задержки по умолчанию
    onVisualSpeedChanged(50);

    visualLayout->addWidget(animControls);

    m_visualStatusLabel = new QLabel("Статус: Готов к запуску", this);
    m_visualStatusLabel->setStyleSheet("color: #a1a1aa; font-weight: bold; padding: 4px;");
    visualLayout->addWidget(m_visualStatusLabel);

    m_mainTabs->addTab(visualWidget, "Интерактивный симулятор");
    mainLayout->addWidget(m_mainTabs, 1);

    // Apply the master Dark Futuristic style sheet
    QString style = R"(
        QMainWindow {
            background-color: #070709;
        }
        #headerWidget {
            background-color: #0c0c0e;
        }
        QGroupBox {
            border: 1px solid #1f1f23;
            border-radius: 12px;
            margin-top: 15px;
            font-size: 11px;
            font-weight: bold;
            color: #3b82f6;
            background-color: #111115;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 15px;
            padding: 0 5px;
        }
        QPushButton {
            background-color: #1c1c24;
            border: 1px solid #2d2d34;
            color: #e4e4e7;
            border-radius: 8px;
            padding: 7px 14px;
            font-size: 11px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #272730;
            border-color: #3f3f46;
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: #121215;
            border-color: #27272a;
        }
        QPushButton:disabled {
            background-color: #0c0c0e;
            border-color: #18181b;
            color: #52525b;
        }
        QPushButton#startBtn, QPushButton[text="Старт тест"], QPushButton[text="Старт"], QPushButton[text="Генерировать"] {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2563eb, stop:1 #4f46e5);
            border: none;
            color: #ffffff;
        }
        QPushButton#startBtn:hover, QPushButton[text="Старт тест"]:hover, QPushButton[text="Старт"]:hover, QPushButton[text="Генерировать"]:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #6366f1);
        }
        QPushButton#startBtn:pressed, QPushButton[text="Старт тест"]:pressed, QPushButton[text="Старт"]:pressed, QPushButton[text="Генерировать"]:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1d4ed8, stop:1 #4338ca);
        }
        QPushButton#stopBtn, QPushButton[text="Стоп"], QPushButton[text="Сброс"] {
            background-color: #271c1c;
            border: 1px solid #4a2424;
            color: #f87171;
        }
        QPushButton#stopBtn:hover, QPushButton[text="Стоп"]:hover, QPushButton[text="Сброс"]:hover {
            background-color: #3b2020;
            border-color: #ef4444;
            color: #ffffff;
        }
        QComboBox, QSpinBox {
            background-color: #121215;
            border: 1px solid #1f1f23;
            border-radius: 6px;
            padding: 4px 8px;
            color: #e4e4e7;
            min-height: 22px;
        }
        QComboBox:hover, QSpinBox:hover {
            border-color: #2563eb;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 20px;
            border-left-width: 0px;
        }
        QListWidget {
            background-color: #121215;
            border: 1px solid #1f1f23;
            border-radius: 10px;
            padding: 4px;
            color: #e4e4e7;
        }
        QListWidget::item {
            border-radius: 6px;
            padding: 6px;
            margin: 1px 0px;
            color: #a1a1aa;
        }
        QListWidget::item:hover {
            background-color: #1c1c24;
            color: #ffffff;
        }
        QListWidget::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2563eb, stop:1 #4f46e5);
            color: #ffffff;
            font-weight: bold;
        }
        QTabWidget::pane {
            border: 1px solid #1f1f23;
            border-radius: 12px;
            background-color: #0c0c0e;
            top: -1px;
        }
        QTabBar::tab {
            background-color: #121215;
            border: 1px solid #1f1f23;
            border-bottom: none;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            padding: 8px 18px;
            color: #a1a1aa;
            font-size: 11px;
            font-weight: bold;
            margin-right: 4px;
        }
        QTabBar::tab:hover {
            background-color: #16161c;
            color: #e4e4e7;
        }
        QTabBar::tab:selected {
            background-color: #0c0c0e;
            border-color: #1f1f23;
            color: #60a5fa;
        }
        QTableWidget {
            background-color: #0c0c0e;
            border: 1px solid #1f1f23;
            border-radius: 10px;
            gridline-color: #1f1f23;
            color: #e4e4e7;
            font-size: 11px;
        }
        QTableWidget::item {
            padding: 6px;
        }
        QHeaderView::section {
            background-color: #121215;
            color: #a1a1aa;
            padding: 8px;
            border: none;
            border-bottom: 2px solid #1f1f23;
            border-right: 1px solid #1f1f23;
            font-weight: bold;
            font-size: 11px;
        }
        QProgressBar {
            background-color: #121215;
            border: 1px solid #1f1f23;
            border-radius: 6px;
            text-align: center;
            color: #ffffff;
            font-size: 10px;
            font-weight: bold;
            height: 16px;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2563eb, stop:1 #4f46e5);
            border-radius: 5px;
        }
        QScrollBar:vertical {
            border: none;
            background: #070709;
            width: 8px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #272730;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background: #3b82f6;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QSlider::groove:horizontal {
            border: none;
            height: 6px;
            background: #1c1c24;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #2563eb;
            border: none;
            width: 14px;
            height: 14px;
            margin: -4px 0;
            border-radius: 7px;
        }
        QSlider::handle:horizontal:hover {
            background: #60a5fa;
        }
    )";
    setStyleSheet(style);
}

void MainWindow::setupCharts() {
    m_chart = new QChart();
    m_chart->setTitle("Гистограмма производительности: CUDA vs CPU (Время, мс)");
    m_chart->setTitleBrush(QBrush(Qt::white));
    m_chart->setBackgroundBrush(QBrush(QColor(24, 24, 27)));
    m_chart->setAnimationOptions(QChart::SeriesAnimations);

    m_barSeries = new QBarSeries(this);
    m_chart->addSeries(m_barSeries);

    m_axisX = new QBarCategoryAxis(this);
    m_axisX->setLabelsBrush(QBrush(Qt::white));
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_barSeries->attachAxis(m_axisX);

    m_axisY = new QValueAxis(this);
    m_axisY->setLabelsBrush(QBrush(Qt::white));
    m_axisY->setLabelFormat("%.3f");
    m_axisY->setTitleText("Время выполнения (мс)");
    m_axisY->setTitleBrush(QBrush(Qt::white));
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_barSeries->attachAxis(m_axisY);

    m_chartView->setChart(m_chart);
    m_chartView->setBackgroundRole(QPalette::Window);
}

void MainWindow::loadAvailableAlgorithms() {
    // CPU алгоритмы (20 шт.)
    m_algListWidget->addItem("CPU: std::sort (STL Intro)");
    m_algListWidget->addItem("CPU: QuickSort (Быстрая)");
    m_algListWidget->addItem("CPU: MergeSort (Слиянием)");
    m_algListWidget->addItem("CPU: HeapSort (Пирамид)");
    m_algListWidget->addItem("CPU: TimSort (Гибридная)");
    m_algListWidget->addItem("CPU: BubbleSort (Пузырек)");
    m_algListWidget->addItem("CPU: SelectionSort (Выбором)");
    m_algListWidget->addItem("CPU: InsertionSort (Вставками)");
    m_algListWidget->addItem("CPU: ShellSort (Шелла)");
    m_algListWidget->addItem("CPU: CocktailSort (Шейкер)");
    m_algListWidget->addItem("CPU: GnomeSort (Гномья)");
    m_algListWidget->addItem("CPU: CombSort (Расческой)");
    m_algListWidget->addItem("CPU: RadixSortLSD (LSD Поразр)");
    m_algListWidget->addItem("CPU: CountingSort (Подсчетом)");
    m_algListWidget->addItem("CPU: BucketSort (Блочная)");
    m_algListWidget->addItem("CPU: PancakeSort (Блинная)");
    m_algListWidget->addItem("CPU: BogoSort (Случайная)");
    m_algListWidget->addItem("CPU: StoogeSort (Придурковатая)");
    m_algListWidget->addItem("CPU: OddEvenSort (Чет-Нечет)");
    m_algListWidget->addItem("CPU: CycleSort (Циклическая)");

    // GPU алгоритмы (20 шт.) – все соответствуют CPU
    m_algListWidget->addItem("GPU: Bitonic Sort (CUDA)");
    m_algListWidget->addItem("GPU: Radix Sort (CUDA)");
    m_algListWidget->addItem("GPU: Odd-Even (CUDA)");
    m_algListWidget->addItem("GPU: std::sort (STL Intro)");
    m_algListWidget->addItem("GPU: QuickSort (Быстрая)");
    m_algListWidget->addItem("GPU: MergeSort (Слиянием)");
    m_algListWidget->addItem("GPU: HeapSort (Пирамид)");
    m_algListWidget->addItem("GPU: TimSort (Гибридная)");
    m_algListWidget->addItem("GPU: BubbleSort (Пузырек)");
    m_algListWidget->addItem("GPU: SelectionSort (Выбором)");
    m_algListWidget->addItem("GPU: InsertionSort (Вставками)");
    m_algListWidget->addItem("GPU: ShellSort (Шелла)");
    m_algListWidget->addItem("GPU: CocktailSort (Шейкер)");
    m_algListWidget->addItem("GPU: GnomeSort (Гномья)");
    m_algListWidget->addItem("GPU: CombSort (Расческой)");
    m_algListWidget->addItem("GPU: RadixSortLSD (LSD Поразр)");
    m_algListWidget->addItem("GPU: CountingSort (Подсчетом)");
    m_algListWidget->addItem("GPU: BucketSort (Блочная)");
    m_algListWidget->addItem("GPU: PancakeSort (Блинная)");
    m_algListWidget->addItem("GPU: BogoSort (Случайная)");
    m_algListWidget->addItem("GPU: StoogeSort (Придурковатая)");
    m_algListWidget->addItem("GPU: OddEvenSort (Чет-Нечет)");
    m_algListWidget->addItem("GPU: CycleSort (Циклическая)");

    // Выбираем по умолчанию несколько алгоритмов
    m_algListWidget->item(0)->setSelected(true);   // CPU std::sort
    m_algListWidget->item(1)->setSelected(true);   // CPU QuickSort
    m_algListWidget->item(20)->setSelected(true);  // GPU Bitonic
    m_algListWidget->item(21)->setSelected(true);  // GPU Radix

    // Заполнение выпадающего списка визуализатора (оставляем как было – только CPU)
    m_visualAlgCombo->clear();
    m_visualAlgCombo->addItem("QuickSort (Быстрая)", "CPU_QuickSort");
    m_visualAlgCombo->addItem("MergeSort (Слиянием)", "CPU_MergeSort");
    m_visualAlgCombo->addItem("HeapSort (Кучей)", "CPU_HeapSort");
    m_visualAlgCombo->addItem("TimSort (Гибридная)", "CPU_TimSort");
    m_visualAlgCombo->addItem("BubbleSort (Пузырек)", "CPU_BubbleSort");
    m_visualAlgCombo->addItem("SelectionSort (Выбор)", "CPU_SelectionSort");
    m_visualAlgCombo->addItem("InsertionSort (Вставки)", "CPU_InsertionSort");
    m_visualAlgCombo->addItem("ShellSort (Шелла)", "CPU_ShellSort");
    m_visualAlgCombo->addItem("CocktailSort (Шейкер)", "CPU_CocktailSort");
    m_visualAlgCombo->addItem("GnomeSort (Гномья)", "CPU_GnomeSort");
    m_visualAlgCombo->addItem("CombSort (Расческой)", "CPU_CombSort");
    m_visualAlgCombo->addItem("Radix LSD (Поразрядная)", "CPU_RadixSortLSD");
    m_visualAlgCombo->addItem("CountingSort (Подсчет)", "CPU_CountingSort");
    m_visualAlgCombo->addItem("BucketSort (Блочная)", "CPU_BucketSort");
    m_visualAlgCombo->addItem("PancakeSort (Блинная)", "CPU_PancakeSort");
    m_visualAlgCombo->addItem("BogoSort (🎲 Богосорт)", "CPU_BogoSort");
    m_visualAlgCombo->addItem("StoogeSort (Студжа)", "CPU_StoogeSort");
    m_visualAlgCombo->addItem("Odd-Even Sort (Чет-нечет)", "CPU_OddEvenSort");
    m_visualAlgCombo->addItem("CycleSort (Циклическая)", "CPU_CycleSort");
    m_visualAlgCombo->addItem("std::sort (STL Intro)", "CPU_stdSort");
}


// --- БЕНЧМАРКИ КЛИЕНТ-ЛОГИКА ---

void MainWindow::onStartBenchmark() {
    m_accumulatedResults.clear();
    m_statsTable->setRowCount(0);
    
    QList<QListWidgetItem*> items = m_algListWidget->selectedItems();
    if (items.isEmpty()) {
        QMessageBox::warning(this, "Выбор алгоритма", "Выберите хотя бы один алгоритм для проведения испытаний.");
        return;
    }

    m_runBenchBtn->setEnabled(false);
    m_stopBenchBtn->setEnabled(true);
    m_benchProgress->setValue(0);

    Benchmark::Config cfg;
    cfg.arraySize = m_arraySizeSpin->value();
    cfg.runsCount = m_runsSpin->value();
    cfg.dist = static_cast<Benchmark::Distribution>(m_distCombo->currentData().toInt());
    cfg.isDoublePrecision = m_dataTypeCombo->currentIndex() > 0;
    cfg.gpuConnected = m_gpuConnected;

   for (auto* item : qAsConst(items)) {
    QString txt = item->text();
    // CPU алгоритмы (20)
    if (txt.contains("CPU: std::sort")) cfg.selectedAlgorithms.push_back("CPU_std::sort");
    else if (txt.contains("CPU: QuickSort")) cfg.selectedAlgorithms.push_back("CPU_QuickSort");
    else if (txt.contains("CPU: MergeSort")) cfg.selectedAlgorithms.push_back("CPU_MergeSort");
    else if (txt.contains("CPU: HeapSort")) cfg.selectedAlgorithms.push_back("CPU_HeapSort");
    else if (txt.contains("CPU: TimSort")) cfg.selectedAlgorithms.push_back("CPU_TimSort");
    else if (txt.contains("CPU: BubbleSort")) cfg.selectedAlgorithms.push_back("CPU_BubbleSort");
    else if (txt.contains("CPU: SelectionSort")) cfg.selectedAlgorithms.push_back("CPU_SelectionSort");
    else if (txt.contains("CPU: InsertionSort")) cfg.selectedAlgorithms.push_back("CPU_InsertionSort");
    else if (txt.contains("CPU: ShellSort")) cfg.selectedAlgorithms.push_back("CPU_ShellSort");
    else if (txt.contains("CPU: CocktailSort")) cfg.selectedAlgorithms.push_back("CPU_CocktailSort");
    else if (txt.contains("CPU: GnomeSort")) cfg.selectedAlgorithms.push_back("CPU_GnomeSort");
    else if (txt.contains("CPU: CombSort")) cfg.selectedAlgorithms.push_back("CPU_CombSort");
    else if (txt.contains("CPU: RadixSortLSD")) cfg.selectedAlgorithms.push_back("CPU_RadixSortLSD");
    else if (txt.contains("CPU: CountingSort")) cfg.selectedAlgorithms.push_back("CPU_CountingSort");
    else if (txt.contains("CPU: BucketSort")) cfg.selectedAlgorithms.push_back("CPU_BucketSort");
    else if (txt.contains("CPU: PancakeSort")) cfg.selectedAlgorithms.push_back("CPU_PancakeSort");
    else if (txt.contains("CPU: BogoSort")) cfg.selectedAlgorithms.push_back("CPU_BogoSort");
    else if (txt.contains("CPU: StoogeSort")) cfg.selectedAlgorithms.push_back("CPU_StoogeSort");
    else if (txt.contains("CPU: OddEvenSort")) cfg.selectedAlgorithms.push_back("CPU_OddEvenSort");
    else if (txt.contains("CPU: CycleSort")) cfg.selectedAlgorithms.push_back("CPU_CycleSort");
    
    // GPU алгоритмы (20)
    else if (txt.contains("GPU: Bitonic")) cfg.selectedAlgorithms.push_back("GPU_Bitonic");
    else if (txt.contains("GPU: Radix")) cfg.selectedAlgorithms.push_back("GPU_Radix");
    else if (txt.contains("GPU: Odd-Even")) cfg.selectedAlgorithms.push_back("GPU_OddEven");
    else if (txt.contains("GPU: std::sort")) cfg.selectedAlgorithms.push_back("GPU_StdSort");
    else if (txt.contains("GPU: QuickSort")) cfg.selectedAlgorithms.push_back("GPU_QuickSort");
    else if (txt.contains("GPU: MergeSort")) cfg.selectedAlgorithms.push_back("GPU_MergeSort");
    else if (txt.contains("GPU: HeapSort")) cfg.selectedAlgorithms.push_back("GPU_HeapSort");
    else if (txt.contains("GPU: TimSort")) cfg.selectedAlgorithms.push_back("GPU_TimSort");
    else if (txt.contains("GPU: BubbleSort")) cfg.selectedAlgorithms.push_back("GPU_BubbleSort");
    else if (txt.contains("GPU: SelectionSort")) cfg.selectedAlgorithms.push_back("GPU_SelectionSort");
    else if (txt.contains("GPU: InsertionSort")) cfg.selectedAlgorithms.push_back("GPU_InsertionSort");
    else if (txt.contains("GPU: ShellSort")) cfg.selectedAlgorithms.push_back("GPU_ShellSort");
    else if (txt.contains("GPU: CocktailSort")) cfg.selectedAlgorithms.push_back("GPU_CocktailSort");
    else if (txt.contains("GPU: GnomeSort")) cfg.selectedAlgorithms.push_back("GPU_GnomeSort");
    else if (txt.contains("GPU: CombSort")) cfg.selectedAlgorithms.push_back("GPU_CombSort");
    else if (txt.contains("GPU: RadixSortLSD")) cfg.selectedAlgorithms.push_back("GPU_RadixSortLSD");
    else if (txt.contains("GPU: CountingSort")) cfg.selectedAlgorithms.push_back("GPU_CountingSort");
    else if (txt.contains("GPU: BucketSort")) cfg.selectedAlgorithms.push_back("GPU_BucketSort");
    else if (txt.contains("GPU: PancakeSort")) cfg.selectedAlgorithms.push_back("GPU_PancakeSort");
    else if (txt.contains("GPU: BogoSort")) cfg.selectedAlgorithms.push_back("GPU_BogoSort");
    else if (txt.contains("GPU: StoogeSort")) cfg.selectedAlgorithms.push_back("GPU_StoogeSort");
    else if (txt.contains("GPU: OddEvenSort")) cfg.selectedAlgorithms.push_back("GPU_OddEvenSort");
    else if (txt.contains("GPU: CycleSort")) cfg.selectedAlgorithms.push_back("GPU_CycleSort");
}

    m_benchRunner->setConfig(cfg);
    m_benchRunner->start();
}

void MainWindow::onStopBenchmark() {
    if (m_benchRunner && m_benchRunner->isRunning()) {
        m_benchRunner->requestStop();
    }
}

void MainWindow::onBenchmarkProgress(int percent) {
    m_benchProgress->setValue(percent);
}

void MainWindow::onAlgorithmCompleted(const Benchmark::StatResults& s) {
    m_accumulatedResults.push_back(s);

    int row = m_statsTable->rowCount();
    m_statsTable->insertRow(row);

    m_statsTable->setItem(row, 0, new QTableWidgetItem(s.algorithmName));
    m_statsTable->setItem(row, 1, new QTableWidgetItem(s.isGPU ? "NVIDIA GPU (CUDA)" : "Intel/AMD CPU"));
    m_statsTable->setItem(row, 2, new QTableWidgetItem(QString::number(s.arraySize)));
    
    if (s.success) {
        m_statsTable->setItem(row, 3, new QTableWidgetItem(QString::number(s.minTimeMs, 'f', 4)));
        m_statsTable->setItem(row, 4, new QTableWidgetItem(QString::number(s.maxTimeMs, 'f', 4)));
        m_statsTable->setItem(row, 5, new QTableWidgetItem(QString::number(s.avgTimeMs, 'f', 4)));
        m_statsTable->setItem(row, 6, new QTableWidgetItem(QString::number(s.medianTimeMs, 'f', 4)));

        if (s.isGPU) {
            m_statsTable->setItem(row, 7, new QTableWidgetItem(QString::number(s.avgUploadTimeMs, 'f', 3) + " ms"));
            m_statsTable->setItem(row, 8, new QTableWidgetItem(QString::number(s.avgKernelTimeMs, 'f', 3) + " ms"));
            m_statsTable->setItem(row, 9, new QTableWidgetItem(QString::number(s.avgDownloadTimeMs, 'f', 3) + " ms"));
        } else {
            m_statsTable->setItem(row, 7, new QTableWidgetItem("-"));
            m_statsTable->setItem(row, 8, new QTableWidgetItem("-"));
            m_statsTable->setItem(row, 9, new QTableWidgetItem("-"));
        }
    } else {
        m_statsTable->setItem(row, 3, new QTableWidgetItem("Ошибка"));
        m_statsTable->setItem(row, 4, new QTableWidgetItem("-"));
        m_statsTable->setItem(row, 5, new QTableWidgetItem("-"));
        m_statsTable->setItem(row, 6, new QTableWidgetItem("-"));
        m_statsTable->setItem(row, 7, new QTableWidgetItem("-"));
        m_statsTable->setItem(row, 8, new QTableWidgetItem("-"));
        m_statsTable->setItem(row, 9, new QTableWidgetItem(s.errorMsg));
    }

    updateCharts();
}

void MainWindow::onBenchmarkFinished() {
    m_runBenchBtn->setEnabled(true);
    m_stopBenchBtn->setEnabled(false);
}

void MainWindow::updateCharts() {
    m_barSeries->clear();
    
    QBarSet* setAvg = new QBarSet("Среднее время (ms)");
    setAvg->setColor(QColor(59, 130, 246)); // Синий

    QBarSet* setMedian = new QBarSet("Медиана (ms)");
    setMedian->setColor(QColor(168, 85, 247)); // Сиреневый

    QStringList categories;
    double maxVal = 0.001;

    for (const auto& res : m_accumulatedResults) {
        if (!res.success) continue;
        categories << res.algorithmName;
        *setAvg << res.avgTimeMs;
        *setMedian << res.medianTimeMs;
        maxVal = std::max({maxVal, res.avgTimeMs, res.medianTimeMs});
    }

    m_barSeries->append(setAvg);
    m_barSeries->append(setMedian);

    m_axisX->clear();
    m_axisX->append(categories);

    m_axisY->setRange(0.0, maxVal * 1.15); // Запас в 15% сверху
}


// --- ИНТЕРАКТИВНЫЙ СИМУЛЯТОР СОРТИРОВКИ ---

void MainWindow::onGenerateVisualArray() {
    onStopVisual();
    
    int size = m_visualSizeSpin->value();
    m_visualData.resize(size);
    
    // Простая случайная генерация с заполнением вещественными числами
    for (int i = 0; i < size; ++i) {
        m_visualData[i] = QRandomGenerator::global()->generateDouble() * 500.0 + 10.0;
    }
    
    m_visualizer->updateData(m_visualData);
    m_visualStatusLabel->setText("Статус: Массив инициализирован. Нажмите Старт.");
}

void MainWindow::onVisualSpeedChanged(int value) {
    // Преобразуем slider (1-100) в обратно пропорциональную временную задержку (1 - 500 мс)
    // Чем выше значение слайдера, тем быстрее выполняется сортировка (задержка меньше)
    m_visualDelayMs = static_cast<int>(501 - (value * 5.0));
}

void MainWindow::onStartVisualSort() {
    if (m_visualSortThread && m_visualSortThread->isRunning()) {
        if (m_isVisualPaused) {
            onPauseResumeVisual(); // Снять с паузы
        }
        return;
    }

    m_stopVisualRequested = false;
    m_isVisualPaused = false;

    m_visualGenBtn->setEnabled(false);
    m_visualStartBtn->setEnabled(false);
    m_visualPauseBtn->setEnabled(true);
    m_visualStopBtn->setEnabled(true);
    m_visualSizeSpin->setEnabled(false);
    m_visualAlgCombo->setEnabled(false);

    QString algKey = m_visualAlgCombo->currentData().toString();
    m_visualStatusLabel->setText("Статус: Сортировка запущена...");

    // Создаем поток для покадрового выполнения сортировочного цикла
    m_visualSortThread = QThread::create([this, algKey]() {
        CPU::SortContext ctx;
        ctx.stopRequested = &m_stopVisualRequested;
        
        // Передача обновленного состояния в форму
        ctx.stepCallback = [this](const std::vector<double>& currentArray, int red1, int red2, int pivot) {
            // Кросс-потоковый безопасный вызов GUI в Qt6
            QMetaObject::invokeMethod(this, [this, currentArray, red1, red2, pivot]() {
                m_visualizer->updateData(currentArray, red1, red2, pivot);
            }, Qt::QueuedConnection);

            // Обработка паузы анимации
            while (m_isVisualPaused && !m_stopVisualRequested) {
                QThread::msleep(30);
            }

            // Задержка кадра для анимации
            QThread::msleep(m_visualDelayMs);
        };

        // Запуск алгоритма
        std::vector<double> copyArr = m_visualData;
        if (algKey == "CPU_stdSort") {
            CPU::stdSort(copyArr, ctx);
        } else if (algKey == "CPU_QuickSort") {
            CPU::quickSort(copyArr, ctx);
        } else if (algKey == "CPU_MergeSort") {
            CPU::mergeSort(copyArr, ctx);
        } else if (algKey == "CPU_HeapSort") {
            CPU::heapSort(copyArr, ctx);
        } else if (algKey == "CPU_TimSort") {
            CPU::timSort(copyArr, ctx);
        } else if (algKey == "CPU_BubbleSort") {
            CPU::bubbleSort(copyArr, ctx);
        } else if (algKey == "CPU_SelectionSort") {
            CPU::selectionSort(copyArr, ctx);
        } else if (algKey == "CPU_InsertionSort") {
            CPU::insertionSort(copyArr, ctx);
        } else if (algKey == "CPU_ShellSort") {
            CPU::shellSort(copyArr, ctx);
        } else if (algKey == "CPU_CocktailSort") {
            CPU::cocktailSort(copyArr, ctx);
        } else if (algKey == "CPU_GnomeSort") {
            CPU::gnomeSort(copyArr, ctx);
        } else if (algKey == "CPU_CombSort") {
            CPU::combSort(copyArr, ctx);
        } else if (algKey == "CPU_RadixSortLSD") {
            CPU::radixSortLSD(copyArr, ctx);
        } else if (algKey == "CPU_CountingSort") {
            CPU::countingSort(copyArr, ctx);
        } else if (algKey == "CPU_BucketSort") {
            CPU::bucketSort(copyArr, ctx);
        } else if (algKey == "CPU_PancakeSort") {
            CPU::pancakeSort(copyArr, ctx);
        } else if (algKey == "CPU_BogoSort") {
            CPU::bogoSort(copyArr, ctx);
        } else if (algKey == "CPU_StoogeSort") {
            CPU::stoogeSort(copyArr, ctx);
        } else if (algKey == "CPU_OddEvenSort") {
            CPU::oddEvenSort(copyArr, ctx);
        } else if (algKey == "CPU_CycleSort") {
            CPU::cycleSort(copyArr, ctx);
        }

        // Финальное сообщение в главном потоке
        QMetaObject::invokeMethod(this, [this]() {
            m_visualStatusLabel->setText("Статус: Сортировка успешно завершена!");
            onStopVisual();
        }, Qt::QueuedConnection);
    });

    m_visualSortThread->start();
}

void MainWindow::onPauseResumeVisual() {
    if (m_isVisualPaused) {
        m_isVisualPaused = false;
        m_visualPauseBtn->setText("Пауза");
        m_visualStatusLabel->setText("Статус: Сортировка выполняется...");
    } else {
        m_isVisualPaused = true;
        m_visualPauseBtn->setText("Продолжить");
        m_visualStatusLabel->setText("Статус: Сортировка на паузе");
    }
}

void MainWindow::onStopVisual() {
    m_stopVisualRequested = true;
    m_isVisualPaused = false;

    if (m_visualSortThread) {
        m_visualSortThread->quit();
        m_visualSortThread->wait();
        delete m_visualSortThread;
        m_visualSortThread = nullptr;
    }

    m_visualPauseBtn->setText("Пауза");
    m_visualPauseBtn->setEnabled(false);
    m_visualStopBtn->setEnabled(false);
    m_visualStartBtn->setEnabled(true);
    m_visualGenBtn->setEnabled(true);
    m_visualSizeSpin->setEnabled(true);
    m_visualAlgCombo->setEnabled(true);
}

void MainWindow::onVisualStep(const std::vector<double>& arr, int act1, int act2, int piv) {
    m_visualizer->updateData(arr, act1, act2, piv);
}

void MainWindow::onToggleGpu() {
    m_gpuConnected = !m_gpuConnected;
    if (m_gpuConnected) {
        m_ledIndicator->setStyleSheet("background-color: #10b981; border-radius: 4px;"); // Emerald green
        m_telemetryTextLabel->setText("CPU: Intel Core i9-14900K | GPU: NVIDIA GeForce RTX 4090 24GB PCIe Gen4");
        m_toggleGpuBtn->setText("ОТКЛЮЧИТЬ");
    } else {
        m_ledIndicator->setStyleSheet("background-color: #ef4444; border-radius: 4px;"); // Vibrant red
        m_telemetryTextLabel->setText("Шина отключена (Симуляция сбоя)");
        m_toggleGpuBtn->setText("ПОДКЛЮЧИТЬ");
    }
}


// --- ЭКСПОРТ ДАННЫХ ---

void MainWindow::onExportCSV() {
    if (m_accumulatedResults.empty()) {
        QMessageBox::warning(this, "Экспорт CSV", "Нет доступных результатов для сохранения. Сначала проведите тесты.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить результаты в CSV", "", "CSV Files (*.csv);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка экспорта", "Не удалось создать целевой файл. Проверьте права доступа.");
        return;
    }

    QTextStream out(&file);
    // Пишем заголовок UTF8 CSV
    out << "Algorithm;Device;ArraySize;MinTimeMs;MaxTimeMs;AvgTimeMs;MedianTimeMs;VarianceMs;UploadTimeMs;KernelTimeMs;DownloadTimeMs;Status\n";

    for (const auto& r : m_accumulatedResults) {
        out << r.algorithmName << ";"
            << (r.isGPU ? "GPU" : "CPU") << ";"
            << r.arraySize << ";"
            << r.minTimeMs << ";"
            << r.maxTimeMs << ";"
            << r.avgTimeMs << ";"
            << r.medianTimeMs << ";"
            << r.varianceMs << ";"
            << r.avgUploadTimeMs << ";"
            << r.avgKernelTimeMs << ";"
            << r.avgDownloadTimeMs << ";"
            << (r.success ? "Success" : "Error") << "\n";
    }

    file.close();
    QMessageBox::information(this, "Экспорт CSV", "Результаты бенчмарков успешно выгружены в " + fileName);
}

void MainWindow::onExportPNG() {
    if (m_accumulatedResults.empty()) {
        QMessageBox::warning(this, "Захват экрана", "Сначала наберите статистику запусков, чтобы график отображал метрики.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Экспортировать график в PNG", "", "Mime-PNG Images (*.png);;All Files (*)");
    if (fileName.isEmpty()) return;

    QPixmap pixmap = m_chartView->grab();
    if (pixmap.save(fileName, "PNG")) {
        QMessageBox::information(this, "Сохранение изображения", "Гистограмма сохранена успешно: " + fileName);
    } else {
        QMessageBox::critical(this, "Ошибка экспорта", "Ошибка записи PNG-файла.");
    }
}
