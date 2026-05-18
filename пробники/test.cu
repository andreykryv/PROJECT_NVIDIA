#include <iostream>
#include <cmath>
#include <cuda_runtime.h>
#include <chrono>

// Макрос для проверки ошибок CUDA
#define CUDA_CHECK(call) \
do { \
    cudaError_t error = call; \
    if (error != cudaSuccess) { \
        std::cerr << "CUDA Error: " << cudaGetErrorString(error) \
                  << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); \
    } \
} while(0)

// ============= ТЕСТ 1: Сложение векторов =============
__global__ void vectorAdd(const float *a, const float *b, float *c, int n) {
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (idx < n) {
        c[idx] = a[idx] + b[idx];
    }
}

bool testVectorAdd(int size) {
    std::cout << "\n=== ТЕСТ 1: Сложение векторов (" << size << " элементов) ===" << std::endl;
    
    size_t bytes = size * sizeof(float);
    
    // Выделение памяти на CPU
    float *h_a = new float[size];
    float *h_b = new float[size];
    float *h_c = new float[size];
    
    // Инициализация данных
    for (int i = 0; i < size; i++) {
        h_a[i] = i * 1.0f;
        h_b[i] = i * 2.0f;
    }
    
    // Выделение памяти на GPU
    float *d_a, *d_b, *d_c;
    CUDA_CHECK(cudaMalloc(&d_a, bytes));
    CUDA_CHECK(cudaMalloc(&d_b, bytes));
    CUDA_CHECK(cudaMalloc(&d_c, bytes));
    
    // Копирование на GPU
    auto start = std::chrono::high_resolution_clock::now();
    CUDA_CHECK(cudaMemcpy(d_a, h_a, bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_b, h_b, bytes, cudaMemcpyHostToDevice));
    
    // Запуск kernel
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    
    vectorAdd<<<blocksPerGrid, threadsPerBlock>>>(d_a, d_b, d_c, size);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    
    // Копирование обратно
    CUDA_CHECK(cudaMemcpy(h_c, d_c, bytes, cudaMemcpyDeviceToHost));
    auto end = std::chrono::high_resolution_clock::now();
    
    // Проверка результатов
    bool success = true;
    int errors = 0;
    for (int i = 0; i < size && errors < 10; i++) {
        float expected = h_a[i] + h_b[i];
        if (fabs(h_c[i] - expected) > 1e-5) {
            std::cout << "  ❌ Ошибка в элементе " << i 
                      << ": получено " << h_c[i] 
                      << ", ожидалось " << expected << std::endl;
            success = false;
            errors++;
        }
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    if (success) {
        std::cout << "  ✓ Все результаты корректны!" << std::endl;
    } else {
        std::cout << "  ❌ Найдено ошибок: " << errors << std::endl;
    }
    std::cout << "  ⏱  Время выполнения: " << duration.count() / 1000.0 << " мс" << std::endl;
    std::cout << "  📊 Пропускная способность: " 
              << (3.0 * bytes / 1024.0 / 1024.0) / (duration.count() / 1000000.0) 
              << " МБ/с" << std::endl;
    
    // Очистка памяти
    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);
    delete[] h_a;
    delete[] h_b;
    delete[] h_c;
    
    return success;
}

// ============= ТЕСТ 2: Умножение матриц =============
__global__ void matrixMul(const float *A, const float *B, float *C, int N) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row < N && col < N) {
        float sum = 0.0f;
        for (int k = 0; k < N; k++) {
            sum += A[row * N + k] * B[k * N + col];
        }
        C[row * N + col] = sum;
    }
}

bool testMatrixMultiply(int N) {
    std::cout << "\n=== ТЕСТ 2: Умножение матриц " << N << "x" << N << " ===" << std::endl;
    
    size_t bytes = N * N * sizeof(float);
    
    // Выделение памяти на CPU
    float *h_A = new float[N * N];
    float *h_B = new float[N * N];
    float *h_C = new float[N * N];
    
    // Инициализация матриц
    for (int i = 0; i < N * N; i++) {
        h_A[i] = 1.0f;
        h_B[i] = 2.0f;
    }
    
    // Выделение памяти на GPU
    float *d_A, *d_B, *d_C;
    CUDA_CHECK(cudaMalloc(&d_A, bytes));
    CUDA_CHECK(cudaMalloc(&d_B, bytes));
    CUDA_CHECK(cudaMalloc(&d_C, bytes));
    
    // Копирование на GPU
    CUDA_CHECK(cudaMemcpy(d_A, h_A, bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_B, h_B, bytes, cudaMemcpyHostToDevice));
    
    // Запуск kernel
    dim3 threadsPerBlock(16, 16);
    dim3 blocksPerGrid((N + 15) / 16, (N + 15) / 16);
    
    auto start = std::chrono::high_resolution_clock::now();
    matrixMul<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, N);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    auto end = std::chrono::high_resolution_clock::now();
    
    // Копирование обратно
    CUDA_CHECK(cudaMemcpy(h_C, d_C, bytes, cudaMemcpyDeviceToHost));
    
    // Проверка (каждый элемент должен быть N * 1.0 * 2.0 = 2*N)
    bool success = true;
    float expected = 2.0f * N;
    for (int i = 0; i < N * N; i++) {
        if (fabs(h_C[i] - expected) > 1e-3) {
            std::cout << "  ❌ Ошибка в элементе " << i << std::endl;
            success = false;
            break;
        }
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    float gflops = (2.0f * N * N * N) / (duration.count() * 1000.0);
    
    if (success) {
        std::cout << "  ✓ Результат корректен!" << std::endl;
    }
    std::cout << "  ⏱  Время выполнения: " << duration.count() / 1000.0 << " мс" << std::endl;
    std::cout << "  🚀 Производительность: " << gflops << " GFLOPS" << std::endl;
    
    // Очистка
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);
    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
    
    return success;
}

// ============= ТЕСТ 3: Редукция (сумма массива) =============
__global__ void reduceSum(const float *input, float *output, int n) {
    __shared__ float sdata[256];
    
    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    // Загрузка данных в shared memory
    sdata[tid] = (idx < n) ? input[idx] : 0.0f;
    __syncthreads();
    
    // Редукция в блоке
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] += sdata[tid + s];
        }
        __syncthreads();
    }
    
    // Запись результата блока
    if (tid == 0) {
        output[blockIdx.x] = sdata[0];
    }
}

bool testReduction(int size) {
    std::cout << "\n=== ТЕСТ 3: Редукция (сумма " << size << " элементов) ===" << std::endl;
    
    size_t bytes = size * sizeof(float);
    
    // Выделение памяти на CPU
    float *h_input = new float[size];
    
    // Инициализация (все единицы, сумма должна быть = size)
    for (int i = 0; i < size; i++) {
        h_input[i] = 1.0f;
    }
    
    // Выделение памяти на GPU
    float *d_input, *d_output;
    int threadsPerBlock = 256;
    int blocksPerGrid = (size + threadsPerBlock - 1) / threadsPerBlock;
    
    CUDA_CHECK(cudaMalloc(&d_input, bytes));
    CUDA_CHECK(cudaMalloc(&d_output, blocksPerGrid * sizeof(float)));
    
    CUDA_CHECK(cudaMemcpy(d_input, h_input, bytes, cudaMemcpyHostToDevice));
    
    // Запуск kernel
    auto start = std::chrono::high_resolution_clock::now();
    reduceSum<<<blocksPerGrid, threadsPerBlock>>>(d_input, d_output, size);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    auto end = std::chrono::high_resolution_clock::now();
    
    // Копирование и финальная редукция на CPU
    float *h_output = new float[blocksPerGrid];
    CUDA_CHECK(cudaMemcpy(h_output, d_output, blocksPerGrid * sizeof(float), cudaMemcpyDeviceToHost));
    
    float total = 0.0f;
    for (int i = 0; i < blocksPerGrid; i++) {
        total += h_output[i];
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    bool success = (fabs(total - size) < 1e-3);
    if (success) {
        std::cout << "  ✓ Сумма корректна: " << total << " (ожидалось " << size << ")" << std::endl;
    } else {
        std::cout << "  ❌ Ошибка: получено " << total << ", ожидалось " << size << std::endl;
    }
    std::cout << "  ⏱  Время выполнения: " << duration.count() / 1000.0 << " мс" << std::endl;
    
    // Очистка
    cudaFree(d_input);
    cudaFree(d_output);
    delete[] h_input;
    delete[] h_output;
    
    return success;
}

// ============= ТЕСТ 4: Тест пропускной способности памяти =============
bool testMemoryBandwidth(int size) {
    std::cout << "\n=== ТЕСТ 4: Пропускная способность памяти (" << size / (1024*1024) << " МБ) ===" << std::endl;
    
    size_t bytes = size * sizeof(float);
    float *h_data = new float[size];
    float *d_data;
    
    // Инициализация
    for (int i = 0; i < size; i++) {
        h_data[i] = i;
    }
    
    CUDA_CHECK(cudaMalloc(&d_data, bytes));
    
    // Тест Host -> Device
    auto start = std::chrono::high_resolution_clock::now();
    CUDA_CHECK(cudaMemcpy(d_data, h_data, bytes, cudaMemcpyHostToDevice));
    auto end = std::chrono::high_resolution_clock::now();
    auto h2d_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Тест Device -> Host
    start = std::chrono::high_resolution_clock::now();
    CUDA_CHECK(cudaMemcpy(h_data, d_data, bytes, cudaMemcpyDeviceToHost));
    end = std::chrono::high_resolution_clock::now();
    auto d2h_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    float h2d_bandwidth = (bytes / 1024.0 / 1024.0) / (h2d_time.count() / 1000000.0);
    float d2h_bandwidth = (bytes / 1024.0 / 1024.0) / (d2h_time.count() / 1000000.0);
    
    std::cout << "  📤 Host -> Device: " << h2d_bandwidth << " МБ/с" << std::endl;
    std::cout << "  📥 Device -> Host: " << d2h_bandwidth << " МБ/с" << std::endl;
    
    cudaFree(d_data);
    delete[] h_data;
    
    return true;
}

// ============= Информация о GPU =============
void printGPUInfo() {
    int deviceCount;
    CUDA_CHECK(cudaGetDeviceCount(&deviceCount));
    
    std::cout << "\n╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         ИНФОРМАЦИЯ О CUDA УСТРОЙСТВАХ                  ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    
    for (int i = 0; i < deviceCount; i++) {
        cudaDeviceProp prop;
        CUDA_CHECK(cudaGetDeviceProperties(&prop, i));
        
        std::cout << "\n🖥  Устройство " << i << ": " << prop.name << std::endl;
        std::cout << "  ├─ Compute Capability: " << prop.major << "." << prop.minor << std::endl;
        std::cout << "  ├─ Мультипроцессоров: " << prop.multiProcessorCount << std::endl;
        std::cout << "  ├─ CUDA ядер на SM: " << prop.maxThreadsPerMultiProcessor / prop.warpSize << " warps" << std::endl;
        std::cout << "  ├─ Всего CUDA ядер: ~" << prop.multiProcessorCount * 128 << std::endl;
        std::cout << "  ├─ Глобальная память: " << prop.totalGlobalMem / 1024.0 / 1024.0 / 1024.0 << " ГБ" << std::endl;
        std::cout << "  ├─ Shared память на блок: " << prop.sharedMemPerBlock / 1024.0 << " КБ" << std::endl;
        std::cout << "  ├─ Макс. потоков на блок: " << prop.maxThreadsPerBlock << std::endl;
        std::cout << "  ├─ Warp size: " << prop.warpSize << std::endl;
        std::cout << "  └─ Ширина шины памяти: " << prop.memoryBusWidth << " бит" << std::endl;
    }
}

// ============= ГЛАВНАЯ ФУНКЦИЯ =============
int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     КОМПЛЕКСНОЕ ТЕСТИРОВАНИЕ CUDA                      ║" << std::endl;
    std::cout << "║     RTX 3050 Mobile (sm_86)                            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    
    // Вывод информации о GPU
    printGPUInfo();
    
    std::cout << "\n" << std::endl;
    std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║              ЗАПУСК ТЕСТОВ                             ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    
    // Запуск тестов
    int passed = 0;
    int total = 0;
    
    // Тест 1: Сложение векторов
    total++;
    if (testVectorAdd(1 << 20)) passed++;  // 1 млн элементов
    
    // Тест 2: Умножение матриц
    total++;
    if (testMatrixMultiply(512)) passed++;  // 512x512
    
    // Тест 3: Редукция
    total++;
    if (testReduction(1 << 20)) passed++;  // 1 млн элементов
    
    // Тест 4: Пропускная способность памяти
    total++;
    if (testMemoryBandwidth(100 * 1024 * 1024)) passed++;  // 100 МБ
    
    // Итоги
    std::cout << "\n" << std::endl;
    std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                  РЕЗУЛЬТАТЫ                            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n";
    
    if (passed == total) {
        std::cout << "  🎉 ВСЕ ТЕСТЫ ПРОЙДЕНЫ! (" << passed << "/" << total << ")" << std::endl;
        std::cout << "  ✓ CUDA работает корректно!" << std::endl;
    } else {
        std::cout << "  ⚠️  Пройдено тестов: " << passed << "/" << total << std::endl;
        std::cout << "  ❌ Обнаружены проблемы!" << std::endl;
    }
    
    std::cout << "\n" << std::endl;
    
    return (passed == total) ? 0 : 1;
}