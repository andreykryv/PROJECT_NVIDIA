/**
 * @file benchmark_runner.cpp
 * @brief Реализация потока бенчмарка.
 * Генерирует массивы данных с различным распределением с помощью средств библиотеки <random>.
 * Проводит сравнительные испытания алгоритмов, рассчитывает статистику.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "benchmark_runner.h"
#include "cpu_algorithms.h"
#include "gpu_algorithms.h"
#include <random>
#include <chrono>
#include <numeric>
#include <algorithm>

BenchmarkRunner::BenchmarkRunner(QObject* parent)
    : QThread(parent), m_stopRequested(false) {}

BenchmarkRunner::~BenchmarkRunner() {
    requestStop();
    wait();
}

void BenchmarkRunner::setConfig(const Benchmark::Config& cfg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = cfg;
}

void BenchmarkRunner::requestStop() {
    m_stopRequested = true;
}

std::vector<double> BenchmarkRunner::generateData(int size, Benchmark::Distribution dist) {
    std::vector<double> data(size);
    std::random_device rd;
    std::mt19937 gen(rd());

    if (dist == Benchmark::Distribution::Uniform) {
        std::uniform_real_distribution<double> dis(-10000.0, 10000.0);
        for (int i = 0; i < size; ++i) {
            data[i] = dis(gen);
        }
    } 
    else if (dist == Benchmark::Distribution::Normal) {
        std::normal_distribution<double> dis(0.0, 1.0); // Матожидание 0, СКО 1
        for (int i = 0; i < size; ++i) {
            data[i] = dis(gen) * 1000.0;
        }
    } 
    else if (dist == Benchmark::Distribution::ReverseSorted) {
        for (int i = 0; i < size; ++i) {
            data[i] = static_cast<double>(size - i);
        }
    } 
    else if (dist == Benchmark::Distribution::AlmostSorted) {
        // Сначала генерируем упорядоченный массив
        for (int i = 0; i < size; ++i) {
            data[i] = static_cast<double>(i);
        }
        // Меняем местами случайные 5% элементов
        int swaps = std::max(1, static_cast<int>(size * 0.05));
        std::uniform_int_distribution<int> disIdx(0, size - 1);
        for (int s = 0; s < swaps; ++s) {
            int idx1 = disIdx(gen);
            int idx2 = disIdx(gen);
            std::swap(data[idx1], data[idx2]);
        }
    } 
    else if (dist == Benchmark::Distribution::AllEqual) {
        std::uniform_real_distribution<double> dis(-10000.0, 10000.0);
        double constantValue = dis(gen);
        std::fill(data.begin(), data.end(), constantValue);
    }

    return data;
}

void BenchmarkRunner::run() {
    m_stopRequested = false;
    
    // Считываем конфиг в локальную копию под мутексом
    Benchmark::Config cfg;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        cfg = m_config;
    }

    int totalAlgorithms = cfg.selectedAlgorithms.size();
    if (totalAlgorithms == 0) {
        emit progressUpdated(100);
        emit finishedAll();
        return;
    }

    for (int algIdx = 0; algIdx < totalAlgorithms; ++algIdx) {
        if (m_stopRequested.load()) break;

        QString algName = cfg.selectedAlgorithms[algIdx];
        bool isGPU = algName.startsWith("GPU_");

        std::vector<double> runTimesMs;
        std::vector<double> uploadTimesMs;
        std::vector<double> downloadTimesMs;
        std::vector<double> kernelTimesMs;

        bool success = true;
        QString errorMsg = "";

        for (int run = 0; run < cfg.runsCount; ++run) {
            if (m_stopRequested.load()) break;

            // Генерируем свежий набор данных на каждый прогон во избежание оптимизаций кэша
            std::vector<double> data = generateData(cfg.arraySize, cfg.dist);

            if (isGPU) {
                if (!cfg.gpuConnected) {
                    success = false;
                    errorMsg = "CUDA Error 702: Link disconnected (PCIe bus link down / GPU Bus simulator off)";
                    break;
                }
                // Вызов CUDA бенчмарков
                GPU::GPUBenchmarkResult gRes;
                if (algName == "GPU_Bitonic") {
                    gRes = GPU::runBitonicSort(data);
                } else if (algName == "GPU_Radix") {
                    gRes = GPU::runRadixSort(data);
                } else if (algName == "GPU_OddEven") {
                    gRes = GPU::runOddEvenSort(data);
                }

                if (!gRes.success) {
                    success = false;
                    errorMsg = QString::fromStdString(gRes.errorMessage);
                    break;
                }
                runTimesMs.push_back(gRes.totalTimeMs);
                uploadTimesMs.push_back(gRes.uploadTimeMs);
                downloadTimesMs.push_back(gRes.downloadTimeMs);
                kernelTimesMs.push_back(gRes.kernelTimeMs);
            } 
            else {
                // Вызов CPU бенчмарков
                std::atomic<bool> localStop(false);
                CPU::SortContext ctx;
                ctx.stopRequested = &localStop;
                ctx.stepCallback = nullptr; // В бенчмарках коллбеки отключаем для скорости

                auto start = std::chrono::high_resolution_clock::now();

                if (algName == "CPU_std::sort") {
                    CPU::stdSort(data, ctx);
                } else if (algName == "CPU_QuickSort") {
                    CPU::quickSort(data, ctx);
                } else if (algName == "CPU_MergeSort") {
                    CPU::mergeSort(data, ctx);
                } else if (algName == "CPU_HeapSort") {
                    CPU::heapSort(data, ctx);
                } else if (algName == "CPU_TimSort") {
                    CPU::timSort(data, ctx);
                }

                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> diff = end - start;
                runTimesMs.push_back(diff.count());
            }
        }

        if (m_stopRequested.load()) break;

        // Расчет статистических показателей
        Benchmark::StatResults stats;
        stats.algorithmName = algName;
        stats.isGPU = isGPU;
        stats.arraySize = cfg.arraySize;
        stats.success = success;
        stats.errorMsg = errorMsg;

        if (success && !runTimesMs.empty()) {
            // Min & Max
            auto [minIt, maxIt] = std::minmax_element(runTimesMs.begin(), runTimesMs.end());
            stats.minTimeMs = *minIt;
            stats.maxTimeMs = *maxIt;

            // Average (Среднее арифметическое)
            double sum = std::accumulate(runTimesMs.begin(), runTimesMs.end(), 0.0);
            stats.avgTimeMs = sum / runTimesMs.size();

            // Median (Медиана)
            std::vector<double> sortedTimes = runTimesMs;
            std::sort(sortedTimes.begin(), sortedTimes.end());
            int mid = sortedTimes.size() / 2;
            stats.medianTimeMs = (sortedTimes.size() % 2 != 0) ? sortedTimes[mid] : (sortedTimes[mid - 1] + sortedTimes[mid]) / 2.0;

            // Variance (Дисперсия)
            double accum = 0.0;
            for (double t : runTimesMs) {
                accum += (t - stats.avgTimeMs) * (t - stats.avgTimeMs);
            }
            stats.varianceMs = (runTimesMs.size() > 1) ? accum / (runTimesMs.size() - 1) : 0.0;

            // GPU доп. статистика
            if (isGPU) {
                stats.avgUploadTimeMs = std::accumulate(uploadTimesMs.begin(), uploadTimesMs.end(), 0.0) / uploadTimesMs.size();
                stats.avgDownloadTimeMs = std::accumulate(downloadTimesMs.begin(), downloadTimesMs.end(), 0.0) / downloadTimesMs.size();
                stats.avgKernelTimeMs = std::accumulate(kernelTimesMs.begin(), kernelTimesMs.end(), 0.0) / kernelTimesMs.size();
            } else {
                stats.avgUploadTimeMs = 0;
                stats.avgDownloadTimeMs = 0;
                stats.avgKernelTimeMs = 0;
            }
        } else {
            stats.minTimeMs = 0;
            stats.maxTimeMs = 0;
            stats.avgTimeMs = 0;
            stats.medianTimeMs = 0;
            stats.varianceMs = 0;
            stats.avgUploadTimeMs = 0;
            stats.avgDownloadTimeMs = 0;
            stats.avgKernelTimeMs = 0;
        }

        emit algorithmCompleted(stats);

        int progress = static_cast<int>(static_cast<double>(algIdx + 1) / totalAlgorithms * 100.0);
        emit progressUpdated(progress);
    }

    emit progressUpdated(100);
    emit finishedAll();
}
