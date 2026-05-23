/**
 * @file mainwindow.cpp
 * @brief Реализация главного окна SortBench.
 */
#include "mainwindow.h"
#include "sortingvisualizer.h"
#include "benchmarksimulator.h"
#include <QGroupBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QDateTime>

// Алгоритмы CPU
static const char* CPU_ALGORITHMS[] = {
    "CPU_std::sort", "CPU_QuickSort", "CPU_MergeSort", "CPU_HeapSort",
    "CPU_TimSort", "CPU_ShellSort", "CPU_CombSort", "CPU_RadixSortLSD",
    "CPU_CountingSort", "CPU_BucketSort", "CPU_BubbleSort", "CPU_SelectionSort",
    "CPU_InsertionSort", "CPU_CocktailSort", "CPU_GnomeSort", "CPU_OddEvenSort",
    "CPU_CycleSort", "CPU_PancakeSort", "CPU_StoogeSort", "CPU_BogoSort"
};

// Алгоритмы GPU
static const char* GPU_ALGORITHMS[] = {
    "GPU_Bitonic", "GPU_Radix", "GPU_OddEven", "GPU_QuickSort", "GPU_MergeSort", "GPU_BogoSort"
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_algCheckboxCount(0)
    , m_isSimulating(false)
    , m_isVisualRunning(false)
    , m_isVisualPaused(false)
    , m_simulator(new BenchmarkSimulator(this))
{
    setWindowTitle("SortBench: CUDA vs CPU");
    setupUI();
    
    // Инициализация результатов примерами
    m_results << BenchmarkResult{"CPU_std::sort", false, 100000, 8.52, 8.12, 9.05, 8.51, 0.15, 0, 0, 0, true, ""}
              << BenchmarkResult{"CPU_QuickSort", false, 100000, 11.24, 10.85, 12.11, 11.15, 0.22, 0, 0, 0, true, ""}
              << BenchmarkResult{"GPU_Bitonic", true, 100000, 1.25, 1.15, 1.35, 1.24, 0.05, 0.65, 0.12, 0.48, true, ""}
              << BenchmarkResult{"GPU_Radix", true, 100000, 0.85, 0.78, 0.94, 0.84, 0.02, 0.58, 0.05, 0.22, true, ""};
    
    updateCharts();
    updateResultsTable();
    
    // Загрузка содержимого файлов кода
    m_cppFiles["CMakeLists.txt"] = getCodeFileContent("CMakeLists.txt");
    m_cppFiles["main.cpp"] = getCodeFileContent("main.cpp");
    m_cppFiles["sorting_visualizer.h"] = getCodeFileContent("sorting_visualizer.h");
    m_cppFiles["gpu_algorithms.cu"] = getCodeFileContent("gpu_algorithms.cu");
    m_cppFiles["cpu_algorithms.cpp"] = getCodeFileContent("cpu_algorithms.cpp");
    
    connect(m_simulator, &BenchmarkSimulator::progressUpdated, this, &MainWindow::onSimulationProgress);
    connect(m_simulator, &BenchmarkSimulator::completed, this, &MainWindow::onSimulationComplete);
}

MainWindow::~MainWindow() {
    delete m_simulator;
}

void MainWindow::setupUI() {
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);
    
    setupBenchmarkTab();
    setupVisualizationTab();
    setupTheoryTab();
    setupReportsTab();
    
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
}

void MainWindow::setupBenchmarkTab() {
    m_benchmarkWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(m_benchmarkWidget);
    
    // Верхняя панель с настройками
    QHBoxLayout *settingsLayout = new QHBoxLayout();
    
    // Группа выбора алгоритмов
    QGroupBox *algGroup = new QGroupBox("Алгоритмы");
    QVBoxLayout *algLayout = new QVBoxLayout(algGroup);
    QScrollArea *algScroll = new QScrollArea();
    algScroll->setWidgetResizable(true);
    QWidget *algContainer = new QWidget();
    QVBoxLayout *algContainerLayout = new QVBoxLayout(algContainer);
    
    // CPU алгоритмы
    QLabel *cpuLabel = new QLabel("<b>CPU Алгоритмы (O(N log N))</b>");
    algContainerLayout->addWidget(cpuLabel);
    QHBoxLayout *cpuLayout = new QHBoxLayout();
    for (int i = 0; i < 10 && i < 20; ++i) {
        m_algCheckboxes[m_algCheckboxCount] = new QCheckBox(CPU_ALGORITHMS[i]);
        m_algCheckboxes[m_algCheckboxCount]->setChecked(i < 2);
        cpuLayout->addWidget(m_algCheckboxes[m_algCheckboxCount++]);
    }
    algContainerLayout->addLayout(cpuLayout);
    
    // Медленные CPU алгоритмы
    QLabel *slowCpuLabel = new QLabel("<b>CPU Алгоритмы (O(N²))</b>");
    slowCpuLabel->setStyleSheet("color: #a1a1aa;");
    algContainerLayout->addWidget(slowCpuLabel);
    QHBoxLayout *slowCpuLayout = new QHBoxLayout();
    for (int i = 10; i < 20; ++i) {
        m_algCheckboxes[m_algCheckboxCount] = new QCheckBox(CPU_ALGORITHMS[i]);
        slowCpuLayout->addWidget(m_algCheckboxes[m_algCheckboxCount++]);
    }
    algContainerLayout->addLayout(slowCpuLayout);
    
    // GPU алгоритмы
    QLabel *gpuLabel = new QLabel("<b>GPU CUDA Алгоритмы</b>");
    gpuLabel->setStyleSheet("color: #818cf8;");
    algContainerLayout->addWidget(gpuLabel);
    QHBoxLayout *gpuLayout = new QHBoxLayout();
    for (int i = 0; i < 6; ++i) {
        m_algCheckboxes[m_algCheckboxCount] = new QCheckBox(GPU_ALGORITHMS[i]);
        m_algCheckboxes[m_algCheckboxCount]->setChecked(i < 2);
        gpuLayout->addWidget(m_algCheckboxes[m_algCheckboxCount++]);
    }
    algContainerLayout->addLayout(gpuLayout);
    
    algContainerLayout->addStretch();
    algScroll->setWidget(algContainer);
    algLayout->addWidget(algScroll);
    settingsLayout->addWidget(algGroup, 1);
    
    // Панель настроек теста
    QGroupBox *testSettingsGroup = new QGroupBox("Настройки теста");
    QVBoxLayout *testSettingsLayout = new QVBoxLayout(testSettingsGroup);
    
    QHBoxLayout *sizeLayout = new QHBoxLayout();
    sizeLayout->addWidget(new QLabel("Размер массива:"));
    m_arraySizeSpin = new QSpinBox();
    m_arraySizeSpin->setRange(100, 10000000);
    m_arraySizeSpin->setValue(100000);
    m_arraySizeSpin->setSingleStep(10000);
    sizeLayout->addWidget(m_arraySizeSpin);
    testSettingsLayout->addLayout(sizeLayout);
    
    QHBoxLayout *distLayout = new QHBoxLayout();
    distLayout->addWidget(new QLabel("Распределение:"));
    m_distributionCombo = new QComboBox();
    m_distributionCombo->addItems({"Uniform", "Sorted", "Reverse Sorted", "Nearly Sorted", "Random"});
    distLayout->addWidget(m_distributionCombo);
    testSettingsLayout->addLayout(distLayout);
    
    QHBoxLayout *typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Тип данных:"));
    m_dataTypeCombo = new QComboBox();
    m_dataTypeCombo->addItems({"double", "float", "int"});
    typeLayout->addWidget(m_dataTypeCombo);
    testSettingsLayout->addLayout(typeLayout);
    
    QHBoxLayout *runsLayout = new QHBoxLayout();
    runsLayout->addWidget(new QLabel("Количество запусков:"));
    m_runsCountSpin = new QSpinBox();
    m_runsCountSpin->setRange(1, 100);
    m_runsCountSpin->setValue(5);
    runsLayout->addWidget(m_runsCountSpin);
    testSettingsLayout->addLayout(runsLayout);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_runButton = new QPushButton("Запустить бенчмарк");
    m_runButton->setIcon(QIcon::fromTheme("media-playback-start"));
    m_stopButton = new QPushButton("Стоп");
    m_stopButton->setEnabled(false);
    m_stopButton->setIcon(QIcon::fromTheme("media-playback-stop"));
    btnLayout->addWidget(m_runButton);
    btnLayout->addWidget(m_stopButton);
    testSettingsLayout->addLayout(btnLayout);
    
    connect(m_runButton, &QPushButton::clicked, this, &MainWindow::onRunBenchmark);
    connect(m_stopButton, &QPushButton::clicked, this, &MainWindow::onStopBenchmark);
    
    settingsLayout->addWidget(testSettingsGroup);
    mainLayout->addLayout(settingsLayout);
    
    // Прогресс бар и лог
    m_progressLabel = new QLabel("Готов к запуску");
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressLabel);
    mainLayout->addWidget(m_progressBar);
    
    m_logEdit = new QTextEdit();
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumHeight(100);
    mainLayout->addWidget(m_logEdit);
    
    // Графики и таблица результатов
    QHBoxLayout *resultsLayout = new QHBoxLayout();
    
    m_chartView = new QChartView();
    m_chartView->setMinimumHeight(300);
    resultsLayout->addWidget(m_chartView, 1);
    
    m_resultsTable = new QTableWidget();
    m_resultsTable->setColumnCount(8);
    m_resultsTable->setHorizontalHeaderLabels({"Алгоритм", "Среднее (мс)", "Мин (мс)", "Макс (мс)", 
                                                "Медиана (мс)", "Дисперсия", "Upload (мс)", "Kernel (мс)"});
    m_resultsTable->horizontalHeader()->setStretchLastSection(true);
    resultsLayout->addWidget(m_resultsTable, 1);
    
    mainLayout->addLayout(resultsLayout);
    
    // Кнопки экспорта
    QHBoxLayout *exportLayout = new QHBoxLayout();
    QPushButton *exportCsvBtn = new QPushButton("Экспорт CSV");
    QPushButton *exportJsonBtn = new QPushButton("Экспорт JSON");
    exportLayout->addWidget(exportCsvBtn);
    exportLayout->addWidget(exportJsonBtn);
    exportLayout->addStretch();
    mainLayout->addLayout(exportLayout);
    
    connect(exportCsvBtn, &QPushButton::clicked, this, &MainWindow::onExportCSV);
    connect(exportJsonBtn, &QPushButton::clicked, this, &MainWindow::onExportJSON);
    
    m_tabWidget->addTab(m_benchmarkWidget, "Бенчмарки");
}

void MainWindow::setupVisualizationTab() {
    m_visualWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(m_visualWidget);
    
    // Панель управления
    QHBoxLayout *controlLayout = new QHBoxLayout();
    
    QGroupBox *visualSettingsGroup = new QGroupBox("Настройки визуализации");
    QVBoxLayout *visualSettingsLayout = new QVBoxLayout(visualSettingsGroup);
    
    QHBoxLayout *algLayout = new QHBoxLayout();
    algLayout->addWidget(new QLabel("Алгоритм:"));
    m_visualAlgCombo = new QComboBox();
    m_visualAlgCombo->addItems({
        "QuickSort", "MergeSort", "HeapSort", "BitonicSort", "BubbleSort",
        "SelectionSort", "InsertionSort", "ShellSort", "CocktailSort",
        "GnomeSort", "CombSort", "RadixLSD", "CountingSort", "PancakeSort",
        "BogoSort", "StoogeSort", "OddEvenSort", "CycleSort"
    });
    algLayout->addWidget(m_visualAlgCombo);
    visualSettingsLayout->addLayout(algLayout);
    
    QHBoxLayout *sizeLayout = new QHBoxLayout();
    sizeLayout->addWidget(new QLabel("Размер:"));
    m_visualSizeSpin = new QSpinBox();
    m_visualSizeSpin->setRange(10, 100);
    m_visualSizeSpin->setValue(45);
    sizeLayout->addWidget(m_visualSizeSpin);
    visualSettingsLayout->addLayout(sizeLayout);
    
    QHBoxLayout *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("Скорость:"));
    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(1, 100);
    m_speedSlider->setValue(60);
    speedLayout->addWidget(m_speedSlider);
    visualSettingsLayout->addLayout(speedLayout);
    
    connect(m_speedSlider, &QSlider::valueChanged, this, &MainWindow::onSpeedChanged);
    connect(m_visualAlgCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged), 
            this, &MainWindow::onAlgorithmSelected);
    
    controlLayout->addWidget(visualSettingsGroup);
    
    // Кнопки управления
    QGroupBox *controlGroup = new QGroupBox("Управление");
    QVBoxLayout *controlGroupLayout = new QVBoxLayout(controlGroup);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_generateButton = new QPushButton("Новый массив");
    m_startVisualButton = new QPushButton("Старт");
    m_pauseVisualButton = new QPushButton("Пауза");
    
    btnLayout->addWidget(m_generateButton);
    btnLayout->addWidget(m_startVisualButton);
    btnLayout->addWidget(m_pauseVisualButton);
    controlGroupLayout->addLayout(btnLayout);
    
    m_visualStatusLabel = new QLabel("Готов к визуализации");
    controlGroupLayout->addWidget(m_visualStatusLabel);
    
    connect(m_generateButton, &QPushButton::clicked, this, &MainWindow::onGenerateNewArray);
    connect(m_startVisualButton, &QPushButton::clicked, this, &MainWindow::onStartVisualization);
    connect(m_pauseVisualButton, &QPushButton::clicked, this, &MainWindow::onPauseVisualization);
    
    controlLayout->addWidget(controlGroup);
    mainLayout->addLayout(controlLayout);
    
    // Визуализатор
    m_visualizer = new SortingVisualizer();
    m_visualizer->setMinimumHeight(400);
    mainLayout->addWidget(m_visualizer);
    
    m_tabWidget->addTab(m_visualWidget, "Визуализация");
}

void MainWindow::setupTheoryTab() {
    m_theoryWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_theoryWidget);
    
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("Файл:"));
    m_fileSelector = new QComboBox();
    m_fileSelector->addItems({"CMakeLists.txt", "main.cpp", "sorting_visualizer.h", 
                              "gpu_algorithms.cu", "cpu_algorithms.cpp"});
    topLayout->addWidget(m_fileSelector);
    
    QPushButton *copyBtn = new QPushButton("Копировать");
    connect(copyBtn, &QPushButton::clicked, [this]() {
        QApplication::clipboard()->setText(m_codeViewer->toPlainText());
        m_visualStatusLabel->setText("Код скопирован в буфер обмена");
    });
    topLayout->addWidget(copyBtn);
    topLayout->addStretch();
    
    layout->addLayout(topLayout);
    
    m_codeViewer = new QTextEdit();
    m_codeViewer->setReadOnly(true);
    m_codeViewer->setFont(QFont("Consolas", 10));
    layout->addWidget(m_codeViewer);
    
    connect(m_fileSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
        QString fileName = m_fileSelector->currentText();
        if (m_cppFiles.contains(fileName)) {
            m_codeViewer->setPlainText(m_cppFiles[fileName]);
        }
    });
    
    // Загрузка первого файла
    if (!m_cppFiles.isEmpty()) {
        m_codeViewer->setPlainText(m_cppFiles.begin().value());
    }
    
    m_tabWidget->addTab(m_theoryWidget, "Теория/Код");
}

void MainWindow::setupReportsTab() {
    m_reportsWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_reportsWidget);
    
    m_reportText = new QTextEdit();
    m_reportText->setReadOnly(true);
    m_reportText->setHtml(R"(
        <h1>Отчеты о бенчмарках</h1>
        <h2>Сравнение производительности CPU vs GPU</h2>
        <p>Данный стенд предназначен для сравнения производительности алгоритмов сортировки, 
           реализованных на CPU (C++) и GPU (CUDA).</p>
        
        <h3>Основные выводы:</h3>
        <ul>
            <li><b>GPU алгоритмы</b> показывают значительное ускорение на больших массивах (10-100x)</li>
            <li><b>Bitonic Sort</b> эффективен для GPU благодаря регулярной структуре доступа к памяти</li>
            <li><b>Radix Sort</b> на GPU демонстрирует наилучшую производительность для целочисленных данных</li>
            <li><b>CPU std::sort</b> остается оптимальным выбором для малых и средних массивов</li>
        </ul>
        
        <h3>Метрики:</h3>
        <table border="1" cellpadding="5">
            <tr><th>Метрика</th><th>Описание</th></tr>
            <tr><td>Avg Time</td><td>Среднее время выполнения (мс)</td></tr>
            <tr><td>Min/Max</td><td>Минимальное/максимальное время из серии запусков</td></tr>
            <tr><td>Variance</td><td>Дисперсия времени выполнения</td></tr>
            <tr><td>Upload/Download</td><td>Время передачи данных через PCIe</td></tr>
            <tr><td>Kernel</td><td>Чистое время выполнения ядра GPU</td></tr>
        </table>
    )");
    layout->addWidget(m_reportText);
    
    m_tabWidget->addTab(m_reportsWidget, "Отчеты");
}

void MainWindow::onRunBenchmark() {
    if (m_isSimulating) return;
    
    QVector<QString> selectedAlgs;
    for (int i = 0; i < m_algCheckboxCount; ++i) {
        if (m_algCheckboxes[i]->isChecked()) {
            selectedAlgs.append(m_algCheckboxes[i]->text());
        }
    }
    
    if (selectedAlgs.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Выберите хотя бы один алгоритм");
        return;
    }
    
    m_isSimulating = true;
    m_runButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_progressLabel->setText("Запуск бенчмарков...");
    m_logEdit->append(QDateTime::currentDateTime().toString("[hh:mm:ss] Запуск бенчмарков..."));
    
    int arraySize = m_arraySizeSpin->value();
    QString distribution = m_distributionCombo->currentText();
    QString dataType = m_dataTypeCombo->currentText();
    int runsCount = m_runsCountSpin->value();
    
    m_simulator->runBenchmark(selectedAlgs, arraySize, distribution, dataType, runsCount);
}

void MainWindow::onStopBenchmark() {
    if (!m_isSimulating) return;
    
    m_simulator->stop();
    m_isSimulating = false;
    m_runButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_progressLabel->setText("Остановлено пользователем");
    m_logEdit->append(QDateTime::currentDateTime().toString("[hh:mm:ss] Остановлено пользователем"));
}

void MainWindow::onGenerateNewArray() {
    m_visualizer->generateNewArray(m_visualSizeSpin->value());
    m_visualStatusLabel->setText("Сгенерирован новый случайный массив");
}

void MainWindow::onStartVisualization() {
    if (m_isVisualRunning) {
        if (m_isVisualPaused) {
            m_isVisualPaused = false;
            m_visualizer->resume();
            m_visualStatusLabel->setText("Сортировка возобновлена");
        }
        return;
    }
    
    m_isVisualRunning = true;
    m_isVisualPaused = false;
    m_visualStatusLabel->setText("Сортировка запущена...");
    
    QString algName = m_visualAlgCombo->currentText().replace(" ", "");
    m_visualizer->startSort(algName, m_speedSlider->value());
}

void MainWindow::onPauseVisualization() {
    if (!m_isVisualRunning) return;
    
    m_isVisualPaused = true;
    m_visualizer->pause();
    m_visualStatusLabel->setText("Анимация приостановлена");
}

void MainWindow::onSpeedChanged(int value) {
    m_visualizer->setSpeed(value);
}

void MainWindow::onAlgorithmSelected(const QString &alg) {
    Q_UNUSED(alg);
    // Сброс при смене алгоритма
    m_isVisualRunning = false;
    m_isVisualPaused = false;
}

void MainWindow::onExportCSV() {
    if (m_results.isEmpty()) {
        QMessageBox::information(this, "Информация", "Нет результатов для экспорта");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, "Экспорт CSV", "benchmark_results.csv", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "Algorithm,IsGPU,ArraySize,AvgTimeMs,MinTimeMs,MaxTimeMs,MedianTimeMs,VarianceMs,UploadTimeMs,KernelTimeMs,DownloadTimeMs,Success,ErrorMsg\n";
        
        for (const auto &res : m_results) {
            out << res.algorithmName << ","
                << (res.isGPU ? "true" : "false") << ","
                << res.arraySize << ","
                << res.avgTimeMs << ","
                << res.minTimeMs << ","
                << res.maxTimeMs << ","
                << res.medianTimeMs << ","
                << res.varianceMs << ","
                << res.avgUploadTimeMs << ","
                << res.avgKernelTimeMs << ","
                << res.avgDownloadTimeMs << ","
                << (res.success ? "true" : "false") << ","
                << res.errorMsg << "\n";
        }
        
        file.close();
        QMessageBox::information(this, "Успех", "Результаты экспортированы в CSV");
    }
}

void MainWindow::onExportJSON() {
    if (m_results.isEmpty()) {
        QMessageBox::information(this, "Информация", "Нет результатов для экспорта");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this, "Экспорт JSON", "benchmark_results.json", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "{\n  \"results\": [\n";
        
        for (int i = 0; i < m_results.size(); ++i) {
            const auto &res = m_results[i];
            out << "    {\n";
            out << "      \"algorithmName\": \"" << res.algorithmName << "\",\n";
            out << "      \"isGPU\": " << (res.isGPU ? "true" : "false") << ",\n";
            out << "      \"arraySize\": " << res.arraySize << ",\n";
            out << "      \"avgTimeMs\": " << res.avgTimeMs << ",\n";
            out << "      \"minTimeMs\": " << res.minTimeMs << ",\n";
            out << "      \"maxTimeMs\": " << res.maxTimeMs << ",\n";
            out << "      \"medianTimeMs\": " << res.medianTimeMs << ",\n";
            out << "      \"varianceMs\": " << res.varianceMs << ",\n";
            out << "      \"avgUploadTimeMs\": " << res.avgUploadTimeMs << ",\n";
            out << "      \"avgKernelTimeMs\": " << res.avgKernelTimeMs << ",\n";
            out << "      \"avgDownloadTimeMs\": " << res.avgDownloadTimeMs << ",\n";
            out << "      \"success\": " << (res.success ? "true" : "false") << ",\n";
            out << "      \"errorMsg\": \"" << res.errorMsg << "\"\n";
            out << "    }";
            if (i < m_results.size() - 1) out << ",";
            out << "\n";
        }
        
        out << "  ]\n}\n";
        file.close();
        QMessageBox::information(this, "Успех", "Результаты экспортированы в JSON");
    }
}

void MainWindow::onTabChanged(int index) {
    Q_UNUSED(index);
    // Обновление при переключении вкладок
}

void MainWindow::onSimulationProgress(int progress) {
    m_progressBar->setValue(progress);
}

void MainWindow::onSimulationComplete() {
    m_isSimulating = false;
    m_runButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_progressLabel->setText("Бенчмарки завершены");
    m_logEdit->append(QDateTime::currentDateTime().toString("[hh:mm:ss] Бенчмарки завершены"));
    
    // Здесь можно добавить реальные результаты от симулятора
    updateCharts();
    updateResultsTable();
}

void MainWindow::onCopyCode(const QString &code) {
    QApplication::clipboard()->setText(code);
    m_visualStatusLabel->setText("Код скопирован в буфер обмена");
}

void MainWindow::updateCharts() {
    QChart *chart = new QChart();
    chart->setTitle("Сравнение производительности алгоритмов (мс)");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    
    QBarSeries *series = new QBarSeries();
    
    QMap<QString, QBarSet*> barSets;
    QStringList categories;
    
    for (const auto &res : m_results) {
        if (!barSets.contains(res.algorithmName)) {
            QBarSet *barSet = new QBarSet(res.algorithmName);
            barSet->setColor(res.isGPU ? QColor(99, 102, 241) : QColor(59, 130, 246));
            barSet->append(res.avgTimeMs);
            series->append(barSet);
            barSets[res.algorithmName] = barSet;
        }
        if (!categories.contains(res.algorithmName)) {
            categories << res.algorithmName;
        }
    }
    
    chart->addSeries(series);
    
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Время (мс)");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    
    m_chartView->setChart(chart);
}

void MainWindow::updateResultsTable() {
    m_resultsTable->setRowCount(m_results.size());
    
    for (int i = 0; i < m_results.size(); ++i) {
        const auto &res = m_results[i];
        m_resultsTable->setItem(i, 0, new QTableWidgetItem(res.algorithmName));
        m_resultsTable->item(i, 0)->setForeground(res.isGPU ? QColor(99, 102, 241) : Qt::white);
        m_resultsTable->setItem(i, 1, new QTableWidgetItem(QString::number(res.avgTimeMs, 'f', 2)));
        m_resultsTable->setItem(i, 2, new QTableWidgetItem(QString::number(res.minTimeMs, 'f', 2)));
        m_resultsTable->setItem(i, 3, new QTableWidgetItem(QString::number(res.maxTimeMs, 'f', 2)));
        m_resultsTable->setItem(i, 4, new QTableWidgetItem(QString::number(res.medianTimeMs, 'f', 2)));
        m_resultsTable->setItem(i, 5, new QTableWidgetItem(QString::number(res.varianceMs, 'f', 2)));
        m_resultsTable->setItem(i, 6, new QTableWidgetItem(QString::number(res.avgUploadTimeMs, 'f', 2)));
        m_resultsTable->setItem(i, 7, new QTableWidgetItem(QString::number(res.avgKernelTimeMs, 'f', 2)));
    }
}

QString MainWindow::getCodeFileContent(const QString &fileName) {
    // Возвращает статическое содержимое файлов кода
    if (fileName == "CMakeLists.txt") {
        return R"(cmake_minimum_required(VERSION 3.20)
project(SortBench LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

add_executable(SortBench
    main.cpp
    mainwindow.h
    mainwindow.cpp
    sorting_visualizer.h
    sorting_visualizer.cpp
    cpu_algorithms.h
    cpu_algorithms.cpp
    gpu_algorithms.h
    gpu_algorithms.cu
)

target_link_libraries(SortBench PRIVATE
    Qt6::Widgets
    Qt6::Charts
    CUDA::cudart
))";
    } else if (fileName == "main.cpp") {
        return R"(#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(24, 24, 27));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    app.setPalette(darkPalette);
    
    MainWindow w;
    w.showMaximized();
    return app.exec();
})";
    } else if (fileName == "sorting_visualizer.h") {
        return R"(#pragma once
#include <QWidget>
#include <vector>

class SortingVisualizer : public QWidget {
    Q_OBJECT
public:
    explicit SortingVisualizer(QWidget* parent = nullptr);
    void updateData(const std::vector<double>& newData, int active1 = -1, int active2 = -1);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    std::vector<double> m_data;
    int m_activeIdx1 = -1;
    int m_activeIdx2 = -1;
};)";
    } else if (fileName == "gpu_algorithms.cu") {
        return R"(#include "gpu_algorithms.h"
#include <cuda_runtime.h>

__global__ void bitonicSortKernel(double* d_arr, int j, int k, int n) {
    unsigned int i = threadIdx.x + blockDim.x * blockIdx.x;
    unsigned int ixj = i ^ j;
    
    if (ixj > i && i < n && ixj < n) {
        if ((i & k) == 0) {
            if (d_arr[i] > d_arr[ixj]) {
                double temp = d_arr[i];
                d_arr[i] = d_arr[ixj];
                d_arr[ixj] = temp;
            }
        }
    }
})";
    } else if (fileName == "cpu_algorithms.cpp") {
        return R"(#include "cpu_algorithms.h"
#include <algorithm>

namespace CPU {
    void quickSort(std::vector<double>& arr, int low, int high) {
        if (low >= high) return;
        
        double pivot = arr[low + (high - low) / 2];
        int i = low, j = high;
        
        while (i <= j) {
            while (arr[i] < pivot) i++;
            while (arr[j] > pivot) j--;
            if (i <= j) {
                std::swap(arr[i], arr[j]);
                i++; j--;
            }
        }
        
        quickSort(arr, low, j);
        quickSort(arr, i, high);
    }
})";
    }
    
    return "// Файл не найден";
}
