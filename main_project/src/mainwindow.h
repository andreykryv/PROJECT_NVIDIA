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
