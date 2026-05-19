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
#include "core/sortparams.h"
#include "core/sortbenchengine.h"
#include "utils/logger.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Forward declarations - классы в namespace SortBench
namespace SortBench {
class SortBenchEngine;
}

// Include для ControlPanel - требуется полное определение
#include "ui/controlpanel.h"

// Include для виджетов - они НЕ в namespace SortBench  
#include "ui/visualizationwidget.h"
#include "ui/chartwidget.h"
#include "ui/progresspanel.h"
#include "ui/statspanel.h"

// Include для диалогов - они НЕ в namespace SortBench
#include "ui/settingsdialog.h"
#include "ui/aboutdialog.h"

// Класс MainWindow помещаем в пространство имён SortBench
namespace SortBench {

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
    void onRunBenchmark();
    void onStopBenchmark();
    void onPauseResume();
    void onResetAll();
    void onAlgorithmChanged();
    void onArraySizeChanged(int size);
    void onAnimationSpeedChanged(int speed);
    void onBenchmarkFinished(const BenchmarkResult &result);
    void onProgressUpdated(int percent);
    void onVisualizationFrame(const VisFrame &frame);
    void onExportCSV();
    void onExportChart();
    void onOpenSettings();
    void onShowAbout();
    void onThemeToggle();
    void onFullscreenToggle();
    void onFpsTimerTick();
    void onLogMessage(const QString &message, int level);
    void onGPUMemoryUpdated(size_t used, size_t total);

private:
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupDockWidgets();
    void setupShortcuts();
    void applyTheme(const QString &themeName);
    void saveLayout();
    void restoreLayout();
    void connectSignals();
    void updateWindowTitle();
    void updateGPUInfo();

    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    Ui::MainWindow *ui = nullptr;

    SortBench::ControlPanel *controlPanel = nullptr;
    SortBench::VisualizationWidget *vizWidget = nullptr;
    SortBench::ChartWidget *chartWidget = nullptr;
    SortBench::ProgressPanel *progressPanel = nullptr;
    SortBench::StatsPanel *statsPanel = nullptr;
    SortBenchEngine *engine = nullptr;

    QSplitter *mainSplitter = nullptr;
    QTabWidget *tabWidget = nullptr;
    QDockWidget *logDock = nullptr;
    QTextEdit *logView = nullptr;

    QStatusBar *m_statusBar = nullptr;
    QToolBar *toolbar = nullptr;
    QLabel *gpuInfoLabel = nullptr;
    QLabel *fpsLabel = nullptr;

    QTimer *fpsTimer = nullptr;
    SettingsDialog *settingsDialog = nullptr;
    AboutDialog *aboutDialog = nullptr;

    bool m_isDarkTheme = true;
    QList<BenchmarkResult> m_results;
    QAction *m_themeAction = nullptr;
};

} // namespace SortBench

#endif // MAINWINDOW_H