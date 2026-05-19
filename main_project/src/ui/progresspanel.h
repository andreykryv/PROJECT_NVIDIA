#ifndef PROGRESSPANEL_H
#define PROGRESSPANEL_H

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QElapsedTimer>
#include <QTimer>
#include <QPainter>

namespace SortBench {

class CpuGpuSplitBar : public QWidget
{
    Q_OBJECT

public:
    explicit CpuGpuSplitBar(QWidget *parent = nullptr);
    void setCpuGpuTimes(double cpuMs, double gpuMs);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_cpuMs = 0.0;
    double m_gpuMs = 0.0;
};

} // namespace SortBench

class ProgressPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ProgressPanel(QWidget *parent = nullptr);
    ~ProgressPanel() override = default;

    void setArraySize(int size);

public slots:
    void setProgress(int percent);
    void setPhase(const QString &phase);
    void setBenchmarkStarted();
    void setBenchmarkFinished(double cpuMs, double gpuMs);
    void reset();

private:
    void createUI();
    void updateElapsed();
    QString formatSpeed(long long elementsPerSec) const;

    QProgressBar *m_progressBar;
    QLabel *m_phaseLabel;
    QLabel *m_elapsedLabel;
    QLabel *m_etaLabel;
    QLabel *m_speedLabel;
    CpuGpuSplitBar *m_cpuGpuSplitBar;

    QElapsedTimer *m_elapsedTimer;
    QTimer *m_updateTimer;
    int m_arraySize = 100000;
    double m_cpuMs = 0.0;
    double m_gpuMs = 0.0;
};

} // namespace SortBench

#endif // PROGRESSPANEL_H
