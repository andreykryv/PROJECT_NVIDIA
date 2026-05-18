////////////////////////////////////////////////////////////////////////////////
// utils/settingsmanager.cpp — реализация менеджера настроек
//
// Внутри: QSettings settings("CUDA Lab", "SortBench").
// Каждый геттер: settings.value(key, defaultValue).toXxx().
// Каждый сеттер: settings.setValue(key, value); emit settingChanged(key).
// load(): вызывается один раз в main.cpp. Нет необходимости в явном чтении —
//   QSettings читает из системного хранилища при первом value() вызове.
// save(): settings.sync() — гарантирует запись на диск немедленно.
// resetToDefaults(): settings.clear(); settings.sync().
////////////////////////////////////////////////////////////////////////////////

#include "settingsmanager.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

namespace SortBench {

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
}

void SettingsManager::load() {
    QSettings settings(organizationName(), applicationName());
    
    // Чтение всех настроек с дефолтными значениями
    m_defaultCpuAlgorithm = stringToCpuAlgorithm(settings.value("cpu/algorithm", "QuickSort").toString());
    m_defaultGpuAlgorithm = stringToGpuAlgorithm(settings.value("gpu/algorithm", "ThrustRadix").toString());
    m_defaultArraySize = settings.value("bench/arraySize", 1000000).toInt();
    m_defaultDistribution = stringToDistribution(settings.value("bench/distribution", "Random").toString());
    m_minValue = settings.value("bench/minValue", 0).toInt();
    m_maxValue = settings.value("bench/maxValue", 1000000).toInt();
    m_iterations = settings.value("bench/iterations", 3).toInt();
    m_verifyResults = settings.value("bench/verify", true).toBool();
    m_useGpu = settings.value("gpu/enabled", true).toBool();
    m_cudaDeviceId = settings.value("gpu/deviceId", 0).toInt();
    m_showCharts = settings.value("ui/showCharts", true).toBool();
    m_logLevel = static_cast<Logger::Level>(settings.value("ui/logLevel", 1).toInt());
    m_autoScrollLog = settings.value("ui/autoScrollLog", true).toBool();
    m_windowGeometry = settings.value("ui/windowGeometry").toByteArray();
    m_windowState = settings.value("ui/windowState").toByteArray();
    
    emit settingsLoaded();
}

void SettingsManager::save() {
    QSettings settings(organizationName(), applicationName());
    
    settings.setValue("cpu/algorithm", cpuAlgorithmToString(m_defaultCpuAlgorithm));
    settings.setValue("gpu/algorithm", gpuAlgorithmToString(m_defaultGpuAlgorithm));
    settings.setValue("bench/arraySize", m_defaultArraySize);
    settings.setValue("bench/distribution", distributionToString(m_defaultDistribution));
    settings.setValue("bench/minValue", m_minValue);
    settings.setValue("bench/maxValue", m_maxValue);
    settings.setValue("bench/iterations", m_iterations);
    settings.setValue("bench/verify", m_verifyResults);
    settings.setValue("gpu/enabled", m_useGpu);
    settings.setValue("gpu/deviceId", m_cudaDeviceId);
    settings.setValue("ui/showCharts", m_showCharts);
    settings.setValue("ui/logLevel", static_cast<int>(m_logLevel));
    settings.setValue("ui/autoScrollLog", m_autoScrollLog);
    settings.setValue("ui/windowGeometry", m_windowGeometry);
    settings.setValue("ui/windowState", m_windowState);
    
    settings.sync();
    emit settingsSaved();
}

void SettingsManager::resetToDefaults() {
    QSettings settings(organizationName(), applicationName());
    settings.clear();
    settings.sync();
    
    // Восстановление значений по умолчанию
    m_defaultCpuAlgorithm = CpuAlgorithm::QuickSort;
    m_defaultGpuAlgorithm = GpuAlgorithm::ThrustRadix;
    m_defaultArraySize = 1000000;
    m_defaultDistribution = Distribution::Random;
    m_minValue = 0;
    m_maxValue = 1000000;
    m_iterations = 3;
    m_verifyResults = true;
    m_useGpu = true;
    m_cudaDeviceId = 0;
    m_showCharts = true;
    m_logLevel = Logger::Level::Info;
    m_autoScrollLog = true;
    m_windowGeometry.clear();
    m_windowState.clear();
    
    emit settingsReset();
    emit settingsChanged("all");
}

// Геттеры
CpuAlgorithm SettingsManager::defaultCpuAlgorithm() const { return m_defaultCpuAlgorithm; }
GpuAlgorithm SettingsManager::defaultGpuAlgorithm() const { return m_defaultGpuAlgorithm; }
int SettingsManager::defaultArraySize() const { return m_defaultArraySize; }
Distribution SettingsManager::defaultDistribution() const { return m_defaultDistribution; }
int SettingsManager::minValue() const { return m_minValue; }
int SettingsManager::maxValue() const { return m_maxValue; }
int SettingsManager::iterations() const { return m_iterations; }
bool SettingsManager::verifyResults() const { return m_verifyResults; }
bool SettingsManager::useGpu() const { return m_useGpu; }
int SettingsManager::cudaDeviceId() const { return m_cudaDeviceId; }
bool SettingsManager::showCharts() const { return m_showCharts; }
Logger::Level SettingsManager::logLevel() const { return m_logLevel; }
bool SettingsManager::autoScrollLog() const { return m_autoScrollLog; }
QByteArray SettingsManager::windowGeometry() const { return m_windowGeometry; }
QByteArray SettingsManager::windowState() const { return m_windowState; }

// Сеттеры
void SettingsManager::setDefaultCpuAlgorithm(CpuAlgorithm algo) {
    if (m_defaultCpuAlgorithm != algo) {
        m_defaultCpuAlgorithm = algo;
        emit settingChanged("cpu/algorithm");
    }
}

void SettingsManager::setDefaultGpuAlgorithm(GpuAlgorithm algo) {
    if (m_defaultGpuAlgorithm != algo) {
        m_defaultGpuAlgorithm = algo;
        emit settingChanged("gpu/algorithm");
    }
}

void SettingsManager::setDefaultArraySize(int size) {
    if (m_defaultArraySize != size) {
        m_defaultArraySize = size;
        emit settingChanged("bench/arraySize");
    }
}

void SettingsManager::setDefaultDistribution(Distribution dist) {
    if (m_defaultDistribution != dist) {
        m_defaultDistribution = dist;
        emit settingChanged("bench/distribution");
    }
}

void SettingsManager::setMinValue(int val) {
    if (m_minValue != val) {
        m_minValue = val;
        emit settingChanged("bench/minValue");
    }
}

void SettingsManager::setMaxValue(int val) {
    if (m_maxValue != val) {
        m_maxValue = val;
        emit settingChanged("bench/maxValue");
    }
}

void SettingsManager::setIterations(int count) {
    if (m_iterations != count) {
        m_iterations = count;
        emit settingChanged("bench/iterations");
    }
}

void SettingsManager::setVerifyResults(bool verify) {
    if (m_verifyResults != verify) {
        m_verifyResults = verify;
        emit settingChanged("bench/verify");
    }
}

void SettingsManager::setUseGpu(bool enabled) {
    if (m_useGpu != enabled) {
        m_useGpu = enabled;
        emit settingChanged("gpu/enabled");
    }
}

void SettingsManager::setCudaDeviceId(int id) {
    if (m_cudaDeviceId != id) {
        m_cudaDeviceId = id;
        emit settingChanged("gpu/deviceId");
    }
}

void SettingsManager::setShowCharts(bool show) {
    if (m_showCharts != show) {
        m_showCharts = show;
        emit settingChanged("ui/showCharts");
    }
}

void SettingsManager::setLogLevel(Logger::Level level) {
    if (m_logLevel != level) {
        m_logLevel = level;
        emit settingChanged("ui/logLevel");
    }
}

void SettingsManager::setAutoScrollLog(bool scroll) {
    if (m_autoScrollLog != scroll) {
        m_autoScrollLog = scroll;
        emit settingChanged("ui/autoScrollLog");
    }
}

void SettingsManager::setWindowGeometry(const QByteArray &geom) {
    m_windowGeometry = geom;
    emit settingChanged("ui/windowGeometry");
}

void SettingsManager::setWindowState(const QByteArray &state) {
    m_windowState = state;
    emit settingChanged("ui/windowState");
}

QString SettingsManager::organizationName() { return "CUDA Lab"; }
QString SettingsManager::applicationName() { return "SortBench"; }

} // namespace SortBench
