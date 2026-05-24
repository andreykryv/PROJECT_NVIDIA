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
#include "cpu_algorithms.h"

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
    connect(m_benchRunner, &BenchmarkRunner::finished, this, &MainWindow::onBenchmarkFinished);

    setupUI();
    setupCharts();
    loadAvailableAlgorithms();
    onGenerateVisualArray(); // Инициализация первого массива для визуализатора
}

MainWindow::~MainWindow() {
    onStopVisual();
    if (m_benchRunner) {
        m_benchRunner->requestStop();
        m_benchRunner->quit();
        m_benchRunner->wait();
    }
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Левая панель - Управление бенчмарками и глобальные настройки
    QGroupBox* sidebarGroupBox = new QGroupBox("Конфигурация стенда", this);
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
    // Наполнение чекбоксов бенчмарка
    m_algListWidget->addItem("CPU: std::sort");
    m_algListWidget->addItem("CPU: QuickSort");
    m_algListWidget->addItem("CPU: MergeSort");
    m_algListWidget->addItem("CPU: HeapSort");
    m_algListWidget->addItem("CPU: TimSort");
    m_algListWidget->addItem("GPU: Bitonic Sort");
    m_algListWidget->addItem("GPU: Radix Sort");
    m_algListWidget->addItem("GPU: Odd-Even Sort");

    // Выбираем первые три по умолчанию
    m_algListWidget->item(0)->setSelected(true);
    m_algListWidget->item(1)->setSelected(true);
    m_algListWidget->item(5)->setSelected(true);

    // Наполнение выпадающего списка визуализатора (только пошаговые CPU сортировки для наглядности)
    m_visualAlgCombo->addItem("QuickSort", "CPU_QuickSort");
    m_visualAlgCombo->addItem("MergeSort", "CPU_MergeSort");
    m_visualAlgCombo->addItem("HeapSort", "CPU_HeapSort");
    m_visualAlgCombo->addItem("TimSort", "CPU_TimSort");
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

    for (auto* item : items) {
        QString txt = item->text();
        if (txt == "CPU: std::sort") cfg.selectedAlgorithms.push_back("CPU_std::sort");
        else if (txt == "CPU: QuickSort") cfg.selectedAlgorithms.push_back("CPU_QuickSort");
        else if (txt == "CPU: MergeSort") cfg.selectedAlgorithms.push_back("CPU_MergeSort");
        else if (txt == "CPU: HeapSort") cfg.selectedAlgorithms.push_back("CPU_HeapSort");
        else if (txt == "CPU: TimSort") cfg.selectedAlgorithms.push_back("CPU_TimSort");
        else if (txt == "GPU: Bitonic Sort") cfg.selectedAlgorithms.push_back("GPU_Bitonic");
        else if (txt == "GPU: Radix Sort") cfg.selectedAlgorithms.push_back("GPU_Radix");
        else if (txt == "GPU: Odd-Even Sort") cfg.selectedAlgorithms.push_back("GPU_OddEven");
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
        if (algKey == "CPU_QuickSort") {
            CPU::quickSort(copyArr, ctx);
        } else if (algKey == "CPU_MergeSort") {
            CPU::mergeSort(copyArr, ctx);
        } else if (algKey == "CPU_HeapSort") {
            CPU::heapSort(copyArr, ctx);
        } else if (algKey == "CPU_TimSort") {
            CPU::timSort(copyArr, ctx);
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
