#include "cudamemorymanager.h"
#include <cuda_runtime.h>
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

// DeviceBuffer реализация
template<typename T>
DeviceBuffer<T>::DeviceBuffer(size_t count) : m_count(count), m_data(nullptr) {
    if (count > 0) {
        CUDA_CHECK(cudaMalloc(&m_data, count * sizeof(T)));
    }
}

template<typename T>
DeviceBuffer<T>::~DeviceBuffer() {
    if (m_data) {
        cudaFree(m_data);
    }
}

template<typename T>
DeviceBuffer<T>::DeviceBuffer(DeviceBuffer&& other) noexcept 
    : m_count(other.m_count), m_data(other.m_data) {
    other.m_data = nullptr;
    other.m_count = 0;
}

template<typename T>
DeviceBuffer<T>& DeviceBuffer<T>::operator=(DeviceBuffer&& other) noexcept {
    if (this != &other) {
        if (m_data) {
            cudaFree(m_data);
        }
        m_data = other.m_data;
        m_count = other.m_count;
        other.m_data = nullptr;
        other.m_count = 0;
    }
    return *this;
}

template<typename T>
void DeviceBuffer<T>::upload(const T* host, cudaStream_t stream) {
    if (m_data && host && m_count > 0) {
        CUDA_CHECK(cudaMemcpyAsync(m_data, host, m_count * sizeof(T), 
                                   cudaMemcpyHostToDevice, stream));
    }
}

template<typename T>
void DeviceBuffer<T>::download(T* host, cudaStream_t stream) const {
    if (m_data && host && m_count > 0) {
        CUDA_CHECK(cudaMemcpyAsync(host, m_data, m_count * sizeof(T),
                                   cudaMemcpyDeviceToHost, stream));
    }
}

// PinnedBuffer реализация
template<typename T>
PinnedBuffer<T>::PinnedBuffer(size_t count) : m_count(count), m_data(nullptr) {
    if (count > 0) {
        CUDA_CHECK(cudaMallocHost(&m_data, count * sizeof(T)));
    }
}

template<typename T>
PinnedBuffer<T>::~PinnedBuffer() {
    if (m_data) {
        cudaFreeHost(m_data);
    }
}

template<typename T>
PinnedBuffer<T>::PinnedBuffer(PinnedBuffer&& other) noexcept
    : m_count(other.m_count), m_data(other.m_data) {
    other.m_data = nullptr;
    other.m_count = 0;
}

template<typename T>
PinnedBuffer<T>& PinnedBuffer<T>::operator=(PinnedBuffer&& other) noexcept {
    if (this != &other) {
        if (m_data) {
            cudaFreeHost(m_data);
        }
        m_data = other.m_data;
        m_count = other.m_count;
        other.m_data = nullptr;
        other.m_count = 0;
    }
    return *this;
}

// CudaStream реализация
CudaStream::CudaStream() {
    CUDA_CHECK(cudaStreamCreate(&m_stream));
}

CudaStream::~CudaStream() {
    if (m_stream) {
        cudaStreamDestroy(m_stream);
    }
}

void CudaStream::synchronize() {
    CUDA_CHECK(cudaStreamSynchronize(m_stream));
}

// CudaEvent реализация
CudaEvent::CudaEvent() {
    CUDA_CHECK(cudaEventCreate(&m_event));
}

CudaEvent::~CudaEvent() {
    if (m_event) {
        cudaEventDestroy(m_event);
    }
}

void CudaEvent::record(CudaStream& stream) {
    CUDA_CHECK(cudaEventRecord(m_event, stream.get()));
}

float CudaEvent::elapsedMs(CudaEvent& start) {
    float ms = 0.0f;
    CUDA_CHECK(cudaEventSynchronize(m_event));
    CUDA_CHECK(cudaEventElapsedTime(&ms, start.m_event, m_event));
    return ms;
}

// Явные инстанции шаблонов
template class DeviceBuffer<int32_t>;
template class DeviceBuffer<int64_t>;
template class DeviceBuffer<float>;
template class DeviceBuffer<double>;

template class PinnedBuffer<int32_t>;
template class PinnedBuffer<int64_t>;
template class PinnedBuffer<float>;
template class PinnedBuffer<double>;

} // namespace SortBench
