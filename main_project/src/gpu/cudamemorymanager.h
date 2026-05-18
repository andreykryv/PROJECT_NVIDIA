////////////////////////////////////////////////////////////////////////////////
// gpu/cudamemorymanager.h — менеджер памяти CUDA (RAII-обёртки)
//
// НАЗНАЧЕНИЕ:
//   Предоставляет RAII-классы для безопасного управления device и pinned памятью.
//   Исключает утечки при исключениях или ранних return.
//
// ШАБЛОНЫ:
//
//   DeviceBuffer<T>:
//     DeviceBuffer(size_t count)   — cudaMalloc(count * sizeof(T))
//     ~DeviceBuffer()              — cudaFree
//     T* get() const               — указатель на device-память
//     size_t size() const          — число элементов
//     size_t bytes() const         — байт
//     void upload(const T* host, cudaStream_t = 0)   — cudaMemcpyAsync H2D
//     void download(T* host, cudaStream_t = 0) const — cudaMemcpyAsync D2H
//     DeviceBuffer(DeviceBuffer&&) noexcept  — move-конструктор
//     DeviceBuffer(const DeviceBuffer&) = delete  — некопируемый
//
//   PinnedBuffer<T>:
//     PinnedBuffer(size_t count)   — cudaMallocHost
//     ~PinnedBuffer()              — cudaFreeHost
//     T* get() const
//     Позволяет асинхронную H2D/D2H передачу на максимальной скорости.
//
//   CudaStream:
//     CudaStream()                 — cudaStreamCreate
//     ~CudaStream()                — cudaStreamDestroy
//     cudaStream_t get() const
//     void synchronize()           — cudaStreamSynchronize
//
//   CudaEvent:
//     CudaEvent()                  — cudaEventCreate
//     ~CudaEvent()                 — cudaEventDestroy
//     void record(CudaStream&)     — cudaEventRecord
//     float elapsedMs(CudaEvent& start) — cudaEventElapsedTime
////////////////////////////////////////////////////////////////////////////////
