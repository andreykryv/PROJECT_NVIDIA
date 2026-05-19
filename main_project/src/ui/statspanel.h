#ifndef STATSPANEL_H
#define STATSPANEL_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QListWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../core/benchmarkresult.h"

namespace SortBench {

class StatsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StatsPanel(QWidget *parent = nullptr);
    ~StatsPanel() override = default;

    // Методы форматирования
    QString formatTime(double ms) const;
    QString formatBytes(size_t bytes) const;
    QString formatSpeedup(double x) const;

public slots:
    void updateResult(const BenchmarkResult &result);
    void updateGPUMemory(size_t used, size_t total);
    void clearHistory();

signals:
    void resultSelected(const BenchmarkResult &result);

private:
    void createUI();
    void connectSignals();
    QColor speedupColor(double x) const;

    // Секция "Последний результат"
    QGroupBox *m_lastResultGroup;
    QLabel *m_cpuTimeLabel;
    QLabel *m_gpuTimeLabel;
    QLabel *m_gpuKernelLabel;
    QLabel *m_gpuH2DLabel;
    QLabel *m_gpuD2HLabel;
    QLabel *m_speedupLabel;
    QLabel *m_algorithmLabel;
    QLabel *m_arraySizeLabel;

    // Секция "Память GPU"
    QGroupBox *m_gpuMemoryGroup;
    QProgressBar *m_vramProgressBar;
    QLabel *m_vramUsageLabel;
    QLabel *m_deviceNameLabel;

    // Секция "История"
    QGroupBox *m_historyGroup;
    QListWidget *m_historyList;

    int m_maxHistory = 30;
};

} // namespace SortBench

#endif // STATSPANEL_H
