/**
 * @file cpu_algorithms.h
 * @brief Описание алгоритмов сортировки на стороне CPU.
 * Реализованы все 20 алгоритмов сортировки, соответствующие интерфейсу и бенчмаркам.
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

    // Структура для проверки прерывания оператора пользователем
    struct SortContext {
        std::atomic<bool>* stopRequested = nullptr;
        StepCallback stepCallback = nullptr;
    };

    void stdSort(std::vector<double>& arr, SortContext& ctx);
    void quickSort(std::vector<double>& arr, SortContext& ctx);
    void mergeSort(std::vector<double>& arr, SortContext& ctx);
    void heapSort(std::vector<double>& arr, SortContext& ctx);
    void timSort(std::vector<double>& arr, SortContext& ctx);
    void bubbleSort(std::vector<double>& arr, SortContext& ctx);
    void selectionSort(std::vector<double>& arr, SortContext& ctx);
    void insertionSort(std::vector<double>& arr, SortContext& ctx);
    void shellSort(std::vector<double>& arr, SortContext& ctx);
    void cocktailSort(std::vector<double>& arr, SortContext& ctx);
    void gnomeSort(std::vector<double>& arr, SortContext& ctx);
    void combSort(std::vector<double>& arr, SortContext& ctx);
    void radixSortLSD(std::vector<double>& arr, SortContext& ctx);
    void countingSort(std::vector<double>& arr, SortContext& ctx);
    void bucketSort(std::vector<double>& arr, SortContext& ctx);
    void pancakeSort(std::vector<double>& arr, SortContext& ctx);
    void bogoSort(std::vector<double>& arr, SortContext& ctx);
    void stoogeSort(std::vector<double>& arr, SortContext& ctx);
    void oddEvenSort(std::vector<double>& arr, SortContext& ctx);
    void cycleSort(std::vector<double>& arr, SortContext& ctx);

} // namespace CPU
