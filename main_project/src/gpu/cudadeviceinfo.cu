#include "cudadeviceinfo.h"
#include <cuda_runtime.h>
#include <QDebug>

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            qWarning() << "CUDA error:" << cudaGetErrorString(err); \
            return {}; \
        } \
    } while(0)

namespace SortBench {

int CudaDeviceInfo::deviceCount() {
    int count = 0;
    CUDA_CHECK(cudaGetDeviceCount(&count));
    return count;
}

bool CudaDeviceInfo::isCudaAvailable() {
    return deviceCount() > 0;
}

CudaDeviceProperties CudaDeviceInfo::getProperties(int deviceIndex) {
    CudaDeviceProperties props{};
    
    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, deviceIndex));
    
    props.name = QString::fromUtf8(prop.name);
    props.index = deviceIndex;
    props.computeCapabilityMaj = prop.major;
    props.computeCapabilityMin = prop.minor;
    props.multiprocessorCount = prop.multiProcessorCount;
    props.maxThreadsPerBlock = prop.maxThreadsPerBlock;
    props.warpSize = prop.warpSize;
    props.totalGlobalMem = prop.totalGlobalMem;
    props.sharedMemPerBlock = prop.sharedMemPerBlock;
    props.clockRateKHz = prop.clockRate;
    props.memoryBusWidth = prop.memoryBusWidth;
    props.l2CacheSize = prop.l2CacheSize;
    props.unifiedAddressing = prop.unifiedAddressing;
    props.asyncEngineCount = prop.asyncEngineCount;
    
    // Версии CUDA
    int driverVersion = 0, runtimeVersion = 0;
    cudaDriverGetVersion(&driverVersion);
    cudaRuntimeGetVersion(&runtimeVersion);
    
    props.cudaDriverVersion = QString("%1.%2")
        .arg(driverVersion / 1000)
        .arg((driverVersion % 100) / 10);
    
    props.cudaRuntimeVersion = QString("%1.%2")
        .arg(runtimeVersion / 1000)
        .arg((runtimeVersion % 100) / 10);
    
    return props;
}

QList<CudaDeviceProperties> CudaDeviceInfo::queryAllDevices() {
    QList<CudaDeviceProperties> devices;
    int count = deviceCount();
    
    for (int i = 0; i < count; ++i) {
        auto props = getProperties(i);
        if (!props.name.isEmpty()) {
            devices.append(props);
        }
    }
    
    return devices;
}

size_t CudaDeviceInfo::getFreeMemory(int deviceIndex) {
    size_t free = 0, total = 0;
    CUDA_CHECK(cudaSetDevice(deviceIndex));
    CUDA_CHECK(cudaMemGetInfo(&free, &total));
    return free;
}

size_t CudaDeviceInfo::getTotalMemory(int deviceIndex) {
    size_t free = 0, total = 0;
    CUDA_CHECK(cudaSetDevice(deviceIndex));
    CUDA_CHECK(cudaMemGetInfo(&free, &total));
    return total;
}

QString CudaDeviceInfo::formatDeviceInfo(const CudaDeviceProperties& props) {
    return QString("%1 (Compute %2.%3, %4 SM, %5 MB VRAM)")
        .arg(props.name)
        .arg(props.computeCapabilityMaj)
        .arg(props.computeCapabilityMin)
        .arg(props.multiprocessorCount)
        .arg(props.totalGlobalMem / (1024 * 1024));
}

} // namespace SortBench
