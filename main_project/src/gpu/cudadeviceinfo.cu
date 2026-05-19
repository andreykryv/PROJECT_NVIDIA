#include "cudadeviceinfo.h"
#include <cuda_runtime.h>
#include <QDebug>

#define CUDA_CHECK_RETURN(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            qWarning() << "CUDA error:" << cudaGetErrorString(err); \
            return {}; \
        } \
    } while(0)

// Отдельный макрос для функций, возвращающих bool/int
#define CUDA_CHECK_BOOL(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            qWarning() << "CUDA error:" << cudaGetErrorString(err); \
            return false; \
        } \
    } while(0)

namespace SortBench {

int CudaDeviceInfo::deviceCount() {
    int count = 0;
    cudaError_t err = cudaGetDeviceCount(&count);
    if (err != cudaSuccess) return 0;
    return count;
}

bool CudaDeviceInfo::isCudaAvailable() {
    return deviceCount() > 0;
}

CudaDeviceProperties CudaDeviceInfo::getProperties(int deviceIndex) {
    CudaDeviceProperties props{};

    cudaDeviceProp prop;
    cudaError_t err = cudaGetDeviceProperties(&prop, deviceIndex);
    if (err != cudaSuccess) {
        qWarning() << "CUDA error:" << cudaGetErrorString(err);
        return props;
    }

    props.name = QString::fromUtf8(prop.name);
    props.index = deviceIndex;
    props.computeCapabilityMajor = prop.major;   // FIX: было computeCapabilityMaj
    props.computeCapabilityMinor = prop.minor;   // FIX: было computeCapabilityMin
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
    cudaSetDevice(deviceIndex);
    cudaMemGetInfo(&free, &total);
    return free;
}

size_t CudaDeviceInfo::getTotalMemory(int deviceIndex) {
    size_t free = 0, total = 0;
    cudaSetDevice(deviceIndex);
    cudaMemGetInfo(&free, &total);
    return total;
}

int CudaDeviceInfo::getMemoryUsagePercent(int deviceIndex) {
    size_t free = getFreeMemory(deviceIndex);
    size_t total = getTotalMemory(deviceIndex);
    if (total == 0) return 0;
    return static_cast<int>((1.0 - static_cast<double>(free) / total) * 100.0);
}

QString CudaDeviceInfo::formatDeviceInfo(const CudaDeviceProperties& props) {
    // FIX: было computeCapabilityMaj/Min — исправлено на Major/Minor
    return QString("%1 (Compute %2.%3, %4 SM, %5 MB VRAM)")
        .arg(props.name)
        .arg(props.computeCapabilityMajor)
        .arg(props.computeCapabilityMinor)
        .arg(props.multiprocessorCount)
        .arg(props.totalGlobalMem / (1024 * 1024));
}

QString CudaDeviceInfo::getDeviceShortName(int deviceIndex) {
    auto props = getProperties(deviceIndex);
    return QString("[%1] %2").arg(deviceIndex).arg(props.name);
}

bool CudaDeviceInfo::hasDoublePrecision(int deviceIndex) {
    auto props = getProperties(deviceIndex);
    return (props.computeCapabilityMajor > 1) ||
           (props.computeCapabilityMajor == 1 && props.computeCapabilityMinor >= 3);
}

bool CudaDeviceInfo::hasTensorCores(int deviceIndex) {
    auto props = getProperties(deviceIndex);
    return props.computeCapabilityMajor >= 7;
}

bool CudaDeviceInfo::hasRayTracingCores(int deviceIndex) {
    auto props = getProperties(deviceIndex);
    return props.computeCapabilityMajor >= 7 && props.computeCapabilityMinor >= 5;
}

} // namespace SortBench
