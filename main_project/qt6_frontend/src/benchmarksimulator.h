/**
 * @file benchmarksimulator.h
 * @brief Симулятор бенчмарков для эмуляции работы алгоритмов сортировки.
 */
#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <QTimer>

// BenchmarkResult определяется в mainwindow.h
struct BenchmarkResult;

class BenchmarkSimulator : public QObject {
    Q_OBJECT

public:
    explicit BenchmarkSimulator(QObject *parent = nullptr);
    
    void runBenchmark(const QVector<QString> &algorithms, int arraySize, 
                      const QString &distribution, const QString &dataType, int runsCount);
    void stop();

signals:
    void progressUpdated(int progress);
    void resultReady(const BenchmarkResult &result);
    void completed();

private slots:
    void onSimulationStep();

private:
    double estimateTime(const QString &algorithm, int arraySize, const QString &dataType);
    double getComplexityFactor(const QString &algorithm);
    
    QVector<QString> m_algorithms;
    int m_arraySize;
    QString m_distribution;
    QString m_dataType;
    int m_runsCount;
    int m_currentAlgIndex;
    int m_currentRun;
    bool m_isRunning;
    
    QTimer *m_timer;
    QVector<BenchmarkResult> m_results;
};
