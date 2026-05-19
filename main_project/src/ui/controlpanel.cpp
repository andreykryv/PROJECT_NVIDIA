#include "controlpanel.h"
#include "core/algorithmregistry.h"
#include "gpu/cudadeviceinfo.h"
#include "core/sortparams.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QToolTip>
#include <QSettings>
#include <QCursor>
#include <cmath>
#include <random>

namespace SortBench {

// вспомогательные функции преобразования строки → enum
static CpuAlgorithm stringToCpuAlgorithm(const QString &name) {
    if (name == "Bubble Sort")  return CpuAlgorithm::BubbleSort;
    if (name == "Quick Sort")   return CpuAlgorithm::QuickSort;
    if (name == "Merge Sort")   return CpuAlgorithm::MergeSort;
    if (name == "Heap Sort")    return CpuAlgorithm::HeapSort;
    if (name == "Radix Sort")   return CpuAlgorithm::RadixSort;
    if (name == "std::sort")    return CpuAlgorithm::StdSort;
    return CpuAlgorithm::QuickSort;
}

static GpuAlgorithm stringToGpuAlgorithm(const QString &name) {
    if (name == "Bitonic Sort")       return GpuAlgorithm::BitonicSort;
    if (name == "Thrust Radix Sort")  return GpuAlgorithm::ThrustRadixSort;
    if (name == "GPU Quick Sort")     return GpuAlgorithm::GpuQuickSort;
    if (name == "CUB Device Sort")    return GpuAlgorithm::CubDeviceSort;
    return GpuAlgorithm::ThrustRadixSort;
}

ControlPanel::ControlPanel(QWidget *parent)
    : QWidget(parent)
    , m_running(false)
    , m_paused(false)
{
    setupUi();
    populateAlgorithms();
    checkCudaAvailability();
    connectSignals();
}

void ControlPanel::setupUi() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // === Алгоритм сортировки ===
    auto *algoGroup = new QGroupBox("Алгоритм сортировки", this);
    auto *algoLayout = new QVBoxLayout(algoGroup);

    auto *cpuLayout = new QHBoxLayout();
    cpuLayout->addWidget(new QLabel("CPU:", this));
    m_cpuAlgorithmCombo = new QComboBox(this);
    cpuLayout->addWidget(m_cpuAlgorithmCombo, 1);
    m_enableCpuCheck = new QCheckBox("Включить", this);
    m_enableCpuCheck->setChecked(true);
    cpuLayout->addWidget(m_enableCpuCheck);
    algoLayout->addLayout(cpuLayout);

    auto *gpuLayout = new QHBoxLayout();
    gpuLayout->addWidget(new QLabel("GPU:", this));
    m_gpuAlgorithmCombo = new QComboBox(this);
    gpuLayout->addWidget(m_gpuAlgorithmCombo, 1);
    m_enableGpuCheck = new QCheckBox("Включить", this);
    m_enableGpuCheck->setChecked(true);
    gpuLayout->addWidget(m_enableGpuCheck);
    algoLayout->addLayout(gpuLayout);

    m_algoInfoBtn = new QPushButton("?", this);
    m_algoInfoBtn->setFixedSize(30, 30);
    algoLayout->addWidget(m_algoInfoBtn);

    mainLayout->addWidget(algoGroup);

    auto *line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(line1);

    // === Параметры массива ===
    auto *arrayGroup = new QGroupBox("Параметры массива", this);
    auto *arrayLayout = new QVBoxLayout(arrayGroup);

    auto *sizeLayout = new QHBoxLayout();
    sizeLayout->addWidget(new QLabel("Размер:", this));
    m_arraySizeSlider = new QSlider(Qt::Horizontal, this);
    m_arraySizeSlider->setRange(0, 100);
    m_arraySizeSlider->setValue(60);
    sizeLayout->addWidget(m_arraySizeSlider, 1);
    m_arraySizeSpinBox = new QSpinBox(this);
    m_arraySizeSpinBox->setRange(1000, 100000000);
    m_arraySizeSpinBox->setValue(1000000);
    m_arraySizeSpinBox->setSuffix(" эл.");
    sizeLayout->addWidget(m_arraySizeSpinBox);
    arrayLayout->addLayout(sizeLayout);

    auto *typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Тип данных:", this));
    m_dataTypeCombo = new QComboBox(this);
    m_dataTypeCombo->addItems({"int32", "int64", "float", "double"});
    typeLayout->addWidget(m_dataTypeCombo, 1);
    arrayLayout->addLayout(typeLayout);

    auto *distLayout = new QHBoxLayout();
    distLayout->addWidget(new QLabel("Распределение:", this));
    m_distributionCombo = new QComboBox(this);
    m_distributionCombo->addItems({
        "Случайное равномерное", "Почти отсортированное", "Обратный порядок",
        "Много повторов", "Пилообразное", "Шагающий шум", "Нормальное"});
    distLayout->addWidget(m_distributionCombo, 1);
    arrayLayout->addLayout(distLayout);

    auto *seedLayout = new QHBoxLayout();
    seedLayout->addWidget(new QLabel("Seed:", this));
    m_randomSeedSpin = new QSpinBox(this);
    m_randomSeedSpin->setRange(0, INT_MAX);
    m_randomSeedSpin->setValue(42);
    seedLayout->addWidget(m_randomSeedSpin, 1);
    m_autoSeedCheck = new QCheckBox("Авто", this);
    m_autoSeedCheck->setChecked(true);
    seedLayout->addWidget(m_autoSeedCheck);
    arrayLayout->addLayout(seedLayout);

    mainLayout->addWidget(arrayGroup);

    auto *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(line2);

    // === Анимация ===
    auto *animGroup = new QGroupBox("Настройки анимации", this);
    auto *animLayout = new QVBoxLayout(animGroup);

    auto *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("Скорость:", this));
    m_animSpeedSlider = new QSlider(Qt::Horizontal, this);
    m_animSpeedSlider->setRange(0, 100);
    m_animSpeedSlider->setValue(80);
    speedLayout->addWidget(m_animSpeedSlider, 1);
    m_animSpeedLabel = new QLabel("80%", this);
    m_animSpeedLabel->setFixedWidth(50);
    speedLayout->addWidget(m_animSpeedLabel);
    animLayout->addLayout(speedLayout);

    m_showComparisonsCheck = new QCheckBox("Показывать сравнения", this);
    m_showComparisonsCheck->setChecked(true);
    animLayout->addWidget(m_showComparisonsCheck);

    m_showAccessCountCheck = new QCheckBox("Счётчик обращений", this);
    animLayout->addWidget(m_showAccessCountCheck);

    auto *colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new QLabel("Цвета:", this));
    m_colorSchemeCombo = new QComboBox(this);
    m_colorSchemeCombo->addItems({"Радужная", "Тепловая", "Монохром", "Статус"});
    colorLayout->addWidget(m_colorSchemeCombo, 1);
    animLayout->addLayout(colorLayout);

    auto *maxVisLayout = new QHBoxLayout();
    maxVisLayout->addWidget(new QLabel("Макс. столбцов:", this));
    m_maxVisElementsSpin = new QSpinBox(this);
    m_maxVisElementsSpin->setRange(10, 10000);
    m_maxVisElementsSpin->setValue(500);
    maxVisLayout->addWidget(m_maxVisElementsSpin, 1);
    animLayout->addLayout(maxVisLayout);

    mainLayout->addWidget(animGroup);

    auto *line3 = new QFrame(this);
    line3->setFrameShape(QFrame::HLine);
    mainLayout->addWidget(line3);

    // === Серийное тестирование ===
    auto *batchGroup = new QGroupBox("Авто-серия", this);
    auto *batchLayout = new QVBoxLayout(batchGroup);

    m_batchModeCheck = new QCheckBox("Режим серии тестов", this);
    batchLayout->addWidget(m_batchModeCheck);

    auto *batchBtnLayout = new QHBoxLayout();
    m_configureBatchBtn = new QPushButton("Настроить...", this);
    batchBtnLayout->addWidget(m_configureBatchBtn);
    batchBtnLayout->addStretch();
    batchLayout->addLayout(batchBtnLayout);

    m_batchStatusLabel = new QLabel("0 / 0 тестов выполнено", this);
    batchLayout->addWidget(m_batchStatusLabel);

    mainLayout->addWidget(batchGroup);

    // === Кнопки управления ===
    auto *btnLayout = new QHBoxLayout();

    m_runBtn = new QPushButton("▶ Запустить", this);
    m_runBtn->setShortcut(QKeySequence(Qt::Key_F5));
    m_runBtn->setObjectName("runBtn");
    btnLayout->addWidget(m_runBtn);

    m_stopBtn = new QPushButton("■ Стоп", this);
    m_stopBtn->setShortcut(QKeySequence(Qt::Key_Escape));
    m_stopBtn->setEnabled(false);
    m_stopBtn->setObjectName("stopBtn");
    btnLayout->addWidget(m_stopBtn);

    m_pauseResumeBtn = new QPushButton("⏸ Пауза", this);
    m_pauseResumeBtn->setShortcut(QKeySequence(Qt::Key_Space));
    m_pauseResumeBtn->setEnabled(false);
    m_pauseResumeBtn->setObjectName("pauseBtn");
    btnLayout->addWidget(m_pauseResumeBtn);

    m_resetBtn = new QPushButton("↺ Сброс", this);
    btnLayout->addWidget(m_resetBtn);

    mainLayout->addLayout(btnLayout);
    mainLayout->addStretch();
}

void ControlPanel::populateAlgorithms() {
    const auto &registry = AlgorithmRegistry::instance();

    // FIX: было registry.cpuAlgorithms() → allCpuAlgorithms()
    for (const auto &algo : registry.allCpuAlgorithms()) {
        // FIX: algo.name уже QString, fromStdString не нужен
        m_cpuAlgorithmCombo->addItem(algo.name);
    }

    // FIX: было registry.gpuAlgorithms() → allGpuAlgorithms()
    for (const auto &algo : registry.allGpuAlgorithms()) {
        m_gpuAlgorithmCombo->addItem(algo.name);
    }
    m_gpuAlgorithmCombo->addItem("Отключён");
}

void ControlPanel::checkCudaAvailability() {
    // FIX: было CudaDeviceInfo::hasCudaDevice() → isCudaAvailable()
    if (!CudaDeviceInfo::isCudaAvailable()) {
        m_gpuAlgorithmCombo->setEnabled(false);
        m_enableGpuCheck->setEnabled(false);
        m_enableGpuCheck->setChecked(false);
        m_gpuAlgorithmCombo->setToolTip("CUDA недоступна на этом устройстве");
        m_gpuAlgorithmCombo->setCurrentIndex(m_gpuAlgorithmCombo->count() - 1);
    }
}

void ControlPanel::connectSignals() {
    connect(m_arraySizeSlider, &QSlider::valueChanged, this, [this](int value) {
        int size = static_cast<int>(std::pow(10.0, 3.0 + value / 100.0 * 5.0));
        m_arraySizeSpinBox->blockSignals(true);
        m_arraySizeSpinBox->setValue(size);
        m_arraySizeSpinBox->blockSignals(false);
        emit parametersChanged(buildParams());
    });

    connect(m_arraySizeSpinBox, &QSpinBox::valueChanged, this, [this](int value) {
        double logVal = std::log10(static_cast<double>(value));
        int sliderValue = static_cast<int>((logVal - 3.0) / 5.0 * 100.0);
        sliderValue = qBound(0, sliderValue, 100);
        m_arraySizeSlider->blockSignals(true);
        m_arraySizeSlider->setValue(sliderValue);
        m_arraySizeSlider->blockSignals(false);
        emit parametersChanged(buildParams());
    });

    connect(m_cpuAlgorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { emit parametersChanged(buildParams()); });
    connect(m_gpuAlgorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { emit parametersChanged(buildParams()); });
    connect(m_enableCpuCheck, &QCheckBox::stateChanged,
            this, [this]() { emit parametersChanged(buildParams()); });
    connect(m_enableGpuCheck, &QCheckBox::stateChanged,
            this, [this]() { emit parametersChanged(buildParams()); });
    connect(m_dataTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { emit parametersChanged(buildParams()); });
    connect(m_distributionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { emit parametersChanged(buildParams()); });
    connect(m_randomSeedSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]() { emit parametersChanged(buildParams()); });
    connect(m_autoSeedCheck, &QCheckBox::stateChanged, this, [this]() {
        m_randomSeedSpin->setEnabled(!m_autoSeedCheck->isChecked());
        emit parametersChanged(buildParams());
    });
    connect(m_animSpeedSlider, &QSlider::valueChanged, this, [this](int value) {
        m_animSpeedLabel->setText(QString("%1%").arg(value));
        emit animationSpeedChanged(value);
        emit parametersChanged(buildParams());
    });
    connect(m_showComparisonsCheck, &QCheckBox::stateChanged,
            this, [this]() { emit parametersChanged(buildParams()); });
    connect(m_showAccessCountCheck, &QCheckBox::stateChanged,
            this, [this]() { emit parametersChanged(buildParams()); });
    connect(m_colorSchemeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { emit parametersChanged(buildParams()); });
    connect(m_maxVisElementsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this]() { emit parametersChanged(buildParams()); });

    connect(m_runBtn,         &QPushButton::clicked, this, &ControlPanel::runRequested);
    connect(m_stopBtn,        &QPushButton::clicked, this, &ControlPanel::stopRequested);
    connect(m_pauseResumeBtn, &QPushButton::clicked, this, &ControlPanel::pauseResumeRequested);
    connect(m_resetBtn,       &QPushButton::clicked, this, &ControlPanel::resetRequested);
    connect(m_algoInfoBtn,    &QPushButton::clicked, this, &ControlPanel::onAlgoInfoClicked);
    connect(m_configureBatchBtn, &QPushButton::clicked, this, &ControlPanel::onConfigureBatchClicked);
}

SortBench::SortParams ControlPanel::buildParams() const {
    SortBench::SortParams params;

    // FIX: используем локальную stringToCpuAlgorithm (принимает QString)
    params.cpuAlgorithm = stringToCpuAlgorithm(m_cpuAlgorithmCombo->currentText());
    params.enableCPU    = m_enableCpuCheck->isChecked();

    QString gpuText = m_gpuAlgorithmCombo->currentText();
    if (gpuText == "Отключён") {
        // FIX: было params.enableGpu → enableGPU
        params.enableGPU = false;
    } else {
        params.gpuAlgorithm = stringToGpuAlgorithm(gpuText);
        params.enableGPU    = m_enableGpuCheck->isChecked();
    }

    params.arraySize    = m_arraySizeSpinBox->value();
    params.dataType     = static_cast<SortBench::DataType>(m_dataTypeCombo->currentIndex());
    params.distribution = static_cast<SortBench::Distribution>(m_distributionCombo->currentIndex());

    if (m_autoSeedCheck->isChecked()) {
        std::random_device rd;
        params.randomSeed = rd();
    } else {
        params.randomSeed = static_cast<unsigned int>(m_randomSeedSpin->value());
    }

    params.animationFPS     = m_animSpeedSlider->value();
    params.showComparisons  = m_showComparisonsCheck->isChecked();
    params.showAccessCount  = m_showAccessCountCheck->isChecked();
    params.colorScheme      = static_cast<SortBench::ColorSchemeType>(m_colorSchemeCombo->currentIndex());
    params.maxVisElements   = m_maxVisElementsSpin->value();

    return params;
}

SortBench::SortParams ControlPanel::getCurrentParams() const {
    return buildParams();
}

void ControlPanel::setRunning(bool running) {
    m_running = running;
    bool enabled = !running;

    m_cpuAlgorithmCombo->setEnabled(enabled);
    m_gpuAlgorithmCombo->setEnabled(enabled);
    m_enableCpuCheck->setEnabled(enabled);
    m_enableGpuCheck->setEnabled(enabled);
    m_arraySizeSlider->setEnabled(enabled);
    m_arraySizeSpinBox->setEnabled(enabled);
    m_dataTypeCombo->setEnabled(enabled);
    m_distributionCombo->setEnabled(enabled);
    m_randomSeedSpin->setEnabled(enabled && !m_autoSeedCheck->isChecked());
    m_autoSeedCheck->setEnabled(enabled);
    m_animSpeedSlider->setEnabled(enabled);
    m_colorSchemeCombo->setEnabled(enabled);
    m_maxVisElementsSpin->setEnabled(enabled);
    m_configureBatchBtn->setEnabled(enabled);

    m_runBtn->setEnabled(enabled);
    m_stopBtn->setEnabled(!enabled);
    m_pauseResumeBtn->setEnabled(!enabled);
}

void ControlPanel::setPaused(bool paused) {
    m_paused = paused;
    m_pauseResumeBtn->setText(paused ? "▶ Продолжить" : "⏸ Пауза");
}

void ControlPanel::updateBatchProgress(int completed, int total) {
    m_batchStatusLabel->setText(QString("%1 / %2 тестов выполнено").arg(completed).arg(total));
}

void ControlPanel::onAlgoInfoClicked() {
    showAlgorithmInfo();
}

void ControlPanel::onConfigureBatchClicked() {
    emit parametersChanged(buildParams());
}

void ControlPanel::showAlgorithmInfo() {
    QString algoName = m_cpuAlgorithmCombo->currentText();
    const auto &registry = AlgorithmRegistry::instance();
    // FIX: используем локальный stringToCpuAlgorithm (принимает QString)
    SortBench::AlgorithmInfo info = registry.getInfo(stringToCpuAlgorithm(algoName));

    // FIX: info.name — это QString, isEmpty() вместо empty()
    if (!info.name.isEmpty()) {
        // FIX: info.name уже QString, не нужен fromStdString
        QString tooltip = QString(
            "<b>%1</b><br>"
            "<i>Время:</i> %2<br>"
            "<i>Память:</i> %3<br>"
            "<i>Стабильный:</i> %4<br>"
            "<i>Параллельный:</i> %5<br><br>"
            "%6"
        ).arg(
            info.name,
            info.timeComplexity,
            info.spaceComplexity,
            info.stable ? "Да" : "Нет",
            info.parallelizable ? "Да" : "Нет",
            info.description
        );
        QToolTip::showText(QCursor::pos(), tooltip, this);
    }
}

void ControlPanel::loadFromSettings() {
    QSettings settings("CUDA Lab", "SortBench");

    m_cpuAlgorithmCombo->setCurrentText(settings.value("cpu/algorithm", "Quick Sort").toString());
    m_gpuAlgorithmCombo->setCurrentText(settings.value("gpu/algorithm", "Thrust Radix Sort").toString());
    m_enableCpuCheck->setChecked(settings.value("cpu/enabled", true).toBool());
    m_enableGpuCheck->setChecked(settings.value("gpu/enabled", true).toBool());
    m_arraySizeSpinBox->setValue(settings.value("bench/arraySize", 1000000).toInt());
    m_dataTypeCombo->setCurrentIndex(settings.value("bench/dataType", 0).toInt());
    m_distributionCombo->setCurrentIndex(settings.value("bench/distribution", 0).toInt());
    m_randomSeedSpin->setValue(settings.value("bench/seed", 42).toInt());
    m_autoSeedCheck->setChecked(settings.value("bench/autoSeed", true).toBool());
    m_animSpeedSlider->setValue(settings.value("anim/speed", 80).toInt());
    m_showComparisonsCheck->setChecked(settings.value("anim/showComparisons", true).toBool());
    m_showAccessCountCheck->setChecked(settings.value("anim/showAccessCount", false).toBool());
    m_colorSchemeCombo->setCurrentIndex(settings.value("anim/colorScheme", 0).toInt());
    m_maxVisElementsSpin->setValue(settings.value("anim/maxElements", 500).toInt());
    m_batchModeCheck->setChecked(settings.value("batch/enabled", false).toBool());
}

void ControlPanel::saveToSettings() const {
    QSettings settings("CUDA Lab", "SortBench");

    settings.setValue("cpu/algorithm",       m_cpuAlgorithmCombo->currentText());
    settings.setValue("gpu/algorithm",       m_gpuAlgorithmCombo->currentText());
    settings.setValue("cpu/enabled",         m_enableCpuCheck->isChecked());
    settings.setValue("gpu/enabled",         m_enableGpuCheck->isChecked());
    settings.setValue("bench/arraySize",     m_arraySizeSpinBox->value());
    settings.setValue("bench/dataType",      m_dataTypeCombo->currentIndex());
    settings.setValue("bench/distribution",  m_distributionCombo->currentIndex());
    settings.setValue("bench/seed",          m_randomSeedSpin->value());
    settings.setValue("bench/autoSeed",      m_autoSeedCheck->isChecked());
    settings.setValue("anim/speed",          m_animSpeedSlider->value());
    settings.setValue("anim/showComparisons",m_showComparisonsCheck->isChecked());
    settings.setValue("anim/showAccessCount",m_showAccessCountCheck->isChecked());
    settings.setValue("anim/colorScheme",    m_colorSchemeCombo->currentIndex());
    settings.setValue("anim/maxElements",    m_maxVisElementsSpin->value());
    settings.setValue("batch/enabled",       m_batchModeCheck->isChecked());
}

// ControlPanel::onAlgorithmChanged, onArraySizeChanged, onDistributionChanged,
// onAnimSpeedChanged вызывают уже подключённые лямбды — слоты-заглушки:
void ControlPanel::onAlgorithmChanged()   { emit parametersChanged(buildParams()); }
void ControlPanel::onArraySizeChanged()   { emit parametersChanged(buildParams()); }
void ControlPanel::onDistributionChanged(){ emit parametersChanged(buildParams()); }
void ControlPanel::onAnimSpeedChanged(int value) {
    m_animSpeedLabel->setText(QString("%1%").arg(value));
    emit animationSpeedChanged(value);
}
void ControlPanel::onRunClicked()        { emit runRequested(); }
void ControlPanel::onStopClicked()       { emit stopRequested(); }
void ControlPanel::onPauseResumeClicked(){ emit pauseResumeRequested(); }
void ControlPanel::onResetClicked()      { emit resetRequested(); }

int ControlPanel::logScaleToSize(int logValue) const {
    return static_cast<int>(std::pow(10.0, 3.0 + logValue / 100.0 * 5.0));
}

int ControlPanel::sizeToLogScale(int size) const {
    double logVal = std::log10(static_cast<double>(size));
    return static_cast<int>((logVal - 3.0) / 5.0 * 100.0);
}

} // namespace SortBench
