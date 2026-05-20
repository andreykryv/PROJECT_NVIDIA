////////////////////////////////////////////////////////////////////////////////
// gpu/cudadeviceinfo_stub.cpp — заглушка для CUDA-функций когда CUDA отключена
//
// НАЗНАЧЕНИЕ:
//   Предоставляет пустые реализации функций CudaDeviceInfo для сборок без CUDA.
//   Позволяет компилировать проект даже если CUDA Toolkit не установлен или
//   поддержка CUDA отключена в CMake.
////////////////////////////////////////////////////////////////////////////////

#include "gpu/cudadeviceinfo.h"

#ifndef USE_CUDA
#define USE_CUDA 0
#endif

#if !USE_CUDA

bool CudaDeviceInfo::isCudaAvailable() {
    return false;
}

int CudaDeviceInfo::deviceCount() {
    return 0;
}

CudaDeviceProperties CudaDeviceInfo::getProperties(int deviceIndex) {
    Q_UNUSED(deviceIndex);
    return CudaDeviceProperties();
}

QList<CudaDeviceProperties> CudaDeviceInfo::queryAllDevices() {
    return QList<CudaDeviceProperties>();
}

size_t CudaDeviceInfo::getFreeMemory(int deviceIndex) {
    Q_UNUSED(deviceIndex);
    return 0;
}

size_t CudaDeviceInfo::getTotalMemory(int deviceIndex) {
    Q_UNUSED(deviceIndex);
    return 0;
}

int CudaDeviceInfo::getMemoryUsagePercent(int deviceIndex) {
    Q_UNUSED(deviceIndex);
    return 0;
}

QString CudaDeviceInfo::formatDeviceInfo(const CudaDeviceProperties& props) {
    Q_UNUSED(props);
    return QString("CUDA не доступна");
}

QString CudaDeviceInfo::getDeviceShortName(int deviceIndex) {
    Q_UNUSED(deviceIndex);
    return QString("Нет устройства");
}

bool CudaDeviceInfo::hasDoublePrecision(int deviceIndex) {
    Q_UNUSED(deviceIndex);
    return false;
}

bool CudaDeviceInfo::hasTensorCores(int deviceIndex) {
    Q_UNUSED(deviceIndex);
    return false;
}

bool CudaDeviceInfo::hasRayTracingCores(int deviceIndex) {
    Q_UNUSED(deviceIndex);
    return false;
}

#endif // !USE_CUDA
