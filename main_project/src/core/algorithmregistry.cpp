#include "algorithmregistry.h"

namespace SortBench {

AlgorithmRegistry::AlgorithmRegistry() {
    // CPU Algorithms
    m_cpuAlgorithms[static_cast<int>(CpuAlgorithm::BubbleSort)] = AlgorithmInfo{
        .name = "Bubble Sort",
        .shortName = "Bubble",
        .category = "CPU",
        .timeComplexity = "O(n²)",
        .spaceComplexity = "O(1)",
        .description = "Простейший алгоритм: многократные проходы с заменой соседних элементов. O(n²) — нагляден для обучения, непригоден для prod.",
        .stable = true,
        .inPlace = true,
        .parallelizable = false,
        .chartColor = QColor(255, 100, 100)
    };
    
    m_cpuAlgorithms[static_cast<int>(CpuAlgorithm::QuickSort)] = AlgorithmInfo{
        .name = "Quick Sort",
        .shortName = "Quick",
        .category = "CPU",
        .timeComplexity = "O(n log n)",
        .spaceComplexity = "O(log n)",
        .description = "Разделяй и властвуй с трёхпутевым partition (Dutch flag). O(n log n) avg, O(n²) worst. In-place, нестабильный.",
        .stable = false,
        .inPlace = true,
        .parallelizable = true,
        .chartColor = QColor(100, 200, 100)
    };
    
    m_cpuAlgorithms[static_cast<int>(CpuAlgorithm::MergeSort)] = AlgorithmInfo{
        .name = "Merge Sort",
        .shortName = "Merge",
        .category = "CPU",
        .timeComplexity = "O(n log n)",
        .spaceComplexity = "O(n)",
        .description = "Стабильный O(n log n). Рекурсивное слияние. O(n) доп. память.",
        .stable = true,
        .inPlace = false,
        .parallelizable = true,
        .chartColor = QColor(100, 150, 255)
    };
    
    m_cpuAlgorithms[static_cast<int>(CpuAlgorithm::HeapSort)] = AlgorithmInfo{
        .name = "Heap Sort",
        .shortName = "Heap",
        .category = "CPU",
        .timeComplexity = "O(n log n)",
        .spaceComplexity = "O(1)",
        .description = "In-place, O(n log n) worst. Max-heap + извлечение максимума.",
        .stable = false,
        .inPlace = true,
        .parallelizable = false,
        .chartColor = QColor(255, 200, 100)
    };
    
    m_cpuAlgorithms[static_cast<int>(CpuAlgorithm::RadixSort)] = AlgorithmInfo{
        .name = "Radix Sort",
        .shortName = "Radix",
        .category = "CPU",
        .timeComplexity = "O(d·n)",
        .spaceComplexity = "O(n)",
        .description = "Поразрядная сортировка (LSD). O(d·n). Только целые числа.",
        .stable = true,
        .inPlace = false,
        .parallelizable = true,
        .chartColor = QColor(200, 100, 255)
    };
    
    m_cpuAlgorithms[static_cast<int>(CpuAlgorithm::StdSort)] = AlgorithmInfo{
        .name = "std::sort",
        .shortName = "std::sort",
        .category = "CPU",
        .timeComplexity = "O(n log n)",
        .spaceComplexity = "O(log n)",
        .description = "Introsort: quick + heap + insertion. Оптимизирован в libstdc++.",
        .stable = false,
        .inPlace = true,
        .parallelizable = false,
        .chartColor = QColor(150, 150, 150)
    };
    
    // GPU Algorithms
    m_gpuAlgorithms[static_cast<int>(GpuAlgorithm::BitonicSort)] = AlgorithmInfo{
        .name = "Bitonic Sort",
        .shortName = "Bitonic",
        .category = "GPU",
        .timeComplexity = "O(log²n)",
        .spaceComplexity = "O(n)",
        .description = "GPU-оптимальный. O(log²n) фаз, каждый шаг полностью параллелен. Размер должен быть степенью двойки.",
        .stable = false,
        .inPlace = false,
        .parallelizable = true,
        .chartColor = QColor(255, 100, 255)
    };
    
    m_gpuAlgorithms[static_cast<int>(GpuAlgorithm::ThrustRadixSort)] = AlgorithmInfo{
        .name = "Thrust Radix Sort",
        .shortName = "Thrust",
        .category = "GPU",
        .timeComplexity = "O(n)",
        .spaceComplexity = "O(n)",
        .description = "NVIDIA Thrust/CUB Radix Sort. Один из быстрейших GPU-сортировщиков. Автоматически выбирает оптимальные параметры под архитектуру.",
        .stable = true,
        .inPlace = false,
        .parallelizable = true,
        .chartColor = QColor(100, 255, 150)
    };
    
    m_gpuAlgorithms[static_cast<int>(GpuAlgorithm::GpuQuickSort)] = AlgorithmInfo{
        .name = "GPU Quick Sort",
        .shortName = "GPU Quick",
        .category = "GPU",
        .timeComplexity = "O(n log n)",
        .spaceComplexity = "O(n)",
        .description = "GPU Quick Sort с параллельным partition через prefix scan. Эффективен на нестандартных распределениях.",
        .stable = false,
        .inPlace = false,
        .parallelizable = true,
        .chartColor = QColor(255, 200, 50)
    };
    
    m_gpuAlgorithms[static_cast<int>(GpuAlgorithm::CubDeviceSort)] = AlgorithmInfo{
        .name = "CUB Device Sort",
        .shortName = "CUB",
        .category = "GPU",
        .timeComplexity = "O(n)",
        .spaceComplexity = "O(n)",
        .description = "CUB DeviceRadixSort — низкоуровневый API, максимальная скорость при ручной настройке параметров блока и потоков.",
        .stable = true,
        .inPlace = false,
        .parallelizable = true,
        .chartColor = QColor(50, 200, 200)
    };
}

AlgorithmRegistry& AlgorithmRegistry::instance() {
    static AlgorithmRegistry registry;
    return registry;
}

AlgorithmInfo AlgorithmRegistry::getInfo(CpuAlgorithm algo) const {
    auto it = m_cpuAlgorithms.find(static_cast<int>(algo));
    if (it != m_cpuAlgorithms.end()) {
        return *it;
    }
    return AlgorithmInfo{};
}

AlgorithmInfo AlgorithmRegistry::getInfo(GpuAlgorithm algo) const {
    auto it = m_gpuAlgorithms.find(static_cast<int>(algo));
    if (it != m_gpuAlgorithms.end()) {
        return *it;
    }
    return AlgorithmInfo{};
}

QList<AlgorithmInfo> AlgorithmRegistry::allCpuAlgorithms() const {
    QList<AlgorithmInfo> result;
    for (int i = 0; i <= static_cast<int>(CpuAlgorithm::None); ++i) {
        auto it = m_cpuAlgorithms.find(i);
        if (it != m_cpuAlgorithms.end()) {
            result.append(*it);
        }
    }
    return result;
}

QList<AlgorithmInfo> AlgorithmRegistry::allGpuAlgorithms() const {
    QList<AlgorithmInfo> result;
    for (int i = 0; i <= static_cast<int>(GpuAlgorithm::None); ++i) {
        auto it = m_gpuAlgorithms.find(i);
        if (it != m_gpuAlgorithms.end()) {
            result.append(*it);
        }
    }
    return result;
}

} // namespace SortBench
