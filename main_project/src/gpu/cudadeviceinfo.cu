////////////////////////////////////////////////////////////////////////////////
// gpu/cudadeviceinfo.cu — CUDA-реализация опроса устройств
//
// НАЗНАЧЕНИЕ:
//   Компилируется nvcc. Реализует функции CudaDeviceInfo, которые
//   используют CUDA Runtime API (cudaGetDeviceCount, cudaGetDeviceProperties,
//   cudaMemGetInfo).
//
// РЕАЛИЗАЦИЯ:
//   deviceCount():          cudaGetDeviceCount(&count); return count.
//   getProperties(index):   cudaGetDeviceProperties(&prop, index);
//                           Заполняет структуру CudaDeviceProperties из prop.
//   getFreeMemory(index):   cudaSetDevice(index); cudaMemGetInfo(&free, &total);
//   isCudaAvailable():      cudaGetDeviceCount(&n); return n > 0.
//   queryAllDevices():      Цикл по всем устройствам, вызов getProperties().
//
// ОБРАБОТКА ОШИБОК:
//   Все CUDA-вызовы обёрнуты в CUDA_CHECK. При ошибке функция возвращает
//   пустую структуру и логгирует через qWarning() (после включения <QDebug>).
////////////////////////////////////////////////////////////////////////////////
