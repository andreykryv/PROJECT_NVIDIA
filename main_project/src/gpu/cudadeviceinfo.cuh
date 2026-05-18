////////////////////////////////////////////////////////////////////////////////
// gpu/cudadeviceinfo.cuh — CUDA-заголовок (включается только в .cu файлах)
//
// НАЗНАЧЕНИЕ:
//   Объявляет device-side утилиты и расширенные CUDA-специфические
//   функции, которые нельзя объявить в .h без cuda_runtime.h.
//
// СОДЕРЖИМОЕ:
//   — Объявление функции cudaQueryDeviceInfo() реализованной в .cu
//   — Вспомогательные макросы CUDA_CHECK(err) для обработки ошибок:
//       #define CUDA_CHECK(call) { cudaError_t err = (call); \
//           if(err != cudaSuccess) { /* emit error signal или throw */ } }
//   — CUDA_CHECK_LAST() — проверяет cudaGetLastError() после kernel launch
//   — Макрос DIVUP(n, d) — деление с округлением вверх: (n + d - 1) / d
//   — Макрос ALIGN_UP(n, alignment) — выравнивание вверх
//   — Константы: WARP_SIZE = 32, MAX_BLOCK_SIZE = 1024
////////////////////////////////////////////////////////////////////////////////
