////////////////////////////////////////////////////////////////////////////////
// mainwindow.h — заголовочный файл главного окна приложения
//
// НАЗНАЧЕНИЕ:
//   Объявляет класс MainWindow — центральный виджет приложения, который
//   объединяет все панели управления, визуализацию и графики в одном окне.
//
// КЛАСС: MainWindow : public QMainWindow
//
//   ЧЛЕНЫ-ДАННЫЕ:
//     Ui::MainWindow *ui           — указатель на сгенерированный UIC класс,
//                                    связывает C++-код с mainwindow.ui
//     ControlPanel *controlPanel   — панель выбора алгоритмов и параметров
//     VisualizationWidget *vizWidget — виджет анимации сортировки (столбцы)
//     ChartWidget *chartWidget     — виджет с QtCharts-графиками результатов
//     ProgressPanel *progressPanel — строка прогресса и статус-метки
//     StatsPanel *statsPanel       — панель статистики (сравнения, памяти GPU)
//     SortBenchEngine *engine      — движок запуска тестов и управления потоками
//     QSplitter *mainSplitter      — сплиттер между визуализацией и графиком
//     QTabWidget *tabWidget        — вкладки: "Визуализация" / "Графики" / "Таблица"
//     QDockWidget *logDock         — плавающая панель лога (можно прикрепить/отстыковать)
//     QTextEdit *logView           — текстовое поле для вывода логов
//     QStatusBar *statusBar        — нижняя строка состояния
//     QToolBar *toolbar            — панель быстрых действий
//     QLabel *gpuInfoLabel         — строка с именем GPU и VRAM
//     QTimer *fpsTimer             — таймер для обновления счётчика FPS анимации
//
//   СЛОТЫ (slots):
//     onRunBenchmark()             — запускает выбранный тест
//     onStopBenchmark()            — прерывает текущий тест
//     onPauseResume()              — пауза/возобновление анимации
//     onResetAll()                 — сброс всех результатов и визуализации
//     onAlgorithmChanged(...)      — реакция на смену выбранного алгоритма
//     onArraySizeChanged(int)      — обновление UI при изменении размера массива
//     onAnimationSpeedChanged(int) — регулировка FPS анимации (ползунок)
//     onBenchmarkFinished(...)     — получение результатов от движка
//     onProgressUpdated(int)       — обновление прогресс-бара
//     onVisualizationFrame(...)    — получение кадра анимации для отрисовки
//     onExportCSV()                — экспорт результатов в CSV файл
//     onExportChart()              — сохранение графика как PNG/SVG
//     onOpenSettings()             — открытие диалога настроек
//     onShowAbout()                — диалог "О программе"
//     onThemeToggle()              — переключение тёмная/светлая тема
//     onFullscreenToggle()         — полноэкранный режим
//     onFpsTimerTick()             — обновление счётчика FPS в статус-баре
//     onLogMessage(QString, int)   — вывод сообщения в панель лога
//     onGPUMemoryUpdated(size_t, size_t) — обновление индикатора памяти GPU
//
//   МЕТОДЫ:
//     setupMenuBar()               — создание меню: Файл, Вид, Запуск, Справка
//     setupToolBar()               — иконки быстрого доступа с QAction
//     setupStatusBar()             — нижняя строка: FPS, статус GPU, прогресс
//     setupDockWidgets()           — плавающие панели (лог, устройство)
//     setupShortcuts()             — горячие клавиши (F5=запуск, Space=пауза, ...)
//     applyTheme(const QString &)  — применение QSS-темы во время выполнения
//     saveLayout()                 — сохранение геометрии окна в QSettings
//     restoreLayout()              — восстановление геометрии из QSettings
//     connectSignals()             — подключение всех signal-slot соединений
//     updateWindowTitle()          — обновление заголовка с текущим режимом
//
//   ПЕРЕОПРЕДЕЛЁННЫЕ ВИРТУАЛЬНЫЕ МЕТОДЫ:
//     closeEvent(QCloseEvent *)    — перехват закрытия: сохранение настроек,
//                                    остановка фоновых потоков, подтверждение
//     resizeEvent(QResizeEvent *)  — адаптивная перестройка layout при ресайзе
//     keyPressEvent(QKeyEvent *)   — обработка горячих клавиш
//
//   СИГНАЛЫ (signals):
//     themeChanged(bool isDark)    — смена темы (для дочерних виджетов)
//     benchmarkStarted()
//     benchmarkStopped()
////////////////////////////////////////////////////////////////////////////////

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <QDockWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QTimer>
#include <QList>

#include "core/benchmarkresult.h"
#include "core/sortbenchengine.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ControlPanel;
class VisualizationWidget;
class ChartWidget;
class ProgressPanel;
class StatsPanel;
class SortBenchEngine;
class SettingsDialog;
class AboutDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void themeChanged(bool isDark);
    void benchmarkStarted();
    void benchmarkStopped();

private slots:
    // Управление бенчмарком
    void onRunBenchmark();
    void onStopBenchmark();
    void onPauseResume();
    void onResetAll();
    
    // Параметры
    void onAlgorithmChanged();
    void onArraySizeChanged(int size);
    void onAnimationSpeedChanged(int speed);
    
    // Результаты
    void onBenchmarkFinished(const SortBench::BenchmarkResult &result);
    void onProgressUpdated(int percent);
    void onVisualizationFrame(const SortBench::VisFrame &frame);
    
    // Экспорт и настройки
    void onExportCSV();
    void onExportChart();
    void onOpenSettings();
    void onShowAbout();
    
    // Тема и вид
    void onThemeToggle();
    void onFullscreenToggle();
    void onFpsTimerTick();
    
    // Логирование
    void onLogMessage(const QString &message, int level);
    void onGPUMemoryUpdated(size_t used, size_t total);

private:
    // Инициализация UI
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupDockWidgets();
    void setupShortcuts();
    
    // Настройки
    void applyTheme(const QString &themeName);
    void saveLayout();
    void restoreLayout();
    
    // Сигналы
    void connectSignals();
    void updateWindowTitle();
    
    // События
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
private:
    void updateGPUInfo();   // вызывается в конструкторе MainWindow
    QAction *m_themeAction = nullptr;  // используется в setupMenuBar() и onThemeToggle()
private:
    Ui::MainWindow *ui = nullptr;
    
    // Панели
    ControlPanel *controlPanel = nullptr;
    VisualizationWidget *vizWidget = nullptr;
    ChartWidget *chartWidget = nullptr;
    ProgressPanel *progressPanel = nullptr;
    StatsPanel *statsPanel = nullptr;
    
    // Движок
    SortBenchEngine *engine = nullptr;
    
    // Layout
    QSplitter *mainSplitter = nullptr;
    QTabWidget *tabWidget = nullptr;
    
    // Dock widgets
    QDockWidget *logDock = nullptr;
    QTextEdit *logView = nullptr;
    
    // Status bar
    QStatusBar *statusBar = nullptr;
    QToolBar *toolbar = nullptr;
    QLabel *gpuInfoLabel = nullptr;
    QLabel *fpsLabel = nullptr;
    
    // Таймеры
    QTimer *fpsTimer = nullptr;
    
    // Диалоги
    SettingsDialog *settingsDialog = nullptr;
    AboutDialog *aboutDialog = nullptr;
    
    // Состояние
    bool m_isDarkTheme = true;
    QList<SortBench::BenchmarkResult> m_results;
};

#endif // MAINWINDOW_H
