#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/controlpanel.h"
#include "ui/visualizationwidget.h"
#include "ui/chartwidget.h"
#include "ui/progresspanel.h"
#include "ui/statspanel.h"
#include "ui/settingsdialog.h"
#include "ui/aboutdialog.h"
#include "core/sortbenchengine.h"
#include "core/benchmarkresult.h"
#include "core/sortparams.h"
#include "utils/settingsmanager.h"
#include "utils/logger.h"
#include "utils/csvexporter.h"
#include "gpu/cudadeviceinfo.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDateTime>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QScrollBar>
#include <QThread>

namespace SortBench {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qRegisterMetaType<SortParams>("SortBench::SortParams");
    qRegisterMetaType<BenchmarkResult>("SortBench::BenchmarkResult");

    // FIX 1: Do NOT call Logger::initialize() here — main.cpp already did it.
    // Calling it again leaks QFile handles and overwrites settings.
    // Logger::instance().initialize(Logger::Level::Info, "");  // REMOVED

    auto *workerThread = new QThread(this);
    engine = new SortBenchEngine();
    engine->moveToThread(workerThread);
    connect(workerThread, &QThread::started, engine, &SortBenchEngine::startPolling);
    workerThread->start();

    // Get widgets from the .ui file
    controlPanel = qobject_cast<SortBench::ControlPanel*>(ui->controlPanel);
    vizWidget    = qobject_cast<SortBench::VisualizationWidget*>(ui->visualizationWidget);
    chartWidget  = qobject_cast<SortBench::ChartWidget*>(ui->chartWidget);

    tabWidget    = ui->mainTabWidget;
    mainSplitter = ui->mainSplitter;

    progressPanel = new SortBench::ProgressPanel(this);
    statsPanel    = new SortBench::StatsPanel(this);

    // setupDockWidgets MUST come BEFORE setupMenuBar (logDock is used in menu)
    setupDockWidgets();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupShortcuts();

    // Add progressPanel into the container from the .ui
    auto *progressContainer = findChild<QWidget*>("progressContainer");
    if (progressContainer) {
        auto *layout = progressContainer->layout();
        if (layout) {
            layout->addWidget(progressPanel);
        }
    }

    // Add statsPanel as an extra tab
    if (tabWidget) {
        tabWidget->addTab(statsPanel, tr("Статистика"));
    }

    restoreLayout();
    applyTheme(SettingsManager::instance().theme());

    updateGPUInfo();

    fpsTimer = new QTimer(this);
    connect(fpsTimer, &QTimer::timeout, this, &MainWindow::onFpsTimerTick);
    fpsTimer->start(1000);

    connectSignals();
    updateWindowTitle();
}

MainWindow::~MainWindow()
{
    SettingsManager::instance().save();
    saveLayout();
    delete ui;
}

void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu(tr("Файл"));
    auto *newTestAction = fileMenu->addAction(tr("Новый тест"), this, &MainWindow::onResetAll);
    newTestAction->setShortcut(QKeySequence::New);
    fileMenu->addSeparator();
    auto *exportCSVAction = fileMenu->addAction(tr("Экспорт CSV"), this, &MainWindow::onExportCSV);
    exportCSVAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    auto *exportChartAction = fileMenu->addAction(tr("Экспорт графика..."), this, &MainWindow::onExportChart);
    Q_UNUSED(exportChartAction);
    fileMenu->addSeparator();
    auto *exitAction = fileMenu->addAction(tr("Выход"), this, &QWidget::close);
    exitAction->setShortcut(QKeySequence::Quit);

    auto *viewMenu = menuBar()->addMenu(tr("Вид"));
    m_themeAction = viewMenu->addAction(tr("Тёмная тема"), this, &MainWindow::onThemeToggle);
    m_themeAction->setCheckable(true);
    m_themeAction->setChecked(m_isDarkTheme);
    auto *fullscreenAction = viewMenu->addAction(tr("Полный экран"), this, &MainWindow::onFullscreenToggle);
    fullscreenAction->setShortcut(QKeySequence::FullScreen);
    viewMenu->addSeparator();
    viewMenu->addAction(tr("Показать лог"), logDock, &QDockWidget::show);

    auto *runMenu = menuBar()->addMenu(tr("Запуск"));
    auto *runAction = runMenu->addAction(tr("Запустить"), this, &MainWindow::onRunBenchmark);
    runAction->setShortcut(QKeySequence(Qt::Key_F5));
    auto *stopAction = runMenu->addAction(tr("Стоп"), this, &MainWindow::onStopBenchmark);
    stopAction->setShortcut(QKeySequence(Qt::Key_Escape));
    auto *pauseAction = runMenu->addAction(tr("Пауза"), this, &MainWindow::onPauseResume);
    pauseAction->setShortcut(QKeySequence(Qt::Key_Space));

    auto *helpMenu = menuBar()->addMenu(tr("Справка"));
    helpMenu->addAction(tr("О программе"), this, &MainWindow::onShowAbout);
    helpMenu->addAction(tr("Документация"), []() {
        QDesktopServices::openUrl(QUrl("https://github.com/cuda-lab/sortbench"));
    });
}

void MainWindow::setupToolBar()
{
    toolbar = addToolBar(tr("Панель инструментов"));
    toolbar->setMovable(false);
    auto *runAction = toolbar->addAction("▶", this, &MainWindow::onRunBenchmark);
    runAction->setToolTip(tr("Запустить тест (F5)"));
    auto *stopAction = toolbar->addAction("■", this, &MainWindow::onStopBenchmark);
    stopAction->setToolTip(tr("Остановить тест (Esc)"));
    auto *pauseAction = toolbar->addAction("⏸", this, &MainWindow::onPauseResume);
    pauseAction->setToolTip(tr("Пауза/Продолжить (Space)"));
    toolbar->addSeparator();
    auto *csvAction = toolbar->addAction("📄", this, &MainWindow::onExportCSV);
    csvAction->setToolTip(tr("Экспорт в CSV"));
}

void MainWindow::setupStatusBar()
{
    m_statusBar = statusBar();
    gpuInfoLabel = new QLabel(this);
    m_statusBar->addPermanentWidget(gpuInfoLabel);
    fpsLabel = new QLabel("FPS: 0", this);
    m_statusBar->addPermanentWidget(fpsLabel);
}

void MainWindow::setupDockWidgets()
{
    logDock = new QDockWidget(tr("Лог"), this);
    logDock->setObjectName("logDock");
    logView = new QTextEdit(this);
    logView->setReadOnly(true);
    logView->setFont(QFont("Consolas", 9));
    logDock->setWidget(logView);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);
    logDock->hide();   // hidden by default, user can show via menu
}

void MainWindow::setupShortcuts() {}

void MainWindow::connectSignals()
{
    connect(controlPanel, &SortBench::ControlPanel::parametersChanged,
            this, &MainWindow::onAlgorithmChanged);

    // FIX 2: progressUpdated(int, QString) cannot connect directly to setProgress(int)
    // with typed (new-style) connect — signatures differ.  Use a lambda adapter.
    connect(engine, &SortBenchEngine::progressUpdated,
            this, [this](int percent, const QString &phase) {
                progressPanel->setProgress(percent);
                progressPanel->setPhase(phase);
            });

    connect(engine, &SortBenchEngine::frameReady,
            vizWidget, &SortBench::VisualizationWidget::renderFrame);
    connect(engine, &SortBenchEngine::benchmarkFinished,
            this, &MainWindow::onBenchmarkFinished);
    connect(engine, &SortBenchEngine::logMessage,
            this, &MainWindow::onLogMessage);
    connect(engine, &SortBenchEngine::gpuMemoryUpdated,
            this, &MainWindow::onGPUMemoryUpdated);

    connect(controlPanel, &SortBench::ControlPanel::runRequested,
            this, &MainWindow::onRunBenchmark);
    connect(controlPanel, &SortBench::ControlPanel::stopRequested,
            this, &MainWindow::onStopBenchmark);
    connect(controlPanel, &SortBench::ControlPanel::pauseResumeRequested,
            this, &MainWindow::onPauseResume);
    connect(controlPanel, &SortBench::ControlPanel::resetRequested,
            this, &MainWindow::onResetAll);

    connect(controlPanel, &SortBench::ControlPanel::animationSpeedChanged,
            vizWidget, &VisualizationWidget::setAnimationSpeed);

    connect(tabWidget, &QTabWidget::currentChanged, [this](int index) {
        if (index != 0) vizWidget->pause();
        else            vizWidget->resume();
    });
}

void MainWindow::onRunBenchmark()
{
    SortParams params = controlPanel->getCurrentParams();

    if (params.arraySize <= 0 || (!params.enableCPU && !params.enableGPU)) {
        QMessageBox::warning(this, tr("Ошибка"),
            tr("Некорректные параметры бенчмарка"));
        return;
    }

    controlPanel->setRunning(true);
    progressPanel->setBenchmarkStarted();
    QMetaObject::invokeMethod(engine, "startBenchmark",
        Qt::QueuedConnection,
        Q_ARG(SortBench::SortParams, params));

    tabWidget->setCurrentWidget(vizWidget);
}

void MainWindow::onStopBenchmark()
{
    QMetaObject::invokeMethod(engine, "stopBenchmark", Qt::QueuedConnection);
    controlPanel->setRunning(false);
}

void MainWindow::onPauseResume()
{
    // FIX 3: "togglePause" does not exist on SortBenchEngine.
    // Use the correct slot names: pauseBenchmark / resumeBenchmark.
    bool currentlyPaused = controlPanel->isPaused();
    if (!currentlyPaused) {
        QMetaObject::invokeMethod(engine, "pauseBenchmark", Qt::QueuedConnection);
        vizWidget->pause();
    } else {
        QMetaObject::invokeMethod(engine, "resumeBenchmark", Qt::QueuedConnection);
        vizWidget->resume();
    }
    controlPanel->setPaused(!currentlyPaused);
}

void MainWindow::onResetAll()
{
    chartWidget->clearResults();
    vizWidget->reset();
    progressPanel->reset();
    m_results.clear();
    controlPanel->setRunning(false);
    controlPanel->setPaused(false);
    m_statusBar->showMessage(tr("Сброшено"), 2000);
}

void MainWindow::onAlgorithmChanged()
{
    // Reserved for future handling of parameter changes in the UI.
}

void MainWindow::onArraySizeChanged(int size)
{
    Q_UNUSED(size);
}

void MainWindow::onAnimationSpeedChanged(int speed)
{
    vizWidget->setAnimationSpeed(speed);
}

void MainWindow::onBenchmarkFinished(const SortBench::BenchmarkResult &result)
{
    controlPanel->setRunning(false);
    m_results.append(result);
    chartWidget->addResult(result);
    statsPanel->updateResult(result);
    progressPanel->setBenchmarkFinished(result.cpuTimeMs, result.gpuTotalTimeMs);

    QString message;
    if (result.gpuTotalTimeMs > 0.0) {
        double speedup = result.cpuTimeMs / result.gpuTotalTimeMs;
        message = tr("Готово. CPU: %1 мс, GPU: %2 мс, ускорение: %3x")
            .arg(result.cpuTimeMs, 0, 'f', 2)
            .arg(result.gpuTotalTimeMs, 0, 'f', 2)
            .arg(speedup, 0, 'f', 2);
    } else {
        message = tr("Готово. Время: %1 мс").arg(result.cpuTimeMs, 0, 'f', 2);
    }
    m_statusBar->showMessage(message, 5000);
    onLogMessage(message, static_cast<int>(Logger::Level::Info));
}

void MainWindow::onProgressUpdated(int percent)
{
    progressPanel->setProgress(percent);
}

void MainWindow::onVisualizationFrame(const SortBench::VisFrame &frame)
{
    vizWidget->renderFrame(frame);
}

void MainWindow::onExportCSV()
{
    if (m_results.isEmpty()) {
        QMessageBox::information(this, tr("Нет данных"),
            tr("Нет результатов для экспорта"));
        return;
    }
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                          + "/sortbench_results.csv";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Экспорт в CSV"),
                                                    defaultPath, tr("CSV Files (*.csv)"));
    if (fileName.isEmpty()) return;
    CsvExporter::exportCsv(m_results, fileName);
    m_statusBar->showMessage(tr("Экспортировано в ") + fileName, 3000);
}

void MainWindow::onExportChart()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Экспорт графика"),
                                                    "", tr("PNG Images (*.png);;SVG Images (*.svg)"));
    if (fileName.isEmpty()) return;
    chartWidget->exportChart(0, fileName);
    m_statusBar->showMessage(tr("График сохранён"), 2000);
}

void MainWindow::onOpenSettings()
{
    if (!settingsDialog) settingsDialog = new SettingsDialog(this);
    settingsDialog->loadFromSettings();
    if (settingsDialog->exec() == QDialog::Accepted) {
        settingsDialog->saveToSettings();
        applyTheme(SettingsManager::instance().theme());
    }
}

void MainWindow::onShowAbout()
{
    if (!aboutDialog) aboutDialog = new AboutDialog(this);
    aboutDialog->exec();
}

void MainWindow::onThemeToggle()
{
    m_isDarkTheme = !m_isDarkTheme;
    QString theme = m_isDarkTheme ? "dark" : "light";
    SettingsManager::instance().setTheme(theme);
    applyTheme(theme);
    m_themeAction->setChecked(m_isDarkTheme);
}

void MainWindow::onFullscreenToggle()
{
    if (isFullScreen()) showNormal();
    else showFullScreen();
}

void MainWindow::onFpsTimerTick()
{
    int fps = vizWidget->currentFPS();
    fpsLabel->setText(QString("FPS: %1").arg(fps));
}

void MainWindow::onLogMessage(const QString &message, int level)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString color;
    QString prefix;
    switch (static_cast<Logger::Level>(level)) {
        case Logger::Level::Error:   color = "red";    prefix = "[ERR] ";  break;
        case Logger::Level::Warning: color = "orange"; prefix = "[WARN] "; break;
        case Logger::Level::Debug:   color = "gray";   prefix = "[DBG] ";  break;
        default:                     color = "black";  prefix = "[INFO] "; break;
    }
    logView->append(QString("<font color='%1'>%2 %3%4</font>")
        .arg(color, timestamp, prefix, message.toHtmlEscaped()));
    auto *scrollBar = logView->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::onGPUMemoryUpdated(size_t used, size_t total)
{
    // FIX 4: pass raw bytes to updateGPUMemory — it handles the MB conversion itself.
    // Previous code was dividing to MB and then passing those MB values as "bytes",
    // causing a double conversion and garbage display values.
    statsPanel->updateGPUMemory(used, total);

    double usedMB  = static_cast<double>(used)  / (1024.0 * 1024.0);
    double totalMB = static_cast<double>(total) / (1024.0 * 1024.0);
    gpuInfoLabel->setText(tr("GPU: %1/%2 MB").arg(usedMB, 0, 'f', 1).arg(totalMB, 0, 'f', 1));
}

void MainWindow::applyTheme(const QString &themeName)
{
    // FIX 5: The .qrc has prefix="/styles" and file="styles/darktheme.qss",
    // so the full resource path is ":/styles/styles/darktheme.qss", not
    // ":/styles/darktheme.qss" as was written before.
    QString path = (themeName == "dark")
        ? ":/styles/styles/darktheme.qss"
        : ":/styles/styles/lighttheme.qss";

    QFile qssFile(path);
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qApp->setStyleSheet(QString::fromUtf8(qssFile.readAll()));
        qssFile.close();
    } else {
        qWarning() << "Could not open stylesheet:" << path;
    }
    emit themeChanged(m_isDarkTheme);
}

void MainWindow::saveLayout()
{
    SettingsManager::instance().setWindowGeometry(saveGeometry());
    SettingsManager::instance().setWindowState(saveState());
}

void MainWindow::restoreLayout()
{
    QByteArray geometry = SettingsManager::instance().windowGeometry();
    QByteArray state    = SettingsManager::instance().windowState();
    if (!geometry.isEmpty()) restoreGeometry(geometry);
    if (!state.isEmpty())    restoreState(state);
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(tr("SortBench — Бенчмарк сортировок CPU vs GPU"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (controlPanel->isRunning()) {
        auto reply = QMessageBox::question(this, tr("Подтверждение"),
            tr("Тест выполняется. Прервать и закрыть приложение?"),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }
        onStopBenchmark();
    }
    saveLayout();
    SettingsManager::instance().save();
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if      (event->key() == Qt::Key_F5)    onRunBenchmark();
    else if (event->key() == Qt::Key_Escape) onStopBenchmark();
    else if (event->key() == Qt::Key_Space)  onPauseResume();
    else QMainWindow::keyPressEvent(event);
}

void MainWindow::updateGPUInfo()
{
    if (CudaDeviceInfo::isCudaAvailable()) {
        auto props = CudaDeviceInfo::getProperties(SettingsManager::instance().cudaDeviceId());
        gpuInfoLabel->setText(CudaDeviceInfo::formatDeviceInfo(props));
    } else {
        gpuInfoLabel->setText(tr("CUDA недоступна"));
    }
}

} // namespace SortBench