/*
 * ============================================================
 *  CPU vs GPU Benchmark  |  RTX 3050 Mobile  vs  CPU
 *  Tasks: Matrix Multiply · N-Body · Monte Carlo Pi · Reduction
 * ============================================================
 *  Build:
 *    nvcc -O3 -arch=sm_86 -std=c++17 benchmark.cu -o benchmark
 *  Run:
 *    ./benchmark
 * ============================================================
 */

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <float.h>

// Stringify helper — converts macro value to 

#define XSTR(x) #x
#define STR(x)  XSTR(x)

#ifdef _WIN32
  #include <windows.h>
  #define GET_TIME_MS() ((double)GetTickCount64())
#else
  #include <sys/time.h>
  static double GET_TIME_MS() {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
  }
#endif

// ─────────────────────────────────────────────────────────────
//  Config
// ─────────────────────────────────────────────────────────────
#define MAT_N       10000 // matrix size NxN
#define NBODY_N     65536        // number of particles
#define NBODY_STEPS 50
#define MC_SAMPLES  (1 << 28)    // ~268M samples
#define REDUCE_N    (1 << 27)    // ~134M floats
#define TILE        32           // CUDA tile for matmul

// ─────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────
#define CHECK(call) \
  do { cudaError_t err = call; \
       if (err != cudaSuccess) { \
           fprintf(stderr, "CUDA error %s:%d  %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
           exit(EXIT_FAILURE); } } while(0)

static void print_header(const char *title) {
    printf("\n\033[1;36m══════════════════════════════════════════════════════\033[0m\n");
    printf("\033[1;33m  %-50s\033[0m\n", title);
    printf("\033[1;36m══════════════════════════════════════════════════════\033[0m\n");
}

static void print_result(const char *label, double cpu_ms, double gpu_ms) {
    double speedup = cpu_ms / gpu_ms;
    const char *winner = (speedup > 1.0) ? "\033[1;32mGPU wins\033[0m" : "\033[1;31mCPU wins\033[0m";
    if (speedup < 1.0) speedup = 1.0 / speedup;
    printf("  %-20s  CPU: %8.1f ms   GPU: %8.1f ms   Speedup: \033[1;35m%.1fx\033[0m  %s\n",
           label, cpu_ms, gpu_ms, speedup, winner);
}

// ─────────────────────────────────────────────────────────────
//  1. MATRIX MULTIPLY
// ─────────────────────────────────────────────────────────────

// GPU kernel – tiled GEMM
__global__ void matmul_kernel(const float* __restrict__ A,
                               const float* __restrict__ B,
                               float* __restrict__ C, int N)
{
    __shared__ float As[TILE][TILE];
    __shared__ float Bs[TILE][TILE];

    int row = blockIdx.y * TILE + threadIdx.y;
    int col = blockIdx.x * TILE + threadIdx.x;
    float acc = 0.0f;

    for (int t = 0; t < N / TILE; ++t) {
        As[threadIdx.y][threadIdx.x] = A[row * N + t * TILE + threadIdx.x];
        Bs[threadIdx.y][threadIdx.x] = B[(t * TILE + threadIdx.y) * N + col];
        __syncthreads();
        #pragma unroll
        for (int k = 0; k < TILE; ++k)
            acc += As[threadIdx.y][k] * Bs[k][threadIdx.x];
        __syncthreads();
    }
    if (row < N && col < N) C[row * N + col] = acc;
}

// CPU: naive triple loop (realistic worst-case)
static void cpu_matmul(const float* A, const float* B, float* C, int N) {
    for (int i = 0; i < N; ++i)
        for (int k = 0; k < N; ++k) {
            float a = A[i * N + k];
            for (int j = 0; j < N; ++j)
                C[i * N + j] += a * B[k * N + j];
        }
}

static void bench_matmul() {
    print_header("BENCHMARK 1 — Matrix Multiply  (FP32, " STR(MAT_N) "x" STR(MAT_N) ")");
    int N = MAT_N;
    size_t bytes = (size_t)N * N * sizeof(float);

    float *hA = (float*)malloc(bytes);
    float *hB = (float*)malloc(bytes);
    float *hC = (float*)calloc(N*N, sizeof(float));

    for (int i = 0; i < N*N; ++i) { hA[i] = (float)rand()/RAND_MAX; hB[i] = (float)rand()/RAND_MAX; }

    // ── CPU ──
    printf("  Running CPU matmul (this takes a while)...\n");
    // Use smaller N for CPU to avoid 10-minute wait; scale result
    int cpuN = 1024;
    size_t cpuBytes = (size_t)cpuN * cpuN * sizeof(float);
    float *cA = (float*)malloc(cpuBytes), *cB = (float*)malloc(cpuBytes), *cC = (float*)calloc(cpuN*cpuN, sizeof(float));
    for (int i = 0; i < cpuN*cpuN; ++i) { cA[i] = hA[i]; cB[i] = hB[i]; }
    double t0 = GET_TIME_MS();
    cpu_matmul(cA, cB, cC, cpuN);
    double cpu_ms_raw = GET_TIME_MS() - t0;
    // Scale: O(N^3), so full N would be (N/cpuN)^3 * raw
    double scale = (double)N/cpuN;
    double cpu_ms = cpu_ms_raw * scale * scale * scale;
    printf("  CPU 1024x1024 done in %.1f ms  =>  estimated %dx%d: %.1f ms\n", cpu_ms_raw, N, N, cpu_ms);
    free(cA); free(cB); free(cC);

    // ── GPU ──
    float *dA, *dB, *dC;
    CHECK(cudaMalloc(&dA, bytes));
    CHECK(cudaMalloc(&dB, bytes));
    CHECK(cudaMalloc(&dC, bytes));
    CHECK(cudaMemcpy(dA, hA, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(dB, hB, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaMemset(dC, 0, bytes));

    dim3 block(TILE, TILE);
    dim3 grid(N/TILE, N/TILE);

    // warm-up
    matmul_kernel<<<grid, block>>>(dA, dB, dC, N);
    cudaDeviceSynchronize();

    cudaEvent_t ev0, ev1;
    cudaEventCreate(&ev0); cudaEventCreate(&ev1);
    cudaEventRecord(ev0);
    matmul_kernel<<<grid, block>>>(dA, dB, dC, N);
    cudaEventRecord(ev1);
    cudaEventSynchronize(ev1);
    float gpu_ms = 0;
    cudaEventElapsedTime(&gpu_ms, ev0, ev1);

    double gflops_gpu = 2.0*N*N*N / (gpu_ms * 1e9);
    printf("  GPU throughput: %.1f GFLOPS\n", gflops_gpu);

    print_result("MatMul", cpu_ms, (double)gpu_ms);

    cudaFree(dA); cudaFree(dB); cudaFree(dC);
    free(hA); free(hB); free(hC);
    cudaEventDestroy(ev0); cudaEventDestroy(ev1);
}

// ─────────────────────────────────────────────────────────────
//  2. N-BODY GRAVITATIONAL SIMULATION
// ─────────────────────────────────────────────────────────────

struct float4_s { float x, y, z, w; };

__global__ void nbody_kernel(float4_s* __restrict__ pos,
                              float4_s* __restrict__ vel,
                              int N, float dt)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;

    float px = pos[i].x, py = pos[i].y, pz = pos[i].z;
    float ax = 0, ay = 0, az = 0;
    const float softening = 1e-9f;

    for (int j = 0; j < N; ++j) {
        float dx = pos[j].x - px;
        float dy = pos[j].y - py;
        float dz = pos[j].z - pz;
        float dist2 = dx*dx + dy*dy + dz*dz + softening;
        float inv_dist3 = rsqrtf(dist2 * dist2 * dist2);
        float mass = pos[j].w;
        ax += mass * dx * inv_dist3;
        ay += mass * dy * inv_dist3;
        az += mass * dz * inv_dist3;
    }

    vel[i].x += ax * dt;
    vel[i].y += ay * dt;
    vel[i].z += az * dt;
    pos[i].x += vel[i].x * dt;
    pos[i].y += vel[i].y * dt;
    pos[i].z += vel[i].z * dt;
}

static void cpu_nbody_step(float4_s* pos, float4_s* vel, int N, float dt) {
    const float softening = 1e-9f;
    for (int i = 0; i < N; ++i) {
        float px = pos[i].x, py = pos[i].y, pz = pos[i].z;
        float ax = 0, ay = 0, az = 0;
        for (int j = 0; j < N; ++j) {
            float dx = pos[j].x - px, dy = pos[j].y - py, dz = pos[j].z - pz;
            float dist2 = dx*dx + dy*dy + dz*dz + softening;
            float inv = 1.0f / sqrtf(dist2 * dist2 * dist2);
            ax += pos[j].w * dx * inv;
            ay += pos[j].w * dy * inv;
            az += pos[j].w * dz * inv;
        }
        vel[i].x += ax*dt; vel[i].y += ay*dt; vel[i].z += az*dt;
        pos[i].x += vel[i].x*dt; pos[i].y += vel[i].y*dt; pos[i].z += vel[i].z*dt;
    }
}

static void bench_nbody() {
    print_header("BENCHMARK 2 — N-Body Gravity  (N=" STR(NBODY_N) ", steps=" STR(NBODY_STEPS) ")");
    int N = NBODY_N;
    size_t bytes = N * sizeof(float4_s);

    float4_s *hPos = (float4_s*)malloc(bytes);
    float4_s *hVel = (float4_s*)calloc(N, sizeof(float4_s));
    for (int i = 0; i < N; ++i) {
        hPos[i].x = (float)rand()/RAND_MAX * 2 - 1;
        hPos[i].y = (float)rand()/RAND_MAX * 2 - 1;
        hPos[i].z = (float)rand()/RAND_MAX * 2 - 1;
        hPos[i].w = 1.0f / N;
    }

    // ── CPU (smaller N, scaled) ──
    int cpuN = 4096;
    float4_s *cPos = (float4_s*)malloc(cpuN * sizeof(float4_s));
    float4_s *cVel = (float4_s*)calloc(cpuN, sizeof(float4_s));
    memcpy(cPos, hPos, cpuN * sizeof(float4_s));
    printf("  Running CPU N-body (N=%d, 1 step)...\n", cpuN);
    double t0 = GET_TIME_MS();
    cpu_nbody_step(cPos, cVel, cpuN, 0.01f);
    double cpu_step_ms = GET_TIME_MS() - t0;
    // Scale to full N×STEPS: O(N^2) per step
    double cpu_ms = cpu_step_ms * ((double)N/cpuN)*((double)N/cpuN) * NBODY_STEPS;
    printf("  CPU %d-body 1 step: %.1f ms  =>  estimated full: %.1f ms\n", cpuN, cpu_step_ms, cpu_ms);
    free(cPos); free(cVel);

    // ── GPU ──
    float4_s *dPos, *dVel;
    CHECK(cudaMalloc(&dPos, bytes));
    CHECK(cudaMalloc(&dVel, bytes));
    CHECK(cudaMemcpy(dPos, hPos, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaMemset(dVel, 0, bytes));

    int threads = 256;
    int blocks  = (N + threads - 1) / threads;

    // warm-up
    nbody_kernel<<<blocks, threads>>>(dPos, dVel, N, 0.01f);
    cudaDeviceSynchronize();

    cudaEvent_t ev0, ev1;
    cudaEventCreate(&ev0); cudaEventCreate(&ev1);
    cudaEventRecord(ev0);
    for (int s = 0; s < NBODY_STEPS; ++s)
        nbody_kernel<<<blocks, threads>>>(dPos, dVel, N, 0.01f);
    cudaEventRecord(ev1);
    cudaEventSynchronize(ev1);
    float gpu_ms = 0;
    cudaEventElapsedTime(&gpu_ms, ev0, ev1);

    long long interactions = (long long)N * N * NBODY_STEPS;
    printf("  GPU interactions/sec: %.2f billion\n", interactions / (gpu_ms * 1e6));

    print_result("N-Body", cpu_ms, (double)gpu_ms);

    cudaFree(dPos); cudaFree(dVel);
    free(hPos); free(hVel);
    cudaEventDestroy(ev0); cudaEventDestroy(ev1);
}

// ─────────────────────────────────────────────────────────────
//  3. MONTE CARLO  π
// ─────────────────────────────────────────────────────────────

__global__ void mc_kernel(unsigned long long* hits, int samples_per_thread, unsigned long long seed) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned long long local_hits = 0;

    // LCG RNG per thread
    unsigned long long state = seed + (unsigned long long)tid * 6364136223846793005ULL + 1442695040888963407ULL;

    for (int i = 0; i < samples_per_thread; ++i) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        float x = (float)(state >> 33) / (float)(1u << 31) - 1.0f;
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        float y = (float)(state >> 33) / (float)(1u << 31) - 1.0f;
        if (x*x + y*y <= 1.0f) ++local_hits;
    }
    atomicAdd(hits, local_hits);
}

static void bench_montecarlo() {
    print_header("BENCHMARK 3 — Monte Carlo  π  (samples=" STR(MC_SAMPLES) ")");
    long long S = MC_SAMPLES;

    // ── CPU ──
    double t0 = GET_TIME_MS();
    long long hits = 0;
    unsigned long long state = 123456789012345ULL;
    for (long long i = 0; i < S; ++i) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        float x = (float)(state >> 33) / (float)(1u << 31) - 1.0f;
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        float y = (float)(state >> 33) / (float)(1u << 31) - 1.0f;
        if (x*x + y*y <= 1.0f) ++hits;
    }
    double cpu_ms = GET_TIME_MS() - t0;
    double pi_cpu = 4.0 * hits / S;
    printf("  CPU π ≈ %.8f  (error: %e)\n", pi_cpu, fabs(pi_cpu - M_PI));

    // ── GPU ──
    int threads = 256;
    int blocks  = 4096;
    int spt     = (int)(S / (threads * blocks));

    unsigned long long *dHits;
    CHECK(cudaMalloc(&dHits, sizeof(unsigned long long)));
    CHECK(cudaMemset(dHits, 0, sizeof(unsigned long long)));

    // warm-up
    mc_kernel<<<blocks, threads>>>(dHits, spt, 999ULL);
    cudaDeviceSynchronize();
    CHECK(cudaMemset(dHits, 0, sizeof(unsigned long long)));

    cudaEvent_t ev0, ev1;
    cudaEventCreate(&ev0); cudaEventCreate(&ev1);
    cudaEventRecord(ev0);
    mc_kernel<<<blocks, threads>>>(dHits, spt, 42ULL);
    cudaEventRecord(ev1);
    cudaEventSynchronize(ev1);
    float gpu_ms = 0;
    cudaEventElapsedTime(&gpu_ms, ev0, ev1);

    unsigned long long hHits = 0;
    CHECK(cudaMemcpy(&hHits, dHits, sizeof(unsigned long long), cudaMemcpyDeviceToHost));
    long long actual_samples = (long long)spt * threads * blocks;
    double pi_gpu = 4.0 * hHits / actual_samples;
    printf("  GPU π ≈ %.8f  (error: %e)\n", pi_gpu, fabs(pi_gpu - M_PI));

    print_result("Monte Carlo π", cpu_ms, (double)gpu_ms);

    cudaFree(dHits);
    cudaEventDestroy(ev0); cudaEventDestroy(ev1);
}

// ─────────────────────────────────────────────────────────────
//  4. PARALLEL REDUCTION (SUM)
// ─────────────────────────────────────────────────────────────

__global__ void reduce_kernel(const float* __restrict__ in, float* __restrict__ out, int N) {
    extern __shared__ float sdata[];
    int tid  = threadIdx.x;
    int gid  = blockIdx.x * blockDim.x * 2 + threadIdx.x;
    float val = 0.0f;
    if (gid < N)          val += in[gid];
    if (gid + blockDim.x < N) val += in[gid + blockDim.x];
    sdata[tid] = val;
    __syncthreads();
    for (int s = blockDim.x >> 1; s > 0; s >>= 1) {
        if (tid < s) sdata[tid] += sdata[tid + s];
        __syncthreads();
    }
    if (tid == 0) out[blockIdx.x] = sdata[0];
}

static void bench_reduction() {
    print_header("BENCHMARK 4 — Parallel Reduction (sum, N=" STR(REDUCE_N) ")");
    int N = REDUCE_N;
    size_t bytes = (size_t)N * sizeof(float);

    float *h = (float*)malloc(bytes);
    for (int i = 0; i < N; ++i) h[i] = (float)rand()/RAND_MAX;

    // ── CPU ──
    double t0 = GET_TIME_MS();
    double sum = 0.0;
    for (int i = 0; i < N; ++i) sum += h[i];
    double cpu_ms = GET_TIME_MS() - t0;
    printf("  CPU sum = %.4f\n", sum);

    // ── GPU (two-pass) ──
    int threads = 256;
    int blocks  = (N + threads*2 - 1) / (threads * 2);
    float *dIn, *dOut, *dOut2;
    CHECK(cudaMalloc(&dIn,   bytes));
    CHECK(cudaMalloc(&dOut,  blocks * sizeof(float)));
    CHECK(cudaMalloc(&dOut2, sizeof(float)));
    CHECK(cudaMemcpy(dIn, h, bytes, cudaMemcpyHostToDevice));

    // warm-up
    reduce_kernel<<<blocks, threads, threads*sizeof(float)>>>(dIn, dOut, N);
    reduce_kernel<<<1, threads, threads*sizeof(float)>>>(dOut, dOut2, blocks);
    cudaDeviceSynchronize();

    cudaEvent_t ev0, ev1;
    cudaEventCreate(&ev0); cudaEventCreate(&ev1);
    cudaEventRecord(ev0);
    for (int rep = 0; rep < 100; ++rep) {
        reduce_kernel<<<blocks, threads, threads*sizeof(float)>>>(dIn, dOut, N);
        reduce_kernel<<<1, threads, threads*sizeof(float)>>>(dOut, dOut2, blocks);
    }
    cudaEventRecord(ev1);
    cudaEventSynchronize(ev1);
    float gpu_total_ms = 0;
    cudaEventElapsedTime(&gpu_total_ms, ev0, ev1);
    float gpu_ms = gpu_total_ms / 100.0f;

    float gpu_sum = 0;
    CHECK(cudaMemcpy(&gpu_sum, dOut2, sizeof(float), cudaMemcpyDeviceToHost));
    printf("  GPU sum = %.4f\n", (double)gpu_sum);

    double bandwidth_gb = 2.0 * bytes / (gpu_ms * 1e6);
    printf("  GPU bandwidth: %.1f GB/s\n", bandwidth_gb);

    print_result("Reduction", cpu_ms, (double)gpu_ms);

    cudaFree(dIn); cudaFree(dOut); cudaFree(dOut2);
    free(h);
    cudaEventDestroy(ev0); cudaEventDestroy(ev1);
}

// ─────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────
int main() {
    printf("\n\033[1;37m╔══════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;37m║       CPU vs GPU BENCHMARK  (RTX 3050 Mobile)         ║\033[0m\n");
    printf("\033[1;37m╚══════════════════════════════════════════════════════╝\033[0m\n");

    // GPU info
    cudaDeviceProp prop;
    CHECK(cudaGetDeviceProperties(&prop, 0));
    printf("\n  \033[1;34mGPU:\033[0m %s\n", prop.name);
    printf("  \033[1;34mSMs:\033[0m %d  |  CUDA cores (est.): %d\n",
           prop.multiProcessorCount,
           prop.multiProcessorCount * 128);
    printf("  \033[1;34mVRAM:\033[0m %.1f GB  |  Bus width: %d-bit\n",
           prop.totalGlobalMem / 1e9,
           prop.memoryBusWidth);
    printf("  \033[1;34mCompute:\033[0m %d.%d  |  Warp size: %d\n",
           prop.major, prop.minor, prop.warpSize);

    srand(12345);

    bench_matmul();
    bench_nbody();
    bench_montecarlo();
    bench_reduction();

    printf("\n\033[1;36m══════════════════════════════════════════════════════\033[0m\n");
    printf("\033[1;33m  LEGEND: Speedup = faster / slower\033[0m\n");
    printf("\033[1;33m  MatMul speedup is EXTRAPOLATED (CPU measured at 1024x1024)\033[0m\n");
    printf("\033[1;33m  N-Body speedup is EXTRAPOLATED (CPU measured at N=4096)\033[0m\n");
    printf("\033[1;36m══════════════════════════════════════════════════════\033[0m\n\n");

    return 0;
}
