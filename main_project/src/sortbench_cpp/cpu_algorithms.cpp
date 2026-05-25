/**
 * @file cpu_algorithms.cpp
 * @brief Реализация классических и гибридных алгоритмов сортировки на CPU.
 * Каждая функция проверяет индикатор принудительной остановки (stopRequested) 
 * и периодически вызывает интерактивный callback визуализации в процессе перестановок.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cpu_algorithms.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>
#include <cstring>
#include <algorithm>

namespace CPU {

    // Вспомогательный макрос для проверки остановки
    #define CHECK_STOP() if (ctx.stopRequested && ctx.stopRequested->load()) return;

    // Вспомогательный макрос вызова шага визуализации
    #define FIRE_STEP(a1, a2, p) if (ctx.stepCallback) { ctx.stepCallback(arr, a1, a2, p); }

    // --- 1. STD::SORT ---
    void stdSort(std::vector<double>& arr, SortContext& ctx) {
        std::sort(arr.begin(), arr.end());
        CHECK_STOP();
        FIRE_STEP(-1, -1, -1);
    }

    // --- 2. QUICK SORT ---
    void quickSortImpl(std::vector<double>& arr, int low, int high, SortContext& ctx) {
        if (low >= high) return;
        CHECK_STOP();

        int mid = low + (high - low) / 2;
        double pivotValue = arr[mid];
        
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

        quickSortImpl(arr, low, i - 1, ctx);
        quickSortImpl(arr, i + 1, high, ctx);
    }

    void quickSort(std::vector<double>& arr, SortContext& ctx) {
        if (arr.empty()) return;
        quickSortImpl(arr, 0, arr.size() - 1, ctx);
    }

    // --- 3. MERGE SORT ---
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

    // --- 4. HEAP SORT ---
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

        for (int i = n / 2 - 1; i >= 0; i--) {
            CHECK_STOP();
            heapify(arr, n, i, ctx);
        }

        for (int i = n - 1; i > 0; i--) {
            CHECK_STOP();
            std::swap(arr[0], arr[i]);
            FIRE_STEP(0, i, -1);
            heapify(arr, i, 0, ctx);
        }
    }

    // --- 5. TIMSORT ---
    const int RUN = 32;

    void insertionSortImpl(std::vector<double>& arr, int left, int right, SortContext& ctx) {
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

        for (int i = 0; i < n; i += RUN) {
            CHECK_STOP();
            insertionSortImpl(arr, i, std::min((i + RUN - 1), (n - 1)), ctx);
        }

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

    // --- 6. BUBBLE SORT ---
    void bubbleSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        for (int i = 0; i < n - 1; i++) {
            bool swapped = false;
            for (int j = 0; j < n - i - 1; j++) {
                CHECK_STOP();
                if (arr[j] > arr[j + 1]) {
                    std::swap(arr[j], arr[j + 1]);
                    FIRE_STEP(j, j + 1, -1);
                    swapped = true;
                }
            }
            if (!swapped) break;
        }
    }

    // --- 7. SELECTION SORT ---
    void selectionSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        for (int i = 0; i < n - 1; i++) {
            int minIdx = i;
            for (int j = i + 1; j < n; j++) {
                CHECK_STOP();
                if (arr[j] < arr[minIdx]) {
                    minIdx = j;
                }
            }
            if (minIdx != i) {
                std::swap(arr[i], arr[minIdx]);
                FIRE_STEP(i, minIdx, -1);
            }
        }
    }

    // --- 8. INSERTION SORT ---
    void insertionSort(std::vector<double>& arr, SortContext& ctx) {
        if (arr.empty()) return;
        insertionSortImpl(arr, 0, arr.size() - 1, ctx);
    }

    // --- 9. SHELL SORT ---
    void shellSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        for (int gap = n / 2; gap > 0; gap /= 2) {
            for (int i = gap; i < n; i++) {
                CHECK_STOP();
                double temp = arr[i];
                int j = i;
                while (j >= gap && arr[j - gap] > temp) {
                    CHECK_STOP();
                    arr[j] = arr[j - gap];
                    FIRE_STEP(j, j - gap, -1);
                    j -= gap;
                }
                arr[j] = temp;
                FIRE_STEP(j, -1, -1);
            }
        }
    }

    // --- 10. COCKTAIL SORT ---
    void cocktailSort(std::vector<double>& arr, SortContext& ctx) {
        bool swapped = true;
        int start = 0;
        int end = arr.size() - 1;

        while (swapped) {
            swapped = false;
            for (int i = start; i < end; ++i) {
                CHECK_STOP();
                if (arr[i] > arr[i + 1]) {
                    std::swap(arr[i], arr[i + 1]);
                    FIRE_STEP(i, i + 1, -1);
                    swapped = true;
                }
            }
            if (!swapped) break;
            swapped = false;
            --end;
            for (int i = end - 1; i >= start; --i) {
                CHECK_STOP();
                if (arr[i] > arr[i + 1]) {
                    std::swap(arr[i], arr[i + 1]);
                    FIRE_STEP(i, i + 1, -1);
                    swapped = true;
                }
            }
            ++start;
        }
    }

    // --- 11. GNOME SORT ---
    void gnomeSort(std::vector<double>& arr, SortContext& ctx) {
        int index = 0;
        int n = arr.size();
        while (index < n) {
            CHECK_STOP();
            if (index == 0) index++;
            if (arr[index] >= arr[index - 1]) {
                index++;
            } else {
                std::swap(arr[index], arr[index - 1]);
                FIRE_STEP(index, index - 1, -1);
                index--;
            }
        }
    }

    // --- 12. COMB SORT ---
    void combSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        int gap = n;
        bool swapped = true;

        while (gap != 1 || swapped) {
            gap = (gap * 10) / 13;
            if (gap < 1) gap = 1;
            swapped = false;

            for (int i = 0; i < n - gap; i++) {
                CHECK_STOP();
                if (arr[i] > arr[i + gap]) {
                    std::swap(arr[i], arr[i + gap]);
                    FIRE_STEP(i, i + gap, -1);
                    swapped = true;
                }
            }
        }
    }

    // --- 13. RADIX SORT LSD ---
    void radixSortLSD(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        if (n <= 1) return;

        // Поразрядная по байтам
        std::vector<double> output(n);
        for (int byteIdx = 0; byteIdx < 8; ++byteIdx) {
            CHECK_STOP();
            int count[256] = {0};

            for (int i = 0; i < n; ++i) {
                uint64_t val;
                std::memcpy(&val, &arr[i], sizeof(double));
                int byteVal = (val >> (byteIdx * 8)) & 0xFF;
                count[byteVal]++;
            }

            for (int i = 1; i < 256; ++i) {
                count[i] += count[i - 1];
            }

            for (int i = n - 1; i >= 0; --i) {
                uint64_t val;
                std::memcpy(&val, &arr[i], sizeof(double));
                int byteVal = (val >> (byteIdx * 8)) & 0xFF;
                output[count[byteVal] - 1] = arr[i];
                count[byteVal]--;
            }

            arr = output;
            FIRE_STEP(-1, -1, -1);
        }
        // Финальный проход коррекции знака для double
        // cpu_algorithms.cpp, строки 366–367
std::stable_partition(arr.begin(), arr.end(), [](double x) { return x < 0; });
std::reverse(arr.begin(), std::stable_partition(arr.begin(), arr.end(), [](double x) { return x < 0; }));
        FIRE_STEP(-1, -1, -1);
    }

    // --- 14. COUNTING SORT ---
    void countingSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        if (n <= 1) return;

        // Нормируем значения double в диапазон для сортировки подсчетом
        auto [minIt, maxIt] = std::minmax_element(arr.begin(), arr.end());
        double minVal = *minIt;
        double maxVal = *maxIt;
        double range = maxVal - minVal;

        if (range < 1e-9) return;

        int K = 2000; // Число ведер/подсчетов
        std::vector<int> count(K, 0);
        std::vector<double> output(n);

        for (int i = 0; i < n; ++i) {
            int bucket = static_cast<int>((arr[i] - minVal) / range * (K - 1));
            count[bucket]++;
        }

        for (int i = 1; i < K; ++i) {
            count[i] += count[i - 1];
        }

        for (int i = n - 1; i >= 0; --i) {
            CHECK_STOP();
            int bucket = static_cast<int>((arr[i] - minVal) / range * (K - 1));
            output[count[bucket] - 1] = arr[i];
            count[bucket]--;
        }

        arr = output;
        FIRE_STEP(-1, -1, -1);
    }

    // --- 15. BUCKET SORT ---
    void bucketSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        if (n <= 1) return;

        auto [minIt, maxIt] = std::minmax_element(arr.begin(), arr.end());
        double minVal = *minIt;
        double maxVal = *maxIt;
        double range = maxVal - minVal;

        if (range < 1e-9) return;

        int bucketCount = std::min(n, 40);
        std::vector<std::vector<double>> buckets(bucketCount);

        for (int i = 0; i < n; ++i) {
            int bucketIdx = static_cast<int>((arr[i] - minVal) / range * (bucketCount - 1));
            buckets[bucketIdx].push_back(arr[i]);
        }

        int idx = 0;
        for (int b = 0; b < bucketCount; ++b) {
            CHECK_STOP();
            std::sort(buckets[b].begin(), buckets[b].end());
            for (double val : buckets[b]) {
                arr[idx++] = val;
                FIRE_STEP(idx - 1, -1, -1);
            }
        }
    }

    // --- 16. PANCAKE SORT ---
    void flip(std::vector<double>& arr, int i, SortContext& ctx) {
        int start = 0;
        while (start < i) {
            std::swap(arr[start], arr[i]);
            FIRE_STEP(start, i, -1);
            start++;
            i--;
        }
    }

    void pancakeSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        for (int currSize = n; currSize > 1; --currSize) {
            CHECK_STOP();
            int maxIdx = 0;
            for (int i = 1; i < currSize; ++i) {
                if (arr[i] > arr[maxIdx]) {
                    maxIdx = i;
                }
            }
            if (maxIdx != currSize - 1) {
                flip(arr, maxIdx, ctx);
                flip(arr, currSize - 1, ctx);
            }
        }
    }

    // --- 17. BOGOSORT ---
    void bogoSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        std::random_device rd;
        std::mt19937 g(rd());

        auto isSorted = [&](const std::vector<double>& a) {
            for (int i = 1; i < n; ++i) {
                if (a[i] < a[i - 1]) return false;
            }
            return true;
        };

        // Будем переставлять элементы, но не бесконечно, чтобы не вешать симулятор в GUI
        int iterations = 0;
        while (!isSorted(arr) && iterations < 2000) {
            CHECK_STOP();
            std::shuffle(arr.begin(), arr.end(), g);
            FIRE_STEP(0, n - 1, -1);
            iterations++;
        }
    }

    // --- 18. STOOGESORT ---
    void stoogeSortImpl(std::vector<double>& arr, int l, int h, SortContext& ctx) {
        if (l >= h) return;
        CHECK_STOP();

        if (arr[l] > arr[h]) {
            std::swap(arr[l], arr[h]);
            FIRE_STEP(l, h, -1);
        }

        if (h - l + 1 > 2) {
            int t = (h - l + 1) / 3;
            stoogeSortImpl(arr, l, h - t, ctx);
            stoogeSortImpl(arr, l + t, h, ctx);
            stoogeSortImpl(arr, l, h - t, ctx);
        }
    }

    void stoogeSort(std::vector<double>& arr, SortContext& ctx) {
        if (arr.empty()) return;
        stoogeSortImpl(arr, 0, arr.size() - 1, ctx);
    }

    // --- 19. ODD-EVEN SORT ---
    void oddEvenSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        bool isSorted = false;

        while (!isSorted) {
            isSorted = true;
            // Четная фаза
            for (int i = 1; i < n - 1; i += 2) {
                CHECK_STOP();
                if (arr[i] > arr[i + 1]) {
                    std::swap(arr[i], arr[i + 1]);
                    FIRE_STEP(i, i + 1, -1);
                    isSorted = false;
                }
            }
            // Нечетная фаза
            for (int i = 0; i < n - 1; i += 2) {
                CHECK_STOP();
                if (arr[i] > arr[i + 1]) {
                    std::swap(arr[i], arr[i + 1]);
                    FIRE_STEP(i, i + 1, -1);
                    isSorted = false;
                }
            }
        }
    }

    // --- 20. CYCLE SORT ---
    void cycleSort(std::vector<double>& arr, SortContext& ctx) {
        int n = arr.size();
        for (int cycleStart = 0; cycleStart <= n - 2; ++cycleStart) {
            CHECK_STOP();
            double item = arr[cycleStart];
            int pos = cycleStart;

            for (int i = cycleStart + 1; i < n; ++i) {
                if (arr[i] < item) pos++;
            }

            if (pos == cycleStart) continue;

            while (item == arr[pos]) pos++;

            if (pos != cycleStart) {
                std::swap(item, arr[pos]);
                FIRE_STEP(cycleStart, pos, -1);
            }

            while (pos != cycleStart) {
                CHECK_STOP();
                pos = cycleStart;
                for (int i = cycleStart + 1; i < n; ++i) {
                    if (arr[i] < item) pos++;
                }

                while (item == arr[pos]) pos++;

                if (item != arr[pos]) {
                    std::swap(item, arr[pos]);
                    FIRE_STEP(cycleStart, pos, -1);
                }
            }
        }
    }

} // namespace CPU
