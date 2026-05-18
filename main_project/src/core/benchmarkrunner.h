#ifndef BENCHMARKRUNNER_H
#define BENCHMARKRUNNER_H

#include <QObject>
#include <QList>
#include <QPointer>
#include <atomic>
#include "sortparams.h"
#include "benchmarkresult.h"

namespace SortBench {

class SortBenchEngine;

struct BatchConfig {
    QList<CpuAlgorithm> cpuAlgorithms;
    QList<GpuAlgorithm> gpuAlgorithms;
    QList<int> arraySizes;
    QList<DataType> dataTypes;
    QList<Distribution> distributions;
    int repeatCount = 3;
    bool randomizeOrder = false;
};

class BenchmarkRunner : public QObject {
    Q_OBJECT

public:
    explicit BenchmarkRunner(QObject* parent = nullptr);
    ~BenchmarkRunner();

    void startBatch(const BatchConfig& config, SortBenchEngine* engine);
    void stopBatch();
    int totalTests() const;
    int completedTests() const;

signals:
    void batchStarted(int total);
    void testStarted(int index, const SortParams& params);
    void testFinished(int index, const BenchmarkResult& result);
    void batchFinished(const QList<BenchmarkResult>& results);
    void progressUpdated(int completed, int total);

private slots:
    void runNextTest();
    void onTestFinished(const BenchmarkResult& result);

private:
    class Private;
    Private* d;
};

} // namespace SortBench

#endif // BENCHMARKRUNNER_H
