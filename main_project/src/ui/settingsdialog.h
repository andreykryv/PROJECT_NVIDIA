#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFrame>

class SettingsManager;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

    void loadFromSettings();
    void saveToSettings();
    void resetToDefaults();

private slots:
    void onDeviceIndexChanged(int index);
    void onApplyClicked();
    void onResetClicked();

private:
    void createGeneralTab();
    void createCudaTab();
    void createVisualizationTab();
    void createPerformanceTab();
    void updateGPUInfo(int deviceIndex);

    // FIX: добавлены объявления членов, которые использовались в .cpp
    QGroupBox* m_generalGroup  = nullptr;
    QGroupBox* m_cudaGroup     = nullptr;
    QGroupBox* m_visGroup      = nullptr;
    QGroupBox* m_perfGroup     = nullptr;

    // Общие настройки
    QComboBox* m_themeCombo     = nullptr;
    QComboBox* m_languageCombo  = nullptr;
    QSpinBox*  m_logSizeSpin    = nullptr;
    QCheckBox* m_autoSaveCheck  = nullptr;
    QLineEdit* m_savePathEdit   = nullptr;
    QCheckBox* m_confirmExitCheck = nullptr;

    // CUDA настройки
    QComboBox* m_cudaDeviceCombo   = nullptr;
    QSpinBox*  m_cudaStreamsSpin   = nullptr;
    QSpinBox*  m_cudaBlockSizeSpin = nullptr;
    QCheckBox* m_pinnedMemoryCheck = nullptr;
    QCheckBox* m_profilingCheck    = nullptr;
    QLabel*    m_gpuInfoLabel      = nullptr;

    // Визуализация
    QSpinBox*  m_maxFpsSpin         = nullptr;
    QSpinBox*  m_maxVisElementsSpin = nullptr;
    QCheckBox* m_useOpenGLCheck     = nullptr;
    QCheckBox* m_showLegendCheck    = nullptr;
    QCheckBox* m_showCountersCheck  = nullptr;
    QSpinBox*  m_frameBufferSpin    = nullptr;

    // Производительность
    QSpinBox*  m_cpuThreadsSpin       = nullptr;
    QCheckBox* m_cpuAffinityCheck     = nullptr;
    QSpinBox*  m_repeatCountSpin      = nullptr;
    QCheckBox* m_excludeOutliersCheck = nullptr;

    // Кнопки
    QDialogButtonBox* m_buttonBox  = nullptr;
    QPushButton*      m_applyButton = nullptr;
    QPushButton*      m_resetButton = nullptr;
};

#endif // SETTINGS_DIALOG_H
