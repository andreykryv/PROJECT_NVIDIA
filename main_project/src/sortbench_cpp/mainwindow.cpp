/**
 * @file mainwindow.cpp
 * @brief SortBench — Teamify Dashboard Style UI.
 *
 * Трёхколоночный макет:
 *   [Left sidebar 64 px] | [Main content flex] | [Right sidebar 300 px]
 *
 * Main content:
 *   TopBar (64 px) → ViewToggle (40 px) → StackedWidget
 *     Page 0 — Benchmark Analytics (chart + metric cards + table)
 *     Page 1 — Interactive Visualiser
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QBarSet>
#include <QChart>
#include <QColor>
#include <QThread>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QFrame>
#include <QPainter>
#include <QResizeEvent>
#include <QtGlobal>
#include "cpu_algorithms.h"
#include <utility>

// ─────────────────────────────────────────────────────────────────────────────
//  Colour tokens (match Teamify screenshot)
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: horizontal separator
// ─────────────────────────────────────────────────────────────────────────────
static QFrame* hLine(QWidget* parent) {
    QFrame* f = new QFrame(parent);
    f->setFrameShape(QFrame::HLine);
    f->setFixedHeight(1);
    f->setStyleSheet(QString("background:%1; border:none;").arg(C_BORDER));
    return f;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: create a stat / metric card widget
// ─────────────────────────────────────────────────────────────────────────────
struct MetricCard {
    QFrame*  frame;
    QLabel*  value;
    QLabel*  badge;
};
static MetricCard makeMetricCard(const QString& icon,
                                  const QString& title,
                                  const QString& defaultVal,
                                  const QString& badgeText,
                                  const QString& badgeColor,
                                  QWidget* parent)
{
    QFrame* card = new QFrame(parent);
    card->setObjectName("metricCard");

    QVBoxLayout* cl = new QVBoxLayout(card);
    cl->setContentsMargins(16, 14, 16, 14);
    cl->setSpacing(6);

    // Icon + title row
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

    // Big value
    QLabel* valLbl = new QLabel(defaultVal, parent);
    valLbl->setStyleSheet(QString("color:%1; font-size:26px; font-weight:700; background:transparent;").arg(C_TEXT1));
    cl->addWidget(valLbl);

    // Badge
    QLabel* badge = new QLabel(badgeText, parent);
    badge->setStyleSheet(QString("color:%1; font-size:10px; font-weight:600; background:transparent;").arg(badgeColor));
    cl->addWidget(badge);

    return {card, valLbl, badge};
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_stopVisualRequested(false)
    , m_isVisualPaused(false)
    , m_visualDelayMs(50)
{
    setWindowTitle("SortBench: CUDA vs CPU  |  Analytics Dashboard");
    resize(1440, 900);
    setMinimumSize(1100, 700);

    m_benchRunner = new BenchmarkRunner(this);
    connect(m_benchRunner, &BenchmarkRunner::algorithmCompleted,
            this, &MainWindow::onAlgorithmCompleted);
    connect(m_benchRunner, &BenchmarkRunner::progressUpdated,
            this, &MainWindow::onBenchmarkProgress);
    connect(m_benchRunner, &BenchmarkRunner::finishedAll,
            this, &MainWindow::onBenchmarkFinished);

    applyMasterStylesheet();
    setupUI();
    setupCharts();
    loadAvailableAlgorithms();
    onGenerateVisualArray();
}

MainWindow::~MainWindow() {
    onStopVisual();
    if (m_benchRunner) {
        m_benchRunner->requestStop();
        m_benchRunner->wait(3000);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Master Stylesheet
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::applyMasterStylesheet() {
    setStyleSheet(QString(R"(
        /* ── Root ── */
        QMainWindow, QWidget { background:%1; color:%2;
            font-family:'Segoe UI','SF Pro Display',sans-serif; font-size:13px; }

        /* ── Scroll bars ── */
        QScrollBar:vertical   { background:%3; width:6px; margin:0; border-radius:3px; }
        QScrollBar::handle:vertical { background:%4; border-radius:3px; min-height:24px; }
        QScrollBar::handle:vertical:hover { background:%5; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; }
        QScrollBar:horizontal { background:%3; height:6px; border-radius:3px; }
        QScrollBar::handle:horizontal { background:%4; border-radius:3px; }

        /* ── Generic frame/cards ── */
        QFrame#metricCard {
            background:%6; border:1px solid %7;
            border-radius:16px;
        }
        QFrame#metricCard:hover { border-color:%5; }

        /* ── Table ── */
        QTableWidget {
            background:%6; border:none; border-radius:0;
            gridline-color:%7; color:%2; font-size:12px;
        }
        QTableWidget::item { padding:8px 12px; }
        QTableWidget::item:selected { background:%5; color:white; }
        QTableWidget::item:alternate { background:#14142a; }
        QHeaderView::section {
            background:#141428; color:%8; padding:10px 12px;
            border:none; border-bottom:1px solid %7; font-weight:600; font-size:11px;
        }

        /* ── QListWidget (algorithm selector) ── */
        QListWidget {
            background:#0f0f22; border:none; border-radius:12px;
            padding:4px; outline:none;
        }
        QListWidget::item {
            border-radius:8px; padding:7px 12px; margin:2px 0; color:%8;
        }
        QListWidget::item:hover  { background:#1e1e38; color:%2; }
        QListWidget::item:selected {
            background:qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 %5, stop:1 #4f8ded); color:white; font-weight:600;
        }

        /* ── Combo / Spin ── */
        QComboBox, QSpinBox {
            background:#141428; border:1px solid %7; border-radius:8px;
            padding:5px 10px; color:%2; min-height:24px;
        }
        QComboBox:hover, QSpinBox:hover { border-color:%5; }
        QComboBox::drop-down { border:none; width:20px; }
        QComboBox QAbstractItemView {
            background:#1a1a30; border:1px solid %7; border-radius:10px;
            selection-background-color:%5; selection-color:white; padding:4px;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            background:#1e1e38; border:none; width:18px; border-radius:4px; }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover { background:%5; }

        /* ── Progress Bar ── */
        QProgressBar {
            background:#0f0f22; border:none; border-radius:6px;
            height:8px; text-align:center; color:transparent;
        }
        QProgressBar::chunk {
            background:qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 %5, stop:1 #3b82f6); border-radius:6px;
        }

        /* ── Slider ── */
        QSlider::groove:horizontal {
            background:#1e1e38; height:4px; border-radius:2px; }
        QSlider::handle:horizontal {
            background:%5; width:14px; height:14px; margin:-5px 0;
            border-radius:7px; border:none; }
        QSlider::handle:horizontal:hover { background:#818cf8; }
        QSlider::sub-page:horizontal {
            background:qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 %5, stop:1 #3b82f6); border-radius:2px; }

        /* ── Tooltip ── */
        QToolTip {
            background:#1e1e38; border:1px solid %7; border-radius:8px;
            color:%2; padding:6px 12px; font-size:12px; }

        /* ── Labels ── */
        QLabel { background:transparent; }

        /* ── Charts ── */
        QChartView { background:transparent; border:none; }
    )")
    .arg(C_BG_ROOT)    // %1  root bg
    .arg(C_TEXT1)      // %2  text primary
    .arg(C_BG_ROOT)    // %3  scrollbar track
    .arg(C_BORDER)     // %4  scrollbar handle
    .arg(C_ACCENT)     // %5  accent
    .arg(C_BG_CARD)    // %6  card bg
    .arg(C_BORDER)     // %7  border
    .arg(C_TEXT2)      // %8  text secondary
    );
}

// ─────────────────────────────────────────────────────────────────────────────
//  setupUI  (root layout: sidebar | content | right panel)
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setupUI() {
    QWidget* root = new QWidget(this);
    setCentralWidget(root);

    QHBoxLayout* rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Left icon sidebar ────────────────────────────────────────────────────
    setupLeftSidebar(rootLayout);

    // Thin vertical separator
    QFrame* vLine = new QFrame(root);
    vLine->setFrameShape(QFrame::VLine);
    vLine->setFixedWidth(1);
    vLine->setStyleSheet(QString("background:%1; border:none;").arg(C_BORDER));
    rootLayout->addWidget(vLine);

    // ── Centre column (topbar + stacked pages) ───────────────────────────────
    QWidget* centreCol = new QWidget(root);
    centreCol->setObjectName("centreCol");
    QVBoxLayout* centreLayout = new QVBoxLayout(centreCol);
    centreLayout->setContentsMargins(0, 0, 0, 0);
    centreLayout->setSpacing(0);

    setupTopBar(centreLayout);

    m_stackedWidget = new QStackedWidget(centreCol);
    setupBenchmarkPage();
    setupVisualizerPage();
    centreLayout->addWidget(m_stackedWidget, 1);

    rootLayout->addWidget(centreCol, 1);

    // ── Right configuration sidebar ──────────────────────────────────────────
    QFrame* vLine2 = new QFrame(root);
    vLine2->setFrameShape(QFrame::VLine);
    vLine2->setFixedWidth(1);
    vLine2->setStyleSheet(QString("background:%1; border:none;").arg(C_BORDER));
    rootLayout->addWidget(vLine2);

    setupRightSidebar(rootLayout);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Left sidebar
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setupLeftSidebar(QHBoxLayout* root) {
    QFrame* sidebar = new QFrame(this);
    sidebar->setObjectName("leftSidebar");
    sidebar->setFixedWidth(68);
    sidebar->setStyleSheet(QString("QFrame#leftSidebar { background:%1; }").arg(C_BG_SIDEBAR));

    QVBoxLayout* lay = new QVBoxLayout(sidebar);
    lay->setContentsMargins(10, 16, 10, 16);
    lay->setSpacing(4);

    // ── Brand logo ───────────────────────────────────────────────────────────
    QLabel* logo = new QLabel("⚡", sidebar);
    logo->setAlignment(Qt::AlignCenter);
    logo->setFixedSize(48, 48);
    logo->setStyleSheet(QString(
        "font-size:22px; color:%1;"
        "background:rgba(99,102,241,0.18);"
        "border-radius:14px; border:1px solid rgba(99,102,241,0.35);"
    ).arg(C_ACCENT));
    lay->addWidget(logo);

    lay->addSpacing(12);
    lay->addWidget(hLine(sidebar));
    lay->addSpacing(12);

    // ── Nav helper ───────────────────────────────────────────────────────────
    auto navBtn = [&](const QString& icon, const QString& tip, bool active) -> QToolButton* {
        QToolButton* btn = new QToolButton(sidebar);
        btn->setText(icon);
        btn->setToolTip(tip);
        btn->setFixedSize(48, 48);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setCheckable(true);
        btn->setChecked(active);
        btn->setStyleSheet(QString(R"(
            QToolButton {
                font-size:20px; border-radius:13px; border:none;
                background:%1; color:%2;
            }
            QToolButton:hover  { background:rgba(99,102,241,0.22); color:#a5b4fc; }
            QToolButton:checked { background:rgba(99,102,241,0.28); color:%3; }
        )").arg(active ? "rgba(99,102,241,0.28)" : "transparent")
           .arg(active ? C_ACCENT : C_TEXT3)
           .arg(C_ACCENT));
        lay->addWidget(btn, 0, Qt::AlignHCenter);
        return btn;
    };

    m_navBenchBtn  = navBtn("📊", "Бенчмарки / Аналитика", true);
    m_navVisualBtn = navBtn("🎬", "Интерактивный визуализатор", false);
    m_navExportBtn = navBtn("💾", "Экспорт результатов", false);
                     navBtn("📈", "Сравнительные графики", false);

    connect(m_navBenchBtn,  &QToolButton::clicked, this, &MainWindow::switchToBenchmarkPage);
    connect(m_navVisualBtn, &QToolButton::clicked, this, &MainWindow::switchToVisualizerPage);
    connect(m_navExportBtn, &QToolButton::clicked, this, &MainWindow::onExportCSV);

    lay->addStretch();
    lay->addWidget(hLine(sidebar));
    lay->addSpacing(8);

    // Settings btn at bottom
    QToolButton* settBtn = new QToolButton(sidebar);
    settBtn->setText("⚙️");
    settBtn->setToolTip("Настройки");
    settBtn->setFixedSize(48, 48);
    settBtn->setStyleSheet(QString(
        "QToolButton { font-size:20px; border-radius:13px; border:none;"
        " background:transparent; color:%1; }"
        "QToolButton:hover { background:rgba(99,102,241,0.2); color:#a5b4fc; }"
    ).arg(C_TEXT3));
    lay->addWidget(settBtn, 0, Qt::AlignHCenter);

    root->addWidget(sidebar);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Top bar
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setupTopBar(QVBoxLayout* parent) {
    QFrame* topBar = new QFrame(this);
    topBar->setObjectName("topBar");
    topBar->setFixedHeight(66);
    topBar->setStyleSheet(QString(
        "QFrame#topBar { background:%1; border-bottom:1px solid %2; }"
    ).arg(C_BG_TOPBAR).arg(C_BORDER));

    QHBoxLayout* lay = new QHBoxLayout(topBar);
    lay->setContentsMargins(24, 0, 20, 0);
    lay->setSpacing(16);

    // ── Hamburger + page title ───────────────────────────────────────────────
    QToolButton* burgerBtn = new QToolButton(topBar);
    burgerBtn->setText("☰");
    burgerBtn->setFixedSize(36, 36);
    burgerBtn->setStyleSheet(QString(
        "QToolButton { font-size:18px; background:transparent; border:none; color:%1; border-radius:8px; }"
        "QToolButton:hover { background:%2; color:%3; }"
    ).arg(C_TEXT2).arg(C_BG_CARD).arg(C_TEXT1));
    lay->addWidget(burgerBtn);

    QVBoxLayout* titleVLay = new QVBoxLayout();
    titleVLay->setSpacing(1);
    m_pageTitle = new QLabel("Dashboard", topBar);
    m_pageTitle->setStyleSheet(QString(
        "color:%1; font-size:17px; font-weight:700; background:transparent;"
    ).arg(C_TEXT1));
    m_pageSubtitle = new QLabel("SortBench Analytics", topBar);
    m_pageSubtitle->setStyleSheet(QString(
        "color:%1; font-size:11px; background:transparent;"
    ).arg(C_TEXT3));
    titleVLay->addWidget(m_pageTitle);
    titleVLay->addWidget(m_pageSubtitle);
    lay->addLayout(titleVLay);

    // ── View toggle pills ────────────────────────────────────────────────────
    auto pillBtn = [&](const QString& txt, bool active) -> QPushButton* {
        QPushButton* b = new QPushButton(txt, topBar);
        b->setCheckable(true);
        b->setChecked(active);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QString(R"(
            QPushButton {
                font-size:11px; font-weight:600; border-radius:8px;
                padding:5px 14px; border:1px solid %1;
                background:%2; color:%3;
            }
            QPushButton:checked { background:%4; color:white; border-color:%4; }
            QPushButton:hover:!checked { background:%5; color:%6; }
        )").arg(C_BORDER)
           .arg(active ? C_ACCENT : "transparent")
           .arg(active ? "white" : C_TEXT2)
           .arg(C_ACCENT)
           .arg(C_BG_CARD)
           .arg(C_TEXT1));
        return b;
    };

    QPushButton* pb1 = pillBtn("Аналитика", true);
    QPushButton* pb2 = pillBtn("Визуализатор", false);
    connect(pb1, &QPushButton::clicked, this, &MainWindow::switchToBenchmarkPage);
    connect(pb2, &QPushButton::clicked, this, &MainWindow::switchToVisualizerPage);
    lay->addWidget(pb1);
    lay->addWidget(pb2);

    lay->addStretch();

    // ── Search field ─────────────────────────────────────────────────────────
    QFrame* searchFrame = new QFrame(topBar);
    searchFrame->setFixedSize(240, 36);
    searchFrame->setStyleSheet(QString(
        "QFrame { background:%1; border:1px solid %2; border-radius:10px; }"
        "QFrame:hover { border-color:%3; }"
    ).arg(C_BG_CARD).arg(C_BORDER).arg(C_ACCENT));

    QHBoxLayout* searchLay = new QHBoxLayout(searchFrame);
    searchLay->setContentsMargins(10, 0, 10, 0);
    searchLay->setSpacing(6);
    QLabel* sIcon = new QLabel("🔍", searchFrame);
    sIcon->setStyleSheet("font-size:14px; background:transparent;");
    QLineEdit* searchEdit = new QLineEdit(searchFrame);
    searchEdit->setPlaceholderText("Поиск алгоритмов...");
    searchEdit->setStyleSheet(QString(
        "QLineEdit { background:transparent; border:none; color:%1; font-size:12px; }"
        "QLineEdit::placeholder { color:%2; }"
    ).arg(C_TEXT1).arg(C_TEXT3));
    searchLay->addWidget(sIcon);
    searchLay->addWidget(searchEdit);
    lay->addWidget(searchFrame);

    // ── + Add / Run button ───────────────────────────────────────────────────
    m_runBenchBtn = new QPushButton("+ Запуск", topBar);
    m_runBenchBtn->setFixedHeight(36);
    m_runBenchBtn->setCursor(Qt::PointingHandCursor);
    m_runBenchBtn->setStyleSheet(QString(R"(
        QPushButton {
            background:qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 %1, stop:1 %2);
            color:white; border:none; border-radius:10px;
            padding:0 18px; font-weight:700; font-size:12px;
        }
        QPushButton:hover {
            background:qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 #818cf8, stop:1 %1); }
        QPushButton:pressed { background:%2; }
        QPushButton:disabled { background:#2a2a44; color:%3; }
    )").arg(C_ACCENT).arg(C_ACCENT2).arg(C_TEXT3));
    connect(m_runBenchBtn, &QPushButton::clicked, this, &MainWindow::onStartBenchmark);
    lay->addWidget(m_runBenchBtn);

    // ── Stop button ──────────────────────────────────────────────────────────
    m_stopBenchBtn = new QPushButton("■ Стоп", topBar);
    m_stopBenchBtn->setFixedHeight(36);
    m_stopBenchBtn->setEnabled(false);
    m_stopBenchBtn->setCursor(Qt::PointingHandCursor);
    m_stopBenchBtn->setStyleSheet(QString(R"(
        QPushButton {
            background:#2a1c1c; color:#f87171; border:1px solid #5a2424;
            border-radius:10px; padding:0 14px; font-weight:600; font-size:12px;
        }
        QPushButton:hover { background:#3b1f1f; border-color:%1; }
        QPushButton:disabled { background:#1a1a28; color:%2; border-color:%3; }
    )").arg(C_DANGER).arg(C_TEXT3).arg(C_BORDER));
    connect(m_stopBenchBtn, &QPushButton::clicked, this, &MainWindow::onStopBenchmark);
    lay->addWidget(m_stopBenchBtn);

    // ── Notification / GPU toggle ────────────────────────────────────────────
    m_toggleGpuBtn = new QPushButton(topBar);
    m_toggleGpuBtn->setFixedSize(38, 38);
    m_toggleGpuBtn->setCursor(Qt::PointingHandCursor);
    m_toggleGpuBtn->setToolTip("Переключить GPU / PCIe шину");
    // Small green dot to act as indicator
    m_ledIndicator = new QWidget(m_toggleGpuBtn);
    m_ledIndicator->setFixedSize(8, 8);
    m_ledIndicator->move(25, 5);
    m_ledIndicator->setStyleSheet(QString(
        "background:%1; border-radius:4px; border:1px solid rgba(0,0,0,0.4);"
    ).arg(C_SUCCESS));

    m_toggleGpuBtn->setStyleSheet(QString(R"(
        QPushButton {
            background:%1; border:1px solid %2; border-radius:10px;
            font-size:18px; color:%3;
        }
        QPushButton:hover { background:%4; border-color:%5; }
    )").arg(C_BG_CARD).arg(C_BORDER).arg(C_TEXT2)
       .arg(C_BG_SIDEBAR).arg(C_ACCENT));
    m_toggleGpuBtn->setText("🎮");
    connect(m_toggleGpuBtn, &QPushButton::clicked, this, &MainWindow::onToggleGpu);
    lay->addWidget(m_toggleGpuBtn);

    // ── User avatar area ─────────────────────────────────────────────────────
    QFrame* userChip = new QFrame(topBar);
    userChip->setStyleSheet(QString(
        "QFrame { background:%1; border:1px solid %2; border-radius:10px; }"
    ).arg(C_BG_CARD).arg(C_BORDER));
    QHBoxLayout* ucLay = new QHBoxLayout(userChip);
    ucLay->setContentsMargins(8, 4, 12, 4);
    ucLay->setSpacing(8);

    QLabel* avatar = new QLabel("🖥", userChip);
    avatar->setFixedSize(30, 30);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet(QString(
        "font-size:16px; background:rgba(99,102,241,0.2);"
        "border-radius:8px; color:%1;"
    ).arg(C_ACCENT));

    QVBoxLayout* nameBlock = new QVBoxLayout();
    nameBlock->setSpacing(0);
    QLabel* sysName = new QLabel("i9-14900K", userChip);
    sysName->setStyleSheet(QString("color:%1; font-size:11px; font-weight:700; background:transparent;").arg(C_TEXT1));
    m_telemetryTextLabel = new QLabel("RTX 4090 · Online", userChip);
    m_telemetryTextLabel->setStyleSheet(QString("color:%1; font-size:10px; background:transparent;").arg(C_TEXT3));
    nameBlock->addWidget(sysName);
    nameBlock->addWidget(m_telemetryTextLabel);

    ucLay->addWidget(avatar);
    ucLay->addLayout(nameBlock);
    lay->addWidget(userChip);

    parent->addWidget(topBar);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Page 0: Benchmark Analytics
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setupBenchmarkPage() {
    QWidget* page = new QWidget(this);
    page->setStyleSheet(QString("background:%1;").arg(C_BG_ROOT));

    QVBoxLayout* pageLay = new QVBoxLayout(page);
    pageLay->setContentsMargins(20, 16, 20, 16);
    pageLay->setSpacing(16);

    // ── Section label ────────────────────────────────────────────────────────
    QHBoxLayout* hdrRow = new QHBoxLayout();
    QLabel* secLabel = new QLabel("Overall Performance", page);
    secLabel->setStyleSheet(QString("color:%1; font-size:16px; font-weight:700;").arg(C_TEXT1));
    QLabel* allTime = new QLabel("All Time", page);
    allTime->setStyleSheet(QString(R"(
        color:%1; font-size:11px; font-weight:600;
        background:rgba(99,102,241,0.18); border-radius:6px;
        padding:3px 10px; border:1px solid rgba(99,102,241,0.3);
    )").arg(C_ACCENT));
    QLabel* thisYear = new QLabel("This Year", page);
    thisYear->setStyleSheet(QString("color:%1; font-size:11px; padding:3px 10px;").arg(C_TEXT3));
    QLabel* thisWeek = new QLabel("This Week", page);
    thisWeek->setStyleSheet(thisYear->styleSheet());

    hdrRow->addWidget(secLabel);
    hdrRow->addSpacing(12);
    hdrRow->addWidget(allTime);
    hdrRow->addWidget(thisYear);
    hdrRow->addWidget(thisWeek);
    hdrRow->addStretch();

    // Export icon button
    QPushButton* dlBtn = new QPushButton("⬇ Отчёт", page);
    dlBtn->setStyleSheet(QString(R"(
        QPushButton {
            background:%1; border:1px solid %2; border-radius:8px;
            color:%3; font-size:11px; font-weight:600; padding:4px 12px;
        }
        QPushButton:hover { border-color:%4; color:%5; }
    )").arg(C_BG_CARD).arg(C_BORDER).arg(C_TEXT2).arg(C_ACCENT).arg(C_TEXT1));
    connect(dlBtn, &QPushButton::clicked, this, &MainWindow::onExportCSV);
    hdrRow->addWidget(dlBtn);
    pageLay->addLayout(hdrRow);

    // ── Chart card ───────────────────────────────────────────────────────────
    QFrame* chartCard = new QFrame(page);
    chartCard->setObjectName("metricCard");
    chartCard->setMinimumHeight(260);
    QVBoxLayout* chartCardLay = new QVBoxLayout(chartCard);
    chartCardLay->setContentsMargins(16, 14, 16, 14);
    chartCardLay->setSpacing(0);

    m_chartView = new QChartView(chartCard);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setStyleSheet("background:transparent; border:none;");
    chartCardLay->addWidget(m_chartView);

    pageLay->addWidget(chartCard, 2);

    // ── Metric cards row ─────────────────────────────────────────────────────
    QHBoxLayout* metricsRow = new QHBoxLayout();
    metricsRow->setSpacing(12);

    auto mc1 = makeMetricCard("⏱", "Часов замеров", "—", "Всего тестов: 0", C_TEXT3, page);
    mc1.frame->setMinimumWidth(140);
    m_metricAlgCount = mc1.value;
    metricsRow->addWidget(mc1.frame, 1);

    auto mc2 = makeMetricCard("🏆", "Лучший CPU", "— ms", "std::sort / QuickSort", C_TEXT3, page);
    mc2.frame->setMinimumWidth(140);
    m_metricBestCpu = mc2.value;
    metricsRow->addWidget(mc2.frame, 1);

    auto mc3 = makeMetricCard("🚀", "Лучший GPU", "— ms", "Bitonic / Radix CUDA", C_TEXT3, page);
    mc3.frame->setMinimumWidth(140);
    m_metricBestGpu = mc3.value;
    metricsRow->addWidget(mc3.frame, 1);

    auto mc4 = makeMetricCard("⚡", "GPU Ускорение", "—×", "vs лучший CPU", C_TEXT3, page);
    mc4.frame->setMinimumWidth(140);
    m_metricSpeedup = mc4.value;
    metricsRow->addWidget(mc4.frame, 1);

    pageLay->addLayout(metricsRow);

    // ── Results table card ───────────────────────────────────────────────────
    QFrame* tableCard = new QFrame(page);
    tableCard->setObjectName("metricCard");
    tableCard->setStyleSheet(QString(
        "QFrame#metricCard { background:%1; border:1px solid %2; border-radius:16px; }"
    ).arg(C_BG_CARD).arg(C_BORDER));

    QVBoxLayout* tableCardLay = new QVBoxLayout(tableCard);
    tableCardLay->setContentsMargins(0, 0, 0, 0);
    tableCardLay->setSpacing(0);

    // Table header bar
    QFrame* tableHdr = new QFrame(tableCard);
    tableHdr->setStyleSheet(QString(
        "background:%1; border-radius:16px 16px 0 0; border-bottom:1px solid %2;"
    ).arg(C_BG_SIDEBAR).arg(C_BORDER));
    QHBoxLayout* thLay = new QHBoxLayout(tableHdr);
    thLay->setContentsMargins(16, 10, 16, 10);
    QLabel* tbTitle = new QLabel("Результаты бенчмарков", tableHdr);
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
        "Алгоритм", "Тип", "N",
        "Min ms", "Max ms", "Avg ms", "Median ms",
        "GPU Kernel", "Статус"
    });
    m_statsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_statsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_statsTable->setAlternatingRowColors(true);
    m_statsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_statsTable->setShowGrid(true);
    m_statsTable->setStyleSheet(QString(R"(
        QTableWidget {
            background:%1; border:none; border-radius:0 0 16px 16px;
            gridline-color:%2; alternate-background-color:%3;
        }
        QTableWidget::item { padding:8px 10px; color:%4; }
        QTableWidget::item:selected { background:%5; color:white; }
    )").arg(C_BG_CARD).arg(C_BORDER).arg(C_BG_ROOT).arg(C_TEXT1).arg(C_ACCENT));
    tableCardLay->addWidget(m_statsTable);

    pageLay->addWidget(tableCard, 1);

    m_stackedWidget->addWidget(page);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Page 1: Interactive Visualiser
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setupVisualizerPage() {
    QWidget* page = new QWidget(this);
    page->setStyleSheet(QString("background:%1;").arg(C_BG_ROOT));

    QVBoxLayout* lay = new QVBoxLayout(page);
    lay->setContentsMargins(20, 16, 20, 12);
    lay->setSpacing(12);

    // Section title
    QLabel* secTitle = new QLabel("Интерактивный симулятор", page);
    secTitle->setStyleSheet(QString("color:%1; font-size:16px; font-weight:700;").arg(C_TEXT1));
    lay->addWidget(secTitle);

    // Visualizer card
    QFrame* vizCard = new QFrame(page);
    vizCard->setObjectName("metricCard");
    vizCard->setStyleSheet(QString(
        "QFrame#metricCard { background:%1; border:1px solid %2; border-radius:16px; }"
    ).arg(C_BG_CARD).arg(C_BORDER));
    QVBoxLayout* vcLay = new QVBoxLayout(vizCard);
    vcLay->setContentsMargins(12, 12, 12, 12);
    vcLay->setSpacing(0);

    m_visualizer = new SortingVisualizer(vizCard);
    m_visualizer->setMinimumHeight(340);
    vcLay->addWidget(m_visualizer, 1);
    lay->addWidget(vizCard, 1);

    // Control bar card
    QFrame* ctrlCard = new QFrame(page);
    ctrlCard->setObjectName("metricCard");
    ctrlCard->setStyleSheet(QString(
        "QFrame#metricCard { background:%1; border:1px solid %2; border-radius:14px; }"
    ).arg(C_BG_CARD).arg(C_BORDER));

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

    // Gen button
    m_visualGenBtn = new QPushButton("⟳ Генерировать", ctrlCard);
    m_visualGenBtn->setStyleSheet(QString(R"(
        QPushButton { background:%1; border:1px solid %2; border-radius:9px;
            padding:6px 14px; color:%3; font-weight:600; font-size:12px; }
        QPushButton:hover { border-color:%4; color:%5; }
    )").arg(C_BG_SIDEBAR).arg(C_BORDER).arg(C_TEXT2).arg(C_ACCENT).arg(C_TEXT1));
    connect(m_visualGenBtn, &QPushButton::clicked, this, &MainWindow::onGenerateVisualArray);
    ctrlLay->addWidget(m_visualGenBtn);

    ctrlLay->addWidget(hLine(ctrlCard));  // Won't work as QHBoxLayout child, use spacer
    ctrlLay->addSpacing(4);

    // Start
    m_visualStartBtn = new QPushButton("▶ Старт", ctrlCard);
    m_visualStartBtn->setStyleSheet(QString(R"(
        QPushButton {
            background:qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 %1, stop:1 %2);
            border:none; border-radius:9px; padding:6px 16px;
            color:white; font-weight:700; font-size:12px; }
        QPushButton:hover { background:%1; }
        QPushButton:disabled { background:#252538; color:%3; }
    )").arg(C_ACCENT).arg(C_ACCENT2).arg(C_TEXT3));
    connect(m_visualStartBtn, &QPushButton::clicked, this, &MainWindow::onStartVisualSort);
    ctrlLay->addWidget(m_visualStartBtn);

    // Pause
    m_visualPauseBtn = new QPushButton("⏸ Пауза", ctrlCard);
    m_visualPauseBtn->setEnabled(false);
    m_visualPauseBtn->setStyleSheet(QString(R"(
        QPushButton { background:%1; border:1px solid %2; border-radius:9px;
            padding:6px 14px; color:%3; font-weight:600; font-size:12px; }
        QPushButton:hover { border-color:%4; }
        QPushButton:disabled { color:%5; border-color:%6; }
    )").arg(C_BG_SIDEBAR).arg(C_BORDER).arg(C_WARNING).arg(C_WARNING).arg(C_TEXT3).arg(C_BORDER));
    connect(m_visualPauseBtn, &QPushButton::clicked, this, &MainWindow::onPauseResumeVisual);
    ctrlLay->addWidget(m_visualPauseBtn);

    // Stop
    m_visualStopBtn = new QPushButton("⏹ Сброс", ctrlCard);
    m_visualStopBtn->setEnabled(false);
    m_visualStopBtn->setStyleSheet(QString(R"(
        QPushButton { background:#221618; border:1px solid #5a2424; border-radius:9px;
            padding:6px 14px; color:#f87171; font-weight:600; font-size:12px; }
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

    // Status bar
    m_visualStatusLabel = new QLabel("Статус: Готов к запуску", page);
    m_visualStatusLabel->setStyleSheet(QString("color:%1; font-size:11px; padding:2px 4px;").arg(C_TEXT3));
    lay->addWidget(m_visualStatusLabel);

    m_stackedWidget->addWidget(page);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Right configuration sidebar
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::setupRightSidebar(QHBoxLayout* root) {
    QWidget* sidebar = new QWidget(this);
    sidebar->setObjectName("rightSidebar");
    sidebar->setFixedWidth(304);
    sidebar->setStyleSheet(QString("background:%1;").arg(C_BG_SIDEBAR));

    QVBoxLayout* lay = new QVBoxLayout(sidebar);
    lay->setContentsMargins(14, 16, 14, 16);
    lay->setSpacing(12);

    // ── Section: Algorithm selector ──────────────────────────────────────────
    auto sectionHeader = [&](const QString& title, const QString& iconEm) -> QLabel* {
        QLabel* l = new QLabel(iconEm + "  " + title, sidebar);
        l->setStyleSheet(QString("color:%1; font-size:12px; font-weight:700;").arg(C_TEXT1));
        return l;
    };

    lay->addWidget(sectionHeader("Алгоритмы", "📋"));

    QLabel* algHint = new QLabel("Ctrl+Click для мультивыбора", sidebar);
    algHint->setStyleSheet(QString("color:%1; font-size:10px;").arg(C_TEXT3));
    lay->addWidget(algHint);

    m_algListWidget = new QListWidget(sidebar);
    m_algListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    m_algListWidget->setMinimumHeight(160);
    m_algListWidget->setMaximumHeight(220);
    lay->addWidget(m_algListWidget);

    lay->addWidget(hLine(sidebar));

    // ── Section: Configuration ───────────────────────────────────────────────
    lay->addWidget(sectionHeader("Конфигурация", "⚙️"));

    QGridLayout* cfgGrid = new QGridLayout();
    cfgGrid->setSpacing(8);
    cfgGrid->setColumnStretch(1, 1);

    auto cfgLabel = [&](const QString& t) {
        QLabel* l = new QLabel(t, sidebar);
        l->setStyleSheet(QString("color:%1; font-size:11px;").arg(C_TEXT2));
        l->setWordWrap(true);
        return l;
    };

    cfgGrid->addWidget(cfgLabel("Размер N:"), 0, 0);
    m_arraySizeSpin = new QSpinBox(sidebar);
    m_arraySizeSpin->setRange(10, 10'000'000);
    m_arraySizeSpin->setValue(10000);
    m_arraySizeSpin->setSingleStep(1000);
    cfgGrid->addWidget(m_arraySizeSpin, 0, 1);

    cfgGrid->addWidget(cfgLabel("Распределение:"), 1, 0);
    m_distCombo = new QComboBox(sidebar);
    m_distCombo->addItem("Равномерное",      static_cast<int>(Benchmark::Distribution::Uniform));
    m_distCombo->addItem("Нормальное (Гаусс)", static_cast<int>(Benchmark::Distribution::Normal));
    m_distCombo->addItem("Обратное",         static_cast<int>(Benchmark::Distribution::ReverseSorted));
    m_distCombo->addItem("Почти упорядоч.",  static_cast<int>(Benchmark::Distribution::AlmostSorted));
    m_distCombo->addItem("Все одинаковы",    static_cast<int>(Benchmark::Distribution::AllEqual));
    cfgGrid->addWidget(m_distCombo, 1, 1);

    cfgGrid->addWidget(cfgLabel("Тип данных:"), 2, 0);
    m_dataTypeCombo = new QComboBox(sidebar);
    m_dataTypeCombo->addItem("float (32-bit)");
    m_dataTypeCombo->addItem("double (64-bit)");
    cfgGrid->addWidget(m_dataTypeCombo, 2, 1);

    cfgGrid->addWidget(cfgLabel("Повторов:"), 3, 0);
    m_runsSpin = new QSpinBox(sidebar);
    m_runsSpin->setRange(1, 20);
    m_runsSpin->setValue(5);
    cfgGrid->addWidget(m_runsSpin, 3, 1);

    lay->addLayout(cfgGrid);

    lay->addWidget(hLine(sidebar));

    // ── GPU status chip ──────────────────────────────────────────────────────
    QFrame* gpuChip = new QFrame(sidebar);
    gpuChip->setStyleSheet(QString(
        "QFrame { background:#0f1f18; border:1px solid #1a4030; border-radius:10px; }"
    ));
    QHBoxLayout* gpuLay = new QHBoxLayout(gpuChip);
    gpuLay->setContentsMargins(10, 8, 10, 8);
    gpuLay->setSpacing(8);

    m_ledIndicator = new QWidget(gpuChip);
    m_ledIndicator->setFixedSize(8, 8);
    m_ledIndicator->setStyleSheet(QString(
        "background:%1; border-radius:4px;"
    ).arg(C_SUCCESS));

    QLabel* gpuLbl = new QLabel("GPU CUDA:", gpuChip);
    gpuLbl->setStyleSheet(QString("color:%1; font-size:11px; font-weight:600;").arg(C_TEXT2));
    m_telemetryTextLabel = new QLabel("RTX 4090 · Online", gpuChip);
    m_telemetryTextLabel->setStyleSheet(QString("color:%1; font-size:10px;").arg(C_SUCCESS));

    gpuLay->addWidget(m_ledIndicator);
    gpuLay->addWidget(gpuLbl);
    gpuLay->addWidget(m_telemetryTextLabel, 1);
    lay->addWidget(gpuChip);

    // ── Export buttons ───────────────────────────────────────────────────────
    lay->addWidget(sectionHeader("Экспорт", "📤"));

    QHBoxLayout* exportRow = new QHBoxLayout();
    exportRow->setSpacing(8);

    m_exportCsvBtn = new QPushButton("CSV", sidebar);
    m_exportCsvBtn->setStyleSheet(QString(R"(
        QPushButton { background:%1; border:1px solid %2; border-radius:8px;
            padding:6px 0; color:%3; font-weight:600; font-size:11px; }
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

    // ── Activity / recent results feed ───────────────────────────────────────
    lay->addWidget(hLine(sidebar));
    lay->addWidget(sectionHeader("Activity", "📡"));

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
    actItem("🎮", "RTX 4090 подключена", "1 мин назад");
    actItem("📊", "Последний запуск: — ", "ожидание");

    actScroll->setWidget(actContent);
    lay->addWidget(actScroll);

    root->addWidget(sidebar);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Charts setup
// ─────────────────────────────────────────────────────────────────────────────
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

// ─────────────────────────────────────────────────────────────────────────────
//  Algorithm lists
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::loadAvailableAlgorithms() {
    // Right sidebar list
    const QStringList cpuItems = {
        "CPU: std::sort", "CPU: QuickSort", "CPU: MergeSort", "CPU: HeapSort",
        "CPU: TimSort", "CPU: BubbleSort", "CPU: SelectionSort", "CPU: InsertionSort",
        "CPU: ShellSort", "CPU: CocktailSort", "CPU: GnomeSort", "CPU: CombSort",
        "CPU: RadixSortLSD", "CPU: CountingSort", "CPU: BucketSort", "CPU: PancakeSort",
        "CPU: BogoSort", "CPU: StoogeSort", "CPU: OddEvenSort", "CPU: CycleSort"
    };
    const QStringList gpuItems = {
        "GPU: Bitonic Sort", "GPU: Radix Sort", "GPU: Odd-Even", "GPU: std::sort",
        "GPU: QuickSort", "GPU: MergeSort", "GPU: HeapSort", "GPU: TimSort",
        "GPU: BubbleSort", "GPU: SelectionSort"
    };

    for (const auto& s : cpuItems) m_algListWidget->addItem(s);
    for (const auto& s : gpuItems) m_algListWidget->addItem(s);

    // Default selection
    m_algListWidget->item(0)->setSelected(true);   // std::sort
    m_algListWidget->item(1)->setSelected(true);   // QuickSort
    m_algListWidget->item(20)->setSelected(true);  // GPU Bitonic
    m_algListWidget->item(21)->setSelected(true);  // GPU Radix

    // Visualiser combo
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
    for (const auto& p : vItems)
        m_visualAlgCombo->addItem(p.first, p.second);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Page switching
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::switchToBenchmarkPage() {
    m_stackedWidget->setCurrentIndex(0);
    m_pageTitle->setText("Dashboard");
    m_pageSubtitle->setText("Benchmark Analytics");
    if (m_navBenchBtn) { m_navBenchBtn->setChecked(true); }
    if (m_navVisualBtn) { m_navVisualBtn->setChecked(false); }
}

void MainWindow::switchToVisualizerPage() {
    m_stackedWidget->setCurrentIndex(1);
    m_pageTitle->setText("Visualiser");
    m_pageSubtitle->setText("Interactive Sort Simulation");
    if (m_navBenchBtn) { m_navBenchBtn->setChecked(false); }
    if (m_navVisualBtn) { m_navVisualBtn->setChecked(true); }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Benchmark logic (same as before)
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::onStartBenchmark() {
    m_accumulatedResults.clear();
    m_statsTable->setRowCount(0);

    QList<QListWidgetItem*> items = m_algListWidget->selectedItems();
    if (items.isEmpty()) {
        QMessageBox::warning(this, "Выбор алгоритма",
                             "Выберите хотя бы один алгоритм в правой панели.");
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

    for (auto* item : std::as_const(items)) {
        const QString t = item->text();
        if (t.contains("CPU: std::sort"))     cfg.selectedAlgorithms.push_back("CPU_std::sort");
        else if (t.contains("CPU: QuickSort")) cfg.selectedAlgorithms.push_back("CPU_QuickSort");
        else if (t.contains("CPU: MergeSort")) cfg.selectedAlgorithms.push_back("CPU_MergeSort");
        else if (t.contains("CPU: HeapSort"))  cfg.selectedAlgorithms.push_back("CPU_HeapSort");
        else if (t.contains("CPU: TimSort"))   cfg.selectedAlgorithms.push_back("CPU_TimSort");
        else if (t.contains("CPU: BubbleSort")) cfg.selectedAlgorithms.push_back("CPU_BubbleSort");
        else if (t.contains("CPU: SelectionSort")) cfg.selectedAlgorithms.push_back("CPU_SelectionSort");
        else if (t.contains("CPU: InsertionSort"))  cfg.selectedAlgorithms.push_back("CPU_InsertionSort");
        else if (t.contains("CPU: ShellSort"))   cfg.selectedAlgorithms.push_back("CPU_ShellSort");
        else if (t.contains("CPU: CocktailSort")) cfg.selectedAlgorithms.push_back("CPU_CocktailSort");
        else if (t.contains("CPU: GnomeSort"))    cfg.selectedAlgorithms.push_back("CPU_GnomeSort");
        else if (t.contains("CPU: CombSort"))     cfg.selectedAlgorithms.push_back("CPU_CombSort");
        else if (t.contains("CPU: RadixSortLSD")) cfg.selectedAlgorithms.push_back("CPU_RadixSortLSD");
        else if (t.contains("CPU: CountingSort")) cfg.selectedAlgorithms.push_back("CPU_CountingSort");
        else if (t.contains("CPU: BucketSort"))   cfg.selectedAlgorithms.push_back("CPU_BucketSort");
        else if (t.contains("CPU: PancakeSort"))  cfg.selectedAlgorithms.push_back("CPU_PancakeSort");
        else if (t.contains("CPU: BogoSort"))     cfg.selectedAlgorithms.push_back("CPU_BogoSort");
        else if (t.contains("CPU: StoogeSort"))   cfg.selectedAlgorithms.push_back("CPU_StoogeSort");
        else if (t.contains("CPU: OddEvenSort"))  cfg.selectedAlgorithms.push_back("CPU_OddEvenSort");
        else if (t.contains("CPU: CycleSort"))    cfg.selectedAlgorithms.push_back("CPU_CycleSort");
        else if (t.contains("GPU: Bitonic"))  cfg.selectedAlgorithms.push_back("GPU_Bitonic");
        else if (t.contains("GPU: Radix"))    cfg.selectedAlgorithms.push_back("GPU_Radix");
        else if (t.contains("GPU: Odd-Even")) cfg.selectedAlgorithms.push_back("GPU_OddEven");
        else if (t.contains("GPU: std::sort")) cfg.selectedAlgorithms.push_back("GPU_StdSort");
        else if (t.contains("GPU: QuickSort")) cfg.selectedAlgorithms.push_back("GPU_QuickSort");
        else if (t.contains("GPU: MergeSort")) cfg.selectedAlgorithms.push_back("GPU_MergeSort");
        else if (t.contains("GPU: HeapSort"))  cfg.selectedAlgorithms.push_back("GPU_HeapSort");
        else if (t.contains("GPU: TimSort"))   cfg.selectedAlgorithms.push_back("GPU_TimSort");
        else if (t.contains("GPU: BubbleSort")) cfg.selectedAlgorithms.push_back("GPU_BubbleSort");
        else if (t.contains("GPU: SelectionSort")) cfg.selectedAlgorithms.push_back("GPU_SelectionSort");
    }

    m_benchRunner->setConfig(cfg);
    m_benchRunner->start();

    switchToBenchmarkPage();
}

void MainWindow::onStopBenchmark() {
    if (m_benchRunner && m_benchRunner->isRunning())
        m_benchRunner->requestStop();
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
    m_statsTable->setItem(row, 1, cell(s.isGPU ? "🚀 GPU" : "💻 CPU",
                                       s.isGPU ? QColor(C_ACCENT) : QColor(C_BLUE)));
    m_statsTable->setItem(row, 2, cell(QString::number(s.arraySize)));

    if (s.success) {
        m_statsTable->setItem(row, 3, cell(QString::number(s.minTimeMs, 'f', 4)));
        m_statsTable->setItem(row, 4, cell(QString::number(s.maxTimeMs, 'f', 4)));
        m_statsTable->setItem(row, 5, cell(QString::number(s.avgTimeMs, 'f', 4)));
        m_statsTable->setItem(row, 6, cell(QString::number(s.medianTimeMs, 'f', 4)));
        m_statsTable->setItem(row, 7, cell(
            s.isGPU ? QString::number(s.avgKernelTimeMs, 'f', 3) + " ms" : "—"));
        m_statsTable->setItem(row, 8, cell("✅ OK", QColor(C_SUCCESS)));
    } else {
        for (int c = 3; c <= 7; ++c)
            m_statsTable->setItem(row, c, cell("—", QColor(C_TEXT3)));
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

// ─────────────────────────────────────────────────────────────────────────────
//  Chart / metric update
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::updateCharts() {
    m_barSeries->clear();

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
        // Shorten label
        QString lbl = r.algorithmName;
        lbl.remove("CPU_").remove("GPU_").replace("Sort","").replace("sort","");
        cats << lbl;
        *setAvg    << r.avgTimeMs;
        *setMedian << r.medianTimeMs;
        maxVal = std::max({maxVal, r.avgTimeMs, r.medianTimeMs});
    }

    m_barSeries->append(setAvg);
    m_barSeries->append(setMedian);
    m_axisX->clear();
    m_axisX->append(cats);
    m_axisY->setRange(0.0, maxVal * 1.20);
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

    if (bestCpu < 1e18)
        m_metricBestCpu->setText(QString::number(bestCpu, 'f', 3) + " ms");
    if (bestGpu < 1e18)
        m_metricBestGpu->setText(QString::number(bestGpu, 'f', 3) + " ms");
    if (bestCpu < 1e18 && bestGpu < 1e18 && bestGpu > 0)
        m_metricSpeedup->setText(QString::number(bestCpu / bestGpu, 'f', 1) + "×");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Visualiser controls
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::onGenerateVisualArray() {
    onStopVisual();
    int size = m_visualSizeSpin->value();
    m_visualData.resize(size);
    for (int i = 0; i < size; ++i)
        m_visualData[i] = QRandomGenerator::global()->generateDouble() * 500.0 + 10.0;
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
            m_visualStatusLabel->setText("Статус: ✅ Сортировка завершена!");
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

// ─────────────────────────────────────────────────────────────────────────────
//  GPU toggle
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::onToggleGpu() {
    m_gpuConnected = !m_gpuConnected;
    if (m_gpuConnected) {
        m_ledIndicator->setStyleSheet(
            QString("background:%1; border-radius:4px;").arg(C_SUCCESS));
        m_telemetryTextLabel->setText("RTX 4090 · Online");
        m_telemetryTextLabel->setStyleSheet(
            QString("color:%1; font-size:10px; background:transparent;").arg(C_SUCCESS));
        m_toggleGpuBtn->setToolTip("GPU подключен (нажмите для отключения)");
    } else {
        m_ledIndicator->setStyleSheet(
            QString("background:%1; border-radius:4px;").arg(C_DANGER));
        m_telemetryTextLabel->setText("GPU OFFLINE");
        m_telemetryTextLabel->setStyleSheet(
            QString("color:%1; font-size:10px; background:transparent;").arg(C_DANGER));
        m_toggleGpuBtn->setToolTip("GPU отключен (нажмите для подключения)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Export
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::onExportCSV() {
    if (m_accumulatedResults.empty()) {
        QMessageBox::information(this, "Экспорт CSV",
            "Нет данных для экспорта. Запустите хотя бы один тест.");
        return;
    }
    QString fn = QFileDialog::getSaveFileName(
        this, "Сохранить CSV", "", "CSV (*.csv);;All (*)");
    if (fn.isEmpty()) return;

    QFile f(fn);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл для записи.");
        return;
    }
    QTextStream out(&f);
    out << "Algorithm;Device;N;Min_ms;Max_ms;Avg_ms;Median_ms;Variance_ms;"
           "Upload_ms;Kernel_ms;Download_ms;Status\n";
    for (const auto& r : m_accumulatedResults) {
        out << r.algorithmName << ";" << (r.isGPU?"GPU":"CPU") << ";"
            << r.arraySize << ";" << r.minTimeMs << ";" << r.maxTimeMs << ";"
            << r.avgTimeMs << ";" << r.medianTimeMs << ";" << r.varianceMs << ";"
            << r.avgUploadTimeMs << ";" << r.avgKernelTimeMs << ";" << r.avgDownloadTimeMs << ";"
            << (r.success?"OK":"Error") << "\n";
    }
    f.close();
    QMessageBox::information(this, "Экспорт CSV",
        "Результаты сохранены:\n" + fn);
}

void MainWindow::onExportPNG() {
    if (m_accumulatedResults.empty()) {
        QMessageBox::information(this, "Экспорт PNG",
            "Нет данных для экспорта графика.");
        return;
    }
    QString fn = QFileDialog::getSaveFileName(
        this, "Сохранить PNG", "", "PNG (*.png);;All (*)");
    if (fn.isEmpty()) return;

    QPixmap px = m_chartView->grab();
    if (px.save(fn, "PNG"))
        QMessageBox::information(this, "Экспорт PNG", "График сохранён:\n" + fn);
    else
        QMessageBox::critical(this, "Ошибка", "Не удалось записать PNG.");
}