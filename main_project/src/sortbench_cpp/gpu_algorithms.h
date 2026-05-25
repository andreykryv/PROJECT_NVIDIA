/**
 * @file gpu_algorithms.h
 * @brief Объявление интерфейсов для алгоритмов сортировки на GPU с использованием CUDA.
 * Описывает структуру результатов бенчмаркинга GPU с разделением по фазам.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>
#include <string>

namespace GPU {

    /**
     * @brief Результаты замера производительности GPU операции.
     */
    struct GPUBenchmarkResult {
        double uploadTimeMs = 0.0;      // Время копирования H2D (в VRAM)
        double kernelTimeMs = 0.0;      // Чистое время выполнения CUDA ядра на GPU
        double downloadTimeMs = 0.0;    // Время копирования D2H (из VRAM в RAM)
        double totalTimeMs = 0.0;       // Суммарное время (включая аллокацию и копирования)
        bool success = false;
        std::string errorMessage;
    };

    /**
     * @brief Проверяет наличие совместимого графического процессора NVIDIA.
     */
    bool isCudaAvailable(std::string& info);

    /**
     * @brief Запуск битонической сортировки на GPU (Bitonic Sort).
     */
    GPUBenchmarkResult runBitonicSort(std::vector<double>& arr);

    /**
     * @brief Быстрая поразрядная сортировка на GPU (Radix Sort).
     */
    GPUBenchmarkResult runRadixSort(std::vector<double>& arr);

    /**
     * @brief Сортировка Чет-Нечет (Odd-Even Transposition Sort) на GPU.
     */
    GPUBenchmarkResult runOddEvenSort(std::vector<double>& arr);

    /**
     * @brief Параллельный QuickSort на GPU.
     */
    GPUBenchmarkResult runQuickSort(std::vector<double>& arr);

    /**
     * @brief Параллельный MergeSort на GPU.
     */
    GPUBenchmarkResult runMergeSort(std::vector<double>& arr);

    /**
     * @brief Параллельный BogoSort на GPU (миллиардный перебор перестановок).
     */
    GPUBenchmarkResult runBogoSort(std::vector<double>& arr);

} // namespace GPU
