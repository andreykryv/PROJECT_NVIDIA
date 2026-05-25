/**
 * @file interactive_visualizer.cpp
 * @brief Реализация окна интерактивной визуализации.
 * 
 * Поддерживает все CPU алгоритмы из библиотеки cpu_algorithms.h,
 * а также отображает GPU алгоритмы с предупреждением о невозможности визуализации.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "interactive_visualizer.h"
#include "cpu_algorithms.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QPushButton>

InteractiveVisualizerWindow::InteractiveVisualizerWindow(QWidget *parent)
    : QDialog(parent)
    , m_stopRequested(false)
    , m_isPaused(false)
    , m_delayMs(50)          // начальная задержка (скорость по умолчанию)
{
    setupUI();
    loadAlgorithms();
    onGenerateArray();       // сгенерировать начальный массив
    setWindowTitle("Интерактивный симулятор сортировки");
    resize(1100, 700);
}

InteractiveVisualizerWindow::~InteractiveVisualizerWindow() {
    onStopSort();            // остановить поток, если активен
    if (m_sortThread) {
        m_sortThread->quit();
        m_sortThread->wait();
        delete m_sortThread;
    }
}

void InteractiveVisualizerWindow::setupUI() {
    // Основной вертикальный layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // --- Область визуализации ---
    m_visualizer = new SortingVisualizer(this);
    m_visualizer->setMinimumHeight(400);
    mainLayout->addWidget(m_visualizer, 1);

    // --- Панель управления ---
    QGroupBox* controlBox = new QGroupBox("Управление симуляцией", this);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlBox);
    controlLayout->setSpacing(12);

    controlLayout->addWidget(new QLabel("Алгоритм:", this));
    m_algCombo = new QComboBox(this);
    m_algCombo->setMinimumWidth(200);
    connect(m_algCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &InteractiveVisualizerWindow::onAlgorithmChanged);
    controlLayout->addWidget(m_algCombo);

    controlLayout->addWidget(new QLabel("Размер (N):", this));
    m_sizeSpin = new QSpinBox(this);
    m_sizeSpin->setRange(5, 1000);
    m_sizeSpin->setValue(60);
    controlLayout->addWidget(m_sizeSpin);

    m_genBtn = new QPushButton("Сгенерировать массив", this);
    connect(m_genBtn, &QPushButton::clicked, this, &InteractiveVisualizerWindow::onGenerateArray);
    controlLayout->addWidget(m_genBtn);

    controlLayout->addStretch();

    m_startBtn = new QPushButton("Старт", this);
    m_startBtn->setObjectName("startBtn");
    connect(m_startBtn, &QPushButton::clicked, this, &InteractiveVisualizerWindow::onStartSort);
    controlLayout->addWidget(m_startBtn);

    m_pauseBtn = new QPushButton("Пауза", this);
    m_pauseBtn->setEnabled(false);
    connect(m_pauseBtn, &QPushButton::clicked, this, &InteractiveVisualizerWindow::onPauseResume);
    controlLayout->addWidget(m_pauseBtn);

    m_stopBtn = new QPushButton("Сброс", this);
    m_stopBtn->setEnabled(false);
    connect(m_stopBtn, &QPushButton::clicked, this, &InteractiveVisualizerWindow::onStopSort);
    controlLayout->addWidget(m_stopBtn);

    controlLayout->addWidget(new QLabel("Скорость:", this));
    m_speedSlider = new QSlider(Qt::Horizontal, this);
    m_speedSlider->setRange(1, 100);
    m_speedSlider->setValue(50);
    m_speedSlider->setFixedWidth(150);
    connect(m_speedSlider, &QSlider::valueChanged, this, &InteractiveVisualizerWindow::onSpeedChanged);
    controlLayout->addWidget(m_speedSlider);

    mainLayout->addWidget(controlBox);

    // Статусная строка
    m_statusLabel = new QLabel("Статус: Готов к работе", this);
    m_statusLabel->setStyleSheet("color: #a1a1aa; font-weight: bold; padding: 4px;");
    mainLayout->addWidget(m_statusLabel);

    // Применяем тёмную стилизацию (как у главного окна)
    setStyleSheet(
        "QDialog { background-color: #0c0c0e; }"
        "QGroupBox { border: 1px solid #1f1f23; border-radius: 12px; margin-top: 15px; font-weight: bold; color: #3b82f6; background-color: #111115; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 15px; padding: 0 5px; }"
        "QPushButton { background-color: #1c1c24; border: 1px solid #2d2d34; color: #e4e4e7; border-radius: 8px; padding: 5px 12px; }"
        "QPushButton:hover { background-color: #272730; border-color: #3f3f46; }"
        "QPushButton:pressed { background-color: #121215; }"
        "QPushButton#startBtn { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2563eb, stop:1 #4f46e5); border: none; }"
        "QPushButton#startBtn:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #6366f1); }"
        "QComboBox, QSpinBox { background-color: #121215; border: 1px solid #1f1f23; border-radius: 6px; padding: 4px; color: #e4e4e7; }"
        "QComboBox::drop-down { width: 20px; border-left-width: 0px; }"
        "QSlider::groove:horizontal { border: none; height: 6px; background: #1c1c24; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #2563eb; border: none; width: 14px; height: 14px; margin: -4px 0; border-radius: 7px; }"
        "QSlider::handle:horizontal:hover { background: #60a5fa; }"
        "QLabel { color: #e4e4e7; }"
    );
}

void InteractiveVisualizerWindow::loadAlgorithms() {
    // --- CPU алгоритмы (20 шт.) ---
    m_algCombo->addItem("CPU: std::sort (STL Intro)", "CPU_stdSort");
    m_algCombo->addItem("CPU: QuickSort (Быстрая)", "CPU_QuickSort");
    m_algCombo->addItem("CPU: MergeSort (Слиянием)", "CPU_MergeSort");
    m_algCombo->addItem("CPU: HeapSort (Пирамид)", "CPU_HeapSort");
    m_algCombo->addItem("CPU: TimSort (Гибридная)", "CPU_TimSort");
    m_algCombo->addItem("CPU: BubbleSort (Пузырёк)", "CPU_BubbleSort");
    m_algCombo->addItem("CPU: SelectionSort (Выбором)", "CPU_SelectionSort");
    m_algCombo->addItem("CPU: InsertionSort (Вставками)", "CPU_InsertionSort");
    m_algCombo->addItem("CPU: ShellSort (Шелла)", "CPU_ShellSort");
    m_algCombo->addItem("CPU: CocktailSort (Шейкер)", "CPU_CocktailSort");
    m_algCombo->addItem("CPU: GnomeSort (Гномья)", "CPU_GnomeSort");
    m_algCombo->addItem("CPU: CombSort (Расчёской)", "CPU_CombSort");
    m_algCombo->addItem("CPU: RadixSortLSD (Поразрядная LSD)", "CPU_RadixSortLSD");
    m_algCombo->addItem("CPU: CountingSort (Подсчётом)", "CPU_CountingSort");
    m_algCombo->addItem("CPU: BucketSort (Блочная)", "CPU_BucketSort");
    m_algCombo->addItem("CPU: PancakeSort (Блинная)", "CPU_PancakeSort");
    m_algCombo->addItem("CPU: BogoSort (Случайная)", "CPU_BogoSort");
    m_algCombo->addItem("CPU: StoogeSort (Придурковатая)", "CPU_StoogeSort");
    m_algCombo->addItem("CPU: OddEvenSort (Чёт-нечет)", "CPU_OddEvenSort");
    m_algCombo->addItem("CPU: CycleSort (Циклическая)", "CPU_CycleSort");

    // --- GPU алгоритмы (6 шт.) – визуализация не поддерживается, но показываем их ---
    m_algCombo->addItem("GPU: Bitonic Sort (CUDA)", "GPU_Bitonic");
    m_algCombo->addItem("GPU: Radix Sort (CUDA)", "GPU_Radix");
    m_algCombo->addItem("GPU: Odd-Even Sort (CUDA)", "GPU_OddEven");
    m_algCombo->addItem("GPU: QuickSort (CUDA)", "GPU_QuickSort");
    m_algCombo->addItem("GPU: MergeSort (CUDA)", "GPU_MergeSort");
    m_algCombo->addItem("GPU: BogoSort (CUDA)", "GPU_BogoSort");
}

void InteractiveVisualizerWindow::onAlgorithmChanged(int index) {
    Q_UNUSED(index);
    QString algKey = m_algCombo->currentData().toString();
    m_isGpuAlgorithm = algKey.startsWith("GPU_");
    if (m_isGpuAlgorithm) {
        updateStatus("ВНИМАНИЕ: Выбран GPU алгоритм. Интерактивная визуализация доступна только для CPU.", true);
    } else {
        updateStatus("Статус: Готов к работе (CPU алгоритм)");
    }
}

void InteractiveVisualizerWindow::onGenerateArray() {
    // Останавливаем текущую сортировку
    onStopSort();

    int size = m_sizeSpin->value();
    m_arrayData.resize(size);
    // Заполняем случайными числами от 10 до 510
    for (int i = 0; i < size; ++i) {
        m_arrayData[i] = QRandomGenerator::global()->generateDouble() * 500.0 + 10.0;
    }
    m_visualizer->updateData(m_arrayData);
    updateStatus(QString("Сгенерирован массив из %1 элементов").arg(size));
}

void InteractiveVisualizerWindow::onStartSort() {
    if (m_sortThread && m_sortThread->isRunning()) {
        // Если уже запущена и на паузе – снимаем паузу
        if (m_isPaused) {
            onPauseResume();
        }
        return;
    }

    if (m_isGpuAlgorithm) {
        QMessageBox::warning(this, "Визуализация недоступна",
                             "Интерактивный режим работы поддерживается только для CPU алгоритмов.\n"
                             "Пожалуйста, выберите любой алгоритм из списка CPU.");
        return;
    }

    m_stopRequested = false;
    m_isPaused = false;
    disableControlsDuringSort(true);

    QString algKey = m_algCombo->currentData().toString();
    updateStatus(QString("Запуск: %1 ...").arg(m_algCombo->currentText()));

    runCpuSort(algKey);
}

void InteractiveVisualizerWindow::runCpuSort(const QString& algKey) {
    // Запускаем сортировку в отдельном потоке с пошаговыми коллбеками
    m_sortThread = QThread::create([this, algKey]() {
        CPU::SortContext ctx;
        ctx.stopRequested = &m_stopRequested;
        ctx.stepCallback = [this](const std::vector<double>& currentArray,
                                  int active1, int active2, int pivot) {
            // Обновляем визуализатор в главном потоке
            QMetaObject::invokeMethod(this, [this, currentArray, active1, active2, pivot]() {
                m_visualizer->updateData(currentArray, active1, active2, pivot);
            }, Qt::QueuedConnection);

            // Обработка паузы
            while (m_isPaused && !m_stopRequested) {
                QThread::msleep(30);
            }
            // Задержка для анимации
            QThread::msleep(m_delayMs.load());
        };

        // Копируем исходный массив (сортировка будет производиться над копией)
        std::vector<double> workArray = m_arrayData;

        // Выбор алгоритма
        if (algKey == "CPU_stdSort") {
            CPU::stdSort(workArray, ctx);
        } else if (algKey == "CPU_QuickSort") {
            CPU::quickSort(workArray, ctx);
        } else if (algKey == "CPU_MergeSort") {
            CPU::mergeSort(workArray, ctx);
        } else if (algKey == "CPU_HeapSort") {
            CPU::heapSort(workArray, ctx);
        } else if (algKey == "CPU_TimSort") {
            CPU::timSort(workArray, ctx);
        } else if (algKey == "CPU_BubbleSort") {
            CPU::bubbleSort(workArray, ctx);
        } else if (algKey == "CPU_SelectionSort") {
            CPU::selectionSort(workArray, ctx);
        } else if (algKey == "CPU_InsertionSort") {
            CPU::insertionSort(workArray, ctx);
        } else if (algKey == "CPU_ShellSort") {
            CPU::shellSort(workArray, ctx);
        } else if (algKey == "CPU_CocktailSort") {
            CPU::cocktailSort(workArray, ctx);
        } else if (algKey == "CPU_GnomeSort") {
            CPU::gnomeSort(workArray, ctx);
        } else if (algKey == "CPU_CombSort") {
            CPU::combSort(workArray, ctx);
        } else if (algKey == "CPU_RadixSortLSD") {
            CPU::radixSortLSD(workArray, ctx);
        } else if (algKey == "CPU_CountingSort") {
            CPU::countingSort(workArray, ctx);
        } else if (algKey == "CPU_BucketSort") {
            CPU::bucketSort(workArray, ctx);
        } else if (algKey == "CPU_PancakeSort") {
            CPU::pancakeSort(workArray, ctx);
        } else if (algKey == "CPU_BogoSort") {
            CPU::bogoSort(workArray, ctx);
        } else if (algKey == "CPU_StoogeSort") {
            CPU::stoogeSort(workArray, ctx);
        } else if (algKey == "CPU_OddEvenSort") {
            CPU::oddEvenSort(workArray, ctx);
        } else if (algKey == "CPU_CycleSort") {
            CPU::cycleSort(workArray, ctx);
        }

        // Финальное обновление статуса и массива
        QMetaObject::invokeMethod(this, [this, workArray]() {
            if (!m_stopRequested) {
                m_arrayData = workArray;
                m_visualizer->updateData(m_arrayData);
                updateStatus("Сортировка успешно завершена!");
            } else {
                updateStatus("Сортировка прервана пользователем.");
            }
            disableControlsDuringSort(false);
        }, Qt::QueuedConnection);
    });

    m_sortThread->start();
}

void InteractiveVisualizerWindow::onPauseResume() {
    if (!m_sortThread || !m_sortThread->isRunning()) return;
    if (m_isPaused) {
        m_isPaused = false;
        m_pauseBtn->setText("Пауза");
        updateStatus("Сортировка продолжается...");
    } else {
        m_isPaused = true;
        m_pauseBtn->setText("Продолжить");
        updateStatus("Сортировка на паузе.");
    }
}

void InteractiveVisualizerWindow::onStopSort() {
    if (m_sortThread && m_sortThread->isRunning()) {
        m_stopRequested = true;
        m_sortThread->quit();
        m_sortThread->wait();
        delete m_sortThread;
        m_sortThread = nullptr;
    }
    m_isPaused = false;
    m_stopRequested = false;
    disableControlsDuringSort(false);
    m_pauseBtn->setText("Пауза");
    updateStatus("Готов (сброс)");
    // Показываем исходный массив заново (без подсветки)
    m_visualizer->updateData(m_arrayData);
}

void InteractiveVisualizerWindow::onSpeedChanged(int value) {
    // Преобразуем ползунок (1–100) в задержку: чем больше скорость (value), тем меньше задержка
    m_delayMs = static_cast<int>(501 - (value * 5.0));
    if (m_delayMs < 1) m_delayMs = 1;
}

void InteractiveVisualizerWindow::disableControlsDuringSort(bool disable) {
    m_genBtn->setDisabled(disable);
    m_sizeSpin->setDisabled(disable);
    m_algCombo->setDisabled(disable);
    m_startBtn->setDisabled(disable);
    m_pauseBtn->setEnabled(disable);
    m_stopBtn->setEnabled(disable);
    if (!disable) {
        m_startBtn->setEnabled(true);
        m_stopBtn->setEnabled(false);
        m_pauseBtn->setEnabled(false);
    }
}

void InteractiveVisualizerWindow::updateStatus(const QString& text, bool isError) {
    QString style = isError ? "color: #f87171; font-weight: bold;" : "color: #a1a1aa; font-weight: bold;";
    m_statusLabel->setStyleSheet(style);
    m_statusLabel->setText("Статус: " + text);
}