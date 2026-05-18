////////////////////////////////////////////////////////////////////////////////
// ui/settingsdialog.h — заголовок диалога настроек приложения
//
// НАЗНАЧЕНИЕ:
//   Модальный диалог для настройки всех параметров приложения, которые
//   не вынесены на главную панель управления.
//
// КЛАСС: SettingsDialog : public QDialog
//
//   ВКЛАДКИ (QTabWidget):
//
//   "Общие":
//     — QComboBox: выбор темы (Тёмная / Светлая / Системная)
//     — QComboBox: язык интерфейса (Русский / English)
//     — QSpinBox: макс. размер лог-файла (МБ)
//     — QCheckBox: сохранять результаты в файл автоматически
//     — QLineEdit: папка для автосохранения
//     — QCheckBox: показывать подтверждение при выходе
//
//   "CUDA / GPU":
//     — QComboBox: выбор CUDA-устройства (если их несколько)
//     — QSpinBox: число CUDA streams (1–16, default 4)
//     — QSpinBox: размер блока CUDA (32/64/128/256/512/1024)
//     — QCheckBox: использовать pinned memory (cudaMallocHost)
//     — QCheckBox: показывать CUDA profiling данные
//     — QLabel: информация о выбранном устройстве (вычислительная способность,
//               количество SM, объём VRAM, тактовая частота)
//
//   "Визуализация":
//     — QSpinBox: макс. FPS анимации (1–144)
//     — QSpinBox: макс. элементов в визуализации (100–10000)
//     — QCheckBox: использовать OpenGL для рендеринга
//     — QCheckBox: показывать легенду цветов
//     — QCheckBox: показывать счётчики сравнений/перестановок
//     — QSpinBox: размер буфера кадров (число кадров в очереди)
//
//   "Производительность":
//     — QSpinBox: число CPU-потоков для параллельных тестов
//     — QCheckBox: использовать CPU affinity (привязка к ядрам)
//     — QSpinBox: число повторений для усреднения результата
//     — QCheckBox: исключать выбросы (outliers) при усреднении
//
//   КНОПКИ: OK, Отмена, Применить, Сбросить по умолчанию
//
//   МЕТОДЫ:
//     loadFromSettings()             — заполнить форму из SettingsManager
//     saveToSettings()               — записать форму в SettingsManager
//     resetToDefaults()              — сбросить все поля на дефолтные значения
//     updateGPUInfo(int deviceIndex) — обновить информацию об устройстве
////////////////////////////////////////////////////////////////////////////////

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QDialogButtonBox>

class SettingsManager;

class SettingsDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() override = default;
    
    // Загрузить настройки из SettingsManager
    void loadFromSettings();
    
    // Сохранить настройки в SettingsManager
    void saveToSettings();
    
    // Сбросить к значениям по умолчанию
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
    
    // Общие настройки
    QComboBox* m_themeCombo;
    QComboBox* m_languageCombo;
    QSpinBox* m_logSizeSpin;
    QCheckBox* m_autoSaveCheck;
    QLineEdit* m_savePathEdit;
    QCheckBox* m_confirmExitCheck;
    
    // CUDA настройки
    QComboBox* m_cudaDeviceCombo;
    QSpinBox* m_cudaStreamsSpin;
    QSpinBox* m_cudaBlockSizeSpin;
    QCheckBox* m_pinnedMemoryCheck;
    QCheckBox* m_profilingCheck;
    QLabel* m_gpuInfoLabel;
    
    // Визуализация
    QSpinBox* m_maxFpsSpin;
    QSpinBox* m_maxVisElementsSpin;
    QCheckBox* m_useOpenGLCheck;
    QCheckBox* m_showLegendCheck;
    QCheckBox* m_showCountersCheck;
    QSpinBox* m_frameBufferSpin;
    
    // Производительность
    QSpinBox* m_cpuThreadsSpin;
    QCheckBox* m_cpuAffinityCheck;
    QSpinBox* m_repeatCountSpin;
    QCheckBox* m_excludeOutliersCheck;
    
    // Кнопки
    QDialogButtonBox* m_buttonBox;
    QPushButton* m_applyButton;
    QPushButton* m_resetButton;
};

#endif // SETTINGS_DIALOG_H
