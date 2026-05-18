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
