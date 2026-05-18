#include "benchmarkrunner.h"
#include "sortbenchengine.h"
#include <QtConcurrent/QtConcurrent>
#include <QRandomGenerator>

namespace SortBench {



class BenchmarkRunner::Private {
public:
    BatchConfig config;
    QList<SortParams> testQueue;
    QList<BenchmarkResult> allResults;
    std::atomic<int> completedTests{0};
    std::atomic<bool> stopRequested{false};
    QPointer<SortBenchEngine> currentEngine;
};

BenchmarkRunner::BenchmarkRunner(QObject* parent) 
    : QObject(parent), d(new Private) {}

BenchmarkRunner::~BenchmarkRunner() {
    delete d;
}

void BenchmarkRunner::startBatch(const BatchConfig& config, SortBenchEngine* engine) {
    d->config = config;
    d->testQueue.clear();
    d->allResults.clear();
    d->completedTests = 0;
    d->stopRequested = false;
    
    // Генерируем декартово произведение всех параметров
    for (int cpuAlgo : std::as_const(config.cpuAlgorithms)) {
        for (int gpuAlgo : std::as_const(config.gpuAlgorithms)) {
            for (int size : config.arraySizes) {
                for (int dataType : std::as_const(config.dataTypes)) {
                    for (int dist : std::as_const(config.distributions)) {
                        for (int rep = 0; rep < config.repeatCount; ++rep) {
                            SortParams params;
                            params.cpuAlgorithm = static_cast<CpuAlgorithm>(cpuAlgo);
                            params.gpuAlgorithm = static_cast<GpuAlgorithm>(gpuAlgo);
                            params.arraySize = size;
                            params.dataType = static_cast<DataType>(dataType);
                            params.distribution = static_cast<Distribution>(dist);
                            params.randomSeed = QRandomGenerator::global()->generate();
                            d->testQueue.append(params);
                        }
                    }
                }
            }
        }
    }
    
    if (config.randomizeOrder) {
        std::shuffle(d->testQueue.begin(), d->testQueue.end(), 
                     *QRandomGenerator::global());
    }
    
    d->currentEngine = engine;
    
    emit batchStarted(totalTests());
    
    if (!d->testQueue.isEmpty()) {
        runNextTest();
    }
}

void BenchmarkRunner::stopBatch() {
    d->stopRequested = true;
    if (d->currentEngine) {
        d->currentEngine->stopBenchmark();
    }
}

int BenchmarkRunner::totalTests() const {
    return d->testQueue.size();
}

int BenchmarkRunner::completedTests() const {
    return d->completedTests.load();
}

void BenchmarkRunner::runNextTest() {
    if (d->stopRequested || d->testQueue.isEmpty()) {
        emit batchFinished(d->allResults);
        return;
    }
    
    SortParams params = d->testQueue.takeFirst();
    int testIndex = totalTests() - d->testQueue.size() - 1;
    
    emit testStarted(testIndex, params);
    
    if (d->currentEngine) {
        connect(d->currentEngine, &SortBenchEngine::benchmarkFinished,
                this, &BenchmarkRunner::onTestFinished, Qt::QueuedConnection);
        d->currentEngine->startBenchmark(params);
    }
}

void BenchmarkRunner::onTestFinished(const BenchmarkResult& result) {
    if (d->currentEngine) {
        disconnect(d->currentEngine, &SortBenchEngine::benchmarkFinished,
                   this, &BenchmarkRunner::onTestFinished);
    }
    
    d->allResults.append(result);
    d->completedTests.fetch_add(1);
    
    int testIndex = d->completedTests.load() - 1;
    emit testFinished(testIndex, result);
    emit progressUpdated(d->completedTests.load(), totalTests());
    
    // Запускаем следующий тест
    QTimer::singleShot(100, this, &BenchmarkRunner::runNextTest);
}

} // namespace SortBench
