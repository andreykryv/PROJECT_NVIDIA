#ifndef RADIXSORT_H
#define RADIXSORT_H

#include <algorithm>
#include <vector>
#include <cstring>
#include "cpusorter.h"

namespace SortBench {

template<typename T>
class RadixSort {
private:
    // Извлечение байта из значения
    static inline uint8_t getByte(T value, int pass) {
        return (static_cast<uint32_t>(value) >> (pass * 8)) & 0xFF;
    }

    // Специализация для float - обработка знакового бита IEEE 754
    static inline uint32_t floatToOrderedFloat(float f) {
        uint32_t bits;
        std::memcpy(&bits, &f, sizeof(float));
        // Для положительных чисел: инвертируем знаковый бит
        // Для отрицательных: инвертируем все биты
        return (bits ^ ((static_cast<int32_t>(bits) >> 31) | 0x80000000u));
    }

    static inline float orderedFloatToFloat(uint32_t bits) {
        bits = (bits ^ ((static_cast<int32_t>(bits) >> 31) | 0x80000000u));
        float f;
        std::memcpy(&f, &bits, sizeof(float));
        return f;
    }

public:
    static void sort(T* data, int size, CpuSorter::SortCallback callback, std::atomic<bool>& stop) {
        if (size <= 1) return;

        long long comparisons = 0;
        long long swaps = 0;

        // Выделяем временный буфер
        std::vector<T> buffer(size);

        // 4 прохода для 32-битных значений (8 бит за проход)
        const int NUM_PASSES = sizeof(T) >= 4 ? 4 : sizeof(T);

        for (int pass = 0; pass < NUM_PASSES && !stop.load(); ++pass) {
            // Счётчики для каждого значения байта (0-255)
            int count[256] = {0};

            // Подсчёт частот
            for (int i = 0; i < size && !stop.load(); ++i) {
                uint8_t byte;
                if constexpr (std::is_floating_point_v<T>) {
                    if (pass == 0) {
                        uint32_t ordered = floatToOrderedFloat(data[i]);
                        byte = ordered & 0xFF;
                    } else {
                        uint32_t ordered = floatToOrderedFloat(data[i]);
                        byte = (ordered >> (pass * 8)) & 0xFF;
                    }
                } else {
                    byte = getByte(data[i], pass);
                }
                count[byte]++;
            }

            // Prefix sum - превращаем в начальные позиции
            int start[256];
            start[0] = 0;
            for (int i = 1; i < 256; ++i) {
                start[i] = start[i - 1] + count[i - 1];
            }

            // Копирование в буфер с сортировкой по текущему разряду
            for (int i = 0; i < size && !stop.load(); ++i) {
                uint8_t byte;
                if constexpr (std::is_floating_point_v<T>) {
                    if (pass == 0) {
                        uint32_t ordered = floatToOrderedFloat(data[i]);
                        byte = ordered & 0xFF;
                    } else {
                        uint32_t ordered = floatToOrderedFloat(data[i]);
                        byte = (ordered >> (pass * 8)) & 0xFF;
                    }
                } else {
                    byte = getByte(data[i], pass);
                }

                int pos = start[byte]++;
                buffer[pos] = data[i];
                ++swaps;

                // Сообщаем о записи
                if (callback) {
                    callback(buffer.data(), size, pos, -1, HighlightType::Swap, comparisons, swaps);
                }
            }

            // Копирование обратно в исходный массив
            for (int i = 0; i < size && !stop.load(); ++i) {
                data[i] = buffer[i];
                if (callback) {
                    callback(data, size, i, -1, HighlightType::Compare, comparisons, swaps);
                }
            }
        }

        // Если это float, нужно конвертировать обратно (если мы модифицировали представления)
        // В нашей реализации мы не модифицируем сами данные, только сортируем

        // Помечаем весь массив как отсортированный
        if (!stop.load() && callback) {
            for (int i = 0; i < size; ++i) {
                callback(data, size, i, -1, HighlightType::Sorted, comparisons, swaps);
            }
        }
    }
};

// Явные специализации для целочисленных типов
template<>
inline void RadixSort<int32_t>::sort(int32_t* data, int size, CpuSorter::SortCallback callback, std::atomic<bool>& stop) {
    if (size <= 1) return;

    long long comparisons = 0;
    long long swaps = 0;

    std::vector<int32_t> buffer(size);
    std::vector<uint32_t> udata(size);

    // Конвертируем в unsigned для корректной обработки знака
    for (int i = 0; i < size; ++i) {
        udata[i] = static_cast<uint32_t>(data[i]) ^ 0x80000000u;
    }

    const int NUM_PASSES = 4;

    for (int pass = 0; pass < NUM_PASSES && !stop.load(); ++pass) {
        int count[256] = {0};

        for (int i = 0; i < size && !stop.load(); ++i) {
            uint8_t byte = (udata[i] >> (pass * 8)) & 0xFF;
            count[byte]++;
        }

        int start[256];
        start[0] = 0;
        for (int i = 1; i < 256; ++i) {
            start[i] = start[i - 1] + count[i - 1];
        }

        for (int i = 0; i < size && !stop.load(); ++i) {
            uint8_t byte = (udata[i] >> (pass * 8)) & 0xFF;
            int pos = start[byte]++;
            buffer[pos] = static_cast<int32_t>(udata[i] ^ 0x80000000u);
            ++swaps;

            if (callback) {
                callback(buffer.data(), size, pos, -1, HighlightType::Swap, comparisons, swaps);
            }
        }

        for (int i = 0; i < size && !stop.load(); ++i) {
            data[i] = buffer[i];
            udata[i] = static_cast<uint32_t>(data[i]) ^ 0x80000000u;
            if (callback) {
                callback(data, size, i, -1, HighlightType::Compare, comparisons, swaps);
            }
        }
    }

    if (!stop.load() && callback) {
        for (int i = 0; i < size; ++i) {
            callback(data, size, i, -1, HighlightType::Sorted, comparisons, swaps);
        }
    }
}

template<>
inline void RadixSort<float>::sort(float* data, int size, CpuSorter::SortCallback callback, std::atomic<bool>& stop) {
    if (size <= 1) return;

    long long comparisons = 0;
    long long swaps = 0;

    std::vector<float> buffer(size);
    std::vector<uint32_t> udata(size);

    // Конвертируем float в упорядоченное представление
    for (int i = 0; i < size; ++i) {
        udata[i] = floatToOrderedFloat(data[i]);
    }

    const int NUM_PASSES = 4;

    for (int pass = 0; pass < NUM_PASSES && !stop.load(); ++pass) {
        int count[256] = {0};

        for (int i = 0; i < size && !stop.load(); ++i) {
            uint8_t byte = (udata[i] >> (pass * 8)) & 0xFF;
            count[byte]++;
        }

        int start[256];
        start[0] = 0;
        for (int i = 1; i < 256; ++i) {
            start[i] = start[i - 1] + count[i - 1];
        }

        for (int i = 0; i < size && !stop.load(); ++i) {
            uint8_t byte = (udata[i] >> (pass * 8)) & 0xFF;
            int pos = start[byte]++;
            buffer[pos] = orderedFloatToFloat(udata[i]);
            ++swaps;

            if (callback) {
                callback(buffer.data(), size, pos, -1, HighlightType::Swap, comparisons, swaps);
            }
        }

        for (int i = 0; i < size && !stop.load(); ++i) {
            data[i] = buffer[i];
            udata[i] = floatToOrderedFloat(data[i]);
            if (callback) {
                callback(data, size, i, -1, HighlightType::Compare, comparisons, swaps);
            }
        }
    }

    if (!stop.load() && callback) {
        for (int i = 0; i < size; ++i) {
            callback(data, size, i, -1, HighlightType::Sorted, comparisons, swaps);
        }
    }
}

} // namespace SortBench

#endif // RADIXSORT_H
