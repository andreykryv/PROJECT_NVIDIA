////////////////////////////////////////////////////////////////////////////////
// utils/settingsmanager.h — менеджер пользовательских настроек
//
// (комментарии опущены для краткости)
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
    enum class Key {
        Theme,
        Language,
        LogLevel,
        AutoSaveResults,
        SaveResultsPath,
        ConfirmOnExit,
        PreferredDeviceIndex,
        CudaStreams,
        CudaBlockSize,
        UsePinnedMemory,
        ShowProfilingData,
        MaxFPS,
        MaxVisElements,
        UseOpenGL,
        ShowLegend,
        ShowCounters,
        FrameBufferSize,
        Geometry,
        WindowState,
        LastCpuAlgorithm,
        LastGpuAlgorithm,
        LastArraySize,
        LastDataType,
        LastDistribution
    };

    static SettingsManager& instance();
    void load();
    void save();
    void resetToDefaults();

    template<typename T> T get(Key key) const;
    template<typename T> void set(Key key, const T& value);

    // General
    QString theme() const { return get<QString>(Key::Theme); }
    QString language() const { return get<QString>(Key::Language); }
    int logLevel() const { return get<int>(Key::LogLevel); }
    bool autoSaveResults() const { return get<bool>(Key::AutoSaveResults); }
    QString saveResultsPath() const { return get<QString>(Key::SaveResultsPath); }
    bool confirmOnExit() const { return get<bool>(Key::ConfirmOnExit); }

    // CUDA
    int preferredDeviceIndex() const { return get<int>(Key::PreferredDeviceIndex); }
    int cudaStreams() const { return get<int>(Key::CudaStreams); }
    int cudaBlockSize() const { return get<int>(Key::CudaBlockSize); }
    bool usePinnedMemory() const { return get<bool>(Key::UsePinnedMemory); }
    bool showProfilingData() const { return get<bool>(Key::ShowProfilingData); }

    // Visualization
    int maxFPS() const { return get<int>(Key::MaxFPS); }
    int maxVisElements() const { return get<int>(Key::MaxVisElements); }
    bool useOpenGL() const { return get<bool>(Key::UseOpenGL); }
    bool showLegend() const { return get<bool>(Key::ShowLegend); }
    bool showCounters() const { return get<bool>(Key::ShowCounters); }
    int frameBufferSize() const { return get<int>(Key::FrameBufferSize); }

    // Window – оригинальные геттеры
    QByteArray geometry() const { return get<QByteArray>(Key::Geometry); }
    QByteArray windowState() const { return get<QByteArray>(Key::WindowState); }

    // LastSession
    int lastCpuAlgorithm() const { return get<int>(Key::LastCpuAlgorithm); }
    int lastGpuAlgorithm() const { return get<int>(Key::LastGpuAlgorithm); }
    int lastArraySize() const { return get<int>(Key::LastArraySize); }
    int lastDataType() const { return get<int>(Key::LastDataType); }
    int lastDistribution() const { return get<int>(Key::LastDistribution); }

    // Дополнительные удобные методы (без дублирования)
    void setWindowGeometry(const QByteArray &geom) { set(Key::Geometry, geom); }
    QByteArray windowGeometry() const { return get<QByteArray>(Key::Geometry); }
    void setWindowState(const QByteArray &state) { set(Key::WindowState, state); }
    void setTheme(const QString &theme) { set(Key::Theme, theme); }
    int cudaDeviceId() const { return preferredDeviceIndex(); }

signals:
    void settingChanged(const QString& key);

private:
    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager() override;

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    void initDefaults();
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