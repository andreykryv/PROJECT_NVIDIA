/**
 * @file gpu_algorithms.h
 * @brief GPU-реализации всех 20 алгоритмов сортировки (CUDA + Thrust).
 * 
 * Все алгоритмы выполняются на GPU без копирования на хост для сортировки.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>
#include <string>

namespace GPU {

    struct GPUBenchmarkResult {
        double uploadTimeMs = 0.0;
        double kernelTimeMs = 0.0;
        double downloadTimeMs = 0.0;
        double totalTimeMs = 0.0;
        bool success = false;
        std::string errorMessage;
    };

    bool isCudaAvailable(std::string& info);

    // === Сортировки с оригинальными CUDA-ядрами ===
    GPUBenchmarkResult runBitonicSort(std::vector<double>& arr);
    GPUBenchmarkResult runRadixSort(std::vector<double>& arr);
    GPUBenchmarkResult runOddEvenSort(std::vector<double>& arr);
    GPUBenchmarkResult runBogoSort(std::vector<double>& arr);   // Своё GPU-ядро

    // === Остальные 17 алгоритмов – используют thrust::sort на GPU ===
    GPUBenchmarkResult runStdSort(std::vector<double>& arr);
    GPUBenchmarkResult runQuickSort(std::vector<double>& arr);
    GPUBenchmarkResult runMergeSort(std::vector<double>& arr);
    GPUBenchmarkResult runHeapSort(std::vector<double>& arr);
    GPUBenchmarkResult runTimSort(std::vector<double>& arr);
    GPUBenchmarkResult runBubbleSort(std::vector<double>& arr);
    GPUBenchmarkResult runSelectionSort(std::vector<double>& arr);
    GPUBenchmarkResult runInsertionSort(std::vector<double>& arr);
    GPUBenchmarkResult runShellSort(std::vector<double>& arr);
    GPUBenchmarkResult runCocktailSort(std::vector<double>& arr);
    GPUBenchmarkResult runGnomeSort(std::vector<double>& arr);
    GPUBenchmarkResult runCombSort(std::vector<double>& arr);
    GPUBenchmarkResult runRadixSortLSD(std::vector<double>& arr);
    GPUBenchmarkResult runCountingSort(std::vector<double>& arr);
    GPUBenchmarkResult runBucketSort(std::vector<double>& arr);
    GPUBenchmarkResult runPancakeSort(std::vector<double>& arr);
    GPUBenchmarkResult runStoogeSort(std::vector<double>& arr);
    GPUBenchmarkResult runOddEvenSortCPU(std::vector<double>& arr);
    GPUBenchmarkResult runCycleSort(std::vector<double>& arr);

} // namespace GPU