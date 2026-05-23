/**
 * @file mainwindow.h
 * @brief Главное окно приложения SortBench с 4 вкладками.
 */
#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QTableWidget>
#include <QChartView>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QStackedWidget>
#include <QTextEdit>
#include <QMap>
#include <QVector>
#include <QTimer>

class SortingVisualizer;
class BenchmarkSimulator;

struct BenchmarkResult {
    QString algorithmName;
    bool isGPU;
    int arraySize;
    double avgTimeMs;
    double minTimeMs;
    double maxTimeMs;
    double medianTimeMs;
    double varianceMs;
    double avgUploadTimeMs;
    double avgKernelTimeMs;
    double avgDownloadTimeMs;
    bool success;
    QString errorMsg;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRunBenchmark();
    void onStopBenchmark();
    void onGenerateNewArray();
    void onStartVisualization();
    void onPauseVisualization();
    void onSpeedChanged(int value);
    void onAlgorithmSelected(const QString &alg);
    void onExportCSV();
    void onExportJSON();
    void onTabChanged(int index);
    void onSimulationProgress(int progress);
    void onSimulationComplete();
    void onCopyCode(const QString &code);

private:
    void setupUI();
    void setupBenchmarkTab();
    void setupVisualizationTab();
    void setupTheoryTab();
    void setupReportsTab();
    void updateCharts();
    void updateResultsTable();
    void loadAlgorithmGroups();
    QString getCodeFileContent(const QString &fileName);

    QTabWidget *m_tabWidget;
    
    // Benchmark tab widgets
    QWidget *m_benchmarkWidget;
    QCheckBox *m_algCheckboxes[30];
    int m_algCheckboxCount;
    QSpinBox *m_arraySizeSpin;
    QComboBox *m_distributionCombo;
    QComboBox *m_dataTypeCombo;
    QSpinBox *m_runsCountSpin;
    QPushButton *m_runButton;
    QPushButton *m_stopButton;
    QProgressBar *m_progressBar;
    QLabel *m_progressLabel;
    QTextEdit *m_logEdit;
    QChartView *m_chartView;
    QTableWidget *m_resultsTable;
    
    // Visualization tab widgets
    QWidget *m_visualWidget;
    SortingVisualizer *m_visualizer;
    QComboBox *m_visualAlgCombo;
    QSpinBox *m_visualSizeSpin;
    QSlider *m_speedSlider;
    QPushButton *m_generateButton;
    QPushButton *m_startVisualButton;
    QPushButton *m_pauseVisualButton;
    QLabel *m_visualStatusLabel;
    
    // Theory tab widgets
    QWidget *m_theoryWidget;
    QTextEdit *m_codeViewer;
    QComboBox *m_fileSelector;
    
    // Reports tab widgets
    QWidget *m_reportsWidget;
    QTextEdit *m_reportText;
    
    // Data
    QVector<BenchmarkResult> m_results;
    BenchmarkSimulator *m_simulator;
    bool m_isSimulating;
    bool m_isVisualRunning;
    bool m_isVisualPaused;
    
    QMap<QString, QString> m_cppFiles;
};
