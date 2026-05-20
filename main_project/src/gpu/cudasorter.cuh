#ifndef CUDASORTER_CUH
#define CUDASORTER_CUH

#include <QObject>
#include <QString>
#include <vector>
#include <cuda_runtime.h>
#include "sortparams.h"

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


class CudaSorter : public QObject {
    Q_OBJECT

public:
    explicit CudaSorter(int deviceIndex = 0, QObject* parent = nullptr);
    ~CudaSorter();

    template<typename T>
    GpuTimings sort(const T* hostInput, T* hostOutput, int size,
                    int gpuAlgo, int blockSize = 256, int numStreams = 4);

    size_t allocateDeviceBuffers(size_t bytes);
    void freeDeviceBuffers();
    bool isReady() const { return m_ready; }
    size_t currentDeviceMemUsed() const { return m_deviceMemUsed; }
    
private:
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

#endif // CUDASORTER_H
