#ifndef SORTPARAMS_H
#define SORTPARAMS_H

#include <QString>
#include <QDebug>
#include <QUuid>
#include <QColor>

namespace SortBench {

enum class CpuAlgorithm { 
    BubbleSort = 0, 
    QuickSort = 1, 
    MergeSort = 2, 
    HeapSort = 3,
    RadixSort = 4, 
    StdSort = 5, 
    None = 6 
};

enum class GpuAlgorithm { 
    BitonicSort = 0, 
    ThrustRadixSort = 1, 
    GpuQuickSort = 2,
    CubDeviceSort = 3, 
    None = 4 
};

enum class DataType { 
    Int32 = 0, 
    Int64 = 1, 
    Float = 2, 
    Double = 3 
};

enum class Distribution { 
    RandomUniform = 0, 
    NearlySorted = 1, 
    ReverseSorted = 2,
    ManyDuplicates = 3, 
    Sawtooth = 4, 
    SteppedNoise = 5, 
    RandomNormal = 6 
};

enum class ColorSchemeType { Rainbow = 0, Heatmap = 1, Monochrome = 2, StatusColors = 3 };


struct SortParams {
    CpuAlgorithm cpuAlgorithm = CpuAlgorithm::QuickSort;
    GpuAlgorithm gpuAlgorithm = GpuAlgorithm::ThrustRadixSort;
    bool enableCPU = true;
    bool enableGPU = true;
    int arraySize = 100000;
    DataType dataType = DataType::Int32;
    Distribution distribution = Distribution::RandomUniform;
    unsigned int randomSeed = 42;
    bool autoSeed = false;
    int animationFPS = 60;
    bool showComparisons = true;
    bool showAccessCount = false;
    ColorSchemeType colorScheme = ColorSchemeType::Rainbow;
    int maxVisElements = 1000;
    int repeatCount = 1;
    bool excludeOutliers = false;
    int cudaBlockSize = 256;
    int cudaStreams = 4;
    bool usePinnedMemory = true;
};

// Вспомогательные функции
inline QString toString(CpuAlgorithm algo) {
    switch (algo) {
        case CpuAlgorithm::BubbleSort: return "Bubble Sort";
        case CpuAlgorithm::QuickSort: return "Quick Sort";
        case CpuAlgorithm::MergeSort: return "Merge Sort";
        case CpuAlgorithm::HeapSort: return "Heap Sort";
        case CpuAlgorithm::RadixSort: return "Radix Sort";
        case CpuAlgorithm::StdSort: return "std::sort";
        case CpuAlgorithm::None: return "None";
        default: return "Unknown";
    }
}

inline QString toString(GpuAlgorithm algo) {
    switch (algo) {
        case GpuAlgorithm::BitonicSort: return "Bitonic Sort";
        case GpuAlgorithm::ThrustRadixSort: return "Thrust Radix Sort";
        case GpuAlgorithm::GpuQuickSort: return "GPU Quick Sort";
        case GpuAlgorithm::CubDeviceSort: return "CUB Device Sort";
        case GpuAlgorithm::None: return "None";
        default: return "Unknown";
    }
}

inline QString toString(DataType dt) {
    switch (dt) {
        case DataType::Int32: return "int32";
        case DataType::Int64: return "int64";
        case DataType::Float: return "float32";
        case DataType::Double: return "float64";
        default: return "unknown";
    }
}

inline QString toString(Distribution dist) {
    switch (dist) {
        case Distribution::RandomUniform: return "Случайное равномерное";
        case Distribution::NearlySorted: return "Почти отсортированное";
        case Distribution::ReverseSorted: return "Обратно отсортированное";
        case Distribution::ManyDuplicates: return "Много повторений";
        case Distribution::Sawtooth: return "Пилообразное";
        case Distribution::SteppedNoise: return "Ступенчатый шум";
        case Distribution::RandomNormal: return "Нормальное распределение";
        default: return "Неизвестное";
    }
}

inline QString toString(ColorSchemeType cs) {
    switch (cs) {
        case ColorSchemeType::Rainbow:     return "Радуга";
        case ColorSchemeType::Heatmap:     return "Тепловая карта";
        case ColorSchemeType::Monochrome:  return "Монохром";
        case ColorSchemeType::StatusColors:return "Цвета статусов";
        default:                           return "Неизвестная";
    }
}

inline size_t elementSize(DataType dt) {
    switch (dt) {
        case DataType::Int32: return 4;
        case DataType::Int64: return 8;
        case DataType::Float: return 4;
        case DataType::Double: return 8;
        default: return 4;
    }
}

inline size_t arrayBytes(const SortParams& params) {
    return static_cast<size_t>(params.arraySize) * elementSize(params.dataType);
}

inline bool operator==(const SortParams& a, const SortParams& b) {
    return a.cpuAlgorithm == b.cpuAlgorithm &&
           a.gpuAlgorithm == b.gpuAlgorithm &&
           a.enableCPU == b.enableCPU &&
           a.enableGPU == b.enableGPU &&
           a.arraySize == b.arraySize &&
           a.dataType == b.dataType &&
           a.distribution == b.distribution &&
           a.randomSeed == b.randomSeed &&
           a.autoSeed == b.autoSeed &&
           a.animationFPS == b.animationFPS &&
           a.showComparisons == b.showComparisons &&
           a.showAccessCount == b.showAccessCount &&
           a.colorScheme == b.colorScheme &&
           a.maxVisElements == b.maxVisElements &&
           a.repeatCount == b.repeatCount &&
           a.excludeOutliers == b.excludeOutliers &&
           a.cudaBlockSize == b.cudaBlockSize &&
           a.cudaStreams == b.cudaStreams &&
           a.usePinnedMemory == b.usePinnedMemory;
}

inline bool operator!=(const SortParams& a, const SortParams& b) {
    return !(a == b);
}

inline QDebug operator<<(QDebug debug, const SortParams& params) {
    QDebugStateSaver saver(debug);
    debug.nospace() << "SortParams{"
                    << "cpu=" << toString(params.cpuAlgorithm)
                    << ", gpu=" << toString(params.gpuAlgorithm)
                    << ", size=" << params.arraySize
                    << ", type=" << toString(params.dataType)
                    << ", dist=" << toString(params.distribution)
                    << "}";
    return debug;
}

} // namespace SortBench

#endif // SORTPARAMS_H
