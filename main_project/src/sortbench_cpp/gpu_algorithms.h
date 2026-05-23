/**
 * @file gpu_algorithms.h
 * @brief Объявление интерфейсов для алгоритмов сортировки на GPU с использованием CUDA.
 * Описывает структуру результатов бенчмаркинга GPU с разделением по фазам:
 * - Выделение памяти и передача Host-to-Device (H2D)
 * - Период вычислений ядер CUDA (Kernel Execution)
 * - Передача обратно Device-to-Host (D2H)
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
     * @param info Выходная строка с названием GPU и версией CUDA.
     * @return true, если CUDA доступна, иначе false.
     */
    bool isCudaAvailable(std::string& info);

    /**
     * @brief Запуск битонической сортировки на GPU (Bitonic Sort).
     * Эффективна для массивов типов степеней двойки. Массивы автоматически дополняются
     * бесконечностями, если размер не равен 2^k, а затем обрезаются.
     */
    GPUBenchmarkResult runBitonicSort(std::vector<double>& arr);

    /**
     * @brief Быстрая поразрядная сортировка на GPU (Radix Sort).
     * Сортирует по битам, используя эффективный префиксный массив сканирования.
     * Оптимизирована под параллельные блоки CUDA.
     */
    GPUBenchmarkResult runRadixSort(std::vector<double>& arr);

    /**
     * @brief Сортировка Чет-Нечет (Odd-Even Transposition Sort) на GPU.
     * Максимально параллельный алгоритм с четно-нечетными фазами зацепления.
     * Отличается прямолинейностью и подходит для демонстрации параллелизации.
     */
    GPUBenchmarkResult runOddEvenSort(std::vector<double>& arr);

} // namespace GPU
