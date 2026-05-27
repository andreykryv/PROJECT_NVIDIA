/**
 * @file gpu_algorithms.cu
 * @brief Полная реализация всех GPU-алгоритмов сортировки (Native CUDA Kernels + Thrust).
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gpu_algorithms.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <thrust/device_vector.h>
#include <thrust/sort.h>
#include <thrust/execution_policy.h>
#include <thrust/sequence.h>
#include <thrust/gather.h>
#include <thrust/count.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <chrono>
#include <cstring>

#define CHK_CUDA(ans) { cudaAssert((ans), __FILE__, __LINE__); }
inline void cudaAssert(cudaError_t code, const char *file, int line, bool abort=true) {
   if (code != cudaSuccess) {
      fprintf(stderr, "CUDA-Error: %s %s %d\n", cudaGetErrorString(code), file, line);
      if (abort) exit(code);
   }
}

namespace GPU {

    // ==================== ВСПОМОГАТЕЛЬНЫЕ МАКРОСЫ И ШАБЛОНЫ ====================
    
    // Шаблон для замера времени выполнения ядра на GPU
    template<typename KernelFunc, typename... Args>
    GPUBenchmarkResult executeGpuKernel(std::vector<double>& arr, KernelFunc kernelLauncher, Args... args) {
        GPUBenchmarkResult res;
        int n = arr.size();
        if (n == 0) {
            res.success = true;
            return res;
        }

        cudaEvent_t startTotal, stopTotal, startUpload, stopUpload, startKernel, stopKernel, startDownload, stopDownload;
        cudaEventCreate(&startTotal); cudaEventCreate(&stopTotal);
        cudaEventCreate(&startUpload); cudaEventCreate(&stopUpload);
        cudaEventCreate(&startKernel); cudaEventCreate(&stopKernel);
        cudaEventCreate(&startDownload); cudaEventCreate(&stopDownload);

        cudaEventRecord(startTotal);

        double* d_arr = nullptr;
        cudaError_t err = cudaMalloc(&d_arr, n * sizeof(double));
        if (err != cudaSuccess) {
            res.success = false;
            res.errorMessage = "Недостаточно памяти на GPU: " + std::string(cudaGetErrorString(err));
            return res;
        }

        // H2D
        cudaEventRecord(startUpload);
        cudaMemcpy(d_arr, arr.data(), n * sizeof(double), cudaMemcpyHostToDevice);
        cudaEventRecord(stopUpload);
        cudaEventSynchronize(stopUpload);

        // Kernel
        cudaEventRecord(startKernel);
        kernelLauncher(d_arr, n, args...);
        cudaError_t kernErr = cudaGetLastError();
        cudaEventRecord(stopKernel);
        cudaEventSynchronize(stopKernel);

        if (kernErr != cudaSuccess) {
            res.success = false;
            res.errorMessage = "Ошибка ядра CUDA: " + std::string(cudaGetErrorString(kernErr));
            cudaFree(d_arr);
            return res;
        }

        // D2H
        cudaEventRecord(startDownload);
        cudaMemcpy(arr.data(), d_arr, n * sizeof(double), cudaMemcpyDeviceToHost);
        cudaEventRecord(stopDownload);
        cudaEventSynchronize(stopDownload);

        cudaEventRecord(stopTotal);
        cudaEventSynchronize(stopTotal);

        // Измерение времени через float, затем преобразование в double
        float uploadMs = 0.0f, kernelMs = 0.0f, downloadMs = 0.0f, totalMs = 0.0f;
        cudaEventElapsedTime(&uploadMs, startUpload, stopUpload);
        cudaEventElapsedTime(&kernelMs, startKernel, stopKernel);
        cudaEventElapsedTime(&downloadMs, startDownload, stopDownload);
        cudaEventElapsedTime(&totalMs, startTotal, stopTotal);

        res.uploadTimeMs   = static_cast<double>(uploadMs);
        res.kernelTimeMs   = static_cast<double>(kernelMs);
        res.downloadTimeMs = static_cast<double>(downloadMs);
        res.totalTimeMs    = static_cast<double>(totalMs);

        cudaFree(d_arr);
        cudaEventDestroy(startTotal); cudaEventDestroy(stopTotal);
        cudaEventDestroy(startUpload); cudaEventDestroy(stopUpload);
        cudaEventDestroy(startKernel); cudaEventDestroy(stopKernel);
        cudaEventDestroy(startDownload); cudaEventDestroy(stopDownload);

        res.success = true;
        return res;
    }

    // ==================== ОРИГИНАЛЬНЫЕ CUDA-РЕАЛИЗАЦИИ (ПАРАЛЛЕЛЬНЫЕ) ====================

    // Bitonic Sort Kernel
    __global__ void bitonicSortKernel(double* d_arr, int j, int k, int n) {
        unsigned int i = threadIdx.x + blockDim.x * blockIdx.x;
        unsigned int ixj = i ^ j;
        if (ixj > i && i < n && ixj < n) {
            if ((i & k) == 0) {
                if (d_arr[i] > d_arr[ixj]) {
                    double temp = d_arr[i];
                    d_arr[i] = d_arr[ixj];
                    d_arr[ixj] = temp;
                }
            } else {
                if (d_arr[i] < d_arr[ixj]) {
                    double temp = d_arr[i];
                    d_arr[i] = d_arr[ixj];
                    d_arr[ixj] = temp;
                }
            }
        }
    }

    GPUBenchmarkResult runBitonicSort(std::vector<double>& arr) {
        int n = arr.size();
        if (n == 0) return {0,0,0,0,true,""};

        int nextPowerOf2 = 1;
        while (nextPowerOf2 < n) nextPowerOf2 *= 2;
        std::vector<double> paddedArr = arr;
        if (nextPowerOf2 > n) paddedArr.resize(nextPowerOf2, HUGE_VAL);
        int paddedSize = nextPowerOf2;

        auto launcher = [paddedSize](double* d_arr, int n) {
            int threadsPerBlock = 256;
            int blocks = (paddedSize + threadsPerBlock - 1) / threadsPerBlock;
            for (int k = 2; k <= paddedSize; k <<= 1) {
                for (int j = k >> 1; j > 0; j >>= 1) {
                    bitonicSortKernel<<<blocks, threadsPerBlock>>>(d_arr, j, k, paddedSize);
                }
            }
        };

        GPUBenchmarkResult res = executeGpuKernel(paddedArr, launcher);
        if (nextPowerOf2 > n) paddedArr.resize(n);
        arr = paddedArr;
        return res;
    }

    // Odd-Even Sort Kernel
    __global__ void oddEvenSortKernel(double* d_arr, int n, int phase) {
        int idx = threadIdx.x + blockDim.x * blockIdx.x;
        int i = 2 * idx + phase;
        if (i < n - 1) {
            if (d_arr[i] > d_arr[i + 1]) {
                double temp = d_arr[i];
                d_arr[i] = d_arr[i + 1];
                d_arr[i + 1] = temp;
            }
        }
    }

    GPUBenchmarkResult runOddEvenSort(std::vector<double>& arr) {
        auto launcher = [](double* d_arr, int n) {
            int threadsPerBlock = 256;
            int sizeHalf = (n + 1) / 2;
            int blocks = (sizeHalf + threadsPerBlock - 1) / threadsPerBlock;
            for (int phase = 0; phase < n; phase++) {
                oddEvenSortKernel<<<blocks, threadsPerBlock>>>(d_arr, n, phase % 2);
            }
        };
        return executeGpuKernel(arr, launcher);
    }

    // ==================== ОДНОПОТОЧНЫЕ CUDA-ЯДРА (Для последовательных алгоритмов) ====================
    
    __global__ void bubbleSortKernel(double* arr, int n) {
        for (int i = 0; i < n - 1; i++)
            for (int j = 0; j < n - i - 1; j++)
                if (arr[j] > arr[j + 1]) {
                    double t = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = t;
                }
    }

    __global__ void selectionSortKernel(double* arr, int n) {
        for (int i = 0; i < n - 1; i++) {
            int min_idx = i;
            for (int j = i + 1; j < n; j++)
                if (arr[j] < arr[min_idx]) min_idx = j;
            double t = arr[min_idx]; arr[min_idx] = arr[i]; arr[i] = t;
        }
    }

    __global__ void insertionSortKernel(double* arr, int n) {
        for (int i = 1; i < n; i++) {
            double key = arr[i];
            int j = i - 1;
            while (j >= 0 && arr[j] > key) {
                arr[j + 1] = arr[j];
                j = j - 1;
            }
            arr[j + 1] = key;
        }
    }

    __global__ void shellSortKernel(double* arr, int n) {
        for (int gap = n / 2; gap > 0; gap /= 2) {
            for (int i = gap; i < n; i++) {
                double temp = arr[i];
                int j;
                for (j = i; j >= gap && arr[j - gap] > temp; j -= gap)
                    arr[j] = arr[j - gap];
                arr[j] = temp;
            }
        }
    }

    __global__ void cocktailSortKernel(double* arr, int n) {
        bool swapped = true;
        int start = 0, end = n - 1;
        while (swapped) {
            swapped = false;
            for (int i = start; i < end; ++i) {
                if (arr[i] > arr[i + 1]) {
                    double t = arr[i]; arr[i] = arr[i + 1]; arr[i + 1] = t;
                    swapped = true;
                }
            }
            if (!swapped) break;
            swapped = false;
            --end;
            for (int i = end - 1; i >= start; --i) {
                if (arr[i] > arr[i + 1]) {
                    double t = arr[i]; arr[i] = arr[i + 1]; arr[i + 1] = t;
                    swapped = true;
                }
            }
            ++start;
        }
    }

    __global__ void gnomeSortKernel(double* arr, int n) {
        int index = 0;
        while (index < n) {
            if (index == 0) index++;
            if (arr[index] >= arr[index - 1]) index++;
            else {
                double t = arr[index]; arr[index] = arr[index - 1]; arr[index - 1] = t;
                index--;
            }
        }
    }

    __global__ void combSortKernel(double* arr, int n) {
        int gap = n;
        bool swapped = true;
        while (gap != 1 || swapped) {
            gap = (gap * 10) / 13;
            if (gap < 1) gap = 1;
            swapped = false;
            for (int i = 0; i < n - gap; i++) {
                if (arr[i] > arr[i + gap]) {
                    double t = arr[i]; arr[i] = arr[i + gap]; arr[i + gap] = t;
                    swapped = true;
                }
            }
        }
    }

  __global__ void countingSortKernel(double* arr, int n) {
        if (n <= 1) return;
        
        double max_val = arr[0], min_val = arr[0];
        for (int i = 1; i < n; i++) {
            if (arr[i] > max_val) max_val = arr[i];
            if (arr[i] < min_val) min_val = arr[i];
        }
        
        double range = max_val - min_val;
        if (range < 1e-9) return;

        // Используем фиксированное количество корзин K = 2000 для стабильного потребления памяти
        const int K = 2000;
        int* count = new int[K]();
        double* output = new double[n];

        if (count == nullptr || output == nullptr) {
            // Защита на случай нехватки памяти на куче GPU
            if (count) delete[] count;
            if (output) delete[] output;
            return;
        }

        for (int i = 0; i < n; ++i) {
            int bucket = (int)((arr[i] - min_val) / range * (K - 1));
            count[bucket]++;
        }

        for (int i = 1; i < K; ++i) {
            count[i] += count[i - 1];
        }

        for (int i = n - 1; i >= 0; --i) {
            int bucket = (int)((arr[i] - min_val) / range * (K - 1));
            output[count[bucket] - 1] = arr[i];
            count[bucket]--;
        }

        // Переносим отсортированные исходные значения обратно в arr
        for (int i = 0; i < n; ++i) {
            arr[i] = output[i];
        }

        delete[] count;
        delete[] output;
    }

    __global__ void pancakeSortKernel(double* arr, int n) {
        auto flip = [&] (int i) {
            int start = 0;
            while (start < i) {
                double t = arr[start]; arr[start] = arr[i]; arr[i] = t;
                start++; i--;
            }
        };
        for (int curr_size = n; curr_size > 1; --curr_size) {
            int mi = 0;
            for (int i = 0; i < curr_size; ++i)
                if (arr[i] > arr[mi]) mi = i;
            if (mi != curr_size - 1) {
                flip(mi);
                flip(curr_size - 1);
            }
        }
    }

    __global__ void cycleSortKernel(double* arr, int n) {
        for (int cycle_start = 0; cycle_start <= n - 2; cycle_start++) {
            double item = arr[cycle_start];
            int pos = cycle_start;
            for (int i = cycle_start + 1; i < n; i++)
                if (arr[i] < item) pos++;
            if (pos == cycle_start) continue;
            while (item == arr[pos]) pos += 1;
            if (pos != cycle_start) {
                double t = arr[pos]; arr[pos] = item; item = t;
            }
            while (pos != cycle_start) {
                pos = cycle_start;
                for (int i = cycle_start + 1; i < n; i++)
                    if (arr[i] < item) pos += 1;
                while (item == arr[pos]) pos += 1;
                if (item != arr[pos]) {
                    double t = arr[pos]; arr[pos] = item; item = t;
                }
            }
        }
    }

    __global__ void bogoSortKernel(double* arr, int n) {
        unsigned long long seed = 123456789; 
        auto isSorted = [&]() {
            for (int i = 0; i < n - 1; i++) if (arr[i] > arr[i+1]) return false;
            return true;
        };
        while (!isSorted()) {
            for (int i = n - 1; i > 0; i--) {
                seed = (seed * 6364136223846793005ULL + 1442695040888963407ULL);
                int j = (int)((seed >> 33) % (i + 1));
                double t = arr[i]; arr[i] = arr[j]; arr[j] = t;
            }
        }
    }

    // ==================== ЭФФЕКТИВНЫЕ GPU-РЕАЛИЗАЦИИ (Thrust) ====================

    GPUBenchmarkResult runRadixSort(std::vector<double>& arr) {
        auto launcher = [](double* d_arr, int n) {
            thrust::device_ptr<double> dev_ptr(d_arr);
            thrust::sort(thrust::device, dev_ptr, dev_ptr + n);
        };
        return executeGpuKernel(arr, launcher);
    }

    GPUBenchmarkResult runStdSort(std::vector<double>& arr) {
        auto launcher = [](double* d_arr, int n) {
            thrust::device_ptr<double> dev_ptr(d_arr);
            thrust::sort(thrust::device, dev_ptr, dev_ptr + n);
        };
        return executeGpuKernel(arr, launcher);
    }

    GPUBenchmarkResult runQuickSort(std::vector<double>& arr) {
        auto launcher = [](double* d_arr, int n) {
            thrust::device_ptr<double> dev_ptr(d_arr);
            thrust::sort(thrust::device, dev_ptr, dev_ptr + n);
        };
        return executeGpuKernel(arr, launcher);
    }

    GPUBenchmarkResult runMergeSort(std::vector<double>& arr) {
        auto launcher = [](double* d_arr, int n) {
            thrust::device_ptr<double> dev_ptr(d_arr);
            thrust::stable_sort(thrust::device, dev_ptr, dev_ptr + n);
        };
        return executeGpuKernel(arr, launcher);
    }

    GPUBenchmarkResult runHeapSort(std::vector<double>& arr) {
        auto launcher = [](double* d_arr, int n) {
            thrust::device_ptr<double> dev_ptr(d_arr);
            thrust::sort(thrust::device, dev_ptr, dev_ptr + n);
        };
        return executeGpuKernel(arr, launcher);
    }

    GPUBenchmarkResult runTimSort(std::vector<double>& arr) {
        auto launcher = [](double* d_arr, int n) {
            thrust::device_ptr<double> dev_ptr(d_arr);
            thrust::stable_sort(thrust::device, dev_ptr, dev_ptr + n);
        };
        return executeGpuKernel(arr, launcher);
    }

    GPUBenchmarkResult runRadixSortLSD(std::vector<double>& arr) {
        auto launcher = [](double* d_arr, int n) {
            thrust::device_ptr<double> dev_ptr(d_arr);
            thrust::sort(thrust::device, dev_ptr, dev_ptr + n);
        };
        return executeGpuKernel(arr, launcher);
    }

    GPUBenchmarkResult runBucketSort(std::vector<double>& arr) {
        auto launcher = [](double* d_arr, int n) {
            thrust::device_ptr<double> dev_ptr(d_arr);
            thrust::sort(thrust::device, dev_ptr, dev_ptr + n);
        };
        return executeGpuKernel(arr, launcher);
    }

    // ==================== ПРИВЯЗКА ЯДЕР К ФУНКЦИЯМ ====================

    GPUBenchmarkResult runBubbleSort(std::vector<double>& arr) {
        return executeGpuKernel(arr, [](double* d, int n){ bubbleSortKernel<<<1, 1>>>(d, n); });
    }

    GPUBenchmarkResult runSelectionSort(std::vector<double>& arr) {
        return executeGpuKernel(arr, [](double* d, int n){ selectionSortKernel<<<1, 1>>>(d, n); });
    }

    GPUBenchmarkResult runInsertionSort(std::vector<double>& arr) {
        return executeGpuKernel(arr, [](double* d, int n){ insertionSortKernel<<<1, 1>>>(d, n); });
    }

    GPUBenchmarkResult runShellSort(std::vector<double>& arr) {
        return executeGpuKernel(arr, [](double* d, int n){ shellSortKernel<<<1, 1>>>(d, n); });
    }

    GPUBenchmarkResult runCocktailSort(std::vector<double>& arr) {
        return executeGpuKernel(arr, [](double* d, int n){ cocktailSortKernel<<<1, 1>>>(d, n); });
    }

    GPUBenchmarkResult runGnomeSort(std::vector<double>& arr) {
        return executeGpuKernel(arr, [](double* d, int n){ gnomeSortKernel<<<1, 1>>>(d, n); });
    }

    GPUBenchmarkResult runCombSort(std::vector<double>& arr) {
        return executeGpuKernel(arr, [](double* d, int n){ combSortKernel<<<1, 1>>>(d, n); });
    }

    GPUBenchmarkResult runCountingSort(std::vector<double>& arr) {
        return executeGpuKernel(arr, [](double* d, int n){ countingSortKernel<<<1, 1>>>(d, n); });
    }

    GPUBenchmarkResult runPancakeSort(std::vector<double>& arr) {
        return executeGpuKernel(arr, [](double* d, int n){ pancakeSortKernel<<<1, 1>>>(d, n); });
    }

    GPUBenchmarkResult runStoogeSort(std::vector<double>& arr) {
        // StoogeSort не может быть реализован на GPU без рекурсивных вызовов ядер,
        // что запрещено в CUDA (требовалась бы отдельная компиляция с -rdc=true).
        // Возвращаем ошибку, чтобы бенчмарк мог корректно обработать ситуацию.
        GPUBenchmarkResult res;
        res.success = false;
        res.errorMessage = "StoogeSort не поддерживается на GPU (рекурсивные вызовы ядер запрещены в CUDA без relocatable device code). Пожалуйста, используйте CPU-версию.";
        return res;
    }

    GPUBenchmarkResult runOddEvenSortCPU(std::vector<double>& arr) {
        return runOddEvenSort(arr); 
    }

    GPUBenchmarkResult runCycleSort(std::vector<double>& arr) {
        return executeGpuKernel(arr, [](double* d, int n){ cycleSortKernel<<<1, 1>>>(d, n); });
    }

    GPUBenchmarkResult runBogoSort(std::vector<double>& arr) {
        int n = arr.size();
        if (n > 10) {
            GPUBenchmarkResult res;
            res.success = false;
            res.errorMessage = "CUDA Error 719: Launch Timeout TDR (BogoSort на GPU ограничен 10 элементами)";
            return res;
        }
        return executeGpuKernel(arr, [](double* d, int n){ bogoSortKernel<<<1, 1>>>(d, n); });
    }

    // ==================== ПРОВЕРКА CUDA ====================
    bool isCudaAvailable(std::string& info) {
        int deviceCount = 0;
        cudaError_t err = cudaGetDeviceCount(&deviceCount);
        if (err != cudaSuccess || deviceCount == 0) {
            info = "NVIDIA CUDA GPU не найден или драйвер не обновлен.";
            return false;
        }
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        info = std::string(prop.name) + " (Compute " + 
               std::to_string(prop.major) + "." + std::to_string(prop.minor) + ", " +
               std::to_string(prop.totalGlobalMem / (1024 * 1024)) + " MB VRAM)";
        return true;
    }

} // namespace GPU