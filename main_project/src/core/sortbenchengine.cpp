#include "sortbenchengine.h"
#include <QCoreApplication>
#include <QThread>
#include <QDateTime>
#include <QSysInfo>
#include "cpu/bubblesort.h"
#include "cpu/quicksort.h"
#include "cpu/mergesort.h"
#include "cpu/heapsort.h"
#include "cpu/radixsort.h"
#include "cpu/stdsort.h"

#ifdef USE_CUDA
#include "cudasorter.h"
#include <cuda_runtime.h>
#endif

namespace SortBench {

SortBenchEngine::SortBenchEngine(QObject *parent)
    : QObject(parent), m_cpuSorter(new CpuSorter()), m_gpuSorter(nullptr)
{
    // Инициализация GPU сортировщика только если CUDA доступна
    #ifdef USE_CUDA
    try {
        m_gpuSorter = new CudaSorter(0);
    } catch (...) {
        emit logMessage("CUDA не доступна, GPU сортировка отключена", 1);
    }
    #endif
    
    m_gpuMemPollTimer = new QTimer(this);
    connect(m_gpuMemPollTimer, &QTimer::timeout, this, &SortBenchEngine::pollGpuMemory);
    m_gpuMemPollTimer->start(500);
}

SortBenchEngine::~SortBenchEngine() {
    delete m_cpuSorter;
#ifdef USE_CUDA
    delete m_gpuSorter;
#endif
}

void SortBenchEngine::startBenchmark(const SortParams& params) {
    m_currentParams = params;
    m_stopRequested = false;
    m_pauseRequested = false;
    m_partialResult = BenchmarkResult();
    m_partialResult.params = params;
    
    setState(State::GeneratingArray);
    generateArray();
}

void SortBenchEngine::stopBenchmark() {
    m_stopRequested = true;
    setState(State::Stopped);
    emit benchmarkStopped();
}

void SortBenchEngine::pauseBenchmark() {
    m_pauseRequested = true;
    setState(State::Paused);
}

void SortBenchEngine::resumeBenchmark() {
    m_pauseRequested = false;
    setState(State::SortingCPU);
}

void SortBenchEngine::setState(State newState) {
    m_state = newState;
    emit stateChanged(newState);
}

void SortBenchEngine::generateArray() {
    m_stageTimer.start();
    
    size_t bytes = arrayBytes(m_currentParams);
    m_elementSize = elementSize(m_currentParams.dataType);
    m_originalData.resize(bytes);
    
    switch (m_currentParams.dataType) {
        case DataType::Int32: {
            auto data = ArrayGenerator::generate<int32_t>(
                m_currentParams.arraySize, 
                m_currentParams.distribution, 
                m_currentParams.randomSeed);
            std::memcpy(m_originalData.data(), data.data(), bytes);
            break;
        }
        case DataType::Int64: {
            auto data = ArrayGenerator::generate<int64_t>(
                m_currentParams.arraySize, 
                m_currentParams.distribution, 
                m_currentParams.randomSeed);
            std::memcpy(m_originalData.data(), data.data(), bytes);
            break;
        }
        case DataType::Float: {
            auto data = ArrayGenerator::generate<float>(
                m_currentParams.arraySize, 
                m_currentParams.distribution, 
                m_currentParams.randomSeed);
            std::memcpy(m_originalData.data(), data.data(), bytes);
            break;
        }
        case DataType::Double: {
            auto data = ArrayGenerator::generate<double>(
                m_currentParams.arraySize, 
                m_currentParams.distribution, 
                m_currentParams.randomSeed);
            std::memcpy(m_originalData.data(), data.data(), bytes);
            break;
        }
    }
    
    m_partialResult.arrayGenerationTimeMs = m_stageTimer.elapsed();
    emit progressUpdated(10, "Генерация массива завершена");
    
    // Запускаем CPU сортировку
    runCpuSort();
}

void SortBenchEngine::runCpuSort() {
    if (m_stopRequested) return;
    
    setState(State::SortingCPU);
    m_stageTimer.start();
    
    m_cpuResult = m_originalData;
    
    auto callback = [this](const void* data, int size, int idx1, int idx2, 
                           HighlightType type, long long cmp, long long swp) {
        // Ограничение частоты кадров для анимации
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - m_lastFrameTime < m_frameIntervalMs) return;
        m_lastFrameTime = now;
        
        VisFrame frame;
        frame.totalElements = size;
        frame.comparisons = cmp;
        frame.swaps = swp;
        frame.highlightType = type;
        if (idx1 >= 0) frame.highlightedIdx.append(idx1);
        if (idx2 >= 0) frame.highlightedIdx.append(idx2);
        
        // Нормализация данных для визуализации
        switch (m_currentParams.dataType) {
            case DataType::Int32: {
                const auto* typedData = static_cast<const int32_t*>(data);
                auto [minVal, maxVal] = ArrayGenerator::minMax(
                    std::vector<int32_t>(typedData, typedData + size));
                float range = maxVal - minVal;
                if (range > 0) {
                    for (int i = 0; i < size; ++i) {
                        frame.normalizedValues.push_back(
                            static_cast<float>(typedData[i] - minVal) / range);
                    }
                }
                break;
            }
            case DataType::Int64: {
                const auto* typedData = static_cast<const int64_t*>(data);
                auto [minVal, maxVal] = ArrayGenerator::minMax(
                    std::vector<int64_t>(typedData, typedData + size));
                float range = maxVal - minVal;
                if (range > 0) {
                    for (int i = 0; i < size; ++i) {
                        frame.normalizedValues.push_back(
                            static_cast<float>(typedData[i] - minVal) / range);
                    }
                }
                break;
            }
            case DataType::Float: {
                const auto* typedData = static_cast<const float*>(data);
                auto [minVal, maxVal] = ArrayGenerator::minMax(
                    std::vector<float>(typedData, typedData + size));
                float range = maxVal - minVal;
                if (range > 0) {
                    for (int i = 0; i < size; ++i) {
                        frame.normalizedValues.push_back(
                            (typedData[i] - minVal) / range);
                    }
                }
                break;
            }
            case DataType::Double: {
                const auto* typedData = static_cast<const double*>(data);
                auto [minVal, maxVal] = ArrayGenerator::minMax(
                    std::vector<double>(typedData, typedData + size));
                float range = maxVal - minVal;
                if (range > 0) {
                    for (int i = 0; i < size; ++i) {
                        frame.normalizedValues.push_back(
                            static_cast<float>((typedData[i] - minVal) / range));
                    }
                }
                break;
            }
        }
        
        emit frameReady(frame);
    };
    
    // Диспатч на нужный алгоритм
    switch (m_currentParams.cpuAlgorithm) {
        case CpuAlgorithm::BubbleSort:
            switch (m_currentParams.dataType) {
                case DataType::Int32:
                    BubbleSort<int32_t>::sort(
                        reinterpret_cast<int32_t*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Int64:
                    BubbleSort<int64_t>::sort(
                        reinterpret_cast<int64_t*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Float:
                    BubbleSort<float>::sort(
                        reinterpret_cast<float*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Double:
                    BubbleSort<double>::sort(
                        reinterpret_cast<double*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
            }
            break;
        case CpuAlgorithm::QuickSort:
            switch (m_currentParams.dataType) {
                case DataType::Int32:
                    QuickSort<int32_t>::sort(
                        reinterpret_cast<int32_t*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Int64:
                    QuickSort<int64_t>::sort(                        reinterpret_cast<int64_t*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Float:
                    QuickSort<float>::sort(
                        reinterpret_cast<float*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Double:
                    QuickSort<double>::sort(
                        reinterpret_cast<double*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
            }
            break;
        case CpuAlgorithm::MergeSort:
            switch (m_currentParams.dataType) {
                case DataType::Int32:
                    MergeSort<int32_t>::sort(
                        reinterpret_cast<int32_t*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Int64:
                    MergeSort<int64_t>::sort(
                        reinterpret_cast<int64_t*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Float:
                    MergeSort<float>::sort(
                        reinterpret_cast<float*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Double:
                    MergeSort<double>::sort(
                        reinterpret_cast<double*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
            }
            break;
        case CpuAlgorithm::HeapSort:
            switch (m_currentParams.dataType) {
                case DataType::Int32:
                    HeapSort<int32_t>::sort(
                        reinterpret_cast<int32_t*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Int64:
                    HeapSort<int64_t>::sort(
                        reinterpret_cast<int64_t*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Float:
                    HeapSort<float>::sort(
                        reinterpret_cast<float*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Double:
                    HeapSort<double>::sort(
                        reinterpret_cast<double*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
            }
            break;
        case CpuAlgorithm::RadixSort:
            // RadixSort только для целых типов
            if (m_currentParams.dataType == DataType::Int32) {
                RadixSort<int32_t>::sort(
                    reinterpret_cast<int32_t*>(m_cpuResult.data()),
                    m_currentParams.arraySize, callback, m_stopRequested);
            } else if (m_currentParams.dataType == DataType::Int64) {
                RadixSort<int64_t>::sort(
                    reinterpret_cast<int64_t*>(m_cpuResult.data()),
                    m_currentParams.arraySize, callback, m_stopRequested);
            } else {
                // Для float/double используем std::sort
                StdSort<float>::sort(
                    reinterpret_cast<float*>(m_cpuResult.data()),
                    m_currentParams.arraySize, callback, m_stopRequested);
            }
            break;
        case CpuAlgorithm::StdSort:
            switch (m_currentParams.dataType) {
                case DataType::Int32:
                    StdSort<int32_t>::sort(
                        reinterpret_cast<int32_t*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Int64:
                    StdSort<int64_t>::sort(
                        reinterpret_cast<int64_t*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Float:
                    StdSort<float>::sort(
                        reinterpret_cast<float*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
                case DataType::Double:
                    StdSort<double>::sort(
                        reinterpret_cast<double*>(m_cpuResult.data()),
                        m_currentParams.arraySize, callback, m_stopRequested);
                    break;
            }
            break;
        default:
            break;
    }
    
    m_partialResult.cpuTimeMs = m_stageTimer.elapsed();
    emit progressUpdated(55, "CPU сортировка завершена");
    
    // Запускаем GPU сортировку
    runGpuSort();
}

void SortBenchEngine::runGpuSort() {
    if (m_stopRequested || !m_gpuSorter) {
        verifyResults();
        return;
    }
    
    setState(State::TransferringH2D);
    m_stageTimer.start();
    
    m_gpuResult = m_originalData;
    
    GpuTimings timings{};
    
    #ifdef USE_CUDA
    switch (m_currentParams.dataType) {
        case DataType::Int32:
            timings = m_gpuSorter->sort<int32_t>(
                reinterpret_cast<const int32_t*>(m_originalData.data()),
                reinterpret_cast<int32_t*>(m_gpuResult.data()),
                m_currentParams.arraySize,
                static_cast<int>(m_currentParams.gpuAlgorithm),
                m_currentParams.cudaBlockSize,
                m_currentParams.cudaStreams);
            break;
        case DataType::Int64:
            timings = m_gpuSorter->sort<int64_t>(
                reinterpret_cast<const int64_t*>(m_originalData.data()),
                reinterpret_cast<int64_t*>(m_gpuResult.data()),
                m_currentParams.arraySize,
                static_cast<int>(m_currentParams.gpuAlgorithm),
                m_currentParams.cudaBlockSize,
                m_currentParams.cudaStreams);
            break;
        case DataType::Float:
            timings = m_gpuSorter->sort<float>(
                reinterpret_cast<const float*>(m_originalData.data()),
                reinterpret_cast<float*>(m_gpuResult.data()),
                m_currentParams.arraySize,
                static_cast<int>(m_currentParams.gpuAlgorithm),
                m_currentParams.cudaBlockSize,
                m_currentParams.cudaStreams);
            break;
        case DataType::Double:
            timings = m_gpuSorter->sort<double>(
                reinterpret_cast<const double*>(m_originalData.data()),
                reinterpret_cast<double*>(m_gpuResult.data()),
                m_currentParams.arraySize,
                static_cast<int>(m_currentParams.gpuAlgorithm),
                m_currentParams.cudaBlockSize,
                m_currentParams.cudaStreams);
            break;
    }
    #endif
    
    m_partialResult.gpuH2DTimeMs = timings.h2dMs;
    m_partialResult.gpuKernelTimeMs = timings.kernelMs;
    m_partialResult.gpuD2HTimeMs = timings.d2hMs;
    m_partialResult.gpuSyncOverheadMs = timings.syncMs;
    m_partialResult.gpuTotalTimeMs = timings.totalMs();
    
    emit progressUpdated(85, "GPU сортировка завершена");
    
    verifyResults();
}

void SortBenchEngine::verifyResults() {
    if (m_stopRequested) return;
    
    setState(State::Verifying);
    
    // Проверка CPU результата
    switch (m_currentParams.dataType) {
        case DataType::Int32: {
            size_t n = m_currentParams.arraySize;
            std::vector<int32_t> typed(n);
            std::memcpy(typed.data(), m_cpuResult.data(), n * sizeof(int32_t));
            m_partialResult.isSorted = ArrayGenerator::isSorted(typed);
            break;
        }
        case DataType::Int64: {
            size_t n = m_currentParams.arraySize;
            std::vector<int64_t> typed(n);
            std::memcpy(typed.data(), m_cpuResult.data(), n * sizeof(int64_t));
            m_partialResult.isSorted = ArrayGenerator::isSorted(typed);
            break;
        }
        case DataType::Float: {
            size_t n = m_currentParams.arraySize;
            std::vector<float> typed(n);
            std::memcpy(typed.data(), m_cpuResult.data(), n * sizeof(float));
            m_partialResult.isSorted = ArrayGenerator::isSorted(typed);
            break;
        }
        case DataType::Double: {
            size_t n = m_currentParams.arraySize;
            std::vector<double> typed(n);
            std::memcpy(typed.data(), m_cpuResult.data(), n * sizeof(double));
            m_partialResult.isSorted = ArrayGenerator::isSorted(typed);
            break;
        }
    }
    
    // Сравнение CPU и GPU результатов
    m_partialResult.cpuGpuMatch = (m_cpuResult == m_gpuResult);
    
    collectSystemInfo();
    
    emit progressUpdated(100, "Завершено");
    setState(State::Finished);
    emit benchmarkFinished(m_partialResult);
}

void SortBenchEngine::collectSystemInfo() {
    m_partialResult.qtVersion = QString::fromUtf8(qVersion());
    
    #ifdef USE_CUDA
    if (m_gpuSorter) {
        // Получаем информацию о GPU через CudaDeviceInfo
        // Это упрощённая версия - в реальности нужно вызвать cudaGetDeviceProperties
        cudaDeviceProp prop;
        if (cudaGetDeviceProperties(&prop, 0) == cudaSuccess) {
            m_partialResult.gpuName = QString::fromUtf8(prop.name);
            m_partialResult.gpuComputeCapMajor = prop.major;
            m_partialResult.gpuComputeCapMinor = prop.minor;
            m_partialResult.gpuVramUsedBytes = prop.totalGlobalMem;
        }
        m_partialResult.cudaVersion = "CUDA available";
    }
    #endif
    
    // CPU name (упрощённо)
    m_partialResult.cpuName = QSysInfo::currentCpuArchitecture();
}

void SortBenchEngine::pollGpuMemory() {
#ifdef USE_CUDA
    if (m_gpuSorter && isRunning()) {
        size_t used = m_gpuSorter->currentDeviceMemUsed();
        size_t total = 0;
        cudaMemGetInfo(nullptr, &total);
        emit gpuMemoryUpdated(used, total);
    }
#endif
}

void SortBenchEngine::checkPausePoint() {
    while (m_pauseRequested.load() && !m_stopRequested.load()) {
        QThread::msleep(10);
        QCoreApplication::processEvents();
    }
}

} // namespace SortBench
