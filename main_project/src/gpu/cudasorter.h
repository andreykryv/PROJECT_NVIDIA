////////////////////////////////////////////////////////////////////////////////
// gpu/cudasorter.h — заголовок фасада GPU-алгоритмов (host-side, без CUDA)
//
// НАЗНАЧЕНИЕ:
//   CudaSorter — Qt-совместимый фасад для всех GPU-алгоритмов.
//   Может включаться в любой .cpp. Скрывает CUDA-специфику.
//
// КЛАСС: CudaSorter
//
//   КОНСТРУКТОР/ДЕСТРУКТОР:
//     CudaSorter(int deviceIndex = 0)
//       — cudaSetDevice(deviceIndex)
//       — Создаёт cudaStream_t streams[numStreams]
//       — Если usePinnedMemory: выделяет pinned host буферы
//     ~CudaSorter()
//       — cudaStreamDestroy для всех потоков
//       — Освобождение всей device и pinned памяти
//
//   ОСНОВНОЙ МЕТОД:
//   template<typename T>
//   GpuTimings sort(const T* hostInput, T* hostOutput, int size,
//                   GpuAlgorithm algo, int blockSize, int numStreams):
//     — Диспатч на нужный GPU-алгоритм.
//     — Возвращает GpuTimings { h2dMs, kernelMs, d2hMs, syncMs }.
//
//   СТРУКТУРА GpuTimings:
//     double h2dMs, kernelMs, d2hMs, syncMs
//     double totalMs() const  — сумма всех компонент
//
//   ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ:
//     size_t allocateDeviceBuffers(size_t bytes)  — cudaMalloc с проверкой
//     void freeDeviceBuffers()
//     void setDevice(int index)
//     bool isReady() const
//     size_t currentDeviceMemUsed() const
////////////////////////////////////////////////////////////////////////////////
