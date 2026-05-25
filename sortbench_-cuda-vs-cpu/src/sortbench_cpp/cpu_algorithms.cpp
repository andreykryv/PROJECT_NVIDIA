/**
 * @file cpu_algorithms.cpp
 * @brief Реализация классических алгоритмов сортировки на CPU.
 * Каждая функция проверяет индикатор принудительной остановки (stopRequested) 
 * и периодически вызывает интерактивный callback визуализации в процессе перестановок.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpu_algorithms.h"
#include <algorithm>
#include <cmath>

namespace CPU {

    // Вспомогательный макрос для проверки остановки
    #define CHECK_STOP() if (ctx.stopRequested && ctx.stopRequested->load()) return;

    // Вспомогательный макрос вызова шага визуализации
    #define FIRE_STEP(a1, a2, p) if (ctx.stepCallback) { ctx.stepCallback(arr, a1, a2, p); }

    // --- STD::SORT ---
    void stdSort(std::vector<double>& arr, SortContext& ctx) {
        // Стандартный std::sort из STL - это высокоэффективный интроспективный алгоритм (IntroSort).
        // Так как это библиотечная функция, для визуализации мы можем показать только финальный результат,
        // но для чистых бенчмарков он работает на максимальной скорости.
        std::sort(arr.begin(), arr.end());
        CHECK_STOP();
        FIRE_STEP(-1, -1, -1);
    }

    // --- QUICK SORT ---
    void quickSortImpl(std::vector<double>& arr, int low, int high, SortContext& ctx) {
        if (low >= high) return;
        CHECK_STOP();

        // Будем использовать медиану из трех в качестве опорного элемента (pivot) для стабильности
        int mid = low + (high - low) / 2;
        double pivotValue = arr[mid];
        
        // Перемещаем опорный в конец
        std::swap(arr[mid], arr[high]);
        FIRE_STEP(mid, high, high);

        int i = low;
        for (int j = low; j < high; ++j) {
            CHECK_STOP();
            if (arr[j] < pivotValue) {
                std::swap(arr[i], arr[j]);
                FIRE_STEP(i, j, high);
                i++;
            }
        }
        std::swap(arr[i], arr[high]);
        FIRE_STEP(i, high, i);

        // Рекурсивный спуск
        quickSortImpl(arr, low, i - 1, ctx);
        quickSortImpl(arr, i + 1, high, ctx);
    }

    void quickSort(std::vector<double>& arr, SortContext& ctx) {
        if (arr.empty()) return;
        quickSortImpl(arr, 0, arr.size() - 1, ctx);
    }

    // --- MERGE SORT ---
    void merge(std::vector<double>& arr, int l, int m, int r, SortContext& ctx) {
        CHECK_STOP();
        int n1 = m - l + 1;
        int n2 = r - m;

        std::vector<double> L(n1);
        std::vector<double> R(n2);

        for (int i = 0; i < n1; i++) { L[i] = arr[l + i]; }
        for (int j = 0; j < n2; j++) { R[j] = arr[m + 1 + j]; }

        int i = 0, j = 0, k = l;
        while (i < n1 && j < n2) {
            CHECK_STOP();
            if (L[i] <= R[j]) {
                arr[k] = L[i];
                i++;
            } else {
                arr[k] = R[j];
                j++;
            }
            FIRE_STEP(k, l + i, m + j);
            k++;
        }

        while (i < n1) {
            CHECK_STOP();
            arr[k] = L[i];
            FIRE_STEP(k, l + i, -1);
            i++;
            k++;
        }

        while (j < n2) {
            CHECK_STOP();
            arr[k] = R[j];
            FIRE_STEP(k, m + j, -1);
            j++;
            k++;
        }
    }

    void mergeSortImpl(std::vector<double>& arr, int l, int r, SortContext& ctx) {
        if (l >= r) return;
        CHECK_STOP();
        int m = l + (r - l) / 2;
        mergeSortImpl(arr, l, m, ctx);
        mergeSortImpl(arr, m + 1, r, ctx);
        merge(arr, l, m, r, ctx);
    }

    void mergeSort(std::vector<double>& arr, SortContext& ctx) {
        if (arr.empty()) return;
        mergeSortImpl(arr, 0, arr.size() - 1, ctx);
    }

    // --- HEAP SORT ---
    void heapify(std::vector<double>& arr, int n, int i, SortContext& ctx) {
        CHECK_STOP();
        int largest = i;
        int l = 2 * i + 1;
        int r = 2 * i + 2;

        if (l < n && arr[l] > arr[largest])
            largest = l;

        if (r < n && arr[r] > arr[largest])
            largest = r;

        if (largest != i) {
            std::swap(arr[i], arr[largest]);
            FIRE_STEP(i, largest, -1);
            heapify(arr, n, largest, ctx);
        }
    }

    void heapSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        if (n == 0) return;

        // Построение кучи (перегруппировка массива)
        for (int i = n / 2 - 1; i >= 0; i--) {
            CHECK_STOP();
            heapify(arr, n, i, ctx);
        }

        // Один за другим извлекаем элементы из кучи
        for (int i = n - 1; i > 0; i--) {
            CHECK_STOP();
            std::swap(arr[0], arr[i]);
            FIRE_STEP(0, i, -1);
            heapify(arr, i, 0, ctx);
        }
    }

    // --- TIMSORT ---
    const int RUN = 32;

    void insertionSort(std::vector<double>& arr, int left, int right, SortContext& ctx) {
        for (int i = left + 1; i <= right; i++) {
            CHECK_STOP();
            double temp = arr[i];
            int j = i - 1;
            while (j >= left && arr[j] > temp) {
                CHECK_STOP();
                arr[j + 1] = arr[j];
                FIRE_STEP(j, j + 1, i);
                j--;
            }
            arr[j + 1] = temp;
            FIRE_STEP(j + 1, -1, -1);
        }
    }

    void timSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        if (n == 0) return;

        // Сортируем отдельные подмассивы размера RUN с помощью Insertion Sort
        for (int i = 0; i < n; i += RUN) {
            CHECK_STOP();
            insertionSort(arr, i, std::min((i + RUN - 1), (n - 1)), ctx);
        }

        // Начинаем слияние подмассивов размера RUN. Сначала сливаем размер RUN, затем 2*RUN, затем 4*RUN и т.д.
        for (int size = RUN; size < n; size = 2 * size) {
            CHECK_STOP();
            for (int left = 0; left < n; left += 2 * size) {
                CHECK_STOP();
                int mid = left + size - 1;
                int right = std::min((left + 2 * size - 1), (n - 1));

                if (mid < right) {
                    merge(arr, left, mid, right, ctx);
                }
            }
        }
    }

} // namespace CPU
