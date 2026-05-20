#ifndef CUDASORTER_CUH
#define CUDASORTER_CUH

#include <vector>
#include <cuda_runtime.h>
#include "sortparams.h"

// Этот заголовок используется как в CUDA (.cu), так и в обычном C++ (.cpp) файлах
// В CUDA-файлах мы не можем использовать QObject, поэтому определяем только CUDA-часть

namespace SortBench {

struct GpuTimings {
    float h2dMs = 0.0f;      // Host-to-Device transfer time
    float kernelMs = 0.0f;   // Kernel execution time
    float d2hMs = 0.0f;      // Device-to-Host transfer time
    float syncMs = 0.0f;     // Synchronization overhead

    float totalMs() const {
        return h2dMs + kernelMs + d2hMs + syncMs;
    }
};

// Forward declarations for CUDA kernels (defined in .cu files)
template<typename T>
void bitonicSortWrapper(T* d_data, int size, cudaStream_t stream);

template<typename T>
void thrustRadixSortWrapper(T* d_data, int size, cudaStream_t stream);

template<typename T>
void gpuQuickSortWrapper(T* d_data, int size, int blockSize, cudaStream_t stream);

template<typename T>
void cubDeviceSortWrapper(T* d_data, int size, cudaStream_t stream);

// Базовый класс без зависимости от Qt для использования в CUDA коде
class CudaSorterBase {
public:
    explicit CudaSorterBase(int deviceIndex = 0);
    virtual ~CudaSorterBase();

    template<typename T>
    GpuTimings sort(const T* hostInput, T* hostOutput, int size,
                    int gpuAlgo, int blockSize = 256, int numStreams = 4);

    size_t allocateDeviceBuffers(size_t bytes);
    void freeDeviceBuffers();
    bool isReady() const { return m_ready; }
    size_t currentDeviceMemUsed() const { return m_deviceMemUsed; }
    
protected:
    int m_deviceIndex;
    bool m_ready;
    size_t m_deviceMemUsed;
    std::vector<cudaStream_t> m_streams;
    cudaEvent_t m_startEvent;
    cudaEvent_t m_endEvent;
    
    void* m_d_input;
    void* m_d_output;
    
    bool initCuda(int deviceIndex);
};

} // namespace SortBench

#endif // CUDASORTER_CUH
