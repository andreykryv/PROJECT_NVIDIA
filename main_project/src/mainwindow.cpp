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

    Logger::instance().initialize(Logger::Level::Info, "");

    auto *workerThread = new QThread(this);
    engine = new SortBenchEngine();
    engine->moveToThread(workerThread);
    workerThread->start();

    controlPanel = new ControlPanel(this);
    vizWidget = new VisualizationWidget(this);
    chartWidget = new ChartWidget(this);
    progressPanel = new SortBench::ProgressPanel(this);
    statsPanel = new SortBench::StatsPanel(this);

    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupDockWidgets();
    setupShortcuts();

    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(centralWidget);

    mainSplitter = new QSplitter(Qt::Horizontal, this);

    auto *leftPanel = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(controlPanel);

    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(vizWidget, "Визуализация");
    tabWidget->addTab(chartWidget, "Графики");
    tabWidget->addTab(statsPanel, "Таблица результатов");

    rightLayout->addWidget(tabWidget);
    rightLayout->addWidget(progressPanel);

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 3);

    mainLayout->addWidget(mainSplitter);
    setCentralWidget(centralWidget);

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
}

void MainWindow::setupShortcuts() {}

void MainWindow::connectSignals()
{
    connect(controlPanel, &ControlPanel::parametersChanged,
            this, &MainWindow::onAlgorithmChanged);
    connect(engine, &SortBenchEngine::progressUpdated,
            progressPanel, &SortBench::ProgressPanel::setProgress);
    connect(engine, &SortBenchEngine::frameReady,
            vizWidget, &VisualizationWidget::renderFrame);
    connect(engine, &SortBenchEngine::benchmarkFinished,
            this, &MainWindow::onBenchmarkFinished);
    connect(engine, &SortBenchEngine::logMessage,
            this, &MainWindow::onLogMessage);
    connect(engine, &SortBenchEngine::gpuMemoryUpdated,
            this, &MainWindow::onGPUMemoryUpdated);

    connect(controlPanel, &ControlPanel::runRequested,
            this, &MainWindow::onRunBenchmark);
    connect(controlPanel, &ControlPanel::stopRequested,
            this, &MainWindow::onStopBenchmark);
    connect(controlPanel, &ControlPanel::pauseResumeRequested,
            this, &MainWindow::onPauseResume);
    connect(controlPanel, &ControlPanel::resetRequested,
            this, &MainWindow::onResetAll);

    connect(controlPanel, &ControlPanel::animationSpeedChanged,
            vizWidget, &VisualizationWidget::setAnimationSpeed);

    connect(tabWidget, &QTabWidget::currentChanged, [this](int index) {
        if (index != 0) vizWidget->pause();
        else vizWidget->resume();
    });
}

void MainWindow::onRunBenchmark()
{
    SortParams params = controlPanel->getCurrentParams();

    // Простая проверка вместо params.isValid()
    if (params.arraySize <= 0 || (!params.enableCPU && !params.enableGPU)) {
        QMessageBox::warning(this, tr("Ошибка"),
            tr("Некорректные параметры бенчмарка"));
        return;
    }

    controlPanel->setRunning(true);
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
    QMetaObject::invokeMethod(engine, "togglePause", Qt::QueuedConnection);
    bool paused = !controlPanel->isPaused();
    controlPanel->setPaused(paused);
    if (paused) vizWidget->pause();
    else vizWidget->resume();
}

void MainWindow::onResetAll()
{
    chartWidget->clearResults();
    vizWidget->reset();
    m_results.clear();
    controlPanel->setRunning(false);
    controlPanel->setPaused(false);
    m_statusBar->showMessage(tr("Сброшено"), 2000);
}

void MainWindow::onBenchmarkFinished(const SortBench::BenchmarkResult &result)
{
    controlPanel->setRunning(false);
    m_results.append(result);
    chartWidget->addResult(result);
    statsPanel->updateResult(result);   // было addResult → исправлено на updateResult

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
    QString prefix;
    switch (static_cast<Logger::Level>(level)) {
        case Logger::Level::Error:   prefix = "[ERR] "; break;
        case Logger::Level::Warning: prefix = "[WARN] "; break;
        case Logger::Level::Info:    prefix = "[INFO] "; break;
        case Logger::Level::Debug:   prefix = "[DBG] "; break;
        default: prefix = "";
    }
    logView->append(QString("<font color='%1'>%2%3</font>")
        .arg(level == static_cast<int>(Logger::Level::Error) ? "red" :
             level == static_cast<int>(Logger::Level::Warning) ? "orange" : "black")
        .arg(prefix).arg(message));
    auto *scrollBar = logView->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::onGPUMemoryUpdated(size_t used, size_t total)
{
    double usedMB = used / (1024.0 * 1024.0);
    double totalMB = total / (1024.0 * 1024.0);
    statsPanel->updateGPUMemory(usedMB, totalMB);
    QString text = tr("GPU: %1/%2 MB").arg(usedMB, 0, 'f', 1).arg(totalMB, 0, 'f', 1);
    gpuInfoLabel->setText(text);
}

void MainWindow::applyTheme(const QString &themeName)
{
    QFile qssFile(themeName == "dark" ? ":/styles/darktheme.qss" : ":/styles/lighttheme.qss");
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString styleSheet = QString::fromUtf8(qssFile.readAll());
        qApp->setStyleSheet(styleSheet);
        qssFile.close();
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
    auto geometry = SettingsManager::instance().windowGeometry();
    auto state = SettingsManager::instance().windowState();
    if (!geometry.isEmpty()) restoreGeometry(geometry);
    if (!state.isEmpty()) restoreState(state);
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
    if (event->key() == Qt::Key_F5) onRunBenchmark();
    else if (event->key() == Qt::Key_Escape) onStopBenchmark();
    else if (event->key() == Qt::Key_Space) onPauseResume();
    else QMainWindow::keyPressEvent(event);
}

void MainWindow::updateGPUInfo()
{
    if (CudaDeviceInfo::isCudaAvailable()) {
        auto props = CudaDeviceInfo::getProperties(SettingsManager::instance().cudaDeviceId());
        QString text = CudaDeviceInfo::formatDeviceInfo(props);
        gpuInfoLabel->setText(text);
    } else {
        gpuInfoLabel->setText(tr("CUDA недоступна"));
    }
}

} // namespace SortBench