////////////////////////////////////////////////////////////////////////////////
// gpu/cudadeviceinfo.h — заголовок информации о CUDA-устройствах (host-side)
//
// НАЗНАЧЕНИЕ:
//   Предоставляет Qt-совместимый (non-CUDA) интерфейс для получения информации
//   о CUDA-устройствах. Может включаться в любой .cpp без nvcc.
//
// СТРУКТУРА CudaDeviceProperties:
//   QString  name                  — "NVIDIA GeForce RTX 4090"
//   int      index                 — индекс устройства (0, 1, ...)
//   int      computeCapabilityMaj/Min
//   int      multiprocessorCount   — число SM
//   int      maxThreadsPerBlock    — обычно 1024
//   int      warpSize              — 32
//   size_t   totalGlobalMem        — VRAM в байтах
//   size_t   sharedMemPerBlock
//   int      clockRateKHz
//   int      memoryBusWidth        — 256, 384 бит
//   int      l2CacheSize
//   bool     unifiedAddressing     — UVA поддержка
//   int      asyncEngineCount      — число copy engines
//   QString  cudaDriverVersion     — "12.3"
//   QString  cudaRuntimeVersion    — "12.3"
//
// КЛАСС: CudaDeviceInfo
//   static int  deviceCount()
//   static CudaDeviceProperties getProperties(int deviceIndex)
//   static QList<CudaDeviceProperties> queryAllDevices()
//   static size_t getFreeMemory(int deviceIndex)
//   static size_t getTotalMemory(int deviceIndex)
//   static int getMemoryUsagePercent(int deviceIndex)
//   static QString formatDeviceInfo(const CudaDeviceProperties& props)
//   static QString getDeviceShortName(int deviceIndex)
//   static bool hasDoublePrecision(int deviceIndex)
//   static bool hasTensorCores(int deviceIndex)
//   static bool hasRayTracingCores(int deviceIndex)
//
#ifndef CUDA_DEVICE_INFO_H
#define CUDA_DEVICE_INFO_H

#include <QString>
#include <QList>

// Проверяем, включена ли поддержка CUDA
#ifndef USE_CUDA
#define USE_CUDA 1
#endif

// Если CUDA не включена, то мы не будем использовать CUDA-функции
// и будем использовать заглушку в cudadeviceinfo_stub.cpp
namespace SortBench {
struct CudaDeviceProperties {
    QString name;
    int index = -1;
    int computeCapabilityMajor = 0;
    int computeCapabilityMinor = 0;
    int multiprocessorCount = 0;
    int maxThreadsPerBlock = 0;
    int warpSize = 0;
    size_t totalGlobalMem = 0;
    size_t sharedMemPerBlock = 0;
    int clockRateKHz = 0;
    int memoryBusWidth = 0;
    int l2CacheSize = 0;
    bool unifiedAddressing = false;
    int asyncEngineCount = 0;
    QString cudaDriverVersion;
    QString cudaRuntimeVersion;
    
    QString computeCapabilityString() const {
        return QString("%1.%2").arg(computeCapabilityMajor).arg(computeCapabilityMinor);
    }
};

class CudaDeviceInfo {
public:
    static int deviceCount();
    static bool isCudaAvailable();
    static CudaDeviceProperties getProperties(int deviceIndex);
    static QList<CudaDeviceProperties> queryAllDevices();
    static size_t getFreeMemory(int deviceIndex);
    static size_t getTotalMemory(int deviceIndex);
    static int getMemoryUsagePercent(int deviceIndex);
    static QString formatDeviceInfo(const CudaDeviceProperties& props);
    static QString getDeviceShortName(int deviceIndex);
    static bool hasDoublePrecision(int deviceIndex);
    static bool hasTensorCores(int deviceIndex);
    static bool hasRayTracingCores(int deviceIndex);
};

} // namespace SortBench

#endif // CUDA_DEVICE_INFO_H

