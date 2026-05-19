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

namespace SortBench {

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
bool isRunning() const { return m_running; }
bool isPaused() const { return m_paused; }
    explicit ControlPanel(QWidget *parent = nullptr);
    ~ControlPanel() override = default;

    SortBench::SortParams getCurrentParams() const;
    void setRunning(bool running);
    void setPaused(bool paused);
    void updateBatchProgress(int done, int total);
    void loadFromSettings();
    void saveToSettings() const;

signals:
    void parametersChanged(const SortBench::SortParams& params);
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
    QGroupBox *m_algorithmGroup;
    QComboBox *m_cpuAlgorithmCombo;
    QComboBox *m_gpuAlgorithmCombo;
    QCheckBox *m_enableCpuCheck;
    QCheckBox *m_enableGpuCheck;
    QPushButton *m_algoInfoBtn;

    // Array section
    QGroupBox *m_arrayGroup;
    QSpinBox *m_arraySizeSpinBox;
    QSlider *m_arraySizeSlider;
    QComboBox *m_dataTypeCombo;
    QComboBox *m_distributionCombo;
    QSpinBox *m_randomSeedSpin;
    QCheckBox *m_autoSeedCheck;

    // Animation section
    QGroupBox *m_animationGroup;
    QSlider *m_animSpeedSlider;
    QLabel *m_animSpeedLabel;
    QCheckBox *m_showComparisonsCheck;
    QCheckBox *m_showAccessCountCheck;
    QComboBox *m_colorSchemeCombo;
    QSpinBox *m_maxVisElementsSpin;

    // Batch section
    QGroupBox *m_batchGroup;
    QCheckBox *m_batchModeCheck;
    QPushButton *m_configureBatchBtn;
    QLabel *m_batchStatusLabel;

    // Control buttons
    QPushButton *m_runBtn;
    QPushButton *m_stopBtn;
    QPushButton *m_pauseResumeBtn;
    QPushButton *m_resetBtn;

    // State
    bool m_running;
    bool m_paused;
    
    SortBench::SortParams buildParams() const;
    void setupUi();
    void populateAlgorithms();
    void checkCudaAvailability();
    void showAlgorithmInfo();
};

} // namespace SortBench

#endif // CONTROLPANEL_H
