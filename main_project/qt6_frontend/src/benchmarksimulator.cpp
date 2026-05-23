/**
 * @file benchmarksimulator.cpp
 * @brief Реализация симулятора бенчмарков.
 */
#include "benchmarksimulator.h"
#include <QRandomGenerator>
#include <QtMath>
#include <chrono>

BenchmarkSimulator::BenchmarkSimulator(QObject *parent)
    : QObject(parent)
    , m_arraySize(100000)
    , m_runsCount(5)
    , m_currentAlgIndex(0)
    , m_currentRun(0)
    , m_isRunning(false)
    , m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, &BenchmarkSimulator::onSimulationStep);
}

void BenchmarkSimulator::runBenchmark(const QVector<QString> &algorithms, int arraySize,
                                       const QString &distribution, const QString &dataType, 
                                       int runsCount) {
    m_algorithms = algorithms;
    m_arraySize = arraySize;
    m_distribution = distribution;
    m_dataType = dataType;
    m_runsCount = runsCount;
    m_currentAlgIndex = 0;
    m_currentRun = 0;
    m_isRunning = true;
    m_results.clear();
    
    m_timer->start(100); // Шаг симуляции 100мс
}

void BenchmarkSimulator::stop() {
    m_isRunning = false;
    m_timer->stop();
    emit completed();
}

void BenchmarkSimulator::onSimulationStep() {
    if (!m_isRunning) return;
    
    if (m_currentAlgIndex >= m_algorithms.size()) {
        m_isRunning = false;
        m_timer->stop();
        emit completed();
        return;
    }
    
    QString algName = m_algorithms[m_currentAlgIndex];
    bool isGPU = algName.startsWith("GPU_");
    
    // Эмуляция нескольких запусков
    if (m_currentRun < m_runsCount) {
        m_currentRun++;
        int progress = ((m_currentAlgIndex * m_runsCount + m_currentRun) * 100) / 
                       (m_algorithms.size() * m_runsCount);
        emit progressUpdated(progress);
    } else {
        // Завершение алгоритма - генерация результата
        BenchmarkResult result;
        result.algorithmName = algName;
        result.isGPU = isGPU;
        result.arraySize = m_arraySize;
        
        double baseTime = estimateTime(algName, m_arraySize, m_dataType);
        
        // Добавление вариации для реализма
        double variation = QRandomGenerator::global()->generateDouble() * 0.2 + 0.9;
        
        result.avgTimeMs = baseTime * variation;
        result.minTimeMs = baseTime * (variation - 0.1);
        result.maxTimeMs = baseTime * (variation + 0.15);
        result.medianTimeMs = baseTime * variation;
        result.varianceMs = baseTime * 0.05;
        
        if (isGPU) {
            result.avgUploadTimeMs = baseTime * 0.5;
            result.avgKernelTimeMs = baseTime * 0.1;
            result.avgDownloadTimeMs = baseTime * 0.4;
        } else {
            result.avgUploadTimeMs = 0;
            result.avgKernelTimeMs = 0;
            result.avgDownloadTimeMs = 0;
        }
        
        result.success = true;
        result.errorMsg = "";
        
        m_results.append(result);
        emit resultReady(result);
        
        m_currentAlgIndex++;
        m_currentRun = 0;
    }
}

double BenchmarkSimulator::estimateTime(const QString &algorithm, int arraySize, 
                                         const QString &dataType) {
    double factor = getComplexityFactor(algorithm);
    bool isGPU = algorithm.startsWith("GPU_");
    
    // Базовое время зависит от размера и сложности
    double n = arraySize;
    double timeMs;
    
    if (algorithm.contains("Bubble") || algorithm.contains("Selection") || 
        algorithm.contains("Insertion") || algorithm.contains("Bogo")) {
        // O(N^2)
        timeMs = factor * (n * n) / 1e7;
    } else if (algorithm.contains("Bitonic") || algorithm.contains("OddEven")) {
        // O(log^2 N) для GPU
        timeMs = factor * qLn(n) * qLn(n) / 100;
    } else {
        // O(N log N)
        timeMs = factor * (n * qLn(n)) / 1e6;
    }
    
    // Коррекция для GPU
    if (isGPU) {
        timeMs *= 0.1; // GPU быстрее на больших данных
        // Накладные расходы PCIe
        timeMs += (n * sizeof(double)) / (1024.0 * 1024.0 * 1024.0) * 100; // ~1GB/s PCIe
    }
    
    // Коррекция для типа данных
    if (dataType == "float") {
        timeMs *= 0.8;
    } else if (dataType == "int") {
        timeMs *= 0.6;
    }
    
    // Коррекция для распределения
    if (m_distribution == "Sorted") {
        if (algorithm.contains("Tim") || algorithm.contains("Insertion")) {
            timeMs *= 0.3; // Быстро на отсортированных
        }
    } else if (m_distribution == "Reverse Sorted") {
        if (algorithm.contains("Quick") && !algorithm.contains("GPU")) {
            timeMs *= 2.0; // Медленно для QuickSort
        }
    }
    
    return qMax(0.01, timeMs);
}

double BenchmarkSimulator::getComplexityFactor(const QString &algorithm) {
    if (algorithm.contains("std::sort")) return 0.8;
    if (algorithm.contains("Quick")) return 1.0;
    if (algorithm.contains("Merge")) return 1.1;
    if (algorithm.contains("Heap")) return 1.2;
    if (algorithm.contains("Tim")) return 0.9;
    if (algorithm.contains("Shell")) return 1.5;
    if (algorithm.contains("Comb")) return 1.8;
    if (algorithm.contains("Radix")) return 0.7;
    if (algorithm.contains("Counting")) return 0.5;
    if (algorithm.contains("Bucket")) return 0.8;
    if (algorithm.contains("Bitonic")) return 0.6;
    if (algorithm.contains("Bubble")) return 5.0;
    if (algorithm.contains("Selection")) return 4.0;
    if (algorithm.contains("Insertion")) return 3.0;
    if (algorithm.contains("Bogo")) return 1000.0;
    if (algorithm.contains("Stooge")) return 100.0;
    
    return 1.0;
}
