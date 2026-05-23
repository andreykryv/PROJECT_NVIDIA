#include "cudadeviceinfo.h"
// cuda_runtime.h не включаем напрямую здесь, он уже включен в cudadeviceinfo.h через cudadeviceinfo.cuh
// для .cu файлов нужен отдельный подход - включаем только если это CUDA-компиляция
#ifdef __CUDACC__
#include <cuda_runtime.h>
#endif
#include <iostream>

#define CUDA_CHECK_RETURN(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error:" << cudaGetErrorString(err) << std::endl; \
            return {}; \
        } \
    } while(0)

// Отдельный макрос для функций, возвращающих bool/int
#define CUDA_CHECK_BOOL(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA error:" << cudaGetErrorString(err) << std::endl; \
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
        std::cerr << "CUDA error:" << cudaGetErrorString(err) << std::endl;
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
    
  // clockRate удалён в CUDA 13.x, используем значение по умолчанию
    // Для получения актуальной частоты нужно использовать cudaDeviceGetAttribute
    // с атрибутом cudaDevAttrClockRate
    int clockRateKHz = 0;
    cudaDeviceGetAttribute(&clockRateKHz, cudaDevAttrClockRate, deviceIndex);
    props.clockRateKHz = clockRateKHz;
    
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

////////////////////////////////////////////////////////////////////////////////
// gpu/cudadeviceinfo.cu — реализация CUDA-функций (только при наличии CUDA)
//
// НАЗНАЧЕНИЕ:
//   Реализует функции CudaDeviceInfo для сборок с CUDA.
//   Не должен компилироваться в сборках без CUDA.
////////////////////////////////////////////////////////////////////////////////

#include "gpu/cudadeviceinfo.h"
#include "gpu/cudadeviceinfo.cuh"

#ifndef USE_CUDA
#define USE_CUDA 1
#endif

#if USE_CUDA

#include <cuda_runtime.h>
#include <iostream>
#include <stdexcept>
#include <cstring>

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
        std::cerr << "CUDA error:" << cudaGetErrorString(err) << std::endl;
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

    // clockRate удалён в CUDA 13.x, используем значение по умолчанию
    props.clockRateKHz = 0;
    props.memoryBusWidth = prop.memoryBusWidth;
    props.l2CacheSize = prop.l2CacheSize;
    props.unifiedAddressing = prop.unifiedAddressing;
    props.asyncEngineCount = prop.asyncEngineCount;

    // Получаем версии драйвера и runtime
    // Используем функции, доступные в CUDA 11.x и выше
    int driverVersion, runtimeVersion;
    cudaDriverGetVersion(&driverVersion);
    cudaRuntimeGetVersion(&runtimeVersion);

    props.cudaDriverVersion = QString::number(driverVersion);
    props.cudaRuntimeVersion = QString::number(runtimeVersion);

    return props;
}

QList<CudaDeviceProperties> CudaDeviceInfo::queryAllDevices() {
    QList<CudaDeviceProperties> devices;
    int count = deviceCount();
    for (int i = 0; i < count; ++i) {
        devices.append(getProperties(i));
    }
    return devices;
}
size_t CudaDeviceInfo::getFreeMemory(int deviceIndex) {
    size_t freeMem = 0;
    size_t totalMem = 0;
    cudaError_t err = cudaMemGetInfo(&freeMem, &totalMem);
    if (err != cudaSuccess) return 0;
    return freeMem;
}

size_t CudaDeviceInfo::getTotalMemory(int deviceIndex) {
    size_t freeMem = 0;
    size_t totalMem = 0;
    cudaError_t err = cudaMemGetInfo(&freeMem, &totalMem);
    if (err != cudaSuccess) return 0;
    return totalMem;
}

int CudaDeviceInfo::getMemoryUsagePercent(int deviceIndex) {
    size_t freeMem = 0;
    size_t totalMem = 0;
    cudaError_t err = cudaMemGetInfo(&freeMem, &totalMem);
    if (err != cudaSuccess) return 0;
    if (totalMem == 0) return 0;
    return static_cast<int>((1.0 - static_cast<double>(freeMem) / totalMem) * 100);
}

QString CudaDeviceInfo::formatDeviceInfo(const CudaDeviceProperties& props) {
    QString format = QString("%1\n"
        "Compute Capability: %2\n"
        "SM: %3\n"
        "Max threads/block: %4\n"
        "VRAM: %5 MB\n"
        "Clock Rate: %6 MHz\n"
        "Memory Bus Width: %7 bits\n"
        "L2 Cache: %8 KB\n"
        "Unified Addressing: %9\n"
        "Async Engines: %10\n")
        .arg(props.name)
        .arg(props.computeCapabilityString())
        .arg(props.multiprocessorCount)
        .arg(props.maxThreadsPerBlock)
        .arg(props.totalGlobalMemMB())
        .arg(props.clockRateMHz())
        .arg(props.memoryBusWidth)
        .arg(props.l2CacheSize / 1024)
        .arg(props.unifiedAddressing ? "Yes" : "No")
        .arg(props.asyncEngineCount);

    return format;
}

QString CudaDeviceInfo::getDeviceShortName(int deviceIndex) {
    CudaDeviceProperties props = getProperties(deviceIndex);
    if (props.index == -1) return "Unknown Device";
    return props.name;
}

bool CudaDeviceInfo::hasDoublePrecision(int deviceIndex) {
    CudaDeviceProperties props = getProperties(deviceIndex);
    return props.computeCapabilityMajor >= 1;
}

bool CudaDeviceInfo::hasTensorCores(int deviceIndex) {
    CudaDeviceProperties props = getProperties(deviceIndex);
    return props.computeCapabilityMajor >= 7;
}

bool CudaDeviceInfo::hasRayTracingCores(int deviceIndex) {
    CudaDeviceProperties props = getProperties(deviceIndex);
    return props.computeCapabilityMajor >= 7;
}

} // namespace SortBench

#endif // USE_CUDA
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

 // namespace SortBench
