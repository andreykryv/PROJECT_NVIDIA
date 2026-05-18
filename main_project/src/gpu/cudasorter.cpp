#include "cudasorter.h"
#include <cuda_runtime.h>
#include <thrust/device_ptr.h>
#include <thrust/sort.h>
#include <cub/cub.cuh>
#include <stdexcept>
#include <QDebug>

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            throw std::runtime_error(std::string("CUDA error: ") + cudaGetErrorString(err)); \
        } \
    } while(0)

namespace SortBench {

// Forward declarations для wrapper функций
template<typename T>
void bitonicSortWrapper(T* d_data, int size, cudaStream_t stream);

template<typename T>
void thrustRadixSortWrapper(T* d_data, int size, cudaStream_t stream);

template<typename T>
void gpuQuickSortWrapper(T* d_data, int size, int blockSize, cudaStream_t stream);

template<typename T>
void cubDeviceSortWrapper(T* d_data, int size, cudaStream_t stream);

CudaSorter::CudaSorter(int deviceIndex, QObject* parent)
    : QObject(parent), m_deviceIndex(deviceIndex), m_ready(false), m_deviceMemUsed(0),
      m_d_input(nullptr), m_d_output(nullptr)
{
    try {
        m_ready = initCuda(deviceIndex);
    } catch (const std::exception& e) {
        qWarning() << "CUDA initialization failed:" << e.what();
        m_ready = false;
    }
}

CudaSorter::~CudaSorter() {
    freeDeviceBuffers();
    
    for (auto& stream : m_streams) {
        if (stream) cudaStreamDestroy(stream);
    }
    
    if (m_startEvent) cudaEventDestroy(m_startEvent);
    if (m_endEvent) cudaEventDestroy(m_endEvent);
}

bool CudaSorter::initCuda(int deviceIndex) {
    int deviceCount = 0;
    CUDA_CHECK(cudaGetDeviceCount(&deviceCount));
    if (deviceCount == 0) return false;
    
    CUDA_CHECK(cudaSetDevice(deviceIndex));
    
    // Создаем стримы
    int numStreams = 4;
    m_streams.resize(numStreams);
    for (int i = 0; i < numStreams; ++i) {
        CUDA_CHECK(cudaStreamCreate(&m_streams[i]));
    }
    
    // Создаем события для тайминга
    CUDA_CHECK(cudaEventCreate(&m_startEvent));
    CUDA_CHECK(cudaEventCreate(&m_endEvent));
    
    return true;
}

size_t CudaSorter::allocateDeviceBuffers(size_t bytes) {
    if (!m_ready) return 0;
    
    freeDeviceBuffers();
    
    CUDA_CHECK(cudaMalloc(&m_d_input, bytes));
    CUDA_CHECK(cudaMalloc(&m_d_output, bytes));
    
    m_deviceMemUsed = bytes * 2;
    return m_deviceMemUsed;
}

void CudaSorter::freeDeviceBuffers() {
    if (m_d_input) {
        cudaFree(m_d_input);
        m_d_input = nullptr;
    }
    if (m_d_output) {
        cudaFree(m_d_output);
        m_d_output = nullptr;
    }
    m_deviceMemUsed = 0;
}

template<typename T>
GpuTimings CudaSorter::sort(const T* hostInput, T* hostOutput, int size,
                            int gpuAlgo, int blockSize, int numStreams) {
    GpuTimings timings{};
    
    if (!m_ready || !hostInput || !hostOutput || size <= 0) {
        return timings;
    }
    
    size_t bytes = size * sizeof(T);
    allocateDeviceBuffers(bytes);
    
    cudaStream_t stream = m_streams.empty() ? 0 : m_streams[0];
    
    // H2D transfer
    CUDA_CHECK(cudaEventRecord(m_startEvent, stream));
    CUDA_CHECK(cudaMemcpyAsync(m_d_input, hostInput, bytes, cudaMemcpyHostToDevice, stream));
    CUDA_CHECK(cudaEventRecord(m_endEvent, stream));
    CUDA_CHECK(cudaEventSynchronize(m_endEvent));
    CUDA_CHECK(cudaEventElapsedTime(&timings.h2dMs, m_startEvent, m_endEvent));
    
    // Sorting
    CUDA_CHECK(cudaEventRecord(m_startEvent, stream));
    
    switch (static_cast<GpuAlgorithm>(gpuAlgo)) {
        case GpuAlgorithm::BitonicSort:
            bitonicSortWrapper<T>(reinterpret_cast<T*>(m_d_input), size, stream);
            break;
        case GpuAlgorithm::ThrustRadixSort:
            thrustRadixSortWrapper<T>(reinterpret_cast<T*>(m_d_input), size, stream);
            break;
        case GpuAlgorithm::GpuQuickSort:
            gpuQuickSortWrapper<T>(reinterpret_cast<T*>(m_d_input), size, blockSize, stream);
            break;
        case GpuAlgorithm::CubDeviceSort:
            cubDeviceSortWrapper<T>(reinterpret_cast<T*>(m_d_input), size, stream);
            break;
        default:
            thrustRadixSortWrapper<T>(reinterpret_cast<T*>(m_d_input), size, stream);
            break;
    }
    
    CUDA_CHECK(cudaEventRecord(m_endEvent, stream));
    CUDA_CHECK(cudaStreamSynchronize(stream));
    CUDA_CHECK(cudaEventElapsedTime(&timings.kernelMs, m_startEvent, m_endEvent));
    
    // D2H transfer
    CUDA_CHECK(cudaEventRecord(m_startEvent, stream));
    CUDA_CHECK(cudaMemcpyAsync(hostOutput, m_d_input, bytes, cudaMemcpyDeviceToHost, stream));
    CUDA_CHECK(cudaEventRecord(m_endEvent, stream));
    CUDA_CHECK(cudaEventSynchronize(m_endEvent));
    CUDA_CHECK(cudaEventElapsedTime(&timings.d2hMs, m_startEvent, m_endEvent));
    
    // Sync overhead
    auto syncStart = std::chrono::steady_clock::now();
    CUDA_CHECK(cudaDeviceSynchronize());
    auto syncEnd = std::chrono::steady_clock::now();
    timings.syncMs = std::chrono::duration<double, std::milli>(syncEnd - syncStart).count();
    
    return timings;
}

// Явные инстанции шаблона sort
template GpuTimings CudaSorter::sort<int32_t>(const int32_t*, int32_t*, int, int, int, int);
template GpuTimings CudaSorter::sort<int64_t>(const int64_t*, int64_t*, int, int, int, int);
template GpuTimings CudaSorter::sort<float>(const float*, float*, int, int, int, int);
template GpuTimings CudaSorter::sort<double>(const double*, double*, int, int, int, int);

} // namespace SortBench
