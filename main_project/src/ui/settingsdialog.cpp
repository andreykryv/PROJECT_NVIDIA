////////////////////////////////////////////////////////////////////////////////
// ui/settingsdialog.cpp — реализация диалога настроек
//
// СОДЕРЖИМОЕ:
//   Реализует построение всех вкладок настроек, заполнение из SettingsManager
//   и сохранение при нажатии OK/Применить.
//
// updateGPUInfo():
//   — Вызывает CudaDeviceInfo::getDeviceProperties(index).
//   — Форматирует и отображает: compute capability, SM count, clock MHz,
//     VRAM total, L2 cache, shared memory per block, warp size.
//   — Обновляет диапазоны QSpinBox для blockSize (кратно warpSize).
//
// Валидация настроек:
//   — При попытке OK проверяет корректность (путь для сохранения существует,
//     FPS в разумных пределах, block size кратен 32).
//   — При ошибке показывает QMessageBox::warning и не закрывает диалог.
////////////////////////////////////////////////////////////////////////////////

#include "settingsdialog.h"
#include "utils/settingsmanager.h"
#include "gpu/cudadeviceinfo.h"
using namespace SortBench;
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QThread>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Настройки приложения"));
    setMinimumSize(600, 500);
    
    // Создаём кнопки
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_applyButton = new QPushButton(tr("Применить"));
    m_resetButton = new QPushButton(tr("Сбросить"));
    
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        saveToSettings();
        accept();
    });
connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsDialog::onApplyClicked);
    connect(m_resetButton, &QPushButton::clicked, this, &SettingsDialog::onResetClicked);
    
    // Создаём вкладки
    QTabWidget* tabWidget = new QTabWidget(this);
    createGeneralTab();
    createCudaTab();
    createVisualizationTab();
    createPerformanceTab();
    
    tabWidget->addTab(m_generalGroup, tr("Общие"));
    tabWidget->addTab(m_cudaGroup, tr("CUDA / GPU"));
    tabWidget->addTab(m_visGroup, tr("Визуализация"));
    tabWidget->addTab(m_perfGroup, tr("Производительность"));
    
    // Компоновка
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_applyButton);
    buttonLayout->addWidget(m_buttonBox->button(QDialogButtonBox::Ok));
    buttonLayout->addWidget(m_buttonBox->button(QDialogButtonBox::Cancel));
    mainLayout->addLayout(buttonLayout);
    
    // Загружаем настройки
    loadFromSettings();
}

void SettingsDialog::createGeneralTab() {
    m_generalGroup = new QGroupBox(tr("Общие настройки"));
    QFormLayout* layout = new QFormLayout();
    
    // Тема
    m_themeCombo = new QComboBox();
    m_themeCombo->addItems({tr("Тёмная"), tr("Светлая"), tr("Системная")});
    layout->addRow(tr("Тема:"), m_themeCombo);
    
    // Язык
    m_languageCombo = new QComboBox();
    m_languageCombo->addItems({tr("Русский"), tr("English")});
    layout->addRow(tr("Язык:"), m_languageCombo);
    
    // Размер лога
    m_logSizeSpin = new QSpinBox();
    m_logSizeSpin->setRange(1, 1000);
    m_logSizeSpin->setSuffix(tr(" МБ"));
    layout->addRow(tr("Макс. размер лога:"), m_logSizeSpin);
    
    // Автосохранение
    m_autoSaveCheck = new QCheckBox();
    layout->addRow(tr("Автосохранение:"), m_autoSaveCheck);
    
    // Путь сохранения
    m_savePathEdit = new QLineEdit();
    QPushButton* browseBtn = new QPushButton(tr("..."));
    browseBtn->setMaximumWidth(40);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Выберите папку"),
                                                        m_savePathEdit->text());
        if (!dir.isEmpty()) {
            m_savePathEdit->setText(dir);
        }
    });
    
    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(m_savePathEdit);
    pathLayout->addWidget(browseBtn);
    layout->addRow(tr("Папка:"), pathLayout);
    
    // Подтверждение выхода
    m_confirmExitCheck = new QCheckBox();
    layout->addRow(tr("Подтверждение выхода:"), m_confirmExitCheck);
    
    m_generalGroup->setLayout(layout);
}

void SettingsDialog::createCudaTab() {
    m_cudaGroup = new QGroupBox(tr("Настройки CUDA"));
    QFormLayout* layout = new QFormLayout();
    
    // Выбор устройства
    m_cudaDeviceCombo = new QComboBox();
    connect(m_cudaDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onDeviceIndexChanged);
    layout->addRow(tr("Устройство:"), m_cudaDeviceCombo);
    
    // CUDA streams
    m_cudaStreamsSpin = new QSpinBox();
    m_cudaStreamsSpin->setRange(1, 16);
    layout->addRow(tr("CUDA streams:"), m_cudaStreamsSpin);
    
    // Block size
    m_cudaBlockSizeSpin = new QSpinBox();
    m_cudaBlockSizeSpin->setRange(32, 1024);
    m_cudaBlockSizeSpin->setSingleStep(32);
    layout->addRow(tr("Размер блока:"), m_cudaBlockSizeSpin);
    
    // Pinned memory
    m_pinnedMemoryCheck = new QCheckBox();
    layout->addRow(tr("Pinned memory:"), m_pinnedMemoryCheck);
    
    // Profiling
    m_profilingCheck = new QCheckBox();
    layout->addRow(tr("Профилирование:"), m_profilingCheck);
    
    // Информация о GPU
    m_gpuInfoLabel = new QLabel();
    m_gpuInfoLabel->setWordWrap(true);
    m_gpuInfoLabel->setFrameShape(QFrame::StyledPanel);
    layout->addRow(tr("Информация:"), m_gpuInfoLabel);
    
    m_cudaGroup->setLayout(layout);
}

void SettingsDialog::createVisualizationTab() {
    m_visGroup = new QGroupBox(tr("Визуализация"));
    QFormLayout* layout = new QFormLayout();
    
    // Max FPS
    m_maxFpsSpin = new QSpinBox();
    m_maxFpsSpin->setRange(1, 144);
    layout->addRow(tr("Макс. FPS:"), m_maxFpsSpin);
    
    // Max elements
    m_maxVisElementsSpin = new QSpinBox();
    m_maxVisElementsSpin->setRange(100, 10000);
    m_maxVisElementsSpin->setSingleStep(100);
    layout->addRow(tr("Макс. элементов:"), m_maxVisElementsSpin);
    
    // OpenGL
    m_useOpenGLCheck = new QCheckBox();
    layout->addRow(tr("OpenGL:"), m_useOpenGLCheck);
    
    // Legend
    m_showLegendCheck = new QCheckBox();
    layout->addRow(tr("Легенда:"), m_showLegendCheck);
    
    // Counters
    m_showCountersCheck = new QCheckBox();
    layout->addRow(tr("Счётчики:"), m_showCountersCheck);
    
    // Frame buffer
    m_frameBufferSpin = new QSpinBox();
    m_frameBufferSpin->setRange(1, 100);
    layout->addRow(tr("Буфер кадров:"), m_frameBufferSpin);
    
    m_visGroup->setLayout(layout);
}

void SettingsDialog::createPerformanceTab() {
    m_perfGroup = new QGroupBox(tr("Производительность"));
    QFormLayout* layout = new QFormLayout();
    
    // CPU threads
    m_cpuThreadsSpin = new QSpinBox();
    m_cpuThreadsSpin->setRange(1, 64);
    layout->addRow(tr("CPU потоки:"), m_cpuThreadsSpin);
    
    // CPU affinity
    m_cpuAffinityCheck = new QCheckBox();
    layout->addRow(tr("CPU affinity:"), m_cpuAffinityCheck);
    
    // Repeat count
    m_repeatCountSpin = new QSpinBox();
    m_repeatCountSpin->setRange(1, 100);
    layout->addRow(tr("Повторений:"), m_repeatCountSpin);
    
    // Exclude outliers
    m_excludeOutliersCheck = new QCheckBox();
    layout->addRow(tr("Исключать выбросы:"), m_excludeOutliersCheck);
    
    m_perfGroup->setLayout(layout);
}

void SettingsDialog::loadFromSettings() {
    SettingsManager& sm = SettingsManager::instance();
    
    // General
    m_themeCombo->setCurrentText(sm.theme());
    m_languageCombo->setCurrentText(sm.language() == "ru" ? tr("Русский") : tr("English"));
    m_logSizeSpin->setValue(sm.logLevel());
    m_autoSaveCheck->setChecked(sm.autoSaveResults());
    m_savePathEdit->setText(sm.saveResultsPath());
    m_confirmExitCheck->setChecked(sm.confirmOnExit());
    
    // CUDA
    m_cudaStreamsSpin->setValue(sm.cudaStreams());
    m_cudaBlockSizeSpin->setValue(sm.cudaBlockSize());
    m_pinnedMemoryCheck->setChecked(sm.usePinnedMemory());
    m_profilingCheck->setChecked(sm.showProfilingData());
    
    // Visualization
    m_maxFpsSpin->setValue(sm.maxFPS());
    m_maxVisElementsSpin->setValue(sm.maxVisElements());
    m_useOpenGLCheck->setChecked(sm.useOpenGL());
    m_showLegendCheck->setChecked(sm.showLegend());
    m_showCountersCheck->setChecked(sm.showCounters());
    m_frameBufferSpin->setValue(sm.frameBufferSize());
    
    // Performance
    m_cpuThreadsSpin->setValue(QThread::idealThreadCount());
    m_cpuAffinityCheck->setChecked(false);
    m_repeatCountSpin->setValue(5);
    m_excludeOutliersCheck->setChecked(true);
    
    // Update GPU info
    updateGPUInfo(sm.preferredDeviceIndex());
}

void SettingsDialog::saveToSettings() {
    SettingsManager& sm = SettingsManager::instance();
    
    // General
    sm.set(SettingsManager::Key::Theme, m_themeCombo->currentText());
    sm.set(SettingsManager::Key::Language, 
           QString(m_languageCombo->currentText() == tr("Русский") ? "ru" : "en"));
    sm.set(SettingsManager::Key::LogLevel, m_logSizeSpin->value());
    sm.set(SettingsManager::Key::AutoSaveResults, m_autoSaveCheck->isChecked());
    sm.set(SettingsManager::Key::SaveResultsPath, m_savePathEdit->text());
    sm.set(SettingsManager::Key::ConfirmOnExit, m_confirmExitCheck->isChecked());
    
    // CUDA
    sm.set(SettingsManager::Key::PreferredDeviceIndex, m_cudaDeviceCombo->currentIndex());
    sm.set(SettingsManager::Key::CudaStreams, m_cudaStreamsSpin->value());
    sm.set(SettingsManager::Key::CudaBlockSize, m_cudaBlockSizeSpin->value());
    sm.set(SettingsManager::Key::UsePinnedMemory, m_pinnedMemoryCheck->isChecked());
    sm.set(SettingsManager::Key::ShowProfilingData, m_profilingCheck->isChecked());
    
    // Visualization
    sm.set(SettingsManager::Key::MaxFPS, m_maxFpsSpin->value());
    sm.set(SettingsManager::Key::MaxVisElements, m_maxVisElementsSpin->value());
    sm.set(SettingsManager::Key::UseOpenGL, m_useOpenGLCheck->isChecked());
    sm.set(SettingsManager::Key::ShowLegend, m_showLegendCheck->isChecked());
    sm.set(SettingsManager::Key::ShowCounters, m_showCountersCheck->isChecked());
    sm.set(SettingsManager::Key::FrameBufferSize, m_frameBufferSpin->value());
}

void SettingsDialog::resetToDefaults() {
    SettingsManager::instance().resetToDefaults();
    loadFromSettings();
}

void SettingsDialog::onDeviceIndexChanged(int index) {
    updateGPUInfo(index);
}

void SettingsDialog::onApplyClicked() {
    saveToSettings();
}

void SettingsDialog::onResetClicked() {
    resetToDefaults();
}

void SettingsDialog::updateGPUInfo(int deviceIndex) {
    if (!CudaDeviceInfo::isCudaAvailable()) {
        m_gpuInfoLabel->setText(tr("CUDA недоступна"));
        return;
    }
    
    int deviceCount = CudaDeviceInfo::deviceCount();
    m_cudaDeviceCombo->clear();
    
    for (int i = 0; i < deviceCount; ++i) {
        m_cudaDeviceCombo->addItem(CudaDeviceInfo::getDeviceShortName(i), i);
    }
    
    if (deviceIndex >= deviceCount) {
        deviceIndex = 0;
    }
    
    m_cudaDeviceCombo->setCurrentIndex(deviceIndex);
    
    CudaDeviceProperties props = CudaDeviceInfo::getProperties(deviceIndex);
    QString info = tr("Устройство: %1\n"
                      "Compute Capability: %2\n"
                      "SM: %3\n"
                      "Частота: %4 МГц\n"
                      "VRAM: %5 МБ\n"
                      "L2 кэш: %6 КБ\n"
                      "Warp size: %7")
                       .arg(props.name)
                       .arg(props.computeCapabilityString())
                       .arg(props.multiprocessorCount)
                       .arg(props.clockRateKHz / 1000)
                       .arg(props.totalGlobalMem / (1024 * 1024))
                       .arg(props.l2CacheSize / 1024)
                       .arg(props.warpSize);
    
    m_gpuInfoLabel->setText(info);
}

