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
