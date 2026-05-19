#include "settingsmanager.h"
#include <QDir>
#include <QStandardPaths>

namespace SortBench {

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
    m_settings = new QSettings("CUDA Lab", "SortBench", this);
    initDefaults();
}

SettingsManager::~SettingsManager() = default;

SettingsManager& SettingsManager::instance() {
    static SettingsManager inst;
    return inst;
}

void SettingsManager::load() {
    // QSettings читает из хранилища автоматически при первом обращении к value().
    // Явная загрузка не требуется.
}

void SettingsManager::save() {
    m_settings->sync();
}

void SettingsManager::resetToDefaults() {
    m_settings->clear();
    m_settings->sync();
}

void SettingsManager::initDefaults() {
    m_defaults["General/Theme"]            = "dark";
    m_defaults["General/Language"]         = "ru";
    m_defaults["General/LogLevel"]         = 2;
    m_defaults["General/AutoSaveResults"]  = false;
    m_defaults["General/SaveResultsPath"]  = QDir::homePath() + "/SortBenchResults";
    m_defaults["General/ConfirmOnExit"]    = true;

    m_defaults["CUDA/PreferredDeviceIndex"]= 0;
    m_defaults["CUDA/CudaStreams"]         = 4;
    m_defaults["CUDA/CudaBlockSize"]       = 256;
    m_defaults["CUDA/UsePinnedMemory"]     = true;
    m_defaults["CUDA/ShowProfilingData"]   = false;

    m_defaults["Visualization/MaxFPS"]          = 60;
    m_defaults["Visualization/MaxVisElements"]  = 1000;
    m_defaults["Visualization/UseOpenGL"]       = false;
    m_defaults["Visualization/ShowLegend"]      = true;
    m_defaults["Visualization/ShowCounters"]    = true;
    m_defaults["Visualization/FrameBufferSize"] = 10;

    m_defaults["Window/Geometry"]    = QByteArray();
    m_defaults["Window/WindowState"] = QByteArray();

    m_defaults["LastSession/LastCpuAlgorithm"] = 1;
    m_defaults["LastSession/LastGpuAlgorithm"] = 1;
    m_defaults["LastSession/LastArraySize"]    = 100000;
    m_defaults["LastSession/LastDataType"]     = 0;
    m_defaults["LastSession/LastDistribution"] = 0;
}

QString SettingsManager::keyToString(Key key) {
    switch (key) {
        case Key::Theme:                return "General/Theme";
        case Key::Language:             return "General/Language";
        case Key::LogLevel:             return "General/LogLevel";
        case Key::AutoSaveResults:      return "General/AutoSaveResults";
        case Key::SaveResultsPath:      return "General/SaveResultsPath";
        case Key::ConfirmOnExit:        return "General/ConfirmOnExit";
        case Key::PreferredDeviceIndex: return "CUDA/PreferredDeviceIndex";
        case Key::CudaStreams:          return "CUDA/CudaStreams";
        case Key::CudaBlockSize:        return "CUDA/CudaBlockSize";
        case Key::UsePinnedMemory:      return "CUDA/UsePinnedMemory";
        case Key::ShowProfilingData:    return "CUDA/ShowProfilingData";
        case Key::MaxFPS:               return "Visualization/MaxFPS";
        case Key::MaxVisElements:       return "Visualization/MaxVisElements";
        case Key::UseOpenGL:            return "Visualization/UseOpenGL";
        case Key::ShowLegend:           return "Visualization/ShowLegend";
        case Key::ShowCounters:         return "Visualization/ShowCounters";
        case Key::FrameBufferSize:      return "Visualization/FrameBufferSize";
        case Key::Geometry:             return "Window/Geometry";
        case Key::WindowState:          return "Window/WindowState";
        case Key::LastCpuAlgorithm:     return "LastSession/LastCpuAlgorithm";
        case Key::LastGpuAlgorithm:     return "LastSession/LastGpuAlgorithm";
        case Key::LastArraySize:        return "LastSession/LastArraySize";
        case Key::LastDataType:         return "LastSession/LastDataType";
        case Key::LastDistribution:     return "LastSession/LastDistribution";
        default:                        return "";
    }
}

} // namespace SortBench
