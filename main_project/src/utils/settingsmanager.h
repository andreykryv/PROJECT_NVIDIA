////////////////////////////////////////////////////////////////////////////////
// utils/settingsmanager.h — менеджер пользовательских настроек
//
// НАЗНАЧЕНИЕ:
//   Singleton-обёртка над QSettings. Типизированный доступ ко всем
//   настройкам приложения с дефолтными значениями.
//
// КЛАСС: SettingsManager (Singleton)
//
//   КАТЕГОРИИ НАСТРОЕК:
//
//   General:
//     theme()           → QString  ("dark" / "light")
//     language()        → QString  ("ru" / "en")
//     logLevel()        → int      (0–4)
//     autoSaveResults() → bool
//     saveResultsPath() → QString
//     confirmOnExit()   → bool
//
//   CUDA:
//     preferredDeviceIndex() → int
//     cudaStreams()          → int     (1–16)
//     cudaBlockSize()        → int     (32–1024)
//     usePinnedMemory()      → bool
//     showProfilingData()    → bool
//
//   Visualization:
//     maxFPS()              → int     (1–144)
//     maxVisElements()      → int
//     useOpenGL()           → bool
//     showLegend()          → bool
//     showCounters()        → bool
//     frameBufferSize()     → int
//
//   Window:
//     geometry()            → QByteArray
//     windowState()         → QByteArray  (для QMainWindow::restoreState)
//
//   LastSession:
//     lastCpuAlgorithm()    → int
//     lastGpuAlgorithm()    → int
//     lastArraySize()       → int
//     lastDataType()        → int
//     lastDistribution()    → int
//
//   МЕТОДЫ:
//     void load()                        — загрузить из QSettings
//     void save()                        — сохранить в QSettings
//     void resetToDefaults()             — сбросить все настройки
//     template<T> T get(Key) const
//     template<T> void set(Key, T value)
//
//   СИГНАЛЫ:
//     settingChanged(QString key)        — при изменении любой настройки
////////////////////////////////////////////////////////////////////////////////

#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <QString>
#include <QByteArray>

class SettingsManager : public QObject {
    Q_OBJECT
    
public:
    // Перечисление ключей настроек
    enum class Key {
        // General
        Theme,
        Language,
        LogLevel,
        AutoSaveResults,
        SaveResultsPath,
        ConfirmOnExit,
        
        // CUDA
        PreferredDeviceIndex,
        CudaStreams,
        CudaBlockSize,
        UsePinnedMemory,
        ShowProfilingData,
        
        // Visualization
        MaxFPS,
        MaxVisElements,
        UseOpenGL,
        ShowLegend,
        ShowCounters,
        FrameBufferSize,
        
        // Window
        Geometry,
        WindowState,
        
        // LastSession
        LastCpuAlgorithm,
        LastGpuAlgorithm,
        LastArraySize,
        LastDataType,
        LastDistribution
    };
    
    // Получить единственный экземпляр
    static SettingsManager& instance();
    
    // Загрузить настройки из хранилища
    void load();
    
    // Сохранить настройки в хранилище
    void save();
    
    // Сбросить все настройки к значениям по умолчанию
    void resetToDefaults();
    
    // Получить значение настройки
    template<typename T>
    T get(Key key) const;
    
    // Установить значение настройки
    template<typename T>
    void set(Key key, const T& value);
    
    // Специализированные геттеры для General
    QString theme() const { return get<QString>(Key::Theme); }
    QString language() const { return get<QString>(Key::Language); }
    int logLevel() const { return get<int>(Key::LogLevel); }
    bool autoSaveResults() const { return get<bool>(Key::AutoSaveResults); }
    QString saveResultsPath() const { return get<QString>(Key::SaveResultsPath); }
    bool confirmOnExit() const { return get<bool>(Key::ConfirmOnExit); }
    
    // Специализированные геттеры для CUDA
    int preferredDeviceIndex() const { return get<int>(Key::PreferredDeviceIndex); }
    int cudaStreams() const { return get<int>(Key::CudaStreams); }
    int cudaBlockSize() const { return get<int>(Key::CudaBlockSize); }
    bool usePinnedMemory() const { return get<bool>(Key::UsePinnedMemory); }
    bool showProfilingData() const { return get<bool>(Key::ShowProfilingData); }
    
    // Специализированные геттеры для Visualization
    int maxFPS() const { return get<int>(Key::MaxFPS); }
    int maxVisElements() const { return get<int>(Key::MaxVisElements); }
    bool useOpenGL() const { return get<bool>(Key::UseOpenGL); }
    bool showLegend() const { return get<bool>(Key::ShowLegend); }
    bool showCounters() const { return get<bool>(Key::ShowCounters); }
    int frameBufferSize() const { return get<int>(Key::FrameBufferSize); }
    
    // Специализированные геттеры для Window
    QByteArray geometry() const { return get<QByteArray>(Key::Geometry); }
    QByteArray windowState() const { return get<QByteArray>(Key::WindowState); }
    
    // Специализированные геттеры для LastSession
    int lastCpuAlgorithm() const { return get<int>(Key::LastCpuAlgorithm); }
    int lastGpuAlgorithm() const { return get<int>(Key::LastGpuAlgorithm); }
    int lastArraySize() const { return get<int>(Key::LastArraySize); }
    int lastDataType() const { return get<int>(Key::LastDataType); }
    int lastDistribution() const { return get<int>(Key::LastDistribution); }
    
signals:
    void settingChanged(const QString& key);
    
private:
    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager() override;
    
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    
    // Инициализация значений по умолчанию
    void initDefaults();
    
    // Получить строковое имя ключа
    static QString keyToString(Key key);
    
    QSettings* m_settings;
    QVariantMap m_defaults;
};

// Шаблонные реализации
template<typename T>
T SettingsManager::get(Key key) const {
    QString k = keyToString(key);
    if (m_settings->contains(k)) {
        return m_settings->value(k).value<T>();
    }
    return m_defaults.value(k).value<T>();
}

template<typename T>
void SettingsManager::set(Key key, const T& value) {
    QString k = keyToString(key);
    m_settings->setValue(k, QVariant::fromValue(value));
    emit settingChanged(k);
}

#endif // SETTINGS_MANAGER_H
