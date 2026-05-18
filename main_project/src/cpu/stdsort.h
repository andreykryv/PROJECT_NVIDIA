#ifndef STDSORT_H
#define STDSORT_H

#include <algorithm>
#include "cpusorter.h"

namespace SortBench {

template<typename T>
class StdSort {
public:
    static void sort(T* data, int size, CpuSorter::SortCallback callback, std::atomic<bool>& stop) {
        long long comparisons = 0;
        long long swaps = 0;

        if (size <= 1) {
            if (!stop.load() && callback) {
                callback(data, size, 0, -1, HighlightType::Sorted, comparisons, swaps);
            }
            return;
        }

        // std::sort - это "чёрный ящик", не вызываем callback в процессе
        // Используем кастомный компаратор для подсчёта сравнений
        struct CountingComparator {
            CpuSorter::SortCallback cb;
            long long& compCount;
            std::atomic<bool>& stopFlag;
            
            bool operator()(const T& a, const T& b) {
                ++compCount;
                if (cb) {
                    // Не можем передать индексы, так как std::sort не предоставляет их
                    // Просто сигнализируем о сравнении
                }
                return !stopFlag.load() && a < b;
            }
        };

        CountingComparator comp{callback, comparisons, stop};
        std::sort(data, data + size, comp);

        // После завершения: помечаем весь массив как отсортированный
        if (!stop.load() && callback) {
            for (int i = 0; i < size; ++i) {
                callback(data, size, i, -1, HighlightType::Sorted, comparisons, swaps);
            }
        }
    }
};

} // namespace SortBench

#endif // STDSORT_H
