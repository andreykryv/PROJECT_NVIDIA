/**
 * @file cpu_algorithms.h
 * @brief Описание алгоритмов сортировки на стороне CPU.
 * Реализованы: std::sort, QuickSort, MergeSort, HeapSort и TimSort.
 * Поддерживают как фоновый запуск (бенчмаркинг), так и пошаговый (визуализация).
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>
#include <functional>
#include <string>
#include <atomic>

namespace CPU {

    // Сигнатура коллбека: (текущий массив, индекс_активный1, индекс_активный2, индекс_опоры)
    using StepCallback = std::function<void(const std::vector<double>&, int, int, int)>;

    // Структура для проверки прерывания операции пользователем
    struct SortContext {
        std::atomic<bool>* stopRequested = nullptr;
        StepCallback stepCallback = nullptr;
    };

    /**
     * @brief Запуск быстрой сортировки (std::sort из STL).
     */
    void stdSort(std::vector<double>& arr, SortContext& ctx);

    /**
     * @brief Кастомная быстрая сортировка (Quick Sort).
     */
    void quickSort(std::vector<double>& arr, SortContext& ctx);

    /**
     * @brief Сортировка слиянием (Merge Sort).
     */
    void mergeSort(std::vector<double>& arr, SortContext& ctx);

    /**
     * @brief Пирамидальная сортировка (Heap Sort).
     */
    void heapSort(std::vector<double>& arr, SortContext& ctx);

    /**
     * @brief Сортировка Timsort (модифицированная гибридная сортировка).
     */
    void timSort(std::vector<double>& arr, SortContext& ctx);

} // namespace CPU
