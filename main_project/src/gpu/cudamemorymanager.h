////////////////////////////////////////////////////////////////////////////////
// gpu/cudamemorymanager.h — менеджер памяти CUDA (RAII-обёртки)
//
// НАЗНАЧЕНИЕ:
//   Предоставляет RAII-классы для безопасного управления device и pinned памятью.
//   Исключает утечки при исключениях или ранних return.
//
// ШАБЛОНЫ:
//
//   DeviceBuffer<T>:
//     DeviceBuffer(size_t count)   — cudaMalloc(count * sizeof(T))
//     ~DeviceBuffer()              — cudaFree
//     T* get() const               — указатель на device-память
//     size_t size() const          — число элементов
//     size_t bytes() const         — байт
//     void upload(const T* host, cudaStream_t = 0)   — cudaMemcpyAsync H2D
//     void download(T* host, cudaStream_t = 0) const — cudaMemcpyAsync D2H
//     DeviceBuffer(DeviceBuffer&&) noexcept  — move-конструктор
//     DeviceBuffer(const DeviceBuffer&) = delete  — некопируемый
//
//   PinnedBuffer<T>:
//     PinnedBuffer(size_t count)   — cudaMallocHost
//     ~PinnedBuffer()              — cudaFreeHost
//     T* get() const
//     Позволяет асинхронную H2D/D2H передачу на максимальной скорости.
//
//   CudaStream:
//     CudaStream()                 — cudaStreamCreate
//     ~CudaStream()                — cudaStreamDestroy
//     cudaStream_t get() const
//     void synchronize()           — cudaStreamSynchronize
//
//   CudaEvent:
//     CudaEvent()                  — cudaEventCreate
//     ~CudaEvent()                 — cudaEventDestroy
//     void record(CudaStream&)     — cudaEventRecord
//     float elapsedMs(CudaEvent& start) — cudaEventElapsedTime
////////////////////////////////////////////////////////////////////////////////

#ifndef CUDA_MEMORY_MANAGER_H
#define CUDA_MEMORY_MANAGER_H

#include <cuda_runtime.h>
#include <stdexcept>
#include <string>

// Исключение для ошибок CUDA
class CudaException : public std::runtime_error {
public:
    explicit CudaException(cudaError_t error, const std::string& context = "")
        : std::runtime_error(context + ": " + cudaGetErrorString(error))
        , errorCode(error) {}
    
    cudaError_t errorCode;
};

// Макрос для проверки ошибок CUDA
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            throw CudaException(err, #call); \
        } \
    } while(0)

// Буфер в device-памяти
template<typename T>
class DeviceBuffer {
public:
    explicit DeviceBuffer(size_t count) 
        : mData(nullptr)
        , mSize(count)
        , mOwned(true) {
        if (count > 0) {
            CUDA_CHECK(cudaMalloc(&mData, count * sizeof(T)));
        }
    }
    
    // Конструктор из существующего указателя
    DeviceBuffer(T* ptr, size_t count, bool owned = true)
        : mData(ptr)
        , mSize(count)
        , mOwned(owned) {}
    
    ~DeviceBuffer() {
        if (mOwned && mData) {
            cudaFree(mData);
        }
    }
    
    // Move конструктор
    DeviceBuffer(DeviceBuffer&& other) noexcept
        : mData(other.mData)
        , mSize(other.mSize)
        , mOwned(other.mOwned) {
        other.mData = nullptr;
        other.mSize = 0;
        other.mOwned = false;
    }
    
    // Move оператор присваивания
    DeviceBuffer& operator=(DeviceBuffer&& other) noexcept {
        if (this != &other) {
            if (mOwned && mData) {
                cudaFree(mData);
            }
            mData = other.mData;
            mSize = other.mSize;
            mOwned = other.mOwned;
            other.mData = nullptr;
            other.mSize = 0;
            other.mOwned = false;
        }
        return *this;
    }
    
    // Запрет копирования
    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
    
    // Получить указатель
    T* get() const { return mData; }
    T* data() const { return mData; }
    
    // Оператор разыменования
    T* operator->() const { return mData; }
    
    // Размер в элементах
    size_t size() const { return mSize; }
    
    // Размер в байтах
    size_t bytes() const { return mSize * sizeof(T); }
    
    // Загрузка данных с хоста
    void upload(const T* host, cudaStream_t stream = 0) const {
        if (mData && host) {
            CUDA_CHECK(cudaMemcpyAsync(mData, host, bytes(), 
                                      cudaMemcpyHostToDevice, stream));
        }
    }
    
    // Выгрузка данных на хост
    void download(T* host, cudaStream_t stream = 0) const {
        if (mData && host) {
            CUDA_CHECK(cudaMemcpyAsync(host, mData, bytes(),
                                      cudaMemcpyDeviceToHost, stream));
        }
    }
    
    // Заполнить значением
    void fill(T value, cudaStream_t stream = 0) const {
        if (mData) {
            CUDA_CHECK(cudaMemsetAsync(mData, static_cast<int>(value), 
                                      bytes(), stream));
        }
    }
    
    // Освободить память досрочно
    void release() {
        if (mOwned && mData) {
            cudaFree(mData);
            mData = nullptr;
            mSize = 0;
        }
    }
    
    // Проверка на пустоту
    bool empty() const { return mData == nullptr || mSize == 0; }
    
private:
    T* mData;
    size_t mSize;
    bool mOwned;
};

// Буфер в pinned-памяти (page-locked)
template<typename T>
class PinnedBuffer {
public:
    explicit PinnedBuffer(size_t count)
        : mData(nullptr)
        , mSize(count)
        , mOwned(true) {
        if (count > 0) {
            CUDA_CHECK(cudaMallocHost(&mData, count * sizeof(T)));
        }
    }
    
    PinnedBuffer(T* ptr, size_t count, bool owned = true)
        : mData(ptr)
        , mSize(count)
        , mOwned(owned) {}
    
    ~PinnedBuffer() {
        if (mOwned && mData) {
            cudaFreeHost(mData);
        }
    }
    
    // Move конструктор
    PinnedBuffer(PinnedBuffer&& other) noexcept
        : mData(other.mData)
        , mSize(other.mSize)
        , mOwned(other.mOwned) {
        other.mData = nullptr;
        other.mSize = 0;
        other.mOwned = false;
    }
    
    // Move оператор присваивания
    PinnedBuffer& operator=(PinnedBuffer&& other) noexcept {
        if (this != &other) {
            if (mOwned && mData) {
                cudaFreeHost(mData);
            }
            mData = other.mData;
            mSize = other.mSize;
            mOwned = other.mOwned;
            other.mData = nullptr;
            other.mSize = 0;
            other.mOwned = false;
        }
        return *this;
    }
    
    // Запрет копирования
    PinnedBuffer(const PinnedBuffer&) = delete;
    PinnedBuffer& operator=(const PinnedBuffer&) = delete;
    
    T* get() const { return mData; }
    T* data() const { return mData; }
    T& operator[](size_t i) const { return mData[i]; }
    
    size_t size() const { return mSize; }
    size_t bytes() const { return mSize * sizeof(T); }
    
    void release() {
        if (mOwned && mData) {
            cudaFreeHost(mData);
            mData = nullptr;
            mSize = 0;
        }
    }
    
    bool empty() const { return mData == nullptr || mSize == 0; }
    
private:
    T* mData;
    size_t mSize;
    bool mOwned;
};

// Обёртка для CUDA stream
class CudaStream {
public:
    CudaStream() : mStream(nullptr), mOwned(true) {
        CUDA_CHECK(cudaStreamCreate(&mStream));
    }
    
    explicit CudaStream(cudaStream_t stream, bool owned = false)
        : mStream(stream), mOwned(owned) {}
    
    ~CudaStream() {
        if (mOwned && mStream) {
            cudaStreamDestroy(mStream);
        }
    }
    
    // Move конструктор
    CudaStream(CudaStream&& other) noexcept
        : mStream(other.mStream)
        , mOwned(other.mOwned) {
        other.mStream = nullptr;
        other.mOwned = false;
    }
    
    // Move оператор присваивания
    CudaStream& operator=(CudaStream&& other) noexcept {
        if (this != &other) {
            if (mOwned && mStream) {
                cudaStreamDestroy(mStream);
            }
            mStream = other.mStream;
            mOwned = other.mOwned;
            other.mStream = nullptr;
            other.mOwned = false;
        }
        return *this;
    }
    
    // Запрет копирования
    CudaStream(const CudaStream&) = delete;
    CudaStream& operator=(const CudaStream&) = delete;
    
    cudaStream_t get() const { return mStream; }
    
    void synchronize() const {
        if (mStream) {
            CUDA_CHECK(cudaStreamSynchronize(mStream));
        }
    }
    
    bool empty() const { return mStream == nullptr; }
    
private:
    cudaStream_t mStream;
    bool mOwned;
};

// Обёртка для CUDA event
class CudaEvent {
public:
    CudaEvent() : mEvent(nullptr), mOwned(true) {
        CUDA_CHECK(cudaEventCreate(&mEvent));
    }
    
    explicit CudaEvent(cudaEvent_t event, bool owned = false)
        : mEvent(event), mOwned(owned) {}
    
    ~CudaEvent() {
        if (mOwned && mEvent) {
            cudaEventDestroy(mEvent);
        }
    }
    
    // Move конструктор
    CudaEvent(CudaEvent&& other) noexcept
        : mEvent(other.mEvent)
        , mOwned(other.mOwned) {
        other.mEvent = nullptr;
        other.mOwned = false;
    }
    
    // Move оператор присваивания
    CudaEvent& operator=(CudaEvent&& other) noexcept {
        if (this != &other) {
            if (mOwned && mEvent) {
                cudaEventDestroy(mEvent);
            }
            mEvent = other.mEvent;
            mOwned = other.mOwned;
            other.mEvent = nullptr;
            other.mOwned = false;
        }
        return *this;
    }
    
    // Запрет копирования
    CudaEvent(const CudaEvent&) = delete;
    CudaEvent& operator=(const CudaEvent&) = delete;
    
    cudaEvent_t get() const { return mEvent; }
    
    void record(const CudaStream& stream) const {
        if (mEvent && stream.get()) {
            CUDA_CHECK(cudaEventRecord(mEvent, stream.get()));
        } else if (mEvent) {
            CUDA_CHECK(cudaEventRecord(mEvent, nullptr));
        }
    }
    
    float elapsedMs(const CudaEvent& start) const {
        float ms = 0.0f;
        if (mEvent && start.get()) {
            CUDA_CHECK(cudaEventElapsedTime(&ms, start.get(), mEvent));
        }
        return ms;
    }
    
    void synchronize() const {
        if (mEvent) {
            CUDA_CHECK(cudaEventSynchronize(mEvent));
        }
    }
    
    bool query() const {
        if (!mEvent) return true;
        cudaError_t err = cudaEventQuery(mEvent);
        return err == cudaSuccess;
    }
    
    bool empty() const { return mEvent == nullptr; }
    
private:
    cudaEvent_t mEvent;
    bool mOwned;
};

#endif // CUDA_MEMORY_MANAGER_H
