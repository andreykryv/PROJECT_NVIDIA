/*
 * ================================================================
 *   HEAVY CPU vs GPU BENCHMARK  |  ~60 sec per test
 *   RTX 3050 Mobile (sm_86) vs CPU
 * ----------------------------------------------------------------
 *   Test 1 : SGEMM        8192x8192,  80 repetitions
 *   Test 2 : N-Body       65536 bodies, 1500 steps (tiled shmem)
 *   Test 3 : Heat Stencil 16384x16384, 900 iterations
 *   Test 4 : Bitonic Sort 32M floats,  75 sorts
 * ----------------------------------------------------------------
 *   Build:
 *     nvcc -O3 -arch=sm_86 --use_fast_math -std=c++17 \
 *          benchmark_heavy.cu -o bench_heavy
 *   Run:
 *     ./bench_heavy
 * =========================================================
 */

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* Stringify helper */
#define XSTR(x) #x
#define STR(x)  XSTR(x)

#ifdef _WIN32
  #include <windows.h>
  static double get_ms() { return (double)GetTickCount64(); }
#else
  #include <sys/time.h>
  static double get_ms() {
      struct timeval tv; gettimeofday(&tv, NULL);
      return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
  }
#endif

/* ──────────────────────────────────────────────────────────── */
/*  Workload constants  (tune if needed)                        */
/* ──────────────────────────────────────────────────────────── */
#define SGEMM_N       8192   /* matrix side length               */
#define SGEMM_REPS    80     /* repetitions  → ~30-60s GPU       */

#define NBODY_N       65536  /* must be multiple of NBODY_TILE   */
#define NBODY_TILE    256    /* shared-memory tile               */
#define NBODY_STEPS   1500   /* simulation steps  → ~30-60s GPU  */

#define HEAT_W        16384  /* grid width                       */
#define HEAT_H        16384  /* grid height                      */
#define HEAT_ITERS    900    /* iterations  → ~30-60s GPU        */

#define SORT_N        (1<<25)/* 32M floats  (must be power of 2) */
#define SORT_REPS     75     /* sorts  → ~30-60s GPU             */

/* CPU scale-down factor: measure 1/SCALE work, then extrapolate */
#define CPU_SCALE     40

/* ──────────────────────────────────────────────────────────── */
/*  Helpers                                                     */
/* ──────────────────────────────────────────────────────────── */
#define CHECK(call) \
  do { cudaError_t _e = (call); \
       if (_e != cudaSuccess) { \
           fprintf(stderr, "CUDA %s @ %s:%d\n", \
                   cudaGetErrorString(_e), __FILE__, __LINE__); \
           exit(1); } } while(0)

static void banner(const char *title) {
    printf("\n\033[1;36m╔══════════════════════════════════════════════════════╗\033[0m\n");
    printf(  "\033[1;33m  %s\033[0m\n", title);
    printf(  "\033[1;36m╚══════════════════════════════════════════════════════╝\033[0m\n");
}

static void result(const char *lbl, double cpu_ms, double gpu_ms) {
    double sp = cpu_ms / gpu_ms;
    const char *w = (sp >= 1.0) ? "\033[1;32mGPU\033[0m" : "\033[1;31mCPU\033[0m";
    if (sp < 1.0) sp = 1.0 / sp;
    printf("  \033[1mResult\033[0m  CPU(est): %8.1f s    GPU: %8.1f s    "
           "Speedup: \033[1;35m%.1fx\033[0m  [%s wins]\n",
           cpu_ms / 1000.0, gpu_ms / 1000.0, sp, w);
}

/* ================================================================
   TEST 1 — HEAVY SGEMM  (tiled, FP32)
   ================================================================ */
#define TILE 32

__global__ void sgemm(const float* __restrict__ A,
                      const float* __restrict__ B,
                      float* __restrict__ C, int N)
{
    __shared__ float sA[TILE][TILE], sB[TILE][TILE];
    int row = blockIdx.y * TILE + threadIdx.y;
    int col = blockIdx.x * TILE + threadIdx.x;
    float acc = 0.f;
    for (int t = 0; t < N / TILE; ++t) {
        sA[threadIdx.y][threadIdx.x] = A[row * N + t * TILE + threadIdx.x];
        sB[threadIdx.y][threadIdx.x] = B[(t * TILE + threadIdx.y) * N + col];
        __syncthreads();
        #pragma unroll
        for (int k = 0; k < TILE; ++k) acc += sA[threadIdx.y][k] * sB[k][threadIdx.x];
        __syncthreads();
    }
    C[row * N + col] = acc;
}

static void cpu_sgemm_ikj(const float* A, const float* B, float* C, int N) {
    memset(C, 0, (size_t)N * N * sizeof(float));
    for (int i = 0; i < N; ++i)
        for (int k = 0; k < N; ++k) {
            float a = A[i * N + k];
            for (int j = 0; j < N; ++j)
                C[i * N + j] += a * B[k * N + j];
        }
}

static void bench_sgemm(void) {
    banner("TEST 1 — SGEMM  FP32  " STR(SGEMM_N) "x" STR(SGEMM_N)
           "  x" STR(SGEMM_REPS) " reps");

    int N = SGEMM_N;
    size_t bytes = (size_t)N * N * sizeof(float);

    float *hA = (float*)malloc(bytes), *hB = (float*)malloc(bytes);
    for (int i = 0; i < N * N; ++i) { hA[i] = (float)rand()/RAND_MAX; hB[i] = (float)rand()/RAND_MAX; }

    /* ── CPU: measure on (N/scale) x (N/scale), extrapolate O(N^3 * reps) ── */
    int cN = N / CPU_SCALE;   /* 8192 / 40 ≈ 204, round to nearest TILE multiple */
    cN = (cN / TILE) * TILE;
    if (cN < TILE) cN = TILE;
    float *cA = (float*)malloc((size_t)cN*cN*sizeof(float));
    float *cB = (float*)malloc((size_t)cN*cN*sizeof(float));
    float *cC = (float*)malloc((size_t)cN*cN*sizeof(float));
    memcpy(cA, hA, (size_t)cN*cN*sizeof(float));
    memcpy(cB, hB, (size_t)cN*cN*sizeof(float));

    printf("  CPU sample: %dx%d × 1 rep ...\n", cN, cN);
    double tc0 = get_ms();
    cpu_sgemm_ikj(cA, cB, cC, cN);
    double cpu_sample_ms = get_ms() - tc0;
    double scale3 = (double)N/cN; scale3 = scale3*scale3*scale3;
    double cpu_ms = cpu_sample_ms * scale3 * SGEMM_REPS;
    printf("  CPU %dx%d ×1 = %.1f ms  →  est. full = \033[1;31m%.1f s\033[0m\n",
           cN, cN, cpu_sample_ms, cpu_ms / 1000.0);
    free(cA); free(cB); free(cC);

    /* ── GPU ── */
    float *dA, *dB, *dC;
    CHECK(cudaMalloc(&dA, bytes)); CHECK(cudaMalloc(&dB, bytes)); CHECK(cudaMalloc(&dC, bytes));
    CHECK(cudaMemcpy(dA, hA, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(dB, hB, bytes, cudaMemcpyHostToDevice));

    dim3 blk(TILE, TILE), grd(N/TILE, N/TILE);
    /* warm-up */
    sgemm<<<grd,blk>>>(dA, dB, dC, N); cudaDeviceSynchronize();

    printf("  GPU running %d reps...\n", SGEMM_REPS);
    cudaEvent_t e0, e1; cudaEventCreate(&e0); cudaEventCreate(&e1);
    cudaEventRecord(e0);
    for (int r = 0; r < SGEMM_REPS; ++r) sgemm<<<grd,blk>>>(dA, dB, dC, N);
    cudaEventRecord(e1); cudaEventSynchronize(e1);
    float gpu_ms; cudaEventElapsedTime(&gpu_ms, e0, e1);

    double tflops = 2.0 * N * N * N * SGEMM_REPS / (gpu_ms * 1e9);
    printf("  GPU throughput: \033[1;34m%.2f TFLOPS\033[0m\n", tflops);
    result("SGEMM", cpu_ms, gpu_ms);

    cudaFree(dA); cudaFree(dB); cudaFree(dC);
    free(hA); free(hB);
    cudaEventDestroy(e0); cudaEventDestroy(e1);
}

/* ================================================================
   TEST 2 — N-BODY (tiled shared memory)
   ================================================================ */

__global__ void nbody_tiled(float4* __restrict__ pos,
                             float4* __restrict__ vel,
                             int N, float dt)
{
    extern __shared__ float4 tile[];
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    float4 pi = pos[i];
    float ax = 0.f, ay = 0.f, az = 0.f;

    for (int t = 0; t < N / NBODY_TILE; ++t) {
        tile[threadIdx.x] = pos[t * NBODY_TILE + threadIdx.x];
        __syncthreads();
        #pragma unroll 8
        for (int j = 0; j < NBODY_TILE; ++j) {
            float dx = tile[j].x - pi.x;
            float dy = tile[j].y - pi.y;
            float dz = tile[j].z - pi.z;
            float r2 = dx*dx + dy*dy + dz*dz + 1e-10f;
            float inv = rsqrtf(r2 * r2 * r2) * tile[j].w;
            ax += inv * dx; ay += inv * dy; az += inv * dz;
        }
        __syncthreads();
    }
    float4 vi = vel[i];
    vi.x += ax*dt; vi.y += ay*dt; vi.z += az*dt;
    pi.x += vi.x*dt; pi.y += vi.y*dt; pi.z += vi.z*dt;
    vel[i] = vi; pos[i] = pi;
}

static void cpu_nbody_tiled(float4* pos, float4* vel, int N, float dt) {
    for (int i = 0; i < N; ++i) {
        float px=pos[i].x, py=pos[i].y, pz=pos[i].z;
        float ax=0,ay=0,az=0;
        for (int j = 0; j < N; ++j) {
            float dx=pos[j].x-px, dy=pos[j].y-py, dz=pos[j].z-pz;
            float r2 = dx*dx+dy*dy+dz*dz+1e-10f;
            float inv = 1.f/sqrtf(r2*r2*r2) * pos[j].w;
            ax+=inv*dx; ay+=inv*dy; az+=inv*dz;
        }
        vel[i].x+=ax*dt; vel[i].y+=ay*dt; vel[i].z+=az*dt;
        pos[i].x+=vel[i].x*dt; pos[i].y+=vel[i].y*dt; pos[i].z+=vel[i].z*dt;
    }
}

static void bench_nbody(void) {
    banner("TEST 2 — N-Body  N=" STR(NBODY_N) "  steps=" STR(NBODY_STEPS)
           "  (tiled shmem)");

    int N = NBODY_N;
    size_t bytes = N * sizeof(float4);
    float4 *hPos = (float4*)malloc(bytes), *hVel = (float4*)calloc(N, sizeof(float4));
    for (int i = 0; i < N; ++i) {
        hPos[i] = {(float)rand()/RAND_MAX*2-1,
                   (float)rand()/RAND_MAX*2-1,
                   (float)rand()/RAND_MAX*2-1,
                   1.f/N};
    }

    /* ── CPU sample ── */
    int cN = NBODY_TILE;   /* one tile worth of bodies */
    float4 *cP = (float4*)malloc(cN*sizeof(float4));
    float4 *cV = (float4*)calloc(cN, sizeof(float4));
    memcpy(cP, hPos, cN*sizeof(float4));
    printf("  CPU sample: %d bodies × 1 step ...\n", cN);
    double tc0 = get_ms();
    cpu_nbody_tiled(cP, cV, cN, 0.001f);
    double cpu_one_ms = get_ms() - tc0;
    /* O(N^2 * steps) */
    double sc = (double)N/cN;
    double cpu_ms = cpu_one_ms * sc * sc * NBODY_STEPS;
    printf("  CPU %d × 1 step = %.3f ms  →  est. full = \033[1;31m%.1f s\033[0m\n",
           cN, cpu_one_ms, cpu_ms / 1000.0);
    free(cP); free(cV);

    /* ── GPU ── */
    float4 *dPos, *dVel;
    CHECK(cudaMalloc(&dPos, bytes)); CHECK(cudaMalloc(&dVel, bytes));
    CHECK(cudaMemcpy(dPos, hPos, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaMemset(dVel, 0, bytes));

    int blk = NBODY_TILE, grd = N / blk;
    size_t shmem = blk * sizeof(float4);

    /* warm-up */
    nbody_tiled<<<grd, blk, shmem>>>(dPos, dVel, N, 0.001f);
    cudaDeviceSynchronize();

    printf("  GPU running %d steps...\n", NBODY_STEPS);
    cudaEvent_t e0, e1; cudaEventCreate(&e0); cudaEventCreate(&e1);
    cudaEventRecord(e0);
    for (int s = 0; s < NBODY_STEPS; ++s)
        nbody_tiled<<<grd, blk, shmem>>>(dPos, dVel, N, 0.001f);
    cudaEventRecord(e1); cudaEventSynchronize(e1);
    float gpu_ms; cudaEventElapsedTime(&gpu_ms, e0, e1);

    long long inter = (long long)N * N * NBODY_STEPS;
    printf("  GPU interactions/s: \033[1;34m%.2f billion\033[0m\n",
           inter / (gpu_ms * 1e6));
    result("N-Body", cpu_ms, gpu_ms);

    cudaFree(dPos); cudaFree(dVel);
    free(hPos); free(hVel);
    cudaEventDestroy(e0); cudaEventDestroy(e1);
}

/* ================================================================
   TEST 3 — HEAT DIFFUSION STENCIL  (5-point, ping-pong)
   ================================================================ */
#define HEAT_BX 32
#define HEAT_BY 8

__global__ void heat_step(const float* __restrict__ src,
                                 float* __restrict__ dst,
                                 int W, int H)
{
    int x = blockIdx.x * HEAT_BX + threadIdx.x;
    int y = blockIdx.y * HEAT_BY + threadIdx.y;
    if (x == 0 || x >= W-1 || y == 0 || y >= H-1) {
        if (x < W && y < H) dst[y*W+x] = src[y*W+x];
        return;
    }
    dst[y*W+x] = 0.25f * (src[(y-1)*W+x] + src[(y+1)*W+x] +
                           src[y*W+(x-1)] + src[y*W+(x+1)]);
}

static void bench_heat(void) {
    banner("TEST 3 — Heat Diffusion Stencil  "
           STR(HEAT_W) "x" STR(HEAT_H) "  x" STR(HEAT_ITERS) " iters");

    size_t bytes = (size_t)HEAT_W * HEAT_H * sizeof(float);
    float *hGrid = (float*)malloc(bytes);
    for (size_t i = 0; i < (size_t)HEAT_W * HEAT_H; ++i)
        hGrid[i] = (float)(rand() & 0xFF);

    /* ── CPU sample: small grid for 1 iter, O(W*H*iters) ── */
    int cW = HEAT_W / 32, cH = HEAT_H / 32;
    float *cA = (float*)malloc(cW * cH * sizeof(float));
    float *cB = (float*)malloc(cW * cH * sizeof(float));
    for (int i = 0; i < cW*cH; ++i) cA[i] = (float)(rand()&0xFF);
    printf("  CPU sample: %dx%d × 1 iter ...\n", cW, cH);
    double tc0 = get_ms();
    for (int y = 1; y < cH-1; ++y)
        for (int x = 1; x < cW-1; ++x)
            cB[y*cW+x] = 0.25f*(cA[(y-1)*cW+x]+cA[(y+1)*cW+x]+
                                 cA[y*cW+(x-1)]+cA[y*cW+(x+1)]);
    double cpu_one_ms = get_ms() - tc0;
    double scW = (double)HEAT_W/cW, scH = (double)HEAT_H/cH;
    double cpu_ms = cpu_one_ms * scW * scH * HEAT_ITERS;
    printf("  CPU %dx%d × 1 iter = %.3f ms  →  est. full = \033[1;31m%.1f s\033[0m\n",
           cW, cH, cpu_one_ms, cpu_ms / 1000.0);
    free(cA); free(cB);

    /* ── GPU ── */
    float *dA, *dB;
    CHECK(cudaMalloc(&dA, bytes)); CHECK(cudaMalloc(&dB, bytes));
    CHECK(cudaMemcpy(dA, hGrid, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(dB, hGrid, bytes, cudaMemcpyHostToDevice));

    dim3 blk(HEAT_BX, HEAT_BY);
    dim3 grd((HEAT_W + HEAT_BX - 1) / HEAT_BX,
             (HEAT_H + HEAT_BY - 1) / HEAT_BY);

    /* warm-up 10 iters */
    for (int i = 0; i < 10; ++i) {
        heat_step<<<grd, blk>>>(dA, dB, HEAT_W, HEAT_H);
        float *tmp = dA; dA = dB; dB = tmp;
    }
    cudaDeviceSynchronize();

    printf("  GPU running %d iters...\n", HEAT_ITERS);
    cudaEvent_t e0, e1; cudaEventCreate(&e0); cudaEventCreate(&e1);
    cudaEventRecord(e0);
    for (int it = 0; it < HEAT_ITERS; ++it) {
        heat_step<<<grd, blk>>>(dA, dB, HEAT_W, HEAT_H);
        float *tmp = dA; dA = dB; dB = tmp;
    }
    cudaEventRecord(e1); cudaEventSynchronize(e1);
    float gpu_ms; cudaEventElapsedTime(&gpu_ms, e0, e1);

    double bw = (double)HEAT_ITERS * HEAT_W * HEAT_H * sizeof(float) * 5 / (gpu_ms * 1e6);
    printf("  GPU eff. bandwidth: \033[1;34m%.1f GB/s\033[0m\n", bw);
    result("Heat Stencil", cpu_ms, gpu_ms);

    cudaFree(dA); cudaFree(dB);
    free(hGrid);
    cudaEventDestroy(e0); cudaEventDestroy(e1);
}

/* ================================================================
   TEST 4 — BITONIC SORT  (32M floats)
   ================================================================ */
#define SORT_THREADS 256

__global__ void bitonic_step(float* data, int j, int k) {
    unsigned int i = threadIdx.x + blockDim.x * (unsigned int)blockIdx.x;
    unsigned int l = i ^ (unsigned int)j;
    if (l > i) {
        float a = data[i], b = data[l];
        bool swap_cond = (i & (unsigned int)k) == 0 ? (a > b) : (a < b);
        if (swap_cond) { data[i] = b; data[l] = a; }
    }
}

/* CPU: std qsort comparator */
static int cmp_float(const void *a, const void *b) {
    float fa = *(const float*)a, fb = *(const float*)b;
    return (fa > fb) - (fa < fb);
}

static void bench_sort(void) {
    banner("TEST 4 — Bitonic Sort  N=" STR(SORT_N) " (32M floats)"
           "  x" STR(SORT_REPS) " sorts");

    size_t bytes = (size_t)SORT_N * sizeof(float);
    float *hData = (float*)malloc(bytes);
    float *hOrig = (float*)malloc(bytes);
    for (int i = 0; i < SORT_N; ++i) hOrig[i] = (float)rand()/RAND_MAX;

    /* ── CPU sample: sort N/scale elements, scale by O(N log N) ── */
    int cN = SORT_N / CPU_SCALE;
    float *cD = (float*)malloc(cN * sizeof(float));
    memcpy(cD, hOrig, cN * sizeof(float));
    printf("  CPU sample: %d elements × 1 sort ...\n", cN);
    double tc0 = get_ms();
    qsort(cD, cN, sizeof(float), cmp_float);
    double cpu_one_ms = get_ms() - tc0;
    /* O(N log N): scale = (N/cN) * log(N)/log(cN) * SORT_REPS */
    double ratio = (double)SORT_N / cN;
    double logsc = log((double)SORT_N) / log((double)cN);
    double cpu_ms = cpu_one_ms * ratio * logsc * SORT_REPS;
    printf("  CPU %d elems ×1 = %.2f ms  →  est. full = \033[1;31m%.1f s\033[0m\n",
           cN, cpu_one_ms, cpu_ms / 1000.0);
    free(cD);

    /* ── GPU ── */
    float *dData;
    CHECK(cudaMalloc(&dData, bytes));

    int threads = SORT_THREADS;
    int blocks  = SORT_N / (2 * threads);

    /* warm-up 1 sort */
    memcpy(hData, hOrig, bytes);
    CHECK(cudaMemcpy(dData, hData, bytes, cudaMemcpyHostToDevice));
    for (int k = 2; k <= SORT_N; k <<= 1)
        for (int j = k >> 1; j > 0; j >>= 1)
            bitonic_step<<<blocks, threads>>>(dData, j, k);
    cudaDeviceSynchronize();

    printf("  GPU running %d sorts...\n", SORT_REPS);
    cudaEvent_t e0, e1; cudaEventCreate(&e0); cudaEventCreate(&e1);
    cudaEventRecord(e0);
    for (int rep = 0; rep < SORT_REPS; ++rep) {
        /* re-upload unsorted data each rep */
        CHECK(cudaMemcpyAsync(dData, hOrig, bytes, cudaMemcpyHostToDevice));
        for (int k = 2; k <= SORT_N; k <<= 1)
            for (int j = k >> 1; j > 0; j >>= 1)
                bitonic_step<<<blocks, threads>>>(dData, j, k);
    }
    cudaEventRecord(e1); cudaEventSynchronize(e1);
    float gpu_ms; cudaEventElapsedTime(&gpu_ms, e0, e1);

    printf("  GPU sorts/sec: \033[1;34m%.2f\033[0m\n",
           SORT_REPS / (gpu_ms / 1000.0));
    result("Bitonic Sort", cpu_ms, gpu_ms);

    cudaFree(dData);
    free(hData); free(hOrig);
    cudaEventDestroy(e0); cudaEventDestroy(e1);
}

/* ================================================================
   MAIN
   ================================================================ */
int main(void) {
    printf("\n");
    printf("\033[1;37m╔══════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;37m║        HEAVY CPU vs GPU BENCHMARK  (~60s / test)     ║\033[0m\n");
    printf("\033[1;37m╚══════════════════════════════════════════════════════╝\033[0m\n");

    /* GPU info (CUDA 12.x compatible) */
    cudaDeviceProp p; CHECK(cudaGetDeviceProperties(&p, 0));
    printf("\n  \033[1;34mGPU  :\033[0m %s\n", p.name);
    printf("  \033[1;34mSMs  :\033[0m %d    CUDA cores (est.): %d\n",
           p.multiProcessorCount, p.multiProcessorCount * 128);
    printf("  \033[1;34mVRAM :\033[0m %.1f GB   Bus: %d-bit   Compute: %d.%d\n",
           p.totalGlobalMem / 1e9, p.memoryBusWidth, p.major, p.minor);
    printf("  \033[1;34mWarp :\033[0m %d   Max threads/block: %d\n\n",
           p.warpSize, p.maxThreadsPerBlock);

    srand(0xDEADBEEF);

    bench_sgemm();
    bench_nbody();
    bench_heat();
    bench_sort();

    printf("\n\033[1;36m══════════════════════════════════════════════════════\033[0m\n");
    printf("\033[1;33m  NOTE: CPU times are EXTRAPOLATED from small samples.\033[0m\n");
    printf("\033[1;33m  GPU times are ACTUAL measured wall-clock durations.\033[0m\n");
    printf("\033[1;36m══════════════════════════════════════════════════════\033[0m\n\n");

    return 0;
}
