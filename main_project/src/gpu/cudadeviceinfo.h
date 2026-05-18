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
//   static bool isCudaAvailable()
//   static size_t getFreeMemory(int deviceIndex)    — текущая свободная VRAM
//   static size_t getTotalMemory(int deviceIndex)
//   static QString formatDeviceInfo(CudaDeviceProperties) — форматированная строка
////////////////////////////////////////////////////////////////////////////////

#ifndef CUDA_DEVICE_INFO_H
#define CUDA_DEVICE_INFO_H

#include <QString>
#include <QList>
#include <cstddef>

// Структура свойств CUDA-устройства
struct CudaDeviceProperties {
    QString name;
    int index;
    int computeCapabilityMajor;
    int computeCapabilityMinor;
    int multiprocessorCount;
    int maxThreadsPerBlock;
    int warpSize;
    size_t totalGlobalMem;
    size_t sharedMemPerBlock;
    int clockRateKHz;
    int memoryBusWidth;
    int l2CacheSize;
    bool unifiedAddressing;
    int asyncEngineCount;
    QString cudaDriverVersion;
    QString cudaRuntimeVersion;
    
    // Конструктор по умолчанию
    CudaDeviceProperties() 
        : index(-1)
        , computeCapabilityMajor(0)
        , computeCapabilityMinor(0)
        , multiprocessorCount(0)
        , maxThreadsPerBlock(0)
        , warpSize(0)
        , totalGlobalMem(0)
        , sharedMemPerBlock(0)
        , clockRateKHz(0)
        , memoryBusWidth(0)
        , l2CacheSize(0)
        , unifiedAddressing(false)
        , asyncEngineCount(0)
    {}
    
    // Получить тактовую частоту в МГц
    int clockRateMHz() const { return clockRateKHz / 1000; }
    
    // Получить общий объём памяти в МБ
    size_t totalGlobalMemMB() const { return totalGlobalMem / (1024 * 1024); }
    
    // Получить shared memory в КБ
    size_t sharedMemPerBlockKB() const { return sharedMemPerBlock / 1024; }
    
    // Строковое представление вычислительной способности
    QString computeCapabilityString() const {
        return QString("%1.%2").arg(computeCapabilityMajor).arg(computeCapabilityMinor);
    }
};

// Класс для работы с информацией о CUDA-устройствах
class CudaDeviceInfo {
public:
    // Проверить доступность CUDA
    static bool isCudaAvailable();
    
    // Получить количество CUDA-устройств
    static int deviceCount();
    
    // Получить свойства указанного устройства
    static CudaDeviceProperties getProperties(int deviceIndex);
    
    // Запросить информацию обо всех устройствах
    static QList<CudaDeviceProperties> queryAllDevices();
    
    // Получить свободную память устройства (в байтах)
    static size_t getFreeMemory(int deviceIndex);
    
    // Получить общую память устройства (в байтах)
    static size_t getTotalMemory(int deviceIndex);
    
    // Получить процент использованной памяти
    static int getMemoryUsagePercent(int deviceIndex);
    
    // Форматировать информацию об устройстве для отображения
    static QString formatDeviceInfo(const CudaDeviceProperties& props);
    
    // Краткое описание устройства (для combo box)
    static QString getDeviceShortName(int deviceIndex);
    
    // Проверить поддержку double precision
    static bool hasDoublePrecision(int deviceIndex);
    
    // Проверить поддержку tensor cores
    static bool hasTensorCores(int deviceIndex);
    
    // Проверить поддержку ray tracing cores
    static bool hasRayTracingCores(int deviceIndex);
    
private:
    // Запрет создания экземпляров класса
    CudaDeviceInfo() = delete;
    ~CudaDeviceInfo() = delete;
};

#endif // CUDA_DEVICE_INFO_H
