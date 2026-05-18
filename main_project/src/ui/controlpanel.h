#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <QWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../core/sortparams.h"

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(QWidget *parent = nullptr);
    ~ControlPanel() override = default;

    SortParams getCurrentParams() const;
    void setRunning(bool running);
    void setPaused(bool paused);
    void updateBatchProgress(int done, int total);
    void loadFromSettings();
    void saveToSettings() const;

signals:
    void parametersChanged(const SortParams& params);
    void runRequested();
    void stopRequested();
    void pauseResumeRequested();
    void resetRequested();
    void animationSpeedChanged(int fps);

private slots:
    void onAlgorithmChanged();
    void onArraySizeChanged();
    void onDistributionChanged();
    void onAnimSpeedChanged(int value);
    void onRunClicked();
    void onStopClicked();
    void onPauseResumeClicked();
    void onResetClicked();
    void onAlgoInfoClicked();
    void onConfigureBatchClicked();

private:
    void setupAlgorithmSection();
    void setupArraySection();
    void setupAnimationSection();
    void setupBatchSection();
    void setupControlButtons();
    void connectSignals();
    int logScaleToSize(int logValue) const;
    int sizeToLogScale(int size) const;

    // Algorithm section
    QGroupBox *algorithmGroup;
    QComboBox *cpuAlgorithmCombo;
    QComboBox *gpuAlgorithmCombo;
    QCheckBox *enableCpuCheck;
    QCheckBox *enableGpuCheck;
    QPushButton *algoInfoBtn;

    // Array section
    QGroupBox *arrayGroup;
    QSpinBox *arraySizeSpinBox;
    QSlider *arraySizeSlider;
    QComboBox *dataTypeCombo;
    QComboBox *distributionCombo;
    QSpinBox *randomSeedSpin;
    QCheckBox *autoSeedCheck;

    // Animation section
    QGroupBox *animationGroup;
    QSlider *animSpeedSlider;
    QLabel *animSpeedLabel;
    QCheckBox *showComparisonsCheck;
    QCheckBox *showAccessCountCheck;
    QComboBox *colorSchemeCombo;
    QSpinBox *maxVisElementsSpin;

    // Batch section
    QGroupBox *batchGroup;
    QCheckBox *batchModeCheck;
    QPushButton *configureBatchBtn;
    QLabel *batchStatusLabel;

    // Control buttons
    QPushButton *runBtn;
    QPushButton *stopBtn;
    QPushButton *pauseResumeBtn;
    QPushButton *resetBtn;

    // State
    bool m_running;
    bool m_paused;
    
    SortParams buildParams() const;
    void setupUi();
    void populateAlgorithms();
    void checkCudaAvailability();
    void showAlgorithmInfo();
};

#endif // CONTROLPANEL_H
