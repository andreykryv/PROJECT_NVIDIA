/**
 * @file mainwindow.cpp
 * @brief Главное окно интерфейса SortBench.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mainwindow.h"
#include "settings_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QBarSet>
#include <QBarSeries>
#include <QLineSeries>
#include <QValueAxis>
#include <QBarCategoryAxis>
#include <QColor>
#include <QThread>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QFrame>
#include <QPainter>
#include <QResizeEvent>
#include <QtGlobal>
#include <QSysInfo>
#include <QSettings>
#include <utility>
#include "cpu_algorithms.h" 

#ifdef Q_OS_WIN
#include <QSettings>
#endif
#ifdef Q_OS_LINUX
#include <QFile>
#include <QTextStream>
#endif
#ifdef Q_OS_MAC
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <cuda_runtime.h>

static const char* C_BG_ROOT    = "#0d0d1b";
static const char* C_BG_SIDEBAR = "#11112b";
static const char* C_BG_CARD    = "#191929";
static const char* C_BG_TOPBAR  = "#111128";
static const char* C_BORDER     = "#232342";
static const char* C_ACCENT     = "#6366f1";
static const char* C_ACCENT2    = "#4f46e5";
static const char* C_BLUE       = "#3b82f6";
static const char* C_TEXT1      = "#f1f5f9";
static const char* C_TEXT2      = "#94a3b8";
static const char* C_TEXT3      = "#64748b";
static const char* C_SUCCESS    = "#10b981";
static const char* C_WARNING    = "#f59e0b";
static const char* C_DANGER     = "#ef4444";

// Вспомогательные структуры алгоритмов сайдбара
struct SidebarAlgInfo {
    QString name;
    QString desc;
    QString id;
    bool isGPU;
};

static const QList<SidebarAlgInfo> sidebarAlgs = {
    {"std::sort", "Быстрая сортировка C++ STL (IntroSort)", "CPU_std::sort", false},
    {"QuickSort", "Быстрая сортировка с разделением Хоара", "CPU_QuickSort", false},
    {"MergeSort", "Стабильная сортировка слиянием блоков", "CPU_MergeSort", false},
    {"HeapSort", "Эффективная пирамидальная сортировка", "CPU_HeapSort", false},
    {"TimSort", "Гибридный алгоритм Timsort (вставки + слияние)", "CPU_TimSort", false},
    {"BubbleSort", "Классический пузырьковый обмен соседних пар", "CPU_BubbleSort", false},
    {"SelectionSort", "Сортировка выбором минимального значения", "CPU_SelectionSort", false},
    {"InsertionSort", "Сортировка последовательными вставками", "CPU_InsertionSort", false},
    {"ShellSort", "Сортировка Шелла с уменьшающимся шагом", "CPU_ShellSort", false},
    {"CocktailSort", "Шейкерная двунаправленная сортировка", "CPU_CocktailSort", false},
    {"GnomeSort", "Оригинальная гномья сортировка обменами", "CPU_GnomeSort", false},
    {"CombSort", "Улучшенная пузырьковая сортировка расчёской", "CPU_CombSort", false},
    {"RadixSortLSD", "Поразрядная сортировка по младшим байтам", "CPU_RadixSortLSD", false},
    {"CountingSort", "Линейная сортировка подсчетом частоты ключей", "CPU_CountingSort", false},
    {"BucketSort", "Блочная сортировка распределением по бакетам", "CPU_BucketSort", false},
    {"PancakeSort", "Блинная сортировка переворотами префиксов", "CPU_PancakeSort", false},
    {"BogoSort", "Случайное угадывание правильного порядка", "CPU_BogoSort", false},
    {"StoogeSort", "Рекурсивный неоптимальный алгоритм Stooge", "CPU_StoogeSort", false},
    {"OddEvenSort", "Параллельная четно-нечетная сортировка", "CPU_OddEvenSort", false},
    {"CycleSort", "Сортировка с минимизацией записи в память", "CPU_CycleSort", false},

    {"GPU Bitonic", "Параллельная битоническая сеть CUDA", "GPU_Bitonic", true},
    {"GPU Radix", "Высокоскоростной поразрядный CUDA-сортировщик", "GPU_Radix", true},
    {"GPU Odd-Even", "Четно-нечетная сортировка на CUDA ядрах", "GPU_OddEven", true},
    {"GPU std::sort", "Реализация сортировки Thrust на GPU", "GPU_StdSort", true},
    {"GPU QuickSort", "Быстрая сортировка на GPU Thrust API", "GPU_QuickSort", true},
    {"GPU MergeSort", "Сортировка слиянием в GPU памяти", "GPU_MergeSort", true},
    {"GPU HeapSort", "Пирамидальная сортировка на CUDA Thrust", "GPU_HeapSort", true},
    {"GPU TimSort", "Практический гибрид Timsort на GPU", "GPU_TimSort", true},
    {"GPU BubbleSort", "Пузырьковый обход в GPU потоках", "GPU_BubbleSort", true},
    {"GPU Selection", "Выборочная сортировка на GPU ядрах", "GPU_SelectionSort", true}
};

static QString getRealCpuName() {
    QString cpu = "";
#ifdef Q_OS_WIN
    QSettings settings("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", QSettings::NativeFormat);
    cpu = settings.value("ProcessorNameString").toString().trimmed();
#elif defined(Q_OS_LINUX)
    QFile file("/proc/cpuinfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.trimmed().startsWith("model name", Qt::CaseInsensitive)) {
                QStringList parts = line.split(":");
                if (parts.size() > 1) {
                    cpu = parts[1].trimmed();
                    break;
                }
            }
        }
        file.close();
    }
#elif defined(Q_OS_MAC)
    char buffer[256];
    size_t bufferlen = sizeof(buffer);
    if (sysctlbyname("machdep.cpu.brand_string", &buffer, &bufferlen, NULL, 0) == 0) {
        cpu = QString::fromLocal8Bit(buffer);
    }
#endif
    if (cpu.isEmpty()) {
        cpu = QSysInfo::currentCpuArchitecture().toUpper();
    }
    return cpu;
}

static QString getRealGpuName() {
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err == cudaSuccess && deviceCount > 0) {
        cudaDeviceProp prop;
        if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
            return QString::fromUtf8(prop.name);
        }
    }
    return "GPU Offline / No CUDA Device";
}

AlgTile::AlgTile(const QString& name, const QString& shortDesc, const QString& id, bool gpu, QWidget* parent)
    : QFrame(parent), algId(id), isGPU(gpu) {
    setObjectName("algTile");
    setCursor(Qt::PointingHandCursor);

    QHBoxLayout* mainLay = new QHBoxLayout(this);
    mainLay->setContentsMargins(10, 8, 10, 8);
    mainLay->setSpacing(10);

    checkbox = new QCheckBox(this);
    checkbox->setStyleSheet("QCheckBox::indicator { width: 14px; height: 14px; }");
    mainLay->addWidget(checkbox);

    QVBoxLayout* textLay = new QVBoxLayout();
    textLay->setSpacing(2);

    QHBoxLayout* titleRow = new QHBoxLayout();
    titleRow->setSpacing(6);
    titleLabel = new QLabel(name, this);
    titleLabel->setStyleSheet("font-weight: 700; color: #f1f5f9; font-size: 12px; background: transparent;");
    titleRow->addWidget(titleLabel);

    QLabel* badge = new QLabel(gpu ? "🚀 GPU" : "💻 CPU", this);
    badge->setStyleSheet(gpu ? "color: #6366f1; font-size: 9px; font-weight: 800; background: rgba(99,102,241,0.15); padding: 1px 4px; border-radius: 4px;"
                             : "color: #3b82f6; font-size: 9px; font-weight: 800; background: rgba(59,130,246,0.15); padding: 1px 4px; border-radius: 4px;");
    titleRow->addWidget(badge);
    titleRow->addStretch();
    textLay->addLayout(titleRow);

    descLabel = new QLabel(shortDesc, this);
    descLabel->setStyleSheet("color: #94a3b8; font-size: 10px; background: transparent;");
    descLabel->setWordWrap(true);
    textLay->addWidget(descLabel);

    mainLay->addLayout(textLay, 1);

    setStyleSheet(R"(
        QFrame#algTile { background: #141428; border: 1px solid #232342; border-radius: 10px; }
        QFrame#algTile:hover { border-color: #6366f1; background: #191932; }
    )");
}

void AlgTile::mousePressEvent(QMouseEvent* event) {
    checkbox->setChecked(!checkbox->isChecked());
    QFrame::mousePressEvent(event);
}

DescCard::DescCard(const AlgDescData& data, QWidget* parent) : QFrame(parent) {
    setObjectName("descCard");
    QVBoxLayout* lay = new QVBoxLayout(this);
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(10);

    QHBoxLayout* topRow = new QHBoxLayout();
    titleLabel = new QLabel(data.name, this);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: 700; color: #60a5fa; background: transparent;");
    topRow->addWidget(titleLabel);
    topRow->addStretch();

    auto makeBadge = [&](const QString& label, const QString& val, const QString& bg) {
        QLabel* b = new QLabel(QString("<b>%1</b> %2").arg(label, val), this);
        b->setStyleSheet(QString("background: %1; color: #f1f5f9; font-size: 10px; padding: 3px 8px; border-radius: 6px;").arg(bg));
        return b;
    };

    topRow->addWidget(makeBadge("Best", data.best, "rgba(16,185,129,0.15)"));
    topRow->addWidget(makeBadge("Avg", data.avg, "rgba(59,130,246,0.15)"));
    topRow->addWidget(makeBadge("Worst", data.worst, "rgba(239,68,68,0.15)"));
    topRow->addWidget(makeBadge("Space", data.space, "rgba(245,158,11,0.15)"));

    lay->addLayout(topRow);

    descLabel = new QLabel(data.description, this);
    descLabel->setStyleSheet("color: #cbd5e6; font-size: 12px; line-height: 1.4; background: transparent;");
    descLabel->setWordWrap(true);
    lay->addWidget(descLabel);

    setStyleSheet(R"(
        QFrame#descCard { background: #191929; border: 1px solid #232342; border-radius: 12px; }
        QFrame#descCard:hover { border-color: #6366f1; }
    )");
}

static QFrame* hLine(QWidget* parent) {
    QFrame* f = new QFrame(parent);
    f->setFrameShape(QFrame::HLine);
    f->setFixedHeight(1);
    f->setStyleSheet(QString("background:%1; border:none;").arg(C_BORDER));
    return f;
}

struct MetricCard {
    QFrame* frame;
    QLabel* value;
    QLabel* badge;
};

static MetricCard makeMetricCard(const QString& icon, const QString& title, const QString& defaultVal,
                                  const QString& badgeText, const QString& badgeColor, QWidget* parent) {
    QFrame* card = new QFrame(parent);
    card->setObjectName("metricCard");

    QVBoxLayout* cl = new QVBoxLayout(card);
    cl->setContentsMargins(16, 14, 16, 14);
    cl->setSpacing(6);

    QHBoxLayout* header = new QHBoxLayout();
    header->setSpacing(8);
    QLabel* iconLbl = new QLabel(icon, parent);
    iconLbl->setStyleSheet("font-size:18px; background:transparent;");
    QLabel* titleLbl = new QLabel(title, parent);
    titleLbl->setStyleSheet(QString("color:%1; font-size:11px; font-weight:600; background:transparent;").arg(C_TEXT2));
    QLabel* dotMenu = new QLabel("···", parent);
    dotMenu->setStyleSheet(QString("color:%1; font-size:13px; background:transparent;").arg(C_TEXT3));
    header->addWidget(iconLbl);
    header->addWidget(titleLbl, 1);
    header->addWidget(dotMenu);
    cl->addLayout(header);

    QLabel* valLbl = new QLabel(defaultVal, parent);
    valLbl->setStyleSheet(QString("color:%1; font-size:26px; font-weight:700; background:transparent;").arg(C_TEXT1));
    cl->addWidget(valLbl);

    QLabel* badge = new QLabel(badgeText, parent);
    badge->setStyleSheet(QString("color:%1; font-size:10px; font-weight:600; background:transparent;").arg(badgeColor));
    cl->addWidget(badge);

    return {card, valLbl, badge};
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_stopVisualRequested(false)
    , m_isVisualPaused(false)
    , m_visualDelayMs(50)
{
    setWindowTitle("SortBench: CUDA vs CPU | Analytics Dashboard");
    resize(1440, 900);
    setMinimumSize(1100, 700);

    m_benchRunner = new BenchmarkRunner(this);
    connect(m_benchRunner, &BenchmarkRunner::algorithmCompleted, this, &MainWindow::onAlgorithmCompleted);
    connect(m_benchRunner, &BenchmarkRunner::progressUpdated, this, &MainWindow::onBenchmarkProgress);
    connect(m_benchRunner, &BenchmarkRunner::finishedAll, this, &MainWindow::onBenchmarkFinished);

    applyMasterStylesheet();
    setupUI();
    setupCharts();
    loadAvailableAlgorithms();
    onGenerateVisualArray();

    // Инициализация периодического опроса телеметрии CUDA VRAM
    m_telemetryTimer = new QTimer(this);
    connect(m_telemetryTimer, &QTimer::timeout, this, &MainWindow::updateSystemTelemetry);
    
    QSettings s;
    m_telemetryTimer->start(s.value("telemetry/interval", 1000).toInt()); 

    updateSystemTelemetry();
}

MainWindow::~MainWindow() {
    onStopVisual();
    if (m_benchRunner) {
        m_benchRunner->requestStop();
        m_benchRunner->wait(3000);
    }
}

void MainWindow::applyMasterStylesheet() {
    setStyleSheet(QString(R"(
        QMainWindow, QWidget { background:%1; color:%2; font-family:'Segoe UI',sans-serif; font-size:13px; }
        QScrollBar:vertical   { background:%3; width:6px; margin:0; border-radius:3px; }
        QScrollBar::handle:vertical { background:%4; border-radius:3px; min-height:24px; }
        QScrollBar::handle:vertical:hover { background:%5; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
        QScrollBar:horizontal { background:%3; height:6px; border-radius:3px; }
        QScrollBar::handle:horizontal { background:%4; border-radius:3px; }
        QFrame#metricCard { background:%6; border:1px solid %7; border-radius:16px; }
        QFrame#metricCard:hover { border-color:%5; }
        QTableWidget { background:%6; border:none; gridline-color:%7; color:%2; font-size:12px; }
        QTableWidget::item { padding:8px 12px; }
        QTableWidget::item:selected { background:%5; color:white; }
        QTableWidget::item:alternate { background:#14142a; }
        QHeaderView::section { background:#141428; color:%8; padding:10px 12px; border:none; border-bottom:1px solid %7; font-weight:600; font-size:11px; }
        QComboBox, QSpinBox { background:#141428; border:1px solid %7; border-radius:8px; padding:5px 10px; color:%2; min-height:24px; }
        QComboBox:hover, QSpinBox:hover { border-color:%5; }
        QComboBox::drop-down { border:none; width:20px; }
        QComboBox QAbstractItemView { background:#1a1a30; border:1px solid %7; border-radius:10px; selection-background-color:%5; selection-color:white; padding:4px; }
        QSpinBox::up-button, QSpinBox::down-button { background:#1e1e38; border:none; width:18px; border-radius:4px; }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover { background:%5; }
        QProgressBar { background:#0f0f22; border:none; border-radius:6px; height:8px; text-align:center; color:transparent; }
        QProgressBar::chunk { background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 %5, stop:1 #3b82f6); border-radius:6px; }
        QSlider::groove:horizontal { background:#1e1e38; height:4px; border-radius:2px; }
        QSlider::handle:horizontal { background:%5; width:14px; height:14px; margin:-5px 0; border-radius:7px; border:none; }
        QSlider::handle:horizontal:hover { background:#818cf8; }
        QSlider::sub-page:horizontal { background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 %5, stop:1 #3b82f6); border-radius:2px; }
        QToolTip { background:#1e1e38; border:1px solid %7; border-radius:8px; color:%2; padding:6px 12px; font-size:12px; }
    )")
    .arg(C_BG_ROOT).arg(C_TEXT1).arg(C_BG_ROOT).arg(C_BORDER).arg(C_ACCENT).arg(C_BG_CARD).arg(C_BORDER).arg(C_TEXT2));
}

void MainWindow::setupUI() {
    QWidget* root = new QWidget(this);
    setCentralWidget(root);

    QHBoxLayout* rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    setupLeftSidebar(rootLayout);

    QFrame* vLine = new QFrame(root);
    vLine->setFrameShape(QFrame::VLine);
    vLine->setFixedWidth(1);
    vLine->setStyleSheet(QString("background:%1; border:none;").arg(C_BORDER));
    rootLayout->addWidget(vLine);

    QWidget* centreCol = new QWidget(root);
    centreCol->setObjectName("centreCol");
    QVBoxLayout* centreLayout = new QVBoxLayout(centreCol);
    centreLayout->setContentsMargins(0, 0, 0, 0);
    centreLayout->setSpacing(0);

    setupTopBar(centreLayout);

    m_stackedWidget = new QStackedWidget(centreCol);
    setupBenchmarkPage();
    setupVisualizerPage();
    setupDescriptionPage();
    centreLayout->addWidget(m_stackedWidget, 1);

    rootLayout->addWidget(centreCol, 1);

    QFrame* vLine2 = new QFrame(root);
    vLine2->setFrameShape(QFrame::VLine);
    vLine2->setFixedWidth(1);
    vLine2->setStyleSheet(QString("background:%1; border:none;").arg(C_BORDER));
    rootLayout->addWidget(vLine2);

    setupRightSidebar(rootLayout);
}

void MainWindow::setupLeftSidebar(QHBoxLayout* root) {
    QFrame* sidebar = new QFrame(this);
    sidebar->setObjectName("leftSidebar");
    sidebar->setFixedWidth(68);
    sidebar->setStyleSheet(QString("QFrame#leftSidebar { background:%1; }").arg(C_BG_SIDEBAR));

    QVBoxLayout* lay = new QVBoxLayout(sidebar);
    lay->setContentsMargins(10, 16, 10, 16);
    lay->setSpacing(4);

    QLabel* logo = new QLabel("⚡", sidebar);
    logo->setAlignment(Qt::AlignCenter);
    logo->setFixedSize(48, 48);
    logo->setStyleSheet(QString("font-size:22px; color:%1; background:rgba(99,102,241,0.18); border-radius:14px; border:1px solid rgba(99,102,241,0.35);").arg(C_ACCENT));
    lay->addWidget(logo);

    lay->addSpacing(12);
    lay->addWidget(hLine(sidebar));
    lay->addSpacing(12);

    auto navBtn = [&](const QString& icon, const QString& tip, bool active) -> QToolButton* {
        QToolButton* btn = new QToolButton(sidebar);
        btn->setText(icon);
        btn->setToolTip(tip);
        btn->setFixedSize(48, 48);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setCheckable(true);
        btn->setChecked(active);
        btn->setStyleSheet(QString(R"(
            QToolButton { font-size:20px; border-radius:13px; border:none; background:%1; color:%2; }
            QToolButton:hover  { background:rgba(99,102,241,0.22); color:#a5b4fc; }
            QToolButton:checked { background:rgba(99,102,241,0.28); color:%3; }
        )").arg(active ? "rgba(99,102,241,0.28)" : "transparent").arg(active ? C_ACCENT : C_TEXT3).arg(C_ACCENT));
        lay->addWidget(btn, 0, Qt::AlignHCenter);
        return btn;
    };

    m_navBenchBtn  = navBtn("📊", "Бенчмарки / Аналитика", true);
    m_navVisualBtn = navBtn("🎬", "Интерактивный визуализатор", false);
    m_navDescBtn   = navBtn("📖", "Теоретический справочник", false);
    m_navExportBtn = navBtn("💾", "Экспорт результатов", false);

    connect(m_navBenchBtn,  &QToolButton::clicked, this, &MainWindow::switchToBenchmarkPage);
    connect(m_navVisualBtn, &QToolButton::clicked, this, &MainWindow::switchToVisualizerPage);
    connect(m_navDescBtn,   &QToolButton::clicked, this, &MainWindow::switchToDescriptionPage);
    connect(m_navExportBtn, &QToolButton::clicked, this, &MainWindow::onExportCSV);

    lay->addStretch();
    lay->addWidget(hLine(sidebar));
    lay->addSpacing(8);

    m_settBtn = new QToolButton(sidebar);
    m_settBtn->setText("⚙️");
    m_settBtn->setToolTip("Настройки");
    m_settBtn->setFixedSize(48, 48);
    m_settBtn->setCursor(Qt::PointingHandCursor);
    m_settBtn->setStyleSheet(QString("QToolButton { font-size:20px; border-radius:13px; border:none; background:transparent; color:%1; } QToolButton:hover { background:rgba(99,102,241,0.2); color:#a5b4fc; }").arg(C_TEXT3));
    connect(m_settBtn, &QToolButton::clicked, this, &MainWindow::onOpenSettings);
    lay->addWidget(m_settBtn, 0, Qt::AlignHCenter);

    root->addWidget(sidebar);
}

void MainWindow::setupTopBar(QVBoxLayout* parent) {
    QFrame* topBar = new QFrame(this);
    topBar->setObjectName("topBar");
    topBar->setFixedHeight(66);
    topBar->setStyleSheet(QString("QFrame#topBar { background:%1; border-bottom:1px solid %2; }").arg(C_BG_TOPBAR).arg(C_BORDER));

    QHBoxLayout* lay = new QHBoxLayout(topBar);
    lay->setContentsMargins(24, 0, 20, 0);
    lay->setSpacing(16);

    QToolButton* burgerBtn = new QToolButton(topBar);
    burgerBtn->setText("☰");
    burgerBtn->setFixedSize(36, 36);
    burgerBtn->setStyleSheet(QString("QToolButton { font-size:18px; background:transparent; border:none; color:%1; border-radius:8px; } QToolButton:hover { background:%2; color:%3; }").arg(C_TEXT2).arg(C_BG_CARD).arg(C_TEXT1));
    lay->addWidget(burgerBtn);

    QVBoxLayout* titleVLay = new QVBoxLayout();
    titleVLay->setSpacing(1);
    m_pageTitle = new QLabel("Панель управления", topBar);
    m_pageTitle->setStyleSheet(QString("color:%1; font-size:17px; font-weight:700; background:transparent;").arg(C_TEXT1));
    m_pageSubtitle = new QLabel("SortBench Analytics", topBar);
    m_pageSubtitle->setStyleSheet(QString("color:%1; font-size:11px; background:transparent;").arg(C_TEXT3));
    titleVLay->addWidget(m_pageTitle);
    titleVLay->addWidget(m_pageSubtitle);
    lay->addLayout(titleVLay);

    lay->addStretch();

    // Поиск
    QFrame* searchFrame = new QFrame(topBar);
    searchFrame->setFixedSize(240, 36);
    searchFrame->setStyleSheet(QString("QFrame { background:%1; border:1px solid %2; border-radius:10px; } QFrame:hover { border-color:%3; }").arg(C_BG_CARD).arg(C_BORDER).arg(C_ACCENT));

    QHBoxLayout* searchLay = new QHBoxLayout(searchFrame);
    searchLay->setContentsMargins(10, 0, 10, 0);
    searchLay->setSpacing(6);
    QLabel* sIcon = new QLabel("🔍", searchFrame);
    sIcon->setStyleSheet("font-size:14px; background:transparent;");

    m_topSearchEdit = new QLineEdit(searchFrame);
    m_topSearchEdit->setPlaceholderText("Поиск...");
    m_topSearchEdit->setStyleSheet(QString("QLineEdit { background:transparent; border:none; color:%1; font-size:12px; } QLineEdit::placeholder { color:%2; }").arg(C_TEXT1).arg(C_TEXT3));

    searchLay->addWidget(sIcon);
    searchLay->addWidget(m_topSearchEdit);
    lay->addWidget(searchFrame);

    connect(m_topSearchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (m_sidebarSearch) {
            m_sidebarSearch->setText(text);
        }
        for (DescCard* card : m_descCards) {
            bool matches = card->titleLabel->text().contains(text, Qt::CaseInsensitive) ||
                           card->descLabel->text().contains(text, Qt::CaseInsensitive);
            card->setVisible(matches);
        }
    });

    m_runBenchBtn = new QPushButton("+ Запуск", topBar);
    m_runBenchBtn->setFixedHeight(36);
    m_runBenchBtn->setCursor(Qt::PointingHandCursor);
    m_runBenchBtn->setStyleSheet(QString(R"(
        QPushButton { background:qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 %1, stop:1 %2); color:white; border:none; border-radius:10px; padding:0 18px; font-weight:700; font-size:12px; }
        QPushButton:hover { background:qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #818cf8, stop:1 %1); }
        QPushButton:disabled { background:#2a2a44; color:%3; }
    )").arg(C_ACCENT).arg(C_ACCENT2).arg(C_TEXT3));
    connect(m_runBenchBtn, &QPushButton::clicked, this, &MainWindow::onStartBenchmark);
    lay->addWidget(m_runBenchBtn);

    m_stopBenchBtn = new QPushButton("■ Стоп", topBar);
    m_stopBenchBtn->setFixedHeight(36);
    m_stopBenchBtn->setEnabled(false);
    m_stopBenchBtn->setCursor(Qt::PointingHandCursor);
    m_stopBenchBtn->setStyleSheet(QString(R"(
        QPushButton { background:#2a1c1c; color:#f87171; border:1px solid #5a2424; border-radius:10px; padding:0 14px; font-weight:600; font-size:12px; }
        QPushButton:hover { background:#3b1f1f; border-color:%1; }
        QPushButton:disabled { background:#1a1a28; color:%2; border-color:%3; }
    )").arg(C_DANGER).arg(C_TEXT3).arg(C_BORDER));
    connect(m_stopBenchBtn, &QPushButton::clicked, this, &MainWindow::onStopBenchmark);
    lay->addWidget(m_stopBenchBtn);

    m_toggleGpuBtn = new QPushButton(topBar);
    m_toggleGpuBtn->setFixedSize(38, 38);
    m_toggleGpuBtn->setCursor(Qt::PointingHandCursor);
    m_toggleGpuBtn->setToolTip("Подключение/Отключение GPU шины");

    m_toggleGpuBtn->setStyleSheet(QString(R"(
        QPushButton { background:%1; border:1px solid %2; border-radius:10px; font-size:18px; color:%3; }
        QPushButton:hover { background:%4; border-color:%5; }
    )").arg(C_BG_CARD).arg(C_BORDER).arg(C_TEXT2).arg(C_BG_SIDEBAR).arg(C_ACCENT));
    m_toggleGpuBtn->setText("🎮");
    connect(m_toggleGpuBtn, &QPushButton::clicked, this, &MainWindow::onToggleGpu);
    lay->addWidget(m_toggleGpuBtn);

    QFrame* userChip = new QFrame(topBar);
    userChip->setStyleSheet(QString("QFrame { background:%1; border:1px solid %2; border-radius:10px; }").arg(C_BG_CARD).arg(C_BORDER));
    QHBoxLayout* ucLay = new QHBoxLayout(userChip);
    ucLay->setContentsMargins(8, 4, 12, 4);
    ucLay->setSpacing(8);

    QLabel* avatar = new QLabel("🖥", userChip);
    avatar->setFixedSize(30, 30);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet(QString("font-size:16px; background:rgba(99,102,241,0.2); border-radius:8px; color:%1;").arg(C_ACCENT));

    QVBoxLayout* nameBlock = new QVBoxLayout();
    nameBlock->setSpacing(0);
    m_topCpuLabel = new QLabel("Загрузка...", userChip);
    m_topCpuLabel->setStyleSheet(QString("color:%1; font-size:11px; font-weight:700; background:transparent;").arg(C_TEXT1));
    m_topGpuLabel = new QLabel("GPU Инициализация...", userChip);
    m_topGpuLabel->setStyleSheet(QString("color:%1; font-size:10px; background:transparent;").arg(C_TEXT3));
    nameBlock->addWidget(m_topCpuLabel);
    nameBlock->addWidget(m_topGpuLabel);

    ucLay->addWidget(avatar);
    ucLay->addLayout(nameBlock);
    lay->addWidget(userChip);

    parent->addWidget(topBar);
}

void MainWindow::setupBenchmarkPage() {
    QWidget* page = new QWidget(this);
    page->setStyleSheet(QString("background:%1;").arg(C_BG_ROOT));

    QVBoxLayout* pageLay = new QVBoxLayout(page);
    pageLay->setContentsMargins(20, 16, 20, 16);
    pageLay->setSpacing(16);

    QHBoxLayout* hdrRow = new QHBoxLayout();
    QLabel* secLabel = new QLabel("Общая производительность", page);
    secLabel->setStyleSheet(QString("color:%1; font-size:16px; font-weight:700;").arg(C_TEXT1));
    hdrRow->addWidget(secLabel);
    hdrRow->addStretch();

    QPushButton* dlBtn = new QPushButton("⬇ Отчёт", page);
    dlBtn->setStyleSheet(QString(R"(
        QPushButton { background:%1; border:1px solid %2; border-radius:8px; color:%3; font-size:11px; font-weight:600; padding:4px 12px; }
        QPushButton:hover { border-color:%4; color:%5; }
    )").arg(C_BG_CARD).arg(C_BORDER).arg(C_TEXT2).arg(C_ACCENT).arg(C_TEXT1));
    connect(dlBtn, &QPushButton::clicked, this, &MainWindow::onExportCSV);
    hdrRow->addWidget(dlBtn);
    pageLay->addLayout(hdrRow);

    QFrame* chartCard = new QFrame(page);
    chartCard->setObjectName("metricCard");
    chartCard->setMinimumHeight(260);
    QVBoxLayout* chartCardLay = new QVBoxLayout(chartCard);
    chartCardLay->setContentsMargins(16, 14, 16, 14);
    chartCardLay->setSpacing(0);

    m_chartView = new QChartView(chartCard);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    chartCardLay->addWidget(m_chartView);
    pageLay->addWidget(chartCard, 2);

    QHBoxLayout* metricsRow = new QHBoxLayout();
    metricsRow->setSpacing(12);

    auto mc1 = makeMetricCard("⏱", "Тестов пройдено", "—", "Всего алгоритмов: 0", C_TEXT3, page);
    m_metricAlgCount = mc1.value;
    metricsRow->addWidget(mc1.frame, 1);

    auto mc2 = makeMetricCard("🏆", "Лучший CPU", "— ms", "std::sort / QuickSort", C_TEXT3, page);
    m_metricBestCpu = mc2.value;
    metricsRow->addWidget(mc2.frame, 1);

    auto mc3 = makeMetricCard("🚀", "Лучший GPU", "— ms", "Bitonic / Radix CUDA", C_TEXT3, page);
    m_metricBestGpu = mc3.value;
    metricsRow->addWidget(mc3.frame, 1);

    auto mc4 = makeMetricCard("⚡", "GPU Ускорение", "—×", "vs лучший CPU", C_TEXT3, page);
    m_metricSpeedup = mc4.value;
    metricsRow->addWidget(mc4.frame, 1);

    pageLay->addLayout(metricsRow);

    QFrame* tableCard = new QFrame(page);
    tableCard->setObjectName("metricCard");

    QVBoxLayout* tableCardLay = new QVBoxLayout(tableCard);
    tableCardLay->setContentsMargins(0, 0, 0, 0);
    tableCardLay->setSpacing(0);

    QFrame* tableHdr = new QFrame(tableCard);
    tableHdr->setStyleSheet(QString("background:%1; border-radius:16px 16px 0 0; border-bottom:1px solid %2;").arg(C_BG_SIDEBAR).arg(C_BORDER));
    QHBoxLayout* thLay = new QHBoxLayout(tableHdr);
    thLay->setContentsMargins(16, 10, 16, 10);
    QLabel* tbTitle = new QLabel("Таблица результатов", tableHdr);
    tbTitle->setStyleSheet(QString("color:%1; font-weight:700; font-size:13px;").arg(C_TEXT1));
    thLay->addWidget(tbTitle);
    thLay->addStretch();

    m_benchProgress = new QProgressBar(tableHdr);
    m_benchProgress->setValue(0);
    m_benchProgress->setFixedWidth(180);
    m_benchProgress->setFixedHeight(6);
    thLay->addWidget(m_benchProgress);

    tableCardLay->addWidget(tableHdr);

    m_statsTable = new QTableWidget(tableCard);
    m_statsTable->setColumnCount(9);
    m_statsTable->setHorizontalHeaderLabels({
        "Алгоритм", "Тип", "N", "Min ms", "Max ms", "Avg ms", "Median ms", "GPU Kernel", "Статус"
    });
    m_statsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_statsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_statsTable->setAlternatingRowColors(true);
    m_statsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_statsTable->setShowGrid(true);
    tableCardLay->addWidget(m_statsTable);

    pageLay->addWidget(tableCard, 1);
    m_stackedWidget->addWidget(page);
}

void MainWindow::setupVisualizerPage() {
    QWidget* page = new QWidget(this);
    page->setStyleSheet(QString("background:%1;").arg(C_BG_ROOT));

    QVBoxLayout* lay = new QVBoxLayout(page);
    lay->setContentsMargins(20, 16, 20, 12);
    lay->setSpacing(12);

    QLabel* secTitle = new QLabel("Интерактивный симулятор", page);
    secTitle->setStyleSheet(QString("color:%1; font-size:16px; font-weight:700;").arg(C_TEXT1));
    lay->addWidget(secTitle);

    QFrame* vizCard = new QFrame(page);
    vizCard->setObjectName("metricCard");
    QVBoxLayout* vcLay = new QVBoxLayout(vizCard);
    vcLay->setContentsMargins(12, 12, 12, 12);
    vcLay->setSpacing(0);

    m_visualizer = new SortingVisualizer(vizCard);
    m_visualizer->setMinimumHeight(340);
    vcLay->addWidget(m_visualizer, 1);
    lay->addWidget(vizCard, 1);

    QFrame* ctrlCard = new QFrame(page);
    ctrlCard->setObjectName("metricCard");

    QHBoxLayout* ctrlLay = new QHBoxLayout(ctrlCard);
    ctrlLay->setContentsMargins(16, 12, 16, 12);
    ctrlLay->setSpacing(12);

    auto labelStyle = [&](const QString& t) {
        QLabel* l = new QLabel(t, ctrlCard);
        l->setStyleSheet(QString("color:%1; font-size:11px; font-weight:600;").arg(C_TEXT2));
        return l;
    };

    ctrlLay->addWidget(labelStyle("Алгоритм:"));
    m_visualAlgCombo = new QComboBox(ctrlCard);
    m_visualAlgCombo->setMinimumWidth(190);
    ctrlLay->addWidget(m_visualAlgCombo);

    ctrlLay->addWidget(labelStyle("Элементов:"));
    m_visualSizeSpin = new QSpinBox(ctrlCard);
    m_visualSizeSpin->setRange(5, 1000);
    m_visualSizeSpin->setValue(60);
    m_visualSizeSpin->setFixedWidth(70);
    ctrlLay->addWidget(m_visualSizeSpin);

    m_visualGenBtn = new QPushButton("⟳ Генерировать", ctrlCard);
    m_visualGenBtn->setStyleSheet(QString(R"(
        QPushButton { background:%1; border:1px solid %2; border-radius:9px; padding:6px 14px; color:%3; font-weight:600; font-size:12px; }
        QPushButton:hover { border-color:%4; color:%5; }
    )").arg(C_BG_SIDEBAR).arg(C_BORDER).arg(C_TEXT2).arg(C_ACCENT).arg(C_TEXT1));
    connect(m_visualGenBtn, &QPushButton::clicked, this, &MainWindow::onGenerateVisualArray);
    ctrlLay->addWidget(m_visualGenBtn);

    ctrlLay->addSpacing(4);

    m_visualStartBtn = new QPushButton("▶ Старт", ctrlCard);
    m_visualStartBtn->setStyleSheet(QString(R"(
        QPushButton { background:qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 %1, stop:1 %2); border:none; border-radius:9px; padding:6px 16px; color:white; font-weight:700; font-size:12px; }
        QPushButton:hover { background:%1; }
        QPushButton:disabled { background:#252538; color:%3; }
    )").arg(C_ACCENT).arg(C_ACCENT2).arg(C_TEXT3));
    connect(m_visualStartBtn, &QPushButton::clicked, this, &MainWindow::onStartVisualSort);
    ctrlLay->addWidget(m_visualStartBtn);

    m_visualPauseBtn = new QPushButton("⏸ Пауза", ctrlCard);
    m_visualPauseBtn->setEnabled(false);
    m_visualPauseBtn->setStyleSheet(QString(R"(
        QPushButton { background:%1; border:1px solid %2; border-radius:9px; padding:6px 14px; color:%3; font-weight:600; font-size:12px; }
        QPushButton:hover { border-color:%4; }
        QPushButton:disabled { color:%5; border-color:%6; }
    )").arg(C_BG_SIDEBAR).arg(C_BORDER).arg(C_WARNING).arg(C_WARNING).arg(C_TEXT3).arg(C_BORDER));
    connect(m_visualPauseBtn, &QPushButton::clicked, this, &MainWindow::onPauseResumeVisual);
    ctrlLay->addWidget(m_visualPauseBtn);

    m_visualStopBtn = new QPushButton("⏹ Сброс", ctrlCard);
    m_visualStopBtn->setEnabled(false);
    m_visualStopBtn->setStyleSheet(QString(R"(
        QPushButton { background:#221618; border:1px solid #5a2424; border-radius:9px; padding:6px 14px; color:#f87171; font-weight:600; font-size:12px; }
        QPushButton:hover { background:#2d1c1c; border-color:%1; }
        QPushButton:disabled { background:#1a1a28; color:%2; border-color:%3; }
    )").arg(C_DANGER).arg(C_TEXT3).arg(C_BORDER));
    connect(m_visualStopBtn, &QPushButton::clicked, this, &MainWindow::onStopVisual);
    ctrlLay->addWidget(m_visualStopBtn);

    ctrlLay->addStretch();

    ctrlLay->addWidget(labelStyle("Скорость:"));
    m_visualSpeedSlider = new QSlider(Qt::Horizontal, ctrlCard);
    m_visualSpeedSlider->setRange(1, 100);
    m_visualSpeedSlider->setValue(50);
    m_visualSpeedSlider->setFixedWidth(120);
    connect(m_visualSpeedSlider, &QSlider::valueChanged, this, &MainWindow::onVisualSpeedChanged);
    ctrlLay->addWidget(m_visualSpeedSlider);
    onVisualSpeedChanged(50);

    lay->addWidget(ctrlCard);

    m_visualStatusLabel = new QLabel("Статус: Готов к запуску", page);
    m_visualStatusLabel->setStyleSheet(QString("color:%1; font-size:11px; padding:2px 4px;").arg(C_TEXT3));
    lay->addWidget(m_visualStatusLabel);

    m_stackedWidget->addWidget(page);
}

void MainWindow::setupDescriptionPage() {
    QWidget* page = new QWidget(this);
    page->setStyleSheet(QString("background:%1;").arg(C_BG_ROOT));

    QVBoxLayout* mainLay = new QVBoxLayout(page);
    mainLay->setContentsMargins(20, 16, 20, 16);
    mainLay->setSpacing(16);

    QLabel* secTitle = new QLabel("Теоретический справочник алгоритмов", page);
    secTitle->setStyleSheet(QString("color:%1; font-size:16px; font-weight:700;").arg(C_TEXT1));
    mainLay->addWidget(secTitle);

    QScrollArea* scrollArea = new QScrollArea(page);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background:transparent; border:none;");

    QWidget* container = new QWidget(scrollArea);
    container->setStyleSheet("background:transparent;");
    QVBoxLayout* containerLay = new QVBoxLayout(container);
    containerLay->setContentsMargins(0, 0, 0, 0);
    containerLay->setSpacing(12);

    const QList<AlgDescData> descData = {
        {"std::sort (STL IntroSort)", "O(n log n)", "O(n log n)", "O(n log n)", "O(log n)",
         "Высокооптимизированный гибридный алгоритм сортировки, сочетающий быструю сортировку (QuickSort), пирамидальную сортировку (HeapSort) и сортировку вставками (InsertionSort). Автоматически переключается между ними для обеспечения максимальной производительности во всех случаях."},
        {"QuickSort (Быстрая)", "O(n log n)", "O(n log n)", "O(n^2)", "O(log n)",
         "Разделяй и властвуй: выбирается опорный элемент, относительно которого массив разбивается на меньшие и большие элементы, после чего рекурсивно сортируется. Очень быстра на практике, но деградирует на плохих опорных элементах."},
        {"MergeSort (Слиянием)", "O(n log n)", "O(n log n)", "O(n log n)", "O(n)",
         "Стабильный алгоритм сортировки. Делит массив пополам, рекурсивно сортирует каждую половину и затем сливает отсортированные части в один массив. Требует выделения дополнительной памяти, пропорциональной размеру массива."},
        {"HeapSort (Пирамидальная)", "O(n log n)", "O(n log n)", "O(n log n)", "O(1)",
         "Сортировка с использованием структуры данных двоичной кучи. Сначала строится куча, затем максимальные элементы последовательно извлекаются в конец массива. Не требует дополнительной памяти и гарантирует стабильное время выполнения."},
        {"TimSort (Гибридная)", "O(n)", "O(n log n)", "O(n log n)", "O(n)",
         "Гибридный стабильный алгоритм, разработанный Тимом Петерсом. Ищет в массиве упорядоченные подпоследовательности (раны) и сливает их, используя сортировку вставками для мелких участков. Стандарт в Python и Java."},
        {"BubbleSort (Пузырёк)", "O(n)", "O(n^2)", "O(n^2)", "O(1)",
         "Простейший учебный алгоритм. Проходит по массиву множество раз, сравнивая соседние элементы и меняя их местами, если они расположены неверно. Тяжелые элементы 'опускаются' в конец, словно пузырьки."},
        {"SelectionSort (Выбором)", "O(n^2)", "O(n^2)", "O(n^2)", "O(1)",
         "Ищет наименьший элемент в неотсортированной части массива и меняет его местами с первым неотсортированным элементом. Обладает постоянным числом обменов, но не оптимален для больших объемов данных."},
        {"InsertionSort (Вставками)", "O(n)", "O(n^2)", "O(n^2)", "O(1)",
         "Элементы массива просматриваются поочередно, и каждый новый элемент вставляется в подходящее место среди ранее отсортированных. Чрезвычайно эффективен на почти отсортированных данных и массивах малого размера."},
        {"ShellSort (Шелла)", "O(n log n)", "O(n^1.3)", "O(n^2)", "O(1)",
         "Модификация сортировки вставками, которая сравнивает элементы, стоящие на определенном расстоянии (шаге). Шаг постепенно уменьшается до единицы, что позволяет быстро переместить далеко стоящие элементы на свои места."},
        {"CocktailSort (Шейкерная)", "O(n)", "O(n^2)", "O(n^2)", "O(1)",
         "Двунаправленная сортировка пузырьком. Совершает проходы по массиву поочередно слева направо и справа налево, ускоряя подъем 'легких' и опускание 'тяжелых' элементов в крайние позиции."},
        {"GnomeSort (Гномья)", "O(n)", "O(n^2)", "O(n^2)", "O(1)",
         "Похожа на сортировку вставками, но нахождение правильного места элемента происходит путем последовательных обменов назад до тех пор, пока элемент не окажется больше предыдущего."},
        {"CombSort (Расчёской)", "O(n log n)", "O(n^2)", "O(n^2)", "O(1)",
         "Усовершенствование пузырьковой сортировки. Вводит шаг сравнения, превосходящий единицу, который постепенно сокращается с коэффициентом усадки ~1.3, эффективно устраняя аномалии в начале и конце."},
        {"RadixSortLSD (Поразрядная)", "O(nk)", "O(nk)", "O(nk)", "O(n+k)",
         "Распределительный алгоритм без сравнения ключей. Группирует элементы по отдельным разрядам (байтам), начиная от младшего (LSD) к старшему. Эффективен для чисел и фиксированных битовых структур."},
        {"CountingSort (Подсчётом)", "O(n+k)", "O(n+k)", "O(n+k)", "O(k)",
         "Линейный алгоритм сортировки. Подсчитывает количество вхождений каждого уникального элемента в массиве и использует эти данные для вычисления позиций элементов в финальном массиве."},
        {"BucketSort (Блочная)", "O(n+k)", "O(n+k)", "O(n^2)", "O(n)",
         "Распределяет элементы входного массива по фиксированному числу блоков (корзин), затем сортирует каждую корзину отдельно (например, сортировкой вставками) и объединяет их в финальный список."},
        {"PancakeSort (Блинная)", "O(n)", "O(n)", "O(n)", "O(1)",
         "В этом алгоритме единственная разрешенная операция — разворот элементов префикса массива. Напоминает переворачивание стопки блинов лопаткой для перемещения максимальных блинов вниз."},
        {"BogoSort (Случайная)", "O(n)", "O(n * n!)", "O(inf)", "O(1)",
         "Один из самых неэффективных алгоритмов. Случайно перемешивает элементы массива и проверяет, не оказались ли они отсортированными. Имеет неограниченное худшее время выполнения."},
        {"StoogeSort (Придурковатая)", "O(n^2.71)", "O(n^2.71)", "O(n^2.71)", "O(n)",
         "Медленный рекурсивный алгоритм. Сравнивает первый и последний элементы, затем рекурсивно сортирует первые 2/3 массива, затем последние 2/3, и затем снова первые 2/3."},
        {"OddEvenSort (Чет-Нечет)", "O(n)", "O(n^2)", "O(n^2)", "O(1)",
         "Параллельная версия пузырьковой сортировки. Разбивает работу на две фазы: четную (сравнение пар с нечетным первым индексом) и нечетную (с четным первым индексом)."},
        {"CycleSort (Циклическая)", "O(n^2)", "O(n^2)", "O(n^2)", "O(1)",
         "Теоретически оптимальный алгоритм с точки зрения минимизации числа записей в память. Каждый элемент перемещается на свое конечное место ровно один раз через циклическое вычисление позиций."}
    };

    for (const auto& item : descData) {
        DescCard* card = new DescCard(item, container);
        containerLay->addWidget(card);
        m_descCards.push_back(card);
    }

    containerLay->addStretch();
    scrollArea->setWidget(container);
    mainLay->addWidget(scrollArea, 1);

    m_stackedWidget->addWidget(page);
}

void MainWindow::setupRightSidebar(QHBoxLayout* root) {
    QWidget* sidebar = new QWidget(this);
    sidebar->setObjectName("rightSidebar");
    sidebar->setFixedWidth(304);
    sidebar->setStyleSheet(QString("background:%1;").arg(C_BG_SIDEBAR));

    QVBoxLayout* lay = new QVBoxLayout(sidebar);
    lay->setContentsMargins(14, 16, 14, 16);
    lay->setSpacing(12);

    auto sectionHeader = [&](const QString& title, const QString& iconEm) -> QLabel* {
        QLabel* l = new QLabel(iconEm + "  " + title, sidebar);
        l->setStyleSheet(QString("color:%1; font-size:12px; font-weight:700;").arg(C_TEXT1));
        return l;
    };

    lay->addWidget(sectionHeader("Алгоритмы сортировки", "📋"));

    m_sidebarSearch = new QLineEdit(sidebar);
    m_sidebarSearch->setPlaceholderText("Фильтр сортировок...");
    m_sidebarSearch->setStyleSheet(QString(
        "QLineEdit { background:#141428; border:1px solid %1; border-radius:8px; padding:5px 10px; color:%2; }"
        "QLineEdit:focus { border-color:%3; }"
    ).arg(C_BORDER).arg(C_TEXT1).arg(C_ACCENT));
    lay->addWidget(m_sidebarSearch);

    QScrollArea* tileScroll = new QScrollArea(sidebar);
    tileScroll->setWidgetResizable(true);
    tileScroll->setFrameShape(QFrame::NoFrame);
    tileScroll->setStyleSheet("background:transparent; border:none;");
    tileScroll->setMinimumHeight(160);
    tileScroll->setMaximumHeight(260);

    QWidget* tileContainer = new QWidget(tileScroll);
    tileContainer->setStyleSheet("background:transparent;");
    QVBoxLayout* tileContainerLay = new QVBoxLayout(tileContainer);
    tileContainerLay->setContentsMargins(0, 0, 0, 0);
    tileContainerLay->setSpacing(6);

    for (const auto& item : sidebarAlgs) {
        AlgTile* tile = new AlgTile(item.name, item.desc, item.id, item.isGPU, tileContainer);
        tileContainerLay->addWidget(tile);
        m_tiles.push_back(tile);
    }
    tileContainerLay->addStretch();
    tileScroll->setWidget(tileContainer);
    lay->addWidget(tileScroll);

    connect(m_sidebarSearch, &QLineEdit::textChanged, this, [this](const QString& text) {
        for (AlgTile* tile : m_tiles) {
            bool matches = tile->titleLabel->text().contains(text, Qt::CaseInsensitive) ||
                           tile->descLabel->text().contains(text, Qt::CaseInsensitive);
            tile->setVisible(matches);
        }
    });

    lay->addWidget(hLine(sidebar));

    lay->addWidget(sectionHeader("Конфигурация", "⚙️"));

    QGridLayout* cfgGrid = new QGridLayout();
    cfgGrid->setSpacing(8);
    cfgGrid->setColumnStretch(1, 1);

    auto cfgLabel = [&](const QString& t) {
        QLabel* l = new QLabel(t, sidebar);
        l->setStyleSheet(QString("color:%1; font-size:11px;").arg(C_TEXT2));
        return l;
    };

    cfgGrid->addWidget(cfgLabel("Размер N:"), 0, 0);
    m_arraySizeSpin = new QSpinBox(sidebar);
    m_arraySizeSpin->setRange(10, 10'000'000);
    m_arraySizeSpin->setValue(10000);
    m_arraySizeSpin->setSingleStep(1000);
    cfgGrid->addWidget(m_arraySizeSpin, 0, 1);

    // Чекбокс режима масштабирования (Sweep)
    m_sweepModeCheck = new QCheckBox("Режим Sweep (График Сложности)", sidebar);
    m_sweepModeCheck->setStyleSheet(QString("QCheckBox { color:%1; font-size:11px; font-weight:600; }").arg(C_TEXT2));
    cfgGrid->addWidget(m_sweepModeCheck, 1, 0, 1, 2);

    connect(m_sweepModeCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_arraySizeSpin->setDisabled(checked);
    });

    cfgGrid->addWidget(cfgLabel("Распределение:"), 2, 0);
    m_distCombo = new QComboBox(sidebar);
    m_distCombo->addItem("Равномерное",      static_cast<int>(Benchmark::Distribution::Uniform));
    m_distCombo->addItem("Нормальное (Гаусс)", static_cast<int>(Benchmark::Distribution::Normal));
    m_distCombo->addItem("Обратное",         static_cast<int>(Benchmark::Distribution::ReverseSorted));
    m_distCombo->addItem("Почти упорядоч.",  static_cast<int>(Benchmark::Distribution::AlmostSorted));
    m_distCombo->addItem("Все одинаковы",    static_cast<int>(Benchmark::Distribution::AllEqual));
    m_distCombo->addItem("Синусоидальное",     static_cast<int>(Benchmark::Distribution::Sinusoidal));
    m_distCombo->addItem("Ступенчатое (Блоки)", static_cast<int>(Benchmark::Distribution::Stepwise));
    cfgGrid->addWidget(m_distCombo, 2, 1);

    cfgGrid->addWidget(cfgLabel("Тип данных:"), 3, 0);
    m_dataTypeCombo = new QComboBox(sidebar);
    m_dataTypeCombo->addItem("float (32-bit)");
    m_dataTypeCombo->addItem("double (64-bit)");
    cfgGrid->addWidget(m_dataTypeCombo, 3, 1);

    cfgGrid->addWidget(cfgLabel("Повторов:"), 4, 0);
    m_runsSpin = new QSpinBox(sidebar);
    m_runsSpin->setRange(1, 20);
    
    QSettings s;
    m_runsSpin->setValue(s.value("benchmark/default_runs", 5).toInt());
    cfgGrid->addWidget(m_runsSpin, 4, 1);

    lay->addLayout(cfgGrid);
    lay->addWidget(hLine(sidebar));

    // Информационный блок GPU с телеметрией VRAM
    QFrame* gpuChip = new QFrame(sidebar);
    gpuChip->setStyleSheet("QFrame { background:#0f1f18; border:1px solid #1a4030; border-radius:10px; }");
    QVBoxLayout* gpuVLay = new QVBoxLayout(gpuChip);
    gpuVLay->setContentsMargins(10, 8, 10, 8);
    gpuVLay->setSpacing(6);

    QHBoxLayout* gpuTopRow = new QHBoxLayout();
    m_gpuLedIndicator = new QWidget(gpuChip);
    m_gpuLedIndicator->setFixedSize(8, 8);
    m_gpuLedIndicator->setStyleSheet(QString("background:%1; border-radius:4px;").arg(C_SUCCESS));

    QLabel* gpuLbl = new QLabel("GPU CUDA:", gpuChip);
    gpuLbl->setStyleSheet(QString("color:%1; font-size:11px; font-weight:600;").arg(C_TEXT2));
    m_sidebarGpuLabel = new QLabel("Поиск GPU...", gpuChip);
    m_sidebarGpuLabel->setStyleSheet(QString("color:%1; font-size:10px;").arg(C_SUCCESS));

    gpuTopRow->addWidget(m_gpuLedIndicator);
    gpuTopRow->addWidget(gpuLbl);
    gpuTopRow->addWidget(m_sidebarGpuLabel, 1);
    gpuVLay->addLayout(gpuTopRow);

    m_gpuVramBar = new QProgressBar(gpuChip);
    m_gpuVramBar->setFixedHeight(6);
    m_gpuVramBar->setStyleSheet(R"(
        QProgressBar { background: #08100c; border: none; border-radius: 3px; text-align: center; color: transparent; }
        QProgressBar::chunk { background: #10b981; border-radius: 3px; }
    )");
    m_gpuVramLabel = new QLabel("VRAM: Инициализация...", gpuChip);
    m_gpuVramLabel->setStyleSheet(QString("color: %1; font-size: 10px;").arg(C_TEXT3));

    gpuVLay->addWidget(m_gpuVramBar);
    gpuVLay->addWidget(m_gpuVramLabel);
    lay->addWidget(gpuChip);

    lay->addWidget(sectionHeader("Экспорт", "📤"));

    QHBoxLayout* exportRow = new QHBoxLayout();
    exportRow->setSpacing(8);

    m_exportCsvBtn = new QPushButton("CSV", sidebar);
    m_exportCsvBtn->setStyleSheet(QString(R"(
        QPushButton { background:%1; border:1px solid %2; border-radius:8px; padding:6px 0; color:%3; font-weight:600; font-size:11px; }
        QPushButton:hover { border-color:%4; color:%5; }
    )").arg(C_BG_CARD).arg(C_BORDER).arg(C_TEXT2).arg(C_ACCENT).arg(C_TEXT1));
    connect(m_exportCsvBtn, &QPushButton::clicked, this, &MainWindow::onExportCSV);
    exportRow->addWidget(m_exportCsvBtn, 1);

    m_exportPngBtn = new QPushButton("PNG", sidebar);
    m_exportPngBtn->setStyleSheet(m_exportCsvBtn->styleSheet());
    connect(m_exportPngBtn, &QPushButton::clicked, this, &MainWindow::onExportPNG);
    exportRow->addWidget(m_exportPngBtn, 1);

    lay->addLayout(exportRow);
    lay->addStretch();

    lay->addWidget(hLine(sidebar));
    lay->addWidget(sectionHeader("Активность системы", "📡"));

    QScrollArea* actScroll = new QScrollArea(sidebar);
    actScroll->setWidgetResizable(true);
    actScroll->setFrameShape(QFrame::NoFrame);
    actScroll->setStyleSheet("background:transparent; border:none;");
    actScroll->setMinimumHeight(120);

    QWidget* actContent = new QWidget(actScroll);
    actContent->setStyleSheet("background:transparent;");
    QVBoxLayout* actLay = new QVBoxLayout(actContent);
    actLay->setContentsMargins(0, 0, 0, 0);
    actLay->setSpacing(8);

    auto actItem = [&](const QString& icon, const QString& text, const QString& time) {
        QFrame* row = new QFrame(actContent);
        row->setStyleSheet("background:transparent;");
        QHBoxLayout* rl = new QHBoxLayout(row);
        rl->setContentsMargins(0, 0, 0, 0);
        rl->setSpacing(8);
        QLabel* ico = new QLabel(icon, actContent);
        ico->setFixedSize(28, 28);
        ico->setAlignment(Qt::AlignCenter);
        ico->setStyleSheet("font-size:14px; background:rgba(99,102,241,0.15); border-radius:8px;");
        QVBoxLayout* tl = new QVBoxLayout();
        tl->setSpacing(0);
        QLabel* tl1 = new QLabel(text, actContent);
        tl1->setStyleSheet(QString("color:%1; font-size:11px; background:transparent;").arg(C_TEXT1));
        tl1->setWordWrap(true);
        QLabel* tl2 = new QLabel(time, actContent);
        tl2->setStyleSheet(QString("color:%1; font-size:10px; background:transparent;").arg(C_TEXT3));
        tl->addWidget(tl1);
        tl->addWidget(tl2);
        rl->addWidget(ico);
        rl->addLayout(tl, 1);
        actLay->addWidget(row);
    };

    actItem("⚡", "SortBench готов к работе", "сейчас");
    actItem("🎮", "Мониторинг шины PCIe активен", "сейчас");
    actItem("📊", "Пределы массива установлены", "сейчас");

    actScroll->setWidget(actContent);
    lay->addWidget(actScroll);

    root->addWidget(sidebar);
}

void MainWindow::setupCharts() {
    m_chart = new QChart();
    m_chart->setTitle("");
    m_chart->setBackgroundBrush(QBrush(Qt::transparent));
    m_chart->setPlotAreaBackgroundBrush(QBrush(Qt::transparent));
    m_chart->setPlotAreaBackgroundVisible(true);
    m_chart->setMargins(QMargins(0, 0, 0, 0));
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignTop);
    m_chart->legend()->setLabelColor(QColor(C_TEXT2));
    m_chart->legend()->setBackgroundVisible(false);
    m_chart->setAnimationOptions(QChart::SeriesAnimations);
    m_chart->setAnimationDuration(500);

    m_barSeries = new QBarSeries(this);
    m_chart->addSeries(m_barSeries);

    m_axisX = new QBarCategoryAxis(this);
    m_axisX->setLabelsBrush(QBrush(QColor(C_TEXT2)));
    m_axisX->setLinePen(QPen(QColor(C_BORDER)));
    m_axisX->setGridLinePen(QPen(Qt::transparent));
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_barSeries->attachAxis(m_axisX);

    m_axisY = new QValueAxis(this);
    m_axisY->setLabelsBrush(QBrush(QColor(C_TEXT2)));
    m_axisY->setLabelFormat("%.2f");
    m_axisY->setTitleText("ms");
    m_axisY->setTitleBrush(QBrush(QColor(C_TEXT3)));
    m_axisY->setLinePen(QPen(Qt::transparent));
    m_axisY->setGridLinePen(QPen(QColor(C_BORDER), 1, Qt::DashLine));
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_barSeries->attachAxis(m_axisY);

    m_chartView->setChart(m_chart);
}

void MainWindow::loadAvailableAlgorithms() {
    for (AlgTile* tile : m_tiles) {
        if (tile->algId == "CPU_std::sort" || tile->algId == "CPU_QuickSort" ||
            tile->algId == "GPU_Bitonic" || tile->algId == "GPU_Radix") {
            tile->checkbox->setChecked(true);
        }
    }

    m_visualAlgCombo->clear();
    const QList<QPair<QString,QString>> vItems = {
        {"QuickSort",          "CPU_QuickSort"},
        {"MergeSort",          "CPU_MergeSort"},
        {"HeapSort",           "CPU_HeapSort"},
        {"TimSort",            "CPU_TimSort"},
        {"BubbleSort",         "CPU_BubbleSort"},
        {"SelectionSort",      "CPU_SelectionSort"},
        {"InsertionSort",      "CPU_InsertionSort"},
        {"ShellSort",          "CPU_ShellSort"},
        {"CocktailSort",       "CPU_CocktailSort"},
        {"GnomeSort",          "CPU_GnomeSort"},
        {"CombSort",           "CPU_CombSort"},
        {"RadixSortLSD",       "CPU_RadixSortLSD"},
        {"CountingSort",       "CPU_CountingSort"},
        {"BucketSort",         "CPU_BucketSort"},
        {"PancakeSort",        "CPU_PancakeSort"},
        {"BogoSort",           "CPU_BogoSort"},
        {"StoogeSort",         "CPU_StoogeSort"},
        {"OddEvenSort",        "CPU_OddEvenSort"},
        {"CycleSort",          "CPU_CycleSort"},
        {"std::sort (STL)",    "CPU_stdSort"},
    };
    for (const auto& p : vItems) {
        m_visualAlgCombo->addItem(p.first, p.second);
    }
}

void MainWindow::switchToBenchmarkPage() {
    m_stackedWidget->setCurrentIndex(0);
    m_pageTitle->setText("Панель управления");
    m_pageSubtitle->setText("Сравнительный бенчмарк алгоритмов");
    m_navBenchBtn->setChecked(true);
    m_navVisualBtn->setChecked(false);
    m_navDescBtn->setChecked(false);
}

void MainWindow::switchToVisualizerPage() {
    m_stackedWidget->setCurrentIndex(1);
    m_pageTitle->setText("Визуализатор");
    m_pageSubtitle->setText("Интерактивный симулятор работы алгоритмов");
    m_navBenchBtn->setChecked(false);
    m_navVisualBtn->setChecked(true);
    m_navDescBtn->setChecked(false);
}

void MainWindow::switchToDescriptionPage() {
    m_stackedWidget->setCurrentIndex(2);
    m_pageTitle->setText("Справочник");
    m_pageSubtitle->setText("Теоретическое описание алгоритмов сортировки");
    m_navBenchBtn->setChecked(false);
    m_navVisualBtn->setChecked(false);
    m_navDescBtn->setChecked(true);
}

std::vector<QString> MainWindow::getSelectedAlgorithms() {
    std::vector<QString> list;
    for (AlgTile* tile : m_tiles) {
        if (tile->checkbox->isChecked()) {
            list.push_back(tile->algId);
        }
    }
    return list;
}

void MainWindow::onStartBenchmark() {
    m_accumulatedResults.clear();
    m_statsTable->setRowCount(0);

    std::vector<QString> selected = getSelectedAlgorithms();
    if (selected.empty()) {
        QMessageBox::warning(this, "Выбор алгоритма", "Выберите хотя бы одну сортировку-плитку в боковой панели справа.");
        return;
    }

    m_runBenchBtn->setEnabled(false);
    m_stopBenchBtn->setEnabled(true);
    m_benchProgress->setValue(0);

    Benchmark::Config cfg;
    cfg.arraySize        = m_arraySizeSpin->value();
    cfg.runsCount        = m_runsSpin->value();
    cfg.dist             = static_cast<Benchmark::Distribution>(m_distCombo->currentData().toInt());
    cfg.isDoublePrecision = m_dataTypeCombo->currentIndex() > 0;
    cfg.gpuConnected     = m_gpuConnected;

    cfg.isSweepMode      = m_sweepModeCheck->isChecked();
    if (cfg.isSweepMode) {
        QSettings s;
        QString sweepStr = s.value("benchmark/sweep_sizes", "100,500,1000,5000,10000,25000").toString();
        cfg.sweepSizes.clear();
        for (const QString& part : sweepStr.split(",")) {
            bool ok;
            int size = part.trimmed().toInt(&ok);
            if (ok && size > 0) {
                cfg.sweepSizes.push_back(size);
            }
        }
        if (cfg.sweepSizes.empty()) {
            cfg.sweepSizes = {100, 500, 1000, 5000, 10000, 25000};
        }
    }

    for (const auto& algId : selected) {
        cfg.selectedAlgorithms.push_back(algId);
    }

    m_benchRunner->setConfig(cfg);
    m_benchRunner->start();

    switchToBenchmarkPage();
}

void MainWindow::onStopBenchmark() {
    if (m_benchRunner && m_benchRunner->isRunning()) {
        m_benchRunner->requestStop();
    }
}

void MainWindow::onBenchmarkProgress(int pct) {
    m_benchProgress->setValue(pct);
}

void MainWindow::onAlgorithmCompleted(const Benchmark::StatResults& s) {
    m_accumulatedResults.push_back(s);

    int row = m_statsTable->rowCount();
    m_statsTable->insertRow(row);

    auto cell = [&](const QString& txt, const QColor& fg = QColor(C_TEXT1)) {
        QTableWidgetItem* it = new QTableWidgetItem(txt);
        it->setForeground(fg);
        return it;
    };

    m_statsTable->setItem(row, 0, cell(s.algorithmName));
    m_statsTable->setItem(row, 1, cell(s.isGPU ? "🚀 GPU" : "💻 CPU", s.isGPU ? QColor(C_ACCENT) : QColor(C_BLUE)));
    m_statsTable->setItem(row, 2, cell(QString::number(s.arraySize)));

    if (s.success) {
        m_statsTable->setItem(row, 3, cell(QString::number(s.minTimeMs, 'f', 4)));
        m_statsTable->setItem(row, 4, cell(QString::number(s.maxTimeMs, 'f', 4)));
        m_statsTable->setItem(row, 5, cell(QString::number(s.avgTimeMs, 'f', 4)));
        m_statsTable->setItem(row, 6, cell(QString::number(s.medianTimeMs, 'f', 4)));
        m_statsTable->setItem(row, 7, cell(s.isGPU ? QString::number(s.avgKernelTimeMs, 'f', 3) + " ms" : "—"));
        m_statsTable->setItem(row, 8, cell("✅ OK", QColor(C_SUCCESS)));
    } else {
        for (int c = 3; c <= 7; ++c) {
            m_statsTable->setItem(row, c, cell("—", QColor(C_TEXT3)));
        }
        m_statsTable->setItem(row, 8, cell("❌ " + s.errorMsg.left(40), QColor(C_DANGER)));
    }

    updateCharts();
    updateMetricCards();
}

void MainWindow::onBenchmarkFinished() {
    m_runBenchBtn->setEnabled(true);
    m_stopBenchBtn->setEnabled(false);
    m_benchProgress->setValue(100);
}

void MainWindow::updateCharts() {
    m_chart->removeAllSeries();

    if (!m_sweepModeCheck->isChecked()) {
        // ==================== РЕЖИМ СТОЛБЧАТОЙ ДИАГРАММЫ (ОДИН N) ====================
        m_barSeries = new QBarSeries(this);

        QBarSet* setAvg = new QBarSet("Среднее (ms)");
        setAvg->setColor(QColor(C_ACCENT));
        setAvg->setBorderColor(Qt::transparent);

        QBarSet* setMedian = new QBarSet("Медиана (ms)");
        setMedian->setColor(QColor(C_BLUE));
        setMedian->setBorderColor(Qt::transparent);

        QStringList cats;
        double maxVal = 0.001;

        for (const auto& r : m_accumulatedResults) {
            if (!r.success) continue;
            QString lbl = r.algorithmName;
            lbl.remove("CPU_").remove("GPU_").replace("Sort","").replace("sort","");
            cats << lbl;
            *setAvg    << r.avgTimeMs;
            *setMedian << r.medianTimeMs;
            maxVal = std::max({maxVal, r.avgTimeMs, r.medianTimeMs});
        }

        m_barSeries->append(setAvg);
        m_barSeries->append(setMedian);
        m_chart->addSeries(m_barSeries);

        m_chart->removeAxis(m_axisX);
        m_chart->removeAxis(m_axisY);

        QBarCategoryAxis* barAxisX = new QBarCategoryAxis(this);
        barAxisX->setLabelsBrush(QBrush(QColor(C_TEXT2)));
        barAxisX->setLinePen(QPen(QColor(C_BORDER)));
        barAxisX->setGridLinePen(QPen(Qt::transparent));
        m_chart->addAxis(barAxisX, Qt::AlignBottom);
        m_barSeries->attachAxis(barAxisX);

        m_axisY = new QValueAxis(this);
        m_axisY->setLabelsBrush(QBrush(QColor(C_TEXT2)));
        m_axisY->setLabelFormat("%.2f");
        m_axisY->setTitleText("Время (мс)");
        m_axisY->setTitleBrush(QBrush(QColor(C_TEXT3)));
        m_axisY->setLinePen(QPen(Qt::transparent));
        m_axisY->setGridLinePen(QPen(QColor(C_BORDER), 1, Qt::DashLine));
        m_chart->addAxis(m_axisY, Qt::AlignLeft);
        m_barSeries->attachAxis(m_axisY);

        barAxisX->append(cats);
        m_axisY->setRange(0.0, maxVal * 1.20);
        
        m_axisX = barAxisX;
    } 
    else {
        // ==================== РЕЖИМ ГРАФИКА СЛОЖНОСТИ (НЕСКОЛЬКО N) ====================
        std::map<QString, std::vector<std::pair<double, double>>> algLines;
        double maxVal = 0.001;
        double maxSize = 100.0;

        for (const auto& r : m_accumulatedResults) {
            if (!r.success) continue;
            algLines[r.algorithmName].push_back({static_cast<double>(r.arraySize), r.avgTimeMs});
            maxVal = std::max(maxVal, r.avgTimeMs);
            maxSize = std::max(maxSize, static_cast<double>(r.arraySize));
        }

        m_chart->removeAxis(m_axisX);
        m_chart->removeAxis(m_axisY);

        QValueAxis* axisValX = new QValueAxis(this);
        axisValX->setLabelsBrush(QBrush(QColor(C_TEXT2)));
        axisValX->setTitleText("Размер входного массива (N)");
        axisValX->setTitleBrush(QBrush(QColor(C_TEXT3)));
        axisValX->setLinePen(QPen(QColor(C_BORDER)));
        axisValX->setGridLinePen(QPen(QColor(C_BORDER), 1, Qt::DashLine));
        m_chart->addAxis(axisValX, Qt::AlignBottom);

        m_axisY = new QValueAxis(this);
        m_axisY->setLabelsBrush(QBrush(QColor(C_TEXT2)));
        m_axisY->setLabelFormat("%.2f");
        m_axisY->setTitleText("Среднее время выполнения (мс)");
        m_axisY->setTitleBrush(QBrush(QColor(C_TEXT3)));
        m_axisY->setLinePen(QPen(Qt::transparent)); // Исправлено: Обернуто в QPen
        m_axisY->setGridLinePen(QPen(QColor(C_BORDER), 1, Qt::DashLine));
        m_chart->addAxis(m_axisY, Qt::AlignLeft);

        QList<QColor> colors = {
            QColor("#6366f1"), QColor("#3b82f6"), QColor("#10b981"), QColor("#f59e0b"),
            QColor("#ef4444"), QColor("#84cc16"), QColor("#06b6d4"), QColor("#a855f7")
        };
        int colorIdx = 0;

        for (auto& [algName, points] : algLines) {
            std::sort(points.begin(), points.end(), [](auto& a, auto& b) { return a.first < b.first; });

            QLineSeries* series = new QLineSeries(this);
            series->setName(algName);
            
            QColor col = colors[colorIdx % colors.size()];
            series->setColor(col);
            series->setPointsVisible(true);
            colorIdx++;

            for (const auto& p : points) {
                series->append(p.first, p.second);
            }

            m_chart->addSeries(series);
            series->attachAxis(axisValX);
            series->attachAxis(m_axisY);
        }

        axisValX->setRange(0.0, maxSize);
        m_axisY->setRange(0.0, maxVal * 1.20);
        
        m_axisX = axisValX;
    }
}

void MainWindow::updateMetricCards() {
    if (m_accumulatedResults.empty()) return;

    int total = static_cast<int>(m_accumulatedResults.size());
    m_metricAlgCount->setText(QString::number(total));

    double bestCpu = 1e18, bestGpu = 1e18;
    for (const auto& r : m_accumulatedResults) {
        if (!r.success) continue;
        if (!r.isGPU && r.avgTimeMs < bestCpu) bestCpu = r.avgTimeMs;
        if (r.isGPU  && r.avgTimeMs < bestGpu) bestGpu = r.avgTimeMs;
    }

    if (bestCpu < 1e18) {
        m_metricBestCpu->setText(QString::number(bestCpu, 'f', 3) + " ms");
    }
    if (bestGpu < 1e18) {
        m_metricBestGpu->setText(QString::number(bestGpu, 'f', 3) + " ms");
    }
    if (bestCpu < 1e18 && bestGpu < 1e18 && bestGpu > 0) {
        m_metricSpeedup->setText(QString::number(bestCpu / bestGpu, 'f', 1) + "×");
    }
}

void MainWindow::onGenerateVisualArray() {
    onStopVisual();
    int size = m_visualSizeSpin->value();
    m_visualData.resize(size);
    for (int i = 0; i < size; ++i) {
        m_visualData[i] = QRandomGenerator::global()->generateDouble() * 500.0 + 10.0;
    }
    m_visualizer->updateData(m_visualData);
    m_visualStatusLabel->setText("Статус: Массив инициализирован.");
}

void MainWindow::onVisualSpeedChanged(int v) {
    m_visualDelayMs = static_cast<int>(501 - v * 5.0);
    if (m_visualDelayMs < 1) m_visualDelayMs = 1;
}

void MainWindow::onStartVisualSort() {
    if (m_visualSortThread && m_visualSortThread->isRunning()) {
        if (m_isVisualPaused) onPauseResumeVisual();
        return;
    }
    m_stopVisualRequested = false;
    m_isVisualPaused = false;

    m_visualGenBtn->setEnabled(false);
    m_visualStartBtn->setEnabled(false);
    m_visualPauseBtn->setEnabled(true);
    m_visualStopBtn->setEnabled(true);
    m_visualSizeSpin->setEnabled(false);
    m_visualAlgCombo->setEnabled(false);
    m_visualStatusLabel->setText("Статус: Сортировка выполняется...");

    const QString algKey = m_visualAlgCombo->currentData().toString();

    m_visualSortThread = QThread::create([this, algKey]() {
        CPU::SortContext ctx;
        ctx.stopRequested = &m_stopVisualRequested;
        ctx.stepCallback  = [this](const std::vector<double>& arr, int a1, int a2, int piv) {
            QMetaObject::invokeMethod(this, [this, arr, a1, a2, piv]() {
                m_visualizer->updateData(arr, a1, a2, piv);
            }, Qt::QueuedConnection);
            while (m_isVisualPaused && !m_stopVisualRequested) QThread::msleep(30);
            QThread::msleep(m_visualDelayMs);
        };

        std::vector<double> arr = m_visualData;

        if      (algKey=="CPU_stdSort")       CPU::stdSort(arr, ctx);
        else if (algKey=="CPU_QuickSort")     CPU::quickSort(arr, ctx);
        else if (algKey=="CPU_MergeSort")     CPU::mergeSort(arr, ctx);
        else if (algKey=="CPU_HeapSort")      CPU::heapSort(arr, ctx);
        else if (algKey=="CPU_TimSort")       CPU::timSort(arr, ctx);
        else if (algKey=="CPU_BubbleSort")    CPU::bubbleSort(arr, ctx);
        else if (algKey=="CPU_SelectionSort") CPU::selectionSort(arr, ctx);
        else if (algKey=="CPU_InsertionSort") CPU::insertionSort(arr, ctx);
        else if (algKey=="CPU_ShellSort")     CPU::shellSort(arr, ctx);
        else if (algKey=="CPU_CocktailSort")  CPU::cocktailSort(arr, ctx);
        else if (algKey=="CPU_GnomeSort")     CPU::gnomeSort(arr, ctx);
        else if (algKey=="CPU_CombSort")      CPU::combSort(arr, ctx);
        else if (algKey=="CPU_RadixSortLSD")  CPU::radixSortLSD(arr, ctx);
        else if (algKey=="CPU_CountingSort")  CPU::countingSort(arr, ctx);
        else if (algKey=="CPU_BucketSort")    CPU::bucketSort(arr, ctx);
        else if (algKey=="CPU_PancakeSort")   CPU::pancakeSort(arr, ctx);
        else if (algKey=="CPU_BogoSort")      CPU::bogoSort(arr, ctx);
        else if (algKey=="CPU_StoogeSort")    CPU::stoogeSort(arr, ctx);
        else if (algKey=="CPU_OddEvenSort")   CPU::oddEvenSort(arr, ctx);
        else if (algKey=="CPU_CycleSort")     CPU::cycleSort(arr, ctx);

        QMetaObject::invokeMethod(this, [this]() {
            m_visualStatusLabel->setText("Статус: Сортировка завершена!");
            onStopVisual();
        }, Qt::QueuedConnection);
    });

    m_visualSortThread->start();
}

void MainWindow::onPauseResumeVisual() {
    if (m_isVisualPaused) {
        m_isVisualPaused = false;
        m_visualPauseBtn->setText("⏸ Пауза");
        m_visualStatusLabel->setText("Статус: Выполняется...");
    } else {
        m_isVisualPaused = true;
        m_visualPauseBtn->setText("▶ Продолжить");
        m_visualStatusLabel->setText("Статус: На паузе.");
    }
}

void MainWindow::onStopVisual() {
    m_stopVisualRequested = true;
    m_isVisualPaused = false;
    if (m_visualSortThread) {
        m_visualSortThread->quit();
        m_visualSortThread->wait();
        delete m_visualSortThread;
        m_visualSortThread = nullptr;
    }
    m_visualPauseBtn->setText("⏸ Пауза");
    m_visualPauseBtn->setEnabled(false);
    m_visualStopBtn->setEnabled(false);
    m_visualStartBtn->setEnabled(true);
    m_visualGenBtn->setEnabled(true);
    m_visualSizeSpin->setEnabled(true);
    m_visualAlgCombo->setEnabled(true);
}

void MainWindow::onVisualStep(const std::vector<double>& arr, int a1, int a2, int piv) {
    m_visualizer->updateData(arr, a1, a2, piv);
}

void MainWindow::updateSystemTelemetry() {
    QString cpu = getRealCpuName();
    QString gpu = getRealGpuName();
    bool hasGpu = (gpu != "GPU Offline / No CUDA Device");

    m_topCpuLabel->setText(cpu);
    m_topGpuLabel->setText(gpu + (hasGpu ? " · Online" : ""));
    m_sidebarGpuLabel->setText(gpu);

    if (hasGpu) {
        m_gpuLedIndicator->setStyleSheet("background: #10b981; border-radius: 4px;");
        m_sidebarGpuLabel->setStyleSheet("color: #10b981; font-size: 10px; background: transparent;");

        size_t freeMem = 0;
        size_t totalMem = 0;
        cudaError_t err = cudaMemGetInfo(&freeMem, &totalMem);
        
        if (err == cudaSuccess && totalMem > 0) {
            size_t usedMem = totalMem - freeMem;
            double usedMB = usedMem / (1024.0 * 1024.0);
            double totalMB = totalMem / (1024.0 * 1024.0);
            int percent = static_cast<int>((usedMem * 100) / totalMem);

            m_gpuVramBar->setValue(percent);
            m_gpuVramLabel->setText(QString("VRAM: %1 MB / %2 MB (%3%)")
                .arg(QString::number(usedMB, 'f', 0))
                .arg(QString::number(totalMB, 'f', 0))
                .arg(percent));
            m_gpuVramBar->setVisible(true);
            m_gpuVramLabel->setVisible(true);
        } else {
            m_gpuVramBar->setVisible(false);
            m_gpuVramLabel->setText("VRAM: Ошибка чтения API");
        }
    } else {
        m_gpuLedIndicator->setStyleSheet("background: #ef4444; border-radius: 4px;");
        m_sidebarGpuLabel->setStyleSheet("color: #ef4444; font-size: 10px; background: transparent;");
        m_gpuVramBar->setVisible(false);
        m_gpuVramLabel->setText("VRAM: Недоступно");
    }
}

void MainWindow::onToggleGpu() {
    m_gpuConnected = !m_gpuConnected;
    if (m_gpuConnected) {
        updateSystemTelemetry();
        m_toggleGpuBtn->setToolTip("GPU активен (кликните для отключения)");
    } else {
        m_gpuLedIndicator->setStyleSheet("background: #ef4444; border-radius: 4px;");
        m_sidebarGpuLabel->setText("GPU OFFLINE (SIM)");
        m_sidebarGpuLabel->setStyleSheet("color: #ef4444; font-size: 10px; background: transparent;");
        m_topGpuLabel->setText("GPU SIMULATION OFFLINE");
        m_topGpuLabel->setStyleSheet(QString("color:%1; font-size:10px; background:transparent;").arg(C_DANGER));
        m_toggleGpuBtn->setToolTip("GPU отключен вручную (кликните для подключения)");
        m_gpuVramBar->setVisible(false);
        m_gpuVramLabel->setText("VRAM: Отключено (SIM)");
    }
}

void MainWindow::onOpenSettings() {
    SettingsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        // Применяем новый интервал телеметрии в реальном времени
        QSettings s;
        m_telemetryTimer->setInterval(s.value("telemetry/interval", 1000).toInt());
        m_runsSpin->setValue(s.value("benchmark/default_runs", 5).toInt());

        // Перерисовываем визуализатор с новой палитрой
        onGenerateVisualArray();
    }
}

void MainWindow::onExportCSV() {
    if (m_accumulatedResults.empty()) {
        QMessageBox::information(this, "Экспорт CSV", "Нет данных для экспорта. Запустите хотя бы один тест.");
        return;
    }
    QString fn = QFileDialog::getSaveFileName(this, "Сохранить CSV", "", "CSV (*.csv);;All (*)");
    if (fn.isEmpty()) return;

    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл для записи.");
        return;
    }

    QSettings s;
    QString sep = s.value("csv/separator", ";").toString();

    QTextStream out(&f);
    out << "Algorithm" << sep << "Device" << sep << "N" << sep << "Min_ms" << sep << "Max_ms" << sep 
        << "Avg_ms" << sep << "Median_ms" << sep << "Variance_ms" << sep << "Upload_ms" << sep 
        << "Kernel_ms" << sep << "Download_ms" << sep << "Status\n";

    for (const auto& r : m_accumulatedResults) {
        out << r.algorithmName << sep << (r.isGPU ? "GPU" : "CPU") << sep
            << r.arraySize << sep << r.minTimeMs << sep << r.maxTimeMs << sep
            << r.avgTimeMs << sep << r.medianTimeMs << sep << r.varianceMs << sep
            << r.avgUploadTimeMs << sep << r.avgKernelTimeMs << sep << r.avgDownloadTimeMs << sep
            << (r.success ? "OK" : "Error") << "\n";
    }
    f.close();
    QMessageBox::information(this, "Экспорт CSV", "Результаты сохранены:\n" + fn);
}

void MainWindow::onExportPNG() {
    if (m_accumulatedResults.empty()) {
        QMessageBox::information(this, "Экспорт PNG", "Нет данных для экспорта графика.");
        return;
    }
    QString fn = QFileDialog::getSaveFileName(this, "Сохранить PNG", "", "PNG (*.png);;All (*)");
    if (fn.isEmpty()) return;

    QPixmap px = m_chartView->grab();
    if (px.save(fn, "PNG")) {
        QMessageBox::information(this, "Экспорт PNG", "График сохранён:\n" + fn);
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось записать PNG.");
    }
}