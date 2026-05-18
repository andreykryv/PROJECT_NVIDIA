/**
 * ============================================================
 *  CUDA vs CPU Benchmark
 *  RTX 3050 Mobile (или любая CUDA-совместимая GPU) vs CPU
 * ============================================================
 *
 *  Тесты:
 *    1. Сложение векторов
 *    2. Умножение матриц
 *    3. Редукция (сумма массива)
 *    4. Вычисление числа π методом Монте-Карло
 *
 *  Сборка:
 *    nvcc -O3 -arch=sm_86 cuda_benchmark.cu -o cuda_benchmark
 *    (sm_86 для RTX 30xx mobile; sm_89 для RTX 40xx)
 *
 *  Запуск:
 *    ./cuda_benchmark
 * ============================================================
 */

#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <vector>
#include <string>
#include <iomanip>
#include <iostream>
#include <random>

// ─────────────────────────────────────────────────
//  Вспомогательные макросы
// ─────────────────────────────────────────────────
#define CUDA_CHECK(call)                                                    \
    do {                                                                    \
        cudaError_t err = (call);                                           \
        if (err != cudaSuccess) {                                           \
            fprintf(stderr, "CUDA error in %s:%d — %s\n",                  \
                    __FILE__, __LINE__, cudaGetErrorString(err));           \
            exit(EXIT_FAILURE);                                             \
        }                                                                   \
    } while (0)

// ─────────────────────────────────────────────────
//  Таймер на основе std::chrono
// ─────────────────────────────────────────────────
struct CpuTimer {
    std::chrono::high_resolution_clock::time_point t0;
    void start() { t0 = std::chrono::high_resolution_clock::now(); }
    double elapsedMs() const {
        auto t1 = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(t1 - t0).count();
    }
};

// ─────────────────────────────────────────────────
//  Таймер на основе CUDA Events
// ─────────────────────────────────────────────────
struct GpuTimer {
    cudaEvent_t start_, stop_;
    GpuTimer()  { cudaEventCreate(&start_); cudaEventCreate(&stop_); }
    ~GpuTimer() { cudaEventDestroy(start_); cudaEventDestroy(stop_);  }
    void start() { cudaEventRecord(start_); }
    float elapsedMs() {
        cudaEventRecord(stop_);
        cudaEventSynchronize(stop_);
        float ms = 0;
        cudaEventElapsedTime(&ms, start_, stop_);
        return ms;
    }
};

// ─────────────────────────────────────────────────
//  Утилита вывода
// ─────────────────────────────────────────────────
void printSeparator() {
    std::cout << std::string(68, '-') << "\n";
}

void printResult(const std::string& testName,
                 double cpuMs, double gpuMs,
                 const std::string& note = "")
{
    double speedup = cpuMs / gpuMs;
    std::string winner = (gpuMs < cpuMs) ? "GPU быстрее" : "CPU быстрее";
    double ratio = (gpuMs < cpuMs) ? speedup : (gpuMs / cpuMs);

    printSeparator();
    std::cout << "  Тест: " << testName << "\n";
    if (!note.empty())
        std::cout << "  Примечание: " << note << "\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "  CPU: " << std::setw(10) << cpuMs << " мс\n";
    std::cout << "  GPU: " << std::setw(10) << gpuMs << " мс  (включая H→D→H копирование)\n";
    std::cout << "  ► " << winner << " в " << std::setprecision(2) << ratio << "×\n";
}

// ═══════════════════════════════════════════════════
//  ТЕСТ 1: СЛОЖЕНИЕ ВЕКТОРОВ
// ═══════════════════════════════════════════════════
__global__ void vecAddKernel(const float* __restrict__ A,
                              const float* __restrict__ B,
                              float*       __restrict__ C,
                              int N)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < N) C[i] = A[i] + B[i];
}

void vecAddCPU(const float* A, const float* B, float* C, int N) {
    for (int i = 0; i < N; ++i) C[i] = A[i] + B[i];
}

void benchmarkVecAdd(int N = 1 << 24 /* ~16M элементов */)
{
    size_t bytes = (size_t)N * sizeof(float);
    std::vector<float> h_A(N), h_B(N), h_C(N);
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    for (int i = 0; i < N; ++i) { h_A[i] = dist(rng); h_B[i] = dist(rng); }

    // ── CPU ──
    CpuTimer ct;
    ct.start();
    vecAddCPU(h_A.data(), h_B.data(), h_C.data(), N);
    double cpuMs = ct.elapsedMs();

    // ── GPU ──
    float *d_A, *d_B, *d_C;
    CUDA_CHECK(cudaMalloc(&d_A, bytes));
    CUDA_CHECK(cudaMalloc(&d_B, bytes));
    CUDA_CHECK(cudaMalloc(&d_C, bytes));

    GpuTimer gt;
    gt.start();
    CUDA_CHECK(cudaMemcpy(d_A, h_A.data(), bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_B, h_B.data(), bytes, cudaMemcpyHostToDevice));
    int threads = 256;
    int blocks  = (N + threads - 1) / threads;
    vecAddKernel<<<blocks, threads>>>(d_A, d_B, d_C, N);
    CUDA_CHECK(cudaMemcpy(h_C.data(), d_C, bytes, cudaMemcpyDeviceToHost));
    double gpuMs = gt.elapsedMs();

    cudaFree(d_A); cudaFree(d_B); cudaFree(d_C);

    char note[64];
    snprintf(note, sizeof(note), "N = %d (%.1f МБ × 3)", N, bytes / 1e6);
    printResult("Сложение векторов float", cpuMs, gpuMs, note);
}

// ═══════════════════════════════════════════════════
//  ТЕСТ 2: УМНОЖЕНИЕ МАТРИЦ (наивное)
// ═══════════════════════════════════════════════════
__global__ void matMulKernel(const float* __restrict__ A,
                              const float* __restrict__ B,
                              float*       __restrict__ C,
                              int M, int N, int K)
{
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    if (row < M && col < N) {
        float sum = 0.f;
        for (int k = 0; k < K; ++k)
            sum += A[row * K + k] * B[k * N + col];
        C[row * N + col] = sum;
    }
}

void matMulCPU(const float* A, const float* B, float* C, int M, int N, int K) {
    for (int m = 0; m < M; ++m)
        for (int n = 0; n < N; ++n) {
            float sum = 0.f;
            for (int k = 0; k < K; ++k)
                sum += A[m * K + k] * B[k * N + n];
            C[m * N + n] = sum;
        }
}

void benchmarkMatMul(int M = 512, int K = 512, int N = 512)
{
    size_t sA = (size_t)M * K * sizeof(float);
    size_t sB = (size_t)K * N * sizeof(float);
    size_t sC = (size_t)M * N * sizeof(float);

    std::vector<float> h_A(M * K), h_B(K * N), h_C(M * N, 0.f);
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> d(0.f, 1.f);
    for (auto& v : h_A) v = d(rng);
    for (auto& v : h_B) v = d(rng);

    // ── CPU ──
    CpuTimer ct;
    ct.start();
    matMulCPU(h_A.data(), h_B.data(), h_C.data(), M, N, K);
    double cpuMs = ct.elapsedMs();

    // ── GPU ──
    float *d_A, *d_B, *d_C;
    CUDA_CHECK(cudaMalloc(&d_A, sA));
    CUDA_CHECK(cudaMalloc(&d_B, sB));
    CUDA_CHECK(cudaMalloc(&d_C, sC));

    GpuTimer gt;
    gt.start();
    CUDA_CHECK(cudaMemcpy(d_A, h_A.data(), sA, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_B, h_B.data(), sB, cudaMemcpyHostToDevice));
    dim3 block(16, 16);
    dim3 grid((N + 15) / 16, (M + 15) / 16);
    matMulKernel<<<grid, block>>>(d_A, d_B, d_C, M, N, K);
    CUDA_CHECK(cudaMemcpy(h_C.data(), d_C, sC, cudaMemcpyDeviceToHost));
    double gpuMs = gt.elapsedMs();

    cudaFree(d_A); cudaFree(d_B); cudaFree(d_C);

    char note[64];
    snprintf(note, sizeof(note), "Матрицы %d×%d × %d×%d", M, K, K, N);
    printResult("Умножение матриц (float, наивное)", cpuMs, gpuMs, note);
}

// ═══════════════════════════════════════════════════
//  ТЕСТ 3: РЕДУКЦИЯ (сумма массива)
// ═══════════════════════════════════════════════════
__global__ void reduceKernel(const float* __restrict__ in,
                              float*       __restrict__ out,
                              int N)
{
    extern __shared__ float sdata[];
    int tid = threadIdx.x;
    int i   = blockIdx.x * blockDim.x * 2 + threadIdx.x;

    sdata[tid] = (i < N ? in[i] : 0.f) +
                 (i + blockDim.x < N ? in[i + blockDim.x] : 0.f);
    __syncthreads();

    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) sdata[tid] += sdata[tid + s];
        __syncthreads();
    }
    if (tid == 0) out[blockIdx.x] = sdata[0];
}

void benchmarkReduce(int N = 1 << 25 /* ~33M */)
{
    size_t bytes = (size_t)N * sizeof(float);
    std::vector<float> h_in(N);
    std::mt19937 rng(13);
    std::uniform_real_distribution<float> dist(0.f, 1.f);
    for (auto& v : h_in) v = dist(rng);

    // ── CPU ──
    CpuTimer ct;
    ct.start();
    double cpuSum = 0.0;
    for (int i = 0; i < N; ++i) cpuSum += h_in[i];
    double cpuMs = ct.elapsedMs();

    // ── GPU ──
    const int THREADS = 256;
    int blocks = (N + THREADS * 2 - 1) / (THREADS * 2);

    float *d_in, *d_out;
    CUDA_CHECK(cudaMalloc(&d_in,  bytes));
    CUDA_CHECK(cudaMalloc(&d_out, (size_t)blocks * sizeof(float)));

    GpuTimer gt;
    gt.start();
    CUDA_CHECK(cudaMemcpy(d_in, h_in.data(), bytes, cudaMemcpyHostToDevice));
    reduceKernel<<<blocks, THREADS, THREADS * sizeof(float)>>>(d_in, d_out, N);
    // Финальная редукция на CPU из маленького массива блоков
    std::vector<float> h_out(blocks);
    CUDA_CHECK(cudaMemcpy(h_out.data(), d_out,
                          (size_t)blocks * sizeof(float), cudaMemcpyDeviceToHost));
    double gpuSum = 0.0;
    for (auto v : h_out) gpuSum += v;
    double gpuMs = gt.elapsedMs();

    cudaFree(d_in); cudaFree(d_out);

    char note[64];
    snprintf(note, sizeof(note), "N = %d | CPU=%.2f GPU=%.2f", N, cpuSum, gpuSum);
    printResult("Редукция (сумма float)", cpuMs, gpuMs, note);
}

// ═══════════════════════════════════════════════════
//  ТЕСТ 4: МОНТЕ-КАРЛО ОЦЕНКА π
// ═══════════════════════════════════════════════════
__global__ void monteCarloKernel(unsigned long long seed,
                                  int* __restrict__ count,
                                  int  N)
{
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id >= N) return;

    curandState state;
    curand_init(seed, id, 0, &state);

    float x = curand_uniform(&state);
    float y = curand_uniform(&state);
    if (x * x + y * y <= 1.f)
        atomicAdd(count, 1);
}

void benchmarkMonteCarlo(int N = 1 << 22 /* ~4M точек */)
{
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    // ── CPU ──
    CpuTimer ct;
    ct.start();
    long long cpuHit = 0;
    for (int i = 0; i < N; ++i) {
        double x = dist(rng), y = dist(rng);
        if (x * x + y * y <= 1.0) ++cpuHit;
    }
    double cpuPi = 4.0 * cpuHit / N;
    double cpuMs = ct.elapsedMs();

    // ── GPU ──
    int* d_count;
    CUDA_CHECK(cudaMalloc(&d_count, sizeof(int)));
    CUDA_CHECK(cudaMemset(d_count, 0, sizeof(int)));

    const int THREADS = 256;
    int blocks = (N + THREADS - 1) / THREADS;

    GpuTimer gt;
    gt.start();
    monteCarloKernel<<<blocks, THREADS>>>(123456789ULL, d_count, N);
    int h_count = 0;
    CUDA_CHECK(cudaMemcpy(&h_count, d_count, sizeof(int), cudaMemcpyDeviceToHost));
    double gpuMs = gt.elapsedMs();
    double gpuPi = 4.0 * h_count / N;
    cudaFree(d_count);

    char note[80];
    snprintf(note, sizeof(note), "N = %d | π_cpu=%.6f π_gpu=%.6f", N, cpuPi, gpuPi);
    printResult("Монте-Карло (π)", cpuMs, gpuMs, note);
}

// ═══════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════
int main()
{
    // Информация об устройстве
    int devCount = 0;
    CUDA_CHECK(cudaGetDeviceCount(&devCount));
    if (devCount == 0) {
        fprintf(stderr, "CUDA-устройства не найдены!\n");
        return EXIT_FAILURE;
    }

    cudaDeviceProp prop;
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));

    std::cout << "\n";
    printSeparator();
    std::cout << "  GPU : " << prop.name << "\n";
    std::cout << "  Вычислительные возможности: " << prop.major << "." << prop.minor << "\n";
    std::cout << "  SM  : " << prop.multiProcessorCount << "  |  "
              << "Память: " << prop.totalGlobalMem / (1 << 20) << " МБ  |  "
              << "Шина: " << prop.memoryBusWidth << " бит\n";
    
    printSeparator();
    std::cout << "  Все GPU-тайминги включают копирование данных (H→D→H)\n";
    std::cout << "  Это отражает реальную ситуацию при использовании GPU\n";
    printSeparator();
    std::cout << "\n  Запуск тестов...\n";

    benchmarkVecAdd();
    benchmarkMatMul();
    benchmarkReduce();
    benchmarkMonteCarlo();

    printSeparator();
    std::cout << "\n  Бенчмарк завершён.\n\n";
    return 0;
}
