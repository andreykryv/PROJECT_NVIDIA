/**
 * @file App.tsx
 * @brief Главная панель стенда SortBench: CUDA vs CPU.
 * Объединяет интерактивную симуляцию, аналитические графики Recharts, 
 * проводник по исходному коду проекта C++/CUDA/Qt6 и скачивание ZIP-сборки.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

import React, { useState, useEffect, useRef } from "react";
import { 
  Cpu, Zap, Download, RefreshCw, BarChart3, Code, Info, 
  Play, Pause, SkipForward, Sliders, Check, Clock, Server, 
  Database, AlertCircle, FileCode, Share2, Shield, Settings2, 
  Trash2, ChevronRight, FileSpreadsheet, Eye
} from "lucide-react";
import { 
  BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, 
  Legend, ResponsiveContainer, Cell, PieChart, Pie
} from "recharts";

// Глобальные типы для бенчмарков
interface BenchmarkResult {
  algorithmName: string;
  isGPU: boolean;
  arraySize: number;
  avgTimeMs: number;
  minTimeMs: number;
  maxTimeMs: number;
  medianTimeMs: number;
  varianceMs: number;
  avgUploadTimeMs: number;
  avgKernelTimeMs: number;
  avgDownloadTimeMs: number;
  success: boolean;
  errorMsg: string;
}

// Статический контент для файлов C++/CUDA в Справочнике Кода
const CPP_FILES = {
  "CMakeLists.txt": `cmake_minimum_required(VERSION 3.20)
project(SortBench LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_FLAGS "\${CMAKE_CUDA_FLAGS} -arch=sm_60 -O3 --use_fast_math")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

add_executable(SortBench
    main.cpp
    mainwindow.h
    mainwindow.cpp
    sorting_visualizer.h
    sorting_visualizer.cpp
    cpu_algorithms.h
    cpu_algorithms.cpp
    gpu_algorithms.h
    gpu_algorithms.cu
    benchmark_runner.h
    benchmark_runner.cpp
)

target_link_libraries(SortBench PRIVATE
    Qt6::Widgets
    Qt6::Charts
    CUDA::cudart
)

target_include_directories(SortBench PRIVATE 
    \${CMAKE_CURRENT_SOURCE_DIR}
    \${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
)`,

  "main.cpp": `/**
 * @file main.cpp
 * @brief Точка входа в приложение SortBench.
 * Инициализирует QApplication, накладывает стилизацию темной темы и открывает главное окно.
 */
#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName("SortBench: CUDA vs CPU");
    app.setApplicationVersion("1.0.0");

    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(24, 24, 27));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(39, 39, 42));
    darkPalette.setColor(QPalette::AlternateBase, QColor(24, 24, 27));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(39, 39, 42));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::Highlight, QColor(59, 130, 246));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);

    app.setPalette(darkPalette);
    app.setStyleSheet(
        "QMainWindow { background-color: #18181b; }"
        "QWidget { font-family: 'Segoe UI', sans-serif; font-size: 13px; }"
    );

    MainWindow w;
    w.showMaximized();
    return app.exec();
}`,

  "sorting_visualizer.h": `/**
 * @file sorting_visualizer.h
 * @brief Кастомный QWidget для отрисовки массива вертикальными столбцами.
 */
#pragma once
#include <QWidget>
#include <vector>
#include <mutex>

class SortingVisualizer : public QWidget {
    Q_OBJECT
public:
    explicit SortingVisualizer(QWidget* parent = nullptr);
    void updateData(const std::vector<double>& newData, int active1 = -1, int active2 = -1, int pivot = -1);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<double> m_data;
    int m_activeIdx1 = -1;
    int m_activeIdx2 = -1;
    int m_pivotIdx = -1;
    std::mutex m_mutex;
    double m_minValue = 0.0;
    double m_maxValue = 1.0;
};`,

  "gpu_algorithms.cu": `/**
 * @file gpu_algorithms.cu
 * @brief CUDA-ядра для высокопроизводительной параллельной сортировки на GPU.
 */
#include "gpu_algorithms.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cmath>

namespace GPU {

    // Классическое CUDA-ядро битонической перестановки
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

    // Классическое ядро Чет-Нечет (Odd-Even Transposition)
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

    GPUBenchmarkResult runBitonicSort(std::vector<double>& arr) {
        GPUBenchmarkResult res;
        int n = arr.size();
        
        // Округление размера до степени 2 для корректности схемы слияния
        int nextPowerOf2 = 1;
        while (nextPowerOf2 < n) { nextPowerOf2 *= 2; }
        
        std::vector<double> padded = arr;
        padded.resize(nextPowerOf2, HUGE_VAL);
        
        double* d_arr;
        cudaMalloc(&d_arr, nextPowerOf2 * sizeof(double));
        
        // Замер Upload H2D
        cudaMemcpy(d_arr, padded.data(), nextPowerOf2 * sizeof(double), cudaMemcpyHostToDevice);
        
        int threads = 256;
        int blocks = (nextPowerOf2 + threads - 1) / threads;

        // Парадигма битонического слияния
        for (int k = 2; k <= nextPowerOf2; k <<= 1) {
            for (int j = k >> 1; j > 0; j >>= 1) {
                bitonicSortKernel<<<blocks, threads>>>(d_arr, j, k, nextPowerOf2);
            }
        }
        cudaDeviceSynchronize();
        
        cudaMemcpy(padded.data(), d_arr, nextPowerOf2 * sizeof(double), cudaMemcpyDeviceToHost);
        cudaFree(d_arr);
        
        padded.resize(n);
        arr = padded;
        
        res.success = true;
        return res;
    }
}`,

  "cpu_algorithms.cpp": `/**
 * @file cpu_algorithms.cpp
 * @brief Оптимизированные алгоритмы сортировки на CPU с поддержкой отмены.
 */
#include "cpu_algorithms.h"
#include <algorithm>

namespace CPU {

    void quickSortImpl(std::vector<double>& arr, int low, int high, SortContext& ctx) {
        if (low >= high || (ctx.stopRequested && ctx.stopRequested->load())) return;

        double pivot = arr[low + (high - low) / 2];
        int i = low, j = high;

        while (i <= j) {
            while (arr[i] < pivot) i++;
            while (arr[j] > pivot) j--;
            if (i <= j) {
                std::swap(arr[i], arr[j]);
                if (ctx.stepCallback) ctx.stepCallback(arr, i, j, -1);
                i++;
                j--;
            }
        }

        quickSortImpl(arr, low, j, ctx);
        quickSortImpl(arr, i, high, ctx);
    }

    void quickSort(std::vector<double>& arr, SortContext& ctx) {
        quickSortImpl(arr, 0, arr.size() - 1, ctx);
    }
}`
};

const ALGORITHM_GROUPS = [
  {
    title: "Быстрые & Линейные на x86 CPU",
    complexity: "O(N log N) / O(N)",
    color: "text-blue-400 bg-blue-500/10 border-blue-500/20",
    algs: [
      { id: "CPU_std::sort", label: "std::sort", sub: "Гибридный IntroSort (STL)", gpu: false },
      { id: "CPU_QuickSort", label: "QuickSort", sub: "Хоар с медианой трех", gpu: false },
      { id: "CPU_MergeSort", label: "MergeSort", sub: "Стабильное слияние O(N) RAM", gpu: false },
      { id: "CPU_HeapSort", label: "HeapSort", sub: "Сортировка бинарной кучей", gpu: false },
      { id: "CPU_TimSort", label: "TimSort", sub: "Вставки + слияние (Python/Java)", gpu: false },
      { id: "CPU_ShellSort", label: "ShellSort", sub: "Сдвиги по шагам Сэджвика", gpu: false },
      { id: "CPU_CombSort", label: "CombSort", sub: "Пузырек с сужающимся шагом", gpu: false },
      { id: "CPU_RadixSortLSD", label: "Radix LSD", sub: "Поразрядный проход по байтам", gpu: false },
      { id: "CPU_CountingSort", label: "CountingSort", sub: "Линейный подсчет частот", gpu: false },
      { id: "CPU_BucketSort", label: "BucketSort", sub: "Локальные карманы (Uniform)", gpu: false }
    ]
  },
  {
    title: "Квадратичные & Эзотерические CPU",
    complexity: "O(N²)",
    color: "text-zinc-400 bg-zinc-800/20 border-zinc-700/20",
    algs: [
      { id: "CPU_BubbleSort", label: "BubbleSort", sub: "Классический пузырек O(N²)", gpu: false },
      { id: "CPU_SelectionSort", label: "SelectionSort", sub: "Выбор и замена минимального", gpu: false },
      { id: "CPU_InsertionSort", label: "InsertionSort", sub: "Прямые вставки по месту", gpu: false },
      { id: "CPU_CocktailSort", label: "CocktailSort", sub: "Шейкерный двунаправленный проход", gpu: false },
      { id: "CPU_GnomeSort", label: "GnomeSort", sub: "Сравнение соседней пары (Гном)", gpu: false },
      { id: "CPU_OddEvenSort", label: "OddEvenSort", sub: "Четно-нечетные перестановки", gpu: false },
      { id: "CPU_CycleSort", label: "CycleSort", sub: "Минимизация перезаписей в RAM", gpu: false },
      { id: "CPU_PancakeSort", label: "PancakeSort", sub: "Переворот стека блинов", gpu: false },
      { id: "CPU_StoogeSort", label: "StoogeSort", sub: "Рекурсивный разбор 2/3 массива", gpu: false },
      { id: "CPU_BogoSort", label: "BogoSort", sub: "Случайный хаотичный шаффл", gpu: false }
    ]
  },
  {
    title: "Параллельные на NVIDIA GPU CUDA",
    complexity: "O(log² N) / O(N)",
    color: "text-indigo-400 bg-indigo-500/10 border-indigo-500/20",
    isGPU: true,
    algs: [
      { id: "GPU_Bitonic", label: "Bitonic Sort", sub: "Слияние битонических сетей GPU", gpu: true },
      { id: "GPU_Radix", label: "Radix Sort", sub: "Поразрядная сортировка CUDA", gpu: true },
      { id: "GPU_OddEven", label: "Odd-Even CUDA", sub: "Параллельная чет-нечет сеть", gpu: true },
      { id: "GPU_QuickSort", label: "QuickSort CUDA", sub: "Рекурсивное деление на ядрах", gpu: true },
      { id: "GPU_MergeSort", label: "MergeSort CUDA", sub: "Параллельный сдвиг и слияние", gpu: true },
      { id: "GPU_BogoSort", label: "BogoSort CUDA", sub: "Параллельный богосорт в потоках", gpu: true }
    ]
  }
];

export default function App() {
  // Выбранная вкладка
  const [activeTab, setActiveTab] = useState<"bench" | "visual" | "theory" | "reports">("bench");

  // Статус подключения/телеметрии
  const [gpuConnected, setGpuConnected] = useState<boolean>(true);
  const [gpuInfo, setGpuInfo] = useState<string>("Intel/AMD + NVIDIA RTX 4090 PCIe Gen4");

  // --- Настройки аппаратной платформы ---
  const [pcieGen, setPcieGen] = useState<string>("Gen4");
  const [gpuThrottling, setGpuThrottling] = useState<boolean>(false);
  const [showMemoryInspector, setShowMemoryInspector] = useState<boolean>(false);
  const [showAdvancedSettings, setShowAdvancedSettings] = useState<boolean>(false);
  const [copiedText, setCopiedText] = useState<string | null>(null);

  // --- Состояния Бенчмарков ---
  const [selectedAlgs, setSelectedAlgs] = useState<string[]>([
    "CPU_std::sort",
    "CPU_QuickSort",
    "GPU_Bitonic",
    "GPU_Radix"
  ]);

  const selectAllCpu = () => {
    setSelectedAlgs([
      "CPU_std::sort", "CPU_QuickSort", "CPU_MergeSort", "CPU_HeapSort", 
      "CPU_TimSort", "CPU_ShellSort", "CPU_CombSort", "CPU_RadixSortLSD", 
      "CPU_CountingSort", "CPU_BucketSort"
    ]);
  };
  
  const selectAllGpu = () => {
    if (gpuConnected) {
      setSelectedAlgs([
        "GPU_Bitonic", "GPU_Radix", "GPU_OddEven", 
        "GPU_QuickSort", "GPU_MergeSort"
      ]);
    }
  };

  const selectFastOnly = () => {
    const fast = [
      "CPU_std::sort", "CPU_QuickSort", "CPU_MergeSort", "CPU_HeapSort", 
      "CPU_TimSort", "CPU_ShellSort", "CPU_CombSort", "CPU_RadixSortLSD", 
      "CPU_CountingSort", "CPU_BucketSort"
    ];
    if (gpuConnected) {
      fast.push("GPU_Bitonic", "GPU_Radix", "GPU_QuickSort", "GPU_MergeSort");
    }
    setSelectedAlgs(fast);
  };

  const clearAllSelected = () => {
    setSelectedAlgs([]);
  };
  const [arraySize, setArraySize] = useState<number>(100000);
  const [distribution, setDistribution] = useState<string>("Uniform");
  const [dataType, setDataType] = useState<string>("double");
  const [runsCount, setRunsCount] = useState<number>(5);
  const [isSimulating, setIsSimulating] = useState<boolean>(false);
  const [simulationProgress, setSimulationProgress] = useState<number>(100);
  const [simulationLog, setSimulationLog] = useState<string>("");
  const [results, setResults] = useState<BenchmarkResult[]>([
    {
      algorithmName: "CPU_std::sort",
      isGPU: false,
      arraySize: 100000,
      avgTimeMs: 8.52,
      minTimeMs: 8.12,
      maxTimeMs: 9.05,
      medianTimeMs: 8.51,
      varianceMs: 0.15,
      avgUploadTimeMs: 0,
      avgKernelTimeMs: 0,
      avgDownloadTimeMs: 0,
      success: true,
      errorMsg: ""
    },
    {
      algorithmName: "CPU_QuickSort",
      isGPU: false,
      arraySize: 100000,
      avgTimeMs: 11.24,
      minTimeMs: 10.85,
      maxTimeMs: 12.11,
      medianTimeMs: 11.15,
      varianceMs: 0.22,
      avgUploadTimeMs: 0,
      avgKernelTimeMs: 0,
      avgDownloadTimeMs: 0,
      success: true,
      errorMsg: ""
    },
    {
      algorithmName: "GPU_Bitonic",
      isGPU: true,
      arraySize: 100000,
      avgTimeMs: 1.25,
      minTimeMs: 1.15,
      maxTimeMs: 1.35,
      medianTimeMs: 1.24,
      varianceMs: 0.05,
      avgUploadTimeMs: 0.65, // PCIe H2D
      avgKernelTimeMs: 0.12, // Raw CUDA execution
      avgDownloadTimeMs: 0.48, // PCIe D2H
      success: true,
      errorMsg: ""
    },
    {
      algorithmName: "GPU_Radix",
      isGPU: true,
      arraySize: 100000,
      avgTimeMs: 0.85,
      minTimeMs: 0.78,
      maxTimeMs: 0.94,
      medianTimeMs: 0.84,
      varianceMs: 0.02,
      avgUploadTimeMs: 0.58,
      avgKernelTimeMs: 0.05,
      avgDownloadTimeMs: 0.22,
      success: true,
      errorMsg: ""
    }
  ]);

  // --- Состояния Анимированного симулятора ---
  const [visualAlg, setVisualAlg] = useState<string>("quick");
  const [visualSize, setVisualSize] = useState<number>(45);
  const [visualArray, setVisualArray] = useState<number[]>([]);
  const [isVisualRunning, setIsVisualRunning] = useState<boolean>(false);
  const [isVisualPaused, setIsVisualPaused] = useState<boolean>(false);
  const [visualSpeed, setVisualSpeed] = useState<number>(60); // Скорость анимации 1-100
  const [activeCompareIdx, setActiveCompareIdx] = useState<number[]>([]);
  const [activeSwapIdx, setActiveSwapIdx] = useState<number[]>([]);
  const [pivotIdx, setPivotIdx] = useState<number>(-1);
  const [visualStatus, setVisualStatus] = useState<string>("Готов к визуализации");
  
  const abortControllerRef = useRef<boolean>(false);
  const isPausedRef = useRef<boolean>(false);
  const delayRef = useRef<number>(30);
  const stepRequestedRef = useRef<boolean>(false);

  // Синхронизация задержки со слайдером скорости
  useEffect(() => {
    delayRef.current = Math.max(2, 301 - visualSpeed * 3);
  }, [visualSpeed]);

  // Инициализация визуализируемого массива при старте
  useEffect(() => {
    generateNewVisualArray();
    return () => {
      abortControllerRef.current = true; // Сброс при размонтировании
    };
  }, [visualSize]);

  const generateNewVisualArray = () => {
    abortAndResetVisual();
    const arr = [];
    for (let i = 0; i < visualSize; i++) {
      arr.push(Math.floor(Math.random() * 280) + 15);
    }
    setVisualArray(arr);
    setVisualStatus("Сгенерирован новый случайный массив");
  };

  const abortAndResetVisual = () => {
    abortControllerRef.current = true;
    isPausedRef.current = false;
    setIsVisualRunning(false);
    setIsVisualPaused(false);
    setActiveCompareIdx([]);
    setActiveSwapIdx([]);
    setPivotIdx(-1);
  };

  const triggerCopyNotification = (text: string) => {
    setCopiedText(text);
    setTimeout(() => {
      setCopiedText(null);
    }, 2500);
  };

  // Вспомогательный класс задержки с поддержкой пошагового прохода
  const sleep = async () => {
    if (stepRequestedRef.current) {
      stepRequestedRef.current = false;
      return;
    }
    while (isPausedRef.current && !abortControllerRef.current && !stepRequestedRef.current) {
      await new Promise((r) => setTimeout(r, 40));
    }
    if (stepRequestedRef.current) {
      stepRequestedRef.current = false;
      return;
    }
    await new Promise((r) => setTimeout(r, delayRef.current));
  };

  // --- Реализации алгоритмов сортировки на JS для анимации ---
  const startAnimation = async () => {
    if (isVisualRunning) {
      if (isVisualPaused) {
        isPausedRef.current = false;
        setIsVisualPaused(false);
        setVisualStatus("Сортировка возобновлена");
      }
      return;
    }

    abortControllerRef.current = false;
    isPausedRef.current = false;
    setIsVisualRunning(true);
    setIsVisualPaused(false);
    setVisualStatus("Сортировка запущена...");

    const workingArray = [...visualArray];

    try {
      if (visualAlg === "quick") {
        await visualQuickSort(workingArray, 0, workingArray.length - 1);
      } else if (visualAlg === "merge") {
        await visualMergeSort(workingArray, 0, workingArray.length - 1);
      } else if (visualAlg === "heap") {
        await visualHeapSort(workingArray);
      } else if (visualAlg === "bitonic") {
        // Дополняем до степени 2 для битонической
        let originalLen = workingArray.length;
        let nextPow2 = 1;
        while (nextPow2 < originalLen) nextPow2 *= 2;
        let padded = [...workingArray];
        while (padded.length < nextPow2) padded.push(300); // набиваем "бесконечностями"
        
        await visualBitonicSort(padded, 0, nextPow2, 1);
        
        const sliced = padded.slice(0, originalLen);
        if (!abortControllerRef.current) {
          setVisualArray(sliced);
        }
      } else if (visualAlg === "bubble") {
        await visualBubbleSort(workingArray);
      } else if (visualAlg === "selection") {
        await visualSelectionSort(workingArray);
      } else if (visualAlg === "insertion") {
        await visualInsertionSort(workingArray);
      } else if (visualAlg === "shell") {
        await visualShellSort(workingArray);
      } else if (visualAlg === "cocktail") {
        await visualCocktailSort(workingArray);
      } else if (visualAlg === "gnome") {
        await visualGnomeSort(workingArray);
      } else if (visualAlg === "comb") {
        await visualCombSort(workingArray);
      } else if (visualAlg === "radixLsd") {
        await visualRadixSortLSD(workingArray);
      } else if (visualAlg === "counting") {
        await visualCountingSort(workingArray);
      } else if (visualAlg === "pancake") {
        await visualPancakeSort(workingArray);
      } else if (visualAlg === "bogo") {
        await visualBogoSort(workingArray);
      } else if (visualAlg === "stooge") {
        await visualStoogeSort(workingArray, 0, workingArray.length - 1);
      } else if (visualAlg === "oddEven") {
        await visualOddEvenSort(workingArray);
      } else if (visualAlg === "cycle") {
        await visualCycleSort(workingArray);
      }

      if (!abortControllerRef.current) {
        setVisualStatus("Успешно отсортировано!");
      }
    } catch (e) {
      console.log("Visual sort interrupted");
    } finally {
      setIsVisualRunning(false);
      setIsVisualPaused(false);
      setActiveCompareIdx([]);
      setActiveSwapIdx([]);
      setPivotIdx(-1);
    }
  };

  const pauseAnimation = () => {
    isPausedRef.current = true;
    setIsVisualPaused(true);
    setVisualStatus("Анимация приостановлена");
  };

  // 1. Пошаговая QuickSort
  const visualQuickSort = async (arr: number[], low: number, high: number) => {
    if (low >= high || abortControllerRef.current) return;
    
    let pivotVal = arr[high];
    setPivotIdx(high);
    
    let i = low;
    for (let j = low; j < high; j++) {
      if (abortControllerRef.current) return;
      setActiveCompareIdx([j, high]);
      await sleep();

      if (arr[j] < pivotVal) {
        setActiveSwapIdx([i, j]);
        let temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
        setVisualArray([...arr]);
        await sleep();
        i++;
      }
    }
    
    if (abortControllerRef.current) return;
    setActiveSwapIdx([i, high]);
    let temp = arr[i];
    arr[i] = arr[high];
    arr[high] = temp;
    setVisualArray([...arr]);
    await sleep();

    await visualQuickSort(arr, low, i - 1);
    await visualQuickSort(arr, i + 1, high);
  };

  // 2. Пошаговая MergeSort
  const visualMergeSort = async (arr: number[], l: number, r: number) => {
    if (l >= r || abortControllerRef.current) return;
    const m = Math.floor(l + (r - l) / 2);
    await visualMergeSort(arr, l, m);
    await visualMergeSort(arr, m + 1, r);
    await visualMerge(arr, l, m, r);
  };

  const visualMerge = async (arr: number[], l: number, m: number, r: number) => {
    if (abortControllerRef.current) return;
    let n1 = m - l + 1;
    let n2 = r - m;

    let L = arr.slice(l, m + 1);
    let R = arr.slice(m + 1, r + 1);

    let i = 0, j = 0, k = l;

    while (i < n1 && j < n2) {
      if (abortControllerRef.current) return;
      setActiveCompareIdx([l + i, m + 1 + j]);
      await sleep();

      if (L[i] <= R[j]) {
        arr[k] = L[i];
        i++;
      } else {
        arr[k] = R[j];
        j++;
      }
      setActiveSwapIdx([k]);
      setVisualArray([...arr]);
      await sleep();
      k++;
    }

    while (i < n1) {
      if (abortControllerRef.current) return;
      arr[k] = L[i];
      setVisualArray([...arr]);
      setActiveSwapIdx([k]);
      await sleep();
      i++;
      k++;
    }

    while (j < n2) {
      if (abortControllerRef.current) return;
      arr[k] = R[j];
      setVisualArray([...arr]);
      setActiveSwapIdx([k]);
      await sleep();
      j++;
      k++;
    }
  };

  // 3. Пошаговая HeapSort
  const visualHeapSort = async (arr: number[]) => {
    let n = arr.length;
    for (let i = Math.floor(n / 2) - 1; i >= 0; i--) {
      if (abortControllerRef.current) return;
      await visualHeapify(arr, n, i);
    }
    for (let i = n - 1; i > 0; i--) {
      if (abortControllerRef.current) return;
      setActiveSwapIdx([0, i]);
      let temp = arr[0];
      arr[0] = arr[i];
      arr[i] = temp;
      setVisualArray([...arr]);
      await sleep();
      await visualHeapify(arr, i, 0);
    }
  };

  const visualHeapify = async (arr: number[], n: number, i: number) => {
    if (abortControllerRef.current) return;
    let largest = i;
    let l = 2 * i + 1;
    let r = 2 * i + 2;

    if (l < n) {
      setActiveCompareIdx([l, largest]);
      await sleep();
      if (arr[l] > arr[largest]) largest = l;
    }

    if (r < n) {
      setActiveCompareIdx([r, largest]);
      await sleep();
      if (arr[r] > arr[largest]) largest = r;
    }

    if (largest !== i) {
      if (abortControllerRef.current) return;
      setActiveSwapIdx([i, largest]);
      let swap = arr[i];
      arr[i] = arr[largest];
      arr[largest] = swap;
      setVisualArray([...arr]);
      await sleep();
      await visualHeapify(arr, n, largest);
    }
  };

  // 4. Пошаговая Битоническая (GPU-like в JS)
  const visualBitonicSort = async (arr: number[], low: number, cnt: number, dir: number) => {
    if (cnt <= 1 || abortControllerRef.current) return;
    let k = Math.floor(cnt / 2);
    
    // Сортируем первую половину по возрастанию
    await visualBitonicSort(arr, low, k, 1);
    // Вторую половину по убыванию
    await visualBitonicSort(arr, low + k, k, 0);
    // Сливаем всё
    await visualBitonicMerge(arr, low, cnt, dir);
  };

  const visualBitonicMerge = async (arr: number[], low: number, cnt: number, dir: number) => {
    if (cnt <= 1 || abortControllerRef.current) return;
    let k = Math.floor(cnt / 2);
    for (let i = low; i < low + k; i++) {
      if (abortControllerRef.current) return;
      setActiveCompareIdx([i, i + k]);
      await sleep();
      
      if ((dir === 1 && arr[i] > arr[i + k]) || (dir === 0 && arr[i] < arr[i + k])) {
        setActiveSwapIdx([i, i + k]);
        let temp = arr[i];
        arr[i] = arr[i + k];
        arr[i + k] = temp;
        setVisualArray([...arr]);
        await sleep();
      }
    }
    await visualBitonicMerge(arr, low, k, dir);
    await visualBitonicMerge(arr, low + k, k, dir);
  };

  // 5. Пошаговая Bubble Sort
  const visualBubbleSort = async (arr: number[]) => {
    let n = arr.length;
    for (let i = 0; i < n - 1; i++) {
      for (let j = 0; j < n - i - 1; j++) {
        if (abortControllerRef.current) return;
        setActiveCompareIdx([j, j + 1]);
        await sleep();
        if (arr[j] > arr[j + 1]) {
          setActiveSwapIdx([j, j + 1]);
          let temp = arr[j];
          arr[j] = arr[j + 1];
          arr[j + 1] = temp;
          setVisualArray([...arr]);
          await sleep();
        }
      }
    }
  };

  // 6. Пошаговая Selection Sort
  const visualSelectionSort = async (arr: number[]) => {
    let n = arr.length;
    for (let i = 0; i < n - 1; i++) {
      let minIdx = i;
      setPivotIdx(minIdx);
      for (let j = i + 1; j < n; j++) {
        if (abortControllerRef.current) return;
        setActiveCompareIdx([j, minIdx]);
        await sleep();
        if (arr[j] < arr[minIdx]) {
          minIdx = j;
          setPivotIdx(minIdx);
        }
      }
      if (minIdx !== i) {
        setActiveSwapIdx([i, minIdx]);
        let temp = arr[i];
        arr[i] = arr[minIdx];
        arr[minIdx] = temp;
        setVisualArray([...arr]);
        await sleep();
      }
    }
  };

  // 7. Пошаговая Insertion Sort
  const visualInsertionSort = async (arr: number[]) => {
    let n = arr.length;
    for (let i = 1; i < n; i++) {
      let key = arr[i];
      let j = i - 1;
      setPivotIdx(i);
      while (j >= 0 && arr[j] > key) {
        if (abortControllerRef.current) return;
        setActiveCompareIdx([j, j + 1]);
        await sleep();
        
        setActiveSwapIdx([j, j + 1]);
        arr[j + 1] = arr[j];
        setVisualArray([...arr]);
        await sleep();
        j = j - 1;
      }
      arr[j + 1] = key;
      setVisualArray([...arr]);
      await sleep();
    }
  };

  // 8. Пошаговая Shell Sort
  const visualShellSort = async (arr: number[]) => {
    let n = arr.length;
    for (let gap = Math.floor(n / 2); gap > 0; gap = Math.floor(gap / 2)) {
      for (let i = gap; i < n; i++) {
        if (abortControllerRef.current) return;
        let temp = arr[i];
        let j = i;
        setPivotIdx(i);
        while (j >= gap) {
          setActiveCompareIdx([j - gap, j]);
          await sleep();
          if (arr[j - gap] > temp) {
            setActiveSwapIdx([j - gap, j]);
            arr[j] = arr[j - gap];
            setVisualArray([...arr]);
            await sleep();
            j -= gap;
          } else {
            break;
          }
        }
        arr[j] = temp;
        setVisualArray([...arr]);
        await sleep();
      }
    }
  };

  // 9. Пошаговая Cocktail Shaker Sort
  const visualCocktailSort = async (arr: number[]) => {
    let swapped = true;
    let start = 0;
    let end = arr.length - 1;
    while (swapped) {
      swapped = false;
      for (let i = start; i < end; ++i) {
        if (abortControllerRef.current) return;
        setActiveCompareIdx([i, i + 1]);
        await sleep();
        if (arr[i] > arr[i + 1]) {
          setActiveSwapIdx([i, i + 1]);
          let temp = arr[i];
          arr[i] = arr[i + 1];
          arr[i + 1] = temp;
          setVisualArray([...arr]);
          await sleep();
          swapped = true;
        }
      }
      if (!swapped) break;
      swapped = false;
      end--;
      for (let i = end - 1; i >= start; i--) {
        if (abortControllerRef.current) return;
        setActiveCompareIdx([i, i + 1]);
        await sleep();
        if (arr[i] > arr[i + 1]) {
          setActiveSwapIdx([i, i + 1]);
          let temp = arr[i];
          arr[i] = arr[i + 1];
          arr[i + 1] = temp;
          setVisualArray([...arr]);
          await sleep();
          swapped = true;
        }
      }
      start++;
    }
  };

  // 10. Пошаговая Gnome Sort
  const visualGnomeSort = async (arr: number[]) => {
    let n = arr.length;
    let index = 0;
    while (index < n) {
      if (abortControllerRef.current) return;
      if (index === 0) index++;
      setActiveCompareIdx([index, index - 1]);
      await sleep();
      if (arr[index] >= arr[index - 1]) {
        index++;
      } else {
        setActiveSwapIdx([index, index - 1]);
        let temp = arr[index];
        arr[index] = arr[index - 1];
        arr[index - 1] = temp;
        setVisualArray([...arr]);
        await sleep();
        index--;
      }
    }
  };

  // 11. Пошаговая Comb Sort
  const visualCombSort = async (arr: number[]) => {
    let n = arr.length;
    let gap = n;
    let shrink = 1.3;
    let sorted = false;
    while (!sorted) {
      gap = Math.floor(gap / shrink);
      if (gap <= 1) {
        gap = 1;
        sorted = true;
      }
      for (let i = 0; i + gap < n; i++) {
        if (abortControllerRef.current) return;
        setActiveCompareIdx([i, i + gap]);
        await sleep();
        if (arr[i] > arr[i + gap]) {
          setActiveSwapIdx([i, i + gap]);
          let temp = arr[i];
          arr[i] = arr[i + gap];
          arr[i + gap] = temp;
          setVisualArray([...arr]);
          await sleep();
          sorted = false;
        }
      }
    }
  };

  // 12. Пошаговая Radix Sort LSD
  const visualRadixSortLSD = async (arr: number[]) => {
    let maxNum = Math.max(...arr);
    for (let exp = 1; Math.floor(maxNum / exp) > 0; exp *= 10) {
      if (abortControllerRef.current) return;
      await visualCountingSortForRadix(arr, exp);
    }
  };

  const visualCountingSortForRadix = async (arr: number[], exp: number) => {
    let n = arr.length;
    let output = new Array(n).fill(0);
    let count = new Array(10).fill(0);

    for (let i = 0; i < n; i++) {
      if (abortControllerRef.current) return;
      let digit = Math.floor(arr[i] / exp) % 10;
      count[digit]++;
      setActiveCompareIdx([i]);
      await sleep();
    }

    for (let i = 1; i < 10; i++) {
      count[i] += count[i - 1];
    }

    for (let i = n - 1; i >= 0; i--) {
      if (abortControllerRef.current) return;
      let digit = Math.floor(arr[i] / exp) % 10;
      output[count[digit] - 1] = arr[i];
      count[digit]--;
      setActiveCompareIdx([i]);
      await sleep();
    }

    for (let i = 0; i < n; i++) {
      if (abortControllerRef.current) return;
      arr[i] = output[i];
      setVisualArray([...arr]);
      setActiveSwapIdx([i]);
      await sleep();
    }
  };

  // 13. Пошаговая Counting Sort
  const visualCountingSort = async (arr: number[]) => {
    let n = arr.length;
    let max = Math.max(...arr);
    let min = Math.min(...arr);
    let range = max - min + 1;
    let count = new Array(range).fill(0);
    let output = new Array(n).fill(0);

    for (let i = 0; i < n; i++) {
      if (abortControllerRef.current) return;
      count[arr[i] - min]++;
      setActiveCompareIdx([i]);
      await sleep();
    }

    for (let i = 1; i < count.length; i++) {
      count[i] += count[i - 1];
    }

    for (let i = n - 1; i >= 0; i--) {
      if (abortControllerRef.current) return;
      output[count[arr[i] - min] - 1] = arr[i];
      count[arr[i] - min]--;
      setActiveCompareIdx([i]);
      await sleep();
    }

    for (let i = 0; i < n; i++) {
      if (abortControllerRef.current) return;
      arr[i] = output[i];
      setVisualArray([...arr]);
      setActiveSwapIdx([i]);
      await sleep();
    }
  };

  // 14. Пошаговая Pancake Sort
  const visualPancakeSort = async (arr: number[]) => {
    let n = arr.length;
    const flip = async (subArr: number[], k: number) => {
      let left = 0;
      while (left < k) {
        if (abortControllerRef.current) return;
        setActiveSwapIdx([left, k]);
        let temp = subArr[left];
        subArr[left] = subArr[k];
        subArr[k] = temp;
        setVisualArray([...subArr]);
        await sleep();
        left++;
        k--;
      }
    };

    for (let currSize = n; currSize > 1; currSize--) {
      if (abortControllerRef.current) return;
      let maxIdx = 0;
      for (let i = 1; i < currSize; i++) {
        setActiveCompareIdx([i, maxIdx]);
        await sleep();
        if (arr[i] > arr[maxIdx]) maxIdx = i;
      }

      if (maxIdx !== currSize - 1) {
        if (maxIdx !== 0) {
          await flip(arr, maxIdx);
        }
        await flip(arr, currSize - 1);
      }
    }
  };

  // 15. Пошаговая Bogo Sort
  const visualBogoSort = async (arr: number[]) => {
    const isSorted = (subArr: number[]) => {
      for (let i = 0; i < subArr.length - 1; i++) {
        if (subArr[i] > subArr[i + 1]) return false;
      }
      return true;
    };

    const shuffle = async (subArr: number[]) => {
      for (let i = subArr.length - 1; i > 0; i--) {
        if (abortControllerRef.current) return;
        let j = Math.floor(Math.random() * (i + 1));
        setActiveSwapIdx([i, j]);
        let temp = subArr[i];
        subArr[i] = subArr[j];
        subArr[j] = temp;
        setVisualArray([...subArr]);
        await sleep();
      }
    };

    let attempts = 0;
    while (!isSorted(arr)) {
      if (abortControllerRef.current) return;
      attempts++;
      setVisualStatus(`BogoSort: Случайное перемешивание. Попытка #${attempts}`);
      if (attempts > 30 && arr.length > 5) {
        setVisualStatus(`BogoSort выбросил белый флаг на #${attempts} попытке! Сортируем принудительно...`);
        arr.sort((a, b) => a - b);
        setVisualArray([...arr]);
        await sleep();
        break;
      }
      await shuffle(arr);
    }
  };

  // 16. Пошаговая Stooge Sort
  const visualStoogeSort = async (arr: number[], l: number, h: number) => {
    if (abortControllerRef.current) return;
    setActiveCompareIdx([l, h]);
    await sleep();
    if (arr[l] > arr[h]) {
      setActiveSwapIdx([l, h]);
      let temp = arr[l];
      arr[l] = arr[h];
      arr[h] = temp;
      setVisualArray([...arr]);
      await sleep();
    }

    if (h - l + 1 > 2) {
      let t = Math.floor((h - l + 1) / 3);
      await visualStoogeSort(arr, l, h - t);
      await visualStoogeSort(arr, l + t, h);
      await visualStoogeSort(arr, l, h - t);
    }
  };

  // 17. Пошаговая Odd-Even Sort
  const visualOddEvenSort = async (arr: number[]) => {
    let n = arr.length;
    let isSorted = false;
    while (!isSorted) {
      isSorted = true;
      if (abortControllerRef.current) return;

      // Odd phase
      for (let i = 1; i < n - 1; i += 2) {
        setActiveCompareIdx([i, i + 1]);
        await sleep();
        if (arr[i] > arr[i + 1]) {
          setActiveSwapIdx([i, i + 1]);
          let temp = arr[i];
          arr[i] = arr[i + 1];
          arr[i + 1] = temp;
          setVisualArray([...arr]);
          await sleep();
          isSorted = false;
        }
      }

      // Even phase
      for (let i = 0; i < n - 1; i += 2) {
        setActiveCompareIdx([i, i + 1]);
        await sleep();
        if (arr[i] > arr[i + 1]) {
          setActiveSwapIdx([i, i + 1]);
          let temp = arr[i];
          arr[i] = arr[i + 1];
          arr[i + 1] = temp;
          setVisualArray([...arr]);
          await sleep();
          isSorted = false;
        }
      }
    }
  };

  // 18. Пошаговая Cycle Sort
  const visualCycleSort = async (arr: number[]) => {
    let n = arr.length;
    for (let cycleStart = 0; cycleStart < n - 1; cycleStart++) {
      if (abortControllerRef.current) return;
      let item = arr[cycleStart];
      let pos = cycleStart;

      for (let i = cycleStart + 1; i < n; i++) {
        setActiveCompareIdx([cycleStart, i]);
        await sleep();
        if (arr[i] < item) pos++;
      }

      if (pos === cycleStart) continue;

      while (item === arr[pos]) {
        pos++;
      }

      if (pos !== cycleStart) {
        setActiveSwapIdx([pos, cycleStart]);
        let temp = item;
        item = arr[pos];
        arr[pos] = temp;
        setVisualArray([...arr]);
        await sleep();
      }

      while (pos !== cycleStart) {
        if (abortControllerRef.current) return;
        pos = cycleStart;
        for (let i = cycleStart + 1; i < n; i++) {
          setActiveCompareIdx([cycleStart, i]);
          await sleep();
          if (arr[i] < item) pos++;
        }

        while (item === arr[pos]) {
          pos++;
        }

        if (item !== arr[pos]) {
          setActiveSwapIdx([pos, cycleStart]);
          let temp = item;
          item = arr[pos];
          arr[pos] = temp;
          setVisualArray([...arr]);
          await sleep();
        }
      }
    }
  };


  // --- ФУНКЦИИ БЕНЧМАРКА ---
  const launchBenchmarkSim = async () => {
    if (isSimulating) return;
    
    setIsSimulating(true);
    setSimulationProgress(0);
    setSimulationLog("Запуск бенчмарк-стенда...");

    // Моделируем лог проходов бенчмарка для полного вовлечения пользователя
    const stages = [
      "Связь с агентом CUDA и аллокация девайс-драйверов...",
      `Генерация массива размером N = ${arraySize.toLocaleString()} элементов (${distribution})...`,
      ...selectedAlgs.map(a => `Прогон алгоритма: ${a} (${runsCount} итераций)...`)
    ];

    for (let idx = 0; idx < stages.length; idx++) {
      setSimulationLog(stages[idx]);
      setSimulationProgress(Math.floor(((idx + 1) / stages.length) * 100));
      await new Promise(r => setTimeout(r, 700 + Math.random() * 500));
    }

    try {
      const response = await fetch("/api/benchmark-simulate", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          arraySize,
          distribution,
          algorithms: selectedAlgs,
          isDouble: dataType === "double",
          pcieGen,
          gpuThrottling
        })
      });
      const data = await response.json();
      if (data.results) {
        setResults(data.results);
        setSimulationLog("Все тесты завершены успешно! Графики синхронизированы.");
      }
    } catch (e: any) {
      setSimulationLog("Ошибка получения данных симуляции с backend-сервера. Задействован локальный откат.");
    } finally {
      setIsSimulating(false);
      setSimulationProgress(100);
    }
  };

  // Выкачивание ZIP-архива
  const downloadProjectZip = () => {
    window.location.href = "/api/download-zip";
  };


  // --- КОД-ЭКСПЛОРЕР СОСТОЯНИЯ ---
  const [selectedCodeFile, setSelectedCodeFile] = useState<keyof typeof CPP_FILES>("CMakeLists.txt");


  // --- РАСЧЕТ ДОП. СТАТИСТИКИ (PCIe bottlenecks) ---
  const gpuLatencyData = results
    .filter(r => r.isGPU)
    .map(r => ({
      name: r.algorithmName.replace("GPU_", ""),
      "PCIe Upload (H2D)": Number(r.avgUploadTimeMs.toFixed(3)),
      "CUDA Kernel Compute": Number(r.avgKernelTimeMs.toFixed(3)),
      "PCIe Download (D2H)": Number(r.avgDownloadTimeMs.toFixed(3))
    }));

  const chartData = results.map(r => ({
    name: r.algorithmName,
    "Среднее время (мс)": Number(r.avgTimeMs.toFixed(4)),
    "Медиана (мс)": Number(r.medianTimeMs.toFixed(4)),
    isGPU: r.isGPU
  }));

  return (
    <div className="min-h-screen bg-[#070709] text-zinc-100 flex flex-col font-sans selection:bg-blue-600/30 selection:text-white antialiased">
      
      {/* PREMIUM HEADER */}
      <header className="border-b border-zinc-900 bg-[#0c0c0e]/80 backdrop-blur-md sticky top-0 z-50 px-6 py-4 flex flex-col md:flex-row items-center justify-between gap-4 shadow-sm">
        <div className="flex items-center gap-4">
          <div className="relative group">
            {/* Ambient gold-purple background glow */}
            <div className="absolute -inset-0.5 bg-gradient-to-r from-blue-500 to-indigo-600 rounded-xl blur opacity-30 group-hover:opacity-50 transition duration-1000"></div>
            <div className="relative bg-zinc-900 border border-zinc-800 p-2.5 rounded-xl shadow-lg flex items-center justify-center">
              <Cpu className="text-blue-400 w-6 h-6 animate-pulse" />
            </div>
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h1 className="text-lg font-bold tracking-tight text-white font-sans">
                SortBench Studio
              </h1>
              <span className="text-[10px] bg-blue-900/20 border border-blue-500/20 text-blue-400 px-2.5 py-0.5 rounded-full font-mono font-medium tracking-wide uppercase">
                cuda vs cpu v1.2
              </span>
            </div>
            <p className="text-xs text-zinc-400 font-medium font-sans mt-0.5">Высокоточная лаборатория анализа и моделирования алгоритмов сортировки</p>
          </div>
        </div>

        {/* Telemetry Status Bar */}
        <div className="flex items-center gap-4 bg-zinc-900/40 border border-zinc-850 px-4 py-2.5 rounded-xl shadow-inner text-xs">
          <div className="flex items-center gap-2">
            <span className="relative flex h-2 w-2">
              <span className={`animate-ping absolute inline-flex h-full w-full rounded-full opacity-75 ${gpuConnected ? "bg-emerald-400" : "bg-red-400"}`}></span>
              <span className={`relative inline-flex rounded-full h-2 w-2 ${gpuConnected ? "bg-emerald-500" : "bg-red-500"}`}></span>
            </span>
            <span className="font-semibold text-zinc-400 font-sans">Система CUDA:</span>
          </div>
          <div className="text-zinc-300 font-mono text-[11px] font-medium max-w-[240px] truncate">
            {gpuConnected ? gpuInfo : "Шина отключена (Симуляция сбоя)"}
          </div>
          <button 
            onClick={() => setGpuConnected(!gpuConnected)}
            title="Переключить состояние GPU для симуляции ошибок"
            className="text-[9px] font-bold uppercase tracking-wider bg-[#1c1c24] hover:bg-zinc-700 active:scale-95 px-2 py-1 rounded-lg text-zinc-300 font-mono hover:text-white border border-zinc-700/50 transition cursor-pointer"
          >
            {gpuConnected ? "ОТКЛЮЧИТЬ" : "ПОДКЛЮЧИТЬ"}
          </button>
        </div>
      </header>

      {/* LUXURY SEGMENTED CONTROL TABS */}
      <div className="bg-[#09090b] px-6 py-4 border-b border-zinc-900/60 flex justify-center">
        <div className="bg-[#121215] border border-zinc-800/40 p-1 rounded-2xl flex flex-wrap gap-1 shadow-2xl max-w-full">
          {[
            { id: "bench", label: "Лаборатория бенчмарков", icon: BarChart3 },
            { id: "visual", label: `Симулятор замен (${visualArray.length} эл.)`, icon: Zap },
            { id: "theory", label: "Теория алгоритмов", icon: Info },
            { id: "reports", label: `Отчеты (${results.length})`, icon: FileSpreadsheet, badge: true },
          ].map((tab) => {
            const Icon = tab.icon;
            const active = activeTab === tab.id;
            return (
              <button 
                key={tab.id}
                onClick={() => setActiveTab(tab.id as any)}
                className={`flex items-center gap-2.5 px-5 py-2.5 rounded-xl text-xs font-semibold tracking-wide transition-all cursor-pointer ${
                  active 
                    ? "bg-gradient-to-r from-blue-600 to-indigo-600 text-white shadow-xl shadow-blue-500/10 scale-[1.02]" 
                    : "text-zinc-400 hover:text-zinc-200 hover:bg-zinc-850/40"
                }`}
              >
                <Icon className={`w-4 h-4 ${active ? "text-white" : tab.id === "visual" ? "text-amber-400" : "text-zinc-400"}`} />
                <span>{tab.label}</span>
              </button>
            );
          })}
        </div>
      </div>

      {/* CORE CONTENT */}
      <main className="flex-1 p-6 max-w-7xl mx-auto w-full flex flex-col gap-6">

        {/* --- TAB 1: BENCHMARK LAB --- */}
        {activeTab === "bench" && (
          <div className="grid grid-cols-1 lg:grid-cols-12 gap-8 items-start">
            
            {/* Left side: parameters setup */}
            <div className="lg:col-span-4 bg-[#111115] border border-zinc-900 rounded-2xl p-6 flex flex-col gap-6 shadow-[0_12px_40px_-15px_rgba(0,0,0,0.7)]">
              <div className="flex items-center gap-3 pb-3 border-b border-zinc-800/60">
                <Sliders className="text-blue-500 w-5 h-5" />
                <h3 className="font-semibold text-white tracking-tight text-sm">Параметры эксперимента</h3>
              </div>

              {/* 1. Алгоритмы */}
              <div className="flex flex-col gap-2">
                <div className="flex items-center justify-between">
                  <label className="text-xs text-zinc-400 font-bold uppercase tracking-wider">Алгоритмы для запуска:</label>
                  <span className="text-[10px] text-zinc-500 font-mono font-bold uppercase bg-zinc-900 px-2 py-0.5 rounded border border-zinc-805">активно {selectedAlgs.length} / 26</span>
                </div>

                {/* Batch selection quick controls */}
                <div className="grid grid-cols-4 gap-1 bg-zinc-950 p-1 rounded-xl border border-zinc-900">
                  <button 
                    onClick={selectAllCpu}
                    type="button"
                    className="text-[9px] font-bold py-1 rounded bg-zinc-900 hover:bg-zinc-850 border border-zinc-800 text-zinc-400 hover:text-zinc-200 transition cursor-pointer"
                  >
                    Все CPU
                  </button>
                  <button 
                    disabled={!gpuConnected}
                    type="button"
                    onClick={selectAllGpu}
                    className="text-[9px] font-bold py-1 rounded bg-zinc-900 hover:bg-zinc-850 border border-zinc-800 text-zinc-400 hover:text-zinc-200 disabled:opacity-30 disabled:cursor-not-allowed transition cursor-pointer"
                  >
                    Все GPU
                  </button>
                  <button 
                    onClick={selectFastOnly}
                    type="button"
                    className="text-[9px] font-bold py-1 rounded bg-zinc-900 hover:bg-zinc-850 border border-zinc-805 text-zinc-450 hover:text-zinc-200 transition cursor-pointer"
                  >
                    Быстрые
                  </button>
                  <button 
                    onClick={clearAllSelected}
                    type="button"
                    className="text-[9px] font-bold py-1 rounded bg-zinc-900 hover:bg-[#201010] border border-zinc-800 text-red-400/80 hover:text-red-400 transition cursor-pointer"
                  >
                    Сбросить
                  </button>
                </div>

                <div className="grid grid-cols-1 gap-2 max-h-[300px] overflow-y-auto pr-1">
                  {[
                    { id: "CPU_std::sort", label: "CPU: std::sort (STL IntroSort)" },
                    { id: "CPU_QuickSort", label: "CPU: QuickSort (Быстрая)" },
                    { id: "CPU_MergeSort", label: "CPU: MergeSort (Слиянием)" },
                    { id: "CPU_HeapSort", label: "CPU: HeapSort (Пирамидальная)" },
                    { id: "CPU_TimSort", label: "CPU: TimSort (Гибридная)" },
                    { id: "CPU_ShellSort", label: "CPU: ShellSort (Шелла)" },
                    { id: "CPU_CombSort", label: "CPU: CombSort (Расческой)" },
                    { id: "CPU_BubbleSort", label: "CPU: BubbleSort (Пузырьковая)" },
                    { id: "CPU_SelectionSort", label: "CPU: SelectionSort (Выбором)" },
                    { id: "CPU_InsertionSort", label: "CPU: InsertionSort (Вставками)" },
                    { id: "CPU_CocktailSort", label: "CPU: CocktailSort (Шейкер)" },
                    { id: "CPU_GnomeSort", label: "CPU: GnomeSort (Гномья)" },
                    { id: "CPU_OddEvenSort", label: "CPU: OddEvenSort (Чет-нечет)" },
                    { id: "CPU_CycleSort", label: "CPU: CycleSort (Циклическая)" },
                    { id: "CPU_RadixSortLSD", label: "CPU: RadixSort LSD (Поразрядная)" },
                    { id: "CPU_CountingSort", label: "CPU: CountingSort (Подсчетом)" },
                    { id: "CPU_BucketSort", label: "CPU: BucketSort (Блочная)" },
                    { id: "CPU_PancakeSort", label: "CPU: PancakeSort (Блинная)" },
                    { id: "CPU_StoogeSort", label: "CPU: StoogeSort (Студжа)" },
                    { id: "CPU_BogoSort", label: "CPU: BogoSort (Случайная)" },
                    { id: "GPU_Bitonic", label: "GPU: Bitonic Sort (CUDA блок)", gpu: true },
                    { id: "GPU_Radix", label: "GPU: Radix Sort (CUDA разряды)", gpu: true },
                    { id: "GPU_OddEven", label: "GPU: Odd-Even (CUDA чет-нечет)", gpu: true },
                    { id: "GPU_QuickSort", label: "GPU: QuickSort (CUDA быстрая)", gpu: true },
                    { id: "GPU_MergeSort", label: "GPU: MergeSort (CUDA слияние)", gpu: true },
                    { id: "GPU_BogoSort", label: "GPU: BogoSort (CUDA богосорт)", gpu: true }
                  ].map((alg) => {
                    const selected = selectedAlgs.includes(alg.id);
                    return (
                      <button
                        key={alg.id}
                        disabled={alg.gpu && !gpuConnected}
                        onClick={() => {
                          if (selected) {
                            setSelectedAlgs(selectedAlgs.filter(x => x !== alg.id));
                          } else {
                            setSelectedAlgs([...selectedAlgs, alg.id]);
                          }
                        }}
                        className={`flex items-center justify-between text-left px-3 py-2.5 rounded-xl border text-xs font-mono transition-all cursor-pointer ${
                          selected 
                            ? "bg-blue-950/20 border-blue-500/80 hover:bg-blue-950/30 text-blue-200 shadow-sm" 
                            : alg.gpu && !gpuConnected
                              ? "bg-zinc-950/40 border-zinc-900 text-zinc-600 cursor-not-allowed"
                              : "bg-[#09090b] border-zinc-850 hover:border-zinc-700 text-zinc-400 hover:text-zinc-200"
                        }`}
                      >
                        <div className="flex items-center gap-2.5">
                          <div className={`w-4 h-4 rounded border flex items-center justify-center transition-all ${selected ? "border-blue-500 bg-blue-600 shadow-[0_0_8px_rgba(59,130,246,0.3)]" : "border-zinc-700 bg-zinc-950"}`}>
                            {selected && <Check className="w-3 h-3 text-white stroke-[3px]" />}
                          </div>
                          <span className="font-medium font-mono">{alg.label}</span>
                        </div>
                        {alg.gpu && (
                          <span className={`text-[9px] font-bold px-2 py-0.5 rounded-md border ${gpuConnected ? "bg-indigo-900/30 text-indigo-400 border-indigo-500/30 shadow-[0_0_8px_rgba(79,70,229,0.15)]" : "bg-zinc-900 border-zinc-800 text-zinc-500"}`}>
                            CUDA
                          </span>
                        )}
                      </button>
                    );
                  })}
                </div>
              </div>

              {/* 2. Настройки элементов */}
              <div className="flex flex-col gap-5 pt-3 border-t border-zinc-800/40">
                <div className="flex flex-col gap-2">
                  <div className="flex justify-between text-xs">
                    <span className="text-zinc-400 font-bold uppercase tracking-wider">Размер массива (N):</span>
                    <span className="text-blue-400 font-mono font-bold tracking-tight text-sm">{arraySize.toLocaleString()} элементов</span>
                  </div>
                  <input 
                    type="range"
                    min="1000"
                    max="5000000"
                    step="5000"
                    value={arraySize}
                    onChange={(e) => setArraySize(Number(e.target.value))}
                    className="w-full accent-blue-500 cursor-pointer h-1.5 bg-zinc-900 rounded-lg"
                  />
                  <div className="flex gap-1.5 mt-1 overflow-x-auto">
                    {[10000, 100000, 500000, 1000000, 5000000].map(val => (
                      <button 
                        key={val}
                        onClick={() => setArraySize(val)}
                        className={`text-[10px] px-2.5 py-1 rounded-lg font-mono font-semibold border transition-all cursor-pointer ${arraySize === val ? "bg-zinc-850 border-zinc-700 text-white" : "bg-transparent border-transparent text-zinc-500 hover:text-zinc-300"}`}
                      >
                        {val >= 1000000 ? `${val / 1000000}M` : `${val / 1000}K`}
                      </button>
                    ))}
                  </div>
                </div>

                <div className="flex flex-col gap-2">
                  <label className="text-xs text-zinc-400 font-bold uppercase tracking-wider">Закон распределения:</label>
                  <select 
                    value={distribution}
                    onChange={(e) => setDistribution(e.target.value)}
                    className="bg-[#09090b] border border-zinc-800/80 text-zinc-200 text-xs rounded-xl p-2.5 focus:border-blue-500 focus:outline-none w-full transition font-sans cursor-pointer"
                  >
                    <option value="Uniform">🎲 Равномерное случайное [-10000; 10000]</option>
                    <option value="Normal">📈 Нормальное (Гауссово) [М=0, СКО=1000]</option>
                    <option value="ReverseSorted">📉 Обратно отсортированное [N, N-1, ... 1]</option>
                    <option value="AlmostSorted">⚖️ Почти упорядоченное (5% смещений)</option>
                    <option value="AllEqual">⚓ Идентичные значения (Все константа)</option>
                  </select>
                </div>

                <div className="flex flex-col gap-2">
                  <label className="text-xs text-zinc-400 font-bold uppercase tracking-wider">Тип данных в памяти:</label>
                  <select 
                    value={dataType}
                    onChange={(e) => setDataType(e.target.value)}
                    className="bg-[#09090b] border border-zinc-800/80 text-zinc-200 text-xs rounded-xl p-2.5 focus:border-blue-500 focus:outline-none w-full transition font-sans cursor-pointer"
                  >
                    <option value="float">💧 float (32-bit вещественный)</option>
                    <option value="double">💎 double (64-bit вещественный)</option>
                    <option value="struct">📦 Payload Struct (64 bytes + double key)</option>
                  </select>
                </div>

                <div className="flex flex-col gap-2">
                  <div className="flex justify-between text-xs">
                    <span className="text-zinc-400 font-bold uppercase tracking-wider">Запусков для усреднения:</span>
                    <span className="text-emerald-400 font-mono font-bold">{runsCount} итераций</span>
                  </div>
                  <input 
                    type="range"
                    min="1"
                    max="15"
                    step="1"
                    value={runsCount}
                    onChange={(e) => setRunsCount(Number(e.target.value))}
                    className="w-full accent-[#10b981] cursor-pointer h-1.5 bg-zinc-900 rounded-lg"
                  />
                </div>
              </div>

              {/* ADVANCED ADVANCED HARDWARE CONFIGURATION EXPANDER */}
              <div className="border-t border-zinc-900 pt-4 flex flex-col gap-2">
                <button
                  onClick={() => setShowAdvancedSettings(!showAdvancedSettings)}
                  className="flex items-center justify-between text-xs text-zinc-400 font-bold uppercase tracking-wider hover:text-white transition w-full py-1 cursor-pointer"
                >
                  <div className="flex items-center gap-2">
                    <Settings2 className="w-3.5 h-3.5 text-blue-500" />
                    <span>Эмуляция оборудования</span>
                  </div>
                  <span className="text-[10px] text-blue-400 font-mono font-semibold bg-zinc-900 px-2 py-0.5 rounded border border-zinc-805">
                    {showAdvancedSettings ? "Скрыть" : "Настроить"}
                  </span>
                </button>

                {showAdvancedSettings && (
                  <div className="flex flex-col gap-4 mt-2 bg-[#08080a] p-3.5 rounded-xl border border-zinc-900/60 transition-all duration-300">
                    
                    {/* PCIe Level Picker */}
                    <div className="flex flex-col gap-1.5">
                      <div className="flex justify-between items-center text-[11px]">
                        <span className="text-zinc-400 font-medium">Шина PCIe (H2D/D2H):</span>
                        <span className="text-indigo-400 font-mono font-bold">{pcieGen === "Gen3" ? "PCIe 3.0 x16" : pcieGen === "Gen4" ? "PCIe 4.0 x16" : "PCIe 5.0 x16"}</span>
                      </div>
                      <div className="grid grid-cols-3 gap-1">
                        {["Gen3", "Gen4", "Gen5"].map((gen) => (
                          <button
                            key={gen}
                            onClick={() => setPcieGen(gen)}
                            className={`text-[9px] font-bold font-mono py-1.5 rounded-lg border transition-all cursor-pointer ${
                              pcieGen === gen 
                                ? "bg-indigo-950/25 border-indigo-500 text-indigo-300 font-bold" 
                                : "bg-transparent border-zinc-900 text-zinc-500 hover:text-zinc-350"
                            }`}
                          >
                            {gen}
                          </button>
                        ))}
                      </div>
                    </div>

                    {/* GPU Overheat/Thermal Throttling Toggle */}
                    <div className="flex items-center justify-between gap-3 text-[11px] pt-1.5 border-t border-zinc-900/40">
                      <div className="flex flex-col">
                        <span className="text-zinc-400 font-medium flex items-center gap-1.5">
                          <Shield className="w-3 h-3 text-amber-500" />
                          Температурный лимит:
                        </span>
                        <span className="text-[9px] text-zinc-500 select-none">Троттлинг ядер (Thermal Throttling)</span>
                      </div>
                      
                      <button
                        onClick={() => setGpuThrottling(!gpuThrottling)}
                        className={`px-2.5 py-1.5 rounded-lg border text-[9px] font-mono font-bold transition-all cursor-pointer ${
                          gpuThrottling 
                            ? "bg-amber-950/20 border-amber-500 text-amber-400" 
                            : "bg-[#0c0c0e] border-zinc-900 text-zinc-500 hover:text-zinc-450"
                        }`}
                      >
                        {gpuThrottling ? "🔥 85°C (АКТИВЕН)" : "❄️ 64°C (НОРМА)"}
                      </button>
                    </div>

                  </div>
                )}
              </div>

              {/* ACTION BUTTON */}
              <button
                onClick={launchBenchmarkSim}
                disabled={isSimulating || selectedAlgs.length === 0}
                className="w-full py-3 px-4 bg-gradient-to-r from-blue-600 to-indigo-600 hover:from-blue-500 hover:to-indigo-550 text-white font-semibold rounded-xl shadow-lg hover:shadow-blue-500/10 active:scale-[0.98] disabled:from-zinc-900 disabled:to-zinc-900 disabled:text-zinc-650 disabled:border-zinc-800 disabled:cursor-not-allowed transition-all flex items-center justify-center gap-2 text-xs uppercase tracking-wider cursor-pointer font-sans border border-transparent"
              >
                {isSimulating ? (
                  <>
                    <RefreshCw className="w-4 h-4 animate-spin text-white" />
                    <span>Вычисление Итераций... {simulationProgress}%</span>
                  </>
                ) : (
                  <>
                    <Zap className="w-4 h-4 text-amber-300 fill-amber-300" />
                    <span>Провести замеры стенда</span>
                  </>
                )}
              </button>

              {/* Live Run log */}
              {simulationLog && (
                <div className="bg-[#09090b] border border-zinc-800/80 rounded-xl p-3.5 text-[11px] font-mono text-zinc-400 flex gap-3 items-start mt-1 shadow-inner">
                  <Clock className="w-4 h-4 text-blue-500 shrink-0 mt-0.5" />
                  <div className="flex-1 leading-relaxed">
                    <div className="text-zinc-500 text-[9px] uppercase font-bold tracking-wider">Системные лог-коммиты:</div>
                    <div className="text-zinc-300 mt-1">{simulationLog}</div>
                  </div>
                </div>
              )}
            </div>

            {/* Right side: Charts, Tables, PCIe analysis */}
            <div className="lg:col-span-8 flex flex-col gap-6">
              
              {/* PRIMARY GRAPH - AVERAGES & MEDIANS */}
              <div className="bg-[#111115] border border-zinc-900 rounded-2xl p-6 shadow-xl flex flex-col gap-5">
                <div className="flex flex-col sm:flex-row sm:items-center justify-between border-b border-zinc-800/60 pb-4 gap-2">
                  <div>
                    <h3 className="font-semibold text-white text-sm tracking-tight">Сравнительный график времени выполнения</h3>
                    <p className="text-xs text-zinc-400 mt-0.5">Показано среднее время и медиана (меньше означает быстрее)</p>
                  </div>
                  <div className="flex items-center gap-3 text-xs text-zinc-450 bg-[#09090b] px-3.5 py-2 border border-zinc-800/80 rounded-xl">
                    <div className="flex items-center gap-1.5">
                      <div className="w-2 h-2 bg-blue-500 rounded-full"></div>
                      <span className="font-medium font-mono">CPU (Intel/AMD)</span>
                    </div>
                    <div className="flex items-center gap-1.5">
                      <div className="w-2 h-2 bg-indigo-500 rounded-full"></div>
                      <span className="font-medium font-mono">GPU (CUDA cores)</span>
                    </div>
                  </div>
                </div>

                <div className="h-[280px] w-full mt-1">
                  <ResponsiveContainer width="100%" height="100%">
                    <BarChart
                      data={chartData}
                      margin={{ top: 10, right: 10, left: -20, bottom: 0 }}
                    >
                      <CartesianGrid strokeDasharray="3 3" stroke="#1d1d23" vertical={false} />
                      <XAxis 
                        dataKey="name" 
                        stroke="#71717a" 
                        fontSize={11} 
                        tickLine={false}
                        axisLine={false}
                      />
                      <YAxis 
                        stroke="#71717a" 
                        fontSize={11} 
                        tickLine={false}
                        axisLine={false}
                      />
                      <Tooltip 
                        contentStyle={{ backgroundColor: "#111115", borderColor: "#27272a", borderRadius: "12px", fontSize: "11px", color: "#f4f4f5" }}
                        cursor={{ fill: "rgba(255, 255, 255, 0.02)" }}
                      />
                      <Legend verticalAlign="top" height={36} iconType="circle" wrapperStyle={{ fontSize: '11px', color: '#a1a1aa' }} />
                      <Bar name="Среднее время (мс)" dataKey="Среднее время (мс)" radius={[4, 4, 0, 0]}>
                        {chartData.map((entry, index) => (
                          <Cell key={`cell-avg-${index}`} fill={entry.isGPU ? "#6366f1" : "#3b82f6"} />
                        ))}
                      </Bar>
                      <Bar name="Медиана (мс)" dataKey="Медиана (мс)" radius={[4, 4, 0, 0]}>
                        {chartData.map((entry, index) => (
                          <Cell key={`cell-med-${index}`} fill={entry.isGPU ? "#818cf8" : "#60a5fa"} />
                        ))}
                      </Bar>
                    </BarChart>
                  </ResponsiveContainer>
                </div>
              </div>

              {/* DIRECT ARCHITECTURE MATCH-UP */}
              <div className="bg-[#111115] border border-zinc-900 rounded-2xl p-6 shadow-xl flex flex-col gap-5">
                <div>
                  <h3 className="font-semibold text-white text-sm flex items-center gap-2.5">
                    <Server className="w-5 h-5 text-purple-400" />
                    ⚖️ Парное сравнение архитектур: CPU vs GPU
                  </h3>
                  <p className="text-xs text-zinc-400 mt-1">Прямое сопоставление эквивалентных реализаций алгоритмов сортировки на ядрах CPU и параллельной матрице CUDA</p>
                </div>

                <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
                  {[
                    {
                      name: "QuickSort",
                      cpuId: "CPU_QuickSort",
                      gpuId: "GPU_QuickSort",
                      icon: "⚡"
                    },
                    {
                      name: "MergeSort",
                      cpuId: "CPU_MergeSort",
                      gpuId: "GPU_MergeSort",
                      icon: "🌀"
                    },
                    {
                      name: "Odd-Even Sort",
                      cpuId: "CPU_OddEvenSort",
                      gpuId: "GPU_OddEven",
                      icon: "🎏"
                    },
                    {
                      name: "BogoSort",
                      cpuId: "CPU_BogoSort",
                      gpuId: "GPU_BogoSort",
                      icon: "🤪"
                    }
                  ].map((pair) => {
                    const cpuRun = results.find(r => r.algorithmName === pair.cpuId && r.arraySize === arraySize);
                    const gpuRun = results.find(r => r.algorithmName === pair.gpuId && r.arraySize === arraySize);

                    const bothExist = cpuRun && gpuRun;
                    let speedup = 0;
                    let winner: "CPU" | "GPU" | null = null;
                    if (bothExist) {
                      if (gpuRun.avgTimeMs > 0) {
                        speedup = cpuRun.avgTimeMs / gpuRun.avgTimeMs;
                        winner = speedup >= 1.0 ? "GPU" : "CPU";
                      }
                    }

                    return (
                      <div key={pair.name} className="bg-[#09090b] border border-zinc-850 p-4.5 rounded-2xl flex flex-col justify-between gap-4 relative overflow-hidden group hover:border-zinc-800 transition-all duration-300">
                        <div className="flex items-center justify-between border-b border-zinc-800/60 pb-2">
                          <span className="text-xs font-bold text-zinc-100 flex items-center gap-2 font-sans">
                            <span className="text-sm">{pair.icon}</span>
                            {pair.name}
                          </span>
                          <span className="text-[9px] text-zinc-500 font-mono font-bold bg-[#111115] px-1.5 py-0.5 rounded">N={arraySize >= 1000000 ? `${arraySize / 1000000}M` : `${arraySize / 1000}K`}</span>
                        </div>

                        {bothExist && cpuRun && gpuRun ? (
                          <div className="flex flex-col gap-3">
                            {/* CPU row */}
                            <div className="flex items-center justify-between text-xs">
                              <span className="text-zinc-400 flex items-center gap-2">
                                <span className="w-1.5 h-1.5 rounded-full bg-blue-500"></span>
                                CPU x86:
                              </span>
                              <span className="font-mono text-zinc-200 font-semibold">{cpuRun.avgTimeMs.toFixed(3)} ms</span>
                            </div>

                            {/* GPU row */}
                            <div className="flex items-center justify-between text-xs">
                              <span className="text-zinc-400 flex items-center gap-2">
                                <span className="w-1.5 h-1.5 rounded-full bg-indigo-500 animate-pulse"></span>
                                GPU CUDA:
                              </span>
                              <span className="font-mono text-zinc-200 font-semibold">{gpuRun.avgTimeMs.toFixed(3)} ms</span>
                            </div>

                            {/* Outcome Badge */}
                            <div className={`mt-1 p-2.5 rounded-xl text-[11px] font-bold text-center flex flex-col gap-0.5 border ${
                              winner === "GPU" 
                                ? "bg-emerald-950/20 text-emerald-400 border-emerald-500/20" 
                                : "bg-blue-950/20 text-blue-400 border-blue-500/20"
                            }`}>
                              <span className="tracking-wide uppercase text-[10px]">Победитель: {winner}</span>
                              <span className="text-[11px] font-mono mt-0.5">
                                {winner === "GPU" 
                                  ? `Ускорение: ${speedup.toFixed(1)}x` 
                                  : `Отставание PCIe: ${(1/speedup).toFixed(1)}x`
                                }
                              </span>
                            </div>
                          </div>
                        ) : (
                          <div className="flex flex-col gap-3 items-center justify-center py-1 text-center">
                            <span className="text-[10px] text-zinc-500 font-medium leading-relaxed max-w-[130px]">
                              {!cpuRun && !gpuRun ? "Параллельные замеры отсутствуют" : !cpuRun ? "Нет замера на CPU" : "Нет замера на GPU"}
                            </span>
                            <button
                              disabled={isSimulating}
                              onClick={async () => {
                                if (isSimulating) return;
                                setIsSimulating(true);
                                setSimulationProgress(25);
                                setSimulationLog(`Запись дуэльного лога: Инициализация ${pair.name} (CPU vs GPU)...`);
                                
                                await new Promise(r => setTimeout(r, 600));
                                setSimulationProgress(60);
                                setSimulationLog(`Моделирование вычислений на x86 и ядрах CUDA для N = ${arraySize.toLocaleString()}...`);
                                
                                await new Promise(r => setTimeout(r, 850));
                                setSimulationProgress(90);
                                setSimulationLog(`Замер PCIe таймингов и калибровка кэша...`);
 
                                try {
                                  const response = await fetch("/api/benchmark-simulate", {
                                    method: "POST",
                                    headers: { "Content-Type": "application/json" },
                                    body: JSON.stringify({
                                      arraySize,
                                      distribution,
                                      algorithms: [pair.cpuId, pair.gpuId],
                                      isDouble: dataType === "double"
                                    })
                                  });
                                  const data = await response.json();
                                  if (data.results) {
                                    setResults(prev => {
                                      const filtered = prev.filter(r => 
                                        !(r.arraySize === arraySize && (r.algorithmName === pair.cpuId || r.algorithmName === pair.gpuId))
                                      );
                                      return [...filtered, ...data.results];
                                    });
                                    setSimulationLog(`Результаты баттла ${pair.name} синхронизированы!`);
                                  }
                                } catch (e) {
                                  setSimulationLog("Задействован локальный откат симуляции дуэли.");
                                } finally {
                                  setIsSimulating(false);
                                  setSimulationProgress(100);
                                }
                              }}
                              className="w-full py-2 px-3 bg-indigo-950/20 hover:bg-indigo-900/30 border border-indigo-500/10 hover:border-indigo-500/30 text-indigo-300 text-[10px] font-semibold rounded-xl transition duration-200 flex items-center justify-center gap-1.5 cursor-pointer disabled:opacity-45"
                            >
                              <RefreshCw className="w-3 h-3 text-indigo-400 group-hover:rotate-180 transition duration-300" />
                              <span>Запустить дуэль</span>
                            </button>
                          </div>
                        )}
                      </div>
                    );
                  })}
                </div>
              </div>

              {/* PCIE OVERHEAD ANALYSIS */}
              <div className="bg-[#111115] border border-zinc-900 rounded-2xl p-6 shadow-xl flex flex-col gap-4">
                <div>
                  <h3 className="font-semibold text-white text-sm flex items-center gap-2.5">
                    <Database className="w-5 h-5 text-indigo-400" />
                    Анализ накладных расходов PCIe (GPU)
                  </h3>
                  <p className="text-xs text-zinc-400 mt-1">Таймлайн задержек при передаче данных: Host-To-Device (VRAM) vs Выполнения ядра CUDA vs Device-To-Host</p>
                </div>

                {gpuLatencyData.length === 0 ? (
                  <div className="flex-1 flex flex-col items-center justify-center text-center p-8 border border-dashed border-zinc-800 bg-[#09090b]/40 rounded-2xl min-h-[160px]">
                    <AlertCircle className="w-8 h-8 text-zinc-650 mb-3" />
                    <span className="text-xs text-zinc-500 max-w-sm leading-relaxed">Для генерации сквозной диаграммы PCIe задержек произведите замер хотя бы одного CUDA алгоритма в параметрах слева.</span>
                  </div>
                ) : (
                  <div className="h-[180px] w-full mt-2">
                    <ResponsiveContainer width="100%" height="100%">
                      <BarChart
                        data={gpuLatencyData}
                        layout="vertical"
                        margin={{ top: 10, right: 10, left: -20, bottom: 5 }}
                      >
                        <CartesianGrid strokeDasharray="3 3" stroke="#1d1d23" horizontal={false} />
                        <XAxis type="number" stroke="#71717a" fontSize={10} tickLine={false} axisLine={false} />
                        <YAxis dataKey="name" type="category" stroke="#71717a" fontSize={10} tickLine={false} axisLine={false} />
                        <Tooltip contentStyle={{ backgroundColor: "#111115", borderColor: "#27272a", borderRadius: "12px", fontSize: "11px" }} />
                        <Legend iconType="rect" wrapperStyle={{ fontSize: '10px', color: '#71717a' }} />
                        <Bar name="PCIe Upload (H2D)" dataKey="PCIe Upload (H2D)" stackId="a" fill="#3b82f6" radius={[0, 0, 0, 0]} />
                        <Bar name="CUDA Kernel Compute" dataKey="CUDA Kernel Compute" stackId="a" fill="#10b981" radius={[0, 0, 0, 0]} />
                        <Bar name="PCIe Download (D2H)" dataKey="PCIe Download (D2H)" stackId="a" fill="#f59e0b" radius={[0, 4, 4, 0]} />
                      </BarChart>
                    </ResponsiveContainer>
                  </div>
                )}
              </div>

              {/* HISTORIC RAW RESULTS TABLE */}
              <div className="bg-[#111115] border border-zinc-900 rounded-2xl overflow-hidden shadow-xl">
                <div className="px-6 py-4.5 border-b border-zinc-800/60 flex justify-between items-center bg-[#15151a]">
                  <span className="font-semibold text-xs text-white uppercase tracking-wider">Таблица сырых метрик испытаний</span>
                  <span className="text-[10px] text-zinc-500 font-mono font-bold bg-[#09090b] px-2 py-0.5 border border-zinc-800 rounded">Погрешность: ±0.001 ms</span>
                </div>
                
                <div className="overflow-x-auto">
                  <table className="w-full text-left text-xs text-zinc-350">
                    <thead className="bg-[#16161c] text-zinc-400 text-[10px] font-bold uppercase tracking-wider border-b border-zinc-900">
                      <tr>
                        <th className="px-6 py-4">Алгоритм</th>
                        <th className="px-6 py-4">Архитектура</th>
                        <th className="px-6 py-4">Размер N</th>
                        <th className="px-6 py-4">Среднее (ms)</th>
                        <th className="px-6 py-4">Медиана (ms)</th>
                        <th className="px-6 py-4 font-mono text-[9px] tracking-wide">Variance</th>
                        <th className="px-6 py-4 text-center font-bold">Оценка нагрузки</th>
                      </tr>
                    </thead>
                    <tbody className="divide-y divide-zinc-900 bg-[#111115]">
                      {results.length === 0 ? (
                        <tr>
                          <td colSpan={7} className="text-center px-6 py-8 text-zinc-550">Результаты отсутствуют. Запустите эксперименты выше.</td>
                        </tr>
                      ) : (
                        results.map((res, idx) => (
                          <tr key={idx} className="hover:bg-zinc-850/20 transition duration-100">
                            <td className="px-6 py-3.5 font-mono font-bold text-white text-[12px]">{res.algorithmName}</td>
                            <td className="px-6 py-3.5">
                              <span className={`inline-flex items-center gap-1.5 px-2.5 py-1 rounded-lg text-[10px] font-semibold border ${
                                res.isGPU 
                                  ? "bg-indigo-950/20 text-indigo-300 border-indigo-500/15" 
                                  : "bg-blue-950/20 text-blue-300 border-blue-500/15"
                              }`}>
                                {res.isGPU ? "CUDA GPU" : "Intel/AMD x86"}
                              </span>
                            </td>
                            <td className="px-6 py-3.5 font-mono text-zinc-450">{res.arraySize.toLocaleString()}</td>
                            <td className="px-6 py-3.5 font-mono text-blue-400 font-bold">{res.avgTimeMs.toFixed(3)} ms</td>
                            <td className="px-6 py-3.5 font-mono text-zinc-500">{res.medianTimeMs.toFixed(3)} ms</td>
                            <td className="px-6 py-3.5 font-mono text-zinc-550 text-[11px]">{res.varianceMs.toFixed(6)}</td>
                            <td className="px-6 py-3.5 text-center">
                              {res.isGPU ? (
                                res.avgKernelTimeMs > res.avgUploadTimeMs ? (
                                  <span className="text-[10px] text-zinc-300 bg-emerald-950/20 border border-emerald-500/20 px-2.5 py-0.5 rounded-md font-medium">Compute Bound</span>
                                ) : (
                                  <span className="text-[10px] text-amber-500 bg-amber-950/20 border border-amber-500/20 px-2.5 py-0.5 rounded-md font-medium">PCIe Bottleneck</span>
                                )
                              ) : (
                                <span className="text-[10px] text-zinc-400 bg-zinc-900 border border-zinc-800 px-2.5 py-0.5 rounded-md font-medium">L1/L2 High Cache</span>
                              )}
                            </td>
                          </tr>
                        ))
                      )}
                    </tbody>
                  </table>
                </div>
              </div>

            </div>

          </div>
        )}

        {/* --- TAB 2: SORTING ANIMATION --- */}
        {activeTab === "visual" && (
          <div className="bg-[#111115] border border-zinc-900 rounded-2xl p-6 shadow-[0_12px_40px_-15px_rgba(0,0,0,0.7)] flex flex-col gap-6">
            
            <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between border-b border-zinc-800/60 pb-5 gap-4">
              <div>
                <h2 className="text-base font-bold text-white flex items-center gap-2.5 tracking-tight font-sans">
                  <Zap className="text-amber-400 w-5 h-5 fill-amber-400 animate-pulse" />
                  Динамический пошаговый симулятор
                </h2>
                <p className="text-xs text-zinc-400 mt-1">Наглядный графический разбор перестановок и разметки элементов</p>
              </div>
              
              {/* Controls panel */}
              <div className="flex flex-wrap items-center gap-4">
                <div className="flex items-center gap-2.5">
                  <span className="text-xs text-zinc-400 font-bold uppercase tracking-wider">Сортировка:</span>
                  <select
                    value={visualAlg}
                    onChange={(e: any) => setVisualAlg(e.target.value)}
                    disabled={isVisualRunning}
                    className="bg-[#09090b] border border-zinc-800/80 text-zinc-200 text-xs rounded-xl p-2.5 focus:border-blue-500 focus:outline-none max-w-[240px] md:max-w-none transition cursor-pointer font-mono"
                  >
                    <optgroup label="Быстрые алгоритмы (O(N log N))">
                      <option value="quick">⚡ QuickSort (Опорный элемент)</option>
                      <option value="merge">⚡ MergeSort (Слияние половин)</option>
                      <option value="heap">⚡ HeapSort (Пирамидальная куча)</option>
                      <option value="bitonic">⚡ BitonicSort (Параллельная схема)</option>
                      <option value="shell">⚡ ShellSort (Сортировка Шелла)</option>
                      <option value="comb">⚡ CombSort (Сортировка расческой)</option>
                    </optgroup>
                    
                    <optgroup label="Простые квадратичные алгоритмы (O(N²))">
                      <option value="bubble">🐢 BubbleSort (Пузырьковая классика)</option>
                      <option value="selection">🐢 SelectionSort (Сортировка выбором)</option>
                      <option value="insertion">🐢 InsertionSort (Сортировка вставками)</option>
                      <option value="cocktail">🐢 CocktailSort (Шейкерная двунаправленная)</option>
                      <option value="gnome">🐢 GnomeSort (Гномья сортировка)</option>
                      <option value="oddEven">🐢 Odd-EvenSort (Чет-нечетная)</option>
                      <option value="cycle">🐢 CycleSort (Циклическая сортировка)</option>
                    </optgroup>

                    <optgroup label="Линейные и специальные">
                      <option value="radixLsd">📊 RadixSort LSD (Поразрядная)</option>
                      <option value="counting">📊 CountingSort (Сортировка подсчетом)</option>
                      <option value="pancake">📊 PancakeSort (Блинная сортировка)</option>
                    </optgroup>

                    <optgroup label="Теоретические и экзотические">
                      <option value="bogo">🤪 BogoSort (Случайное перемешивание)</option>
                      <option value="stooge">🤪 StoogeSort (Рекурсивная 2/3 Студжа)</option>
                    </optgroup>
                  </select>
                </div>

                <div className="flex items-center gap-2.5">
                  <span className="text-xs text-zinc-400 font-bold uppercase tracking-wider">Объем:</span>
                  <select
                    value={visualSize}
                    onChange={(e: any) => setVisualSize(Number(e.target.value))}
                    disabled={isVisualRunning}
                    className="bg-[#09090b] border border-zinc-800/80 text-zinc-200 text-xs rounded-xl p-2.5 focus:border-blue-500 focus:outline-none transition cursor-pointer font-mono"
                  >
                    <option value={15}>15 элементов</option>
                    <option value={32}>32 элемента</option>
                    <option value={45}>45 элементов</option>
                    <option value={64}>64 элемента</option>
                    <option value={100}>100 элементов</option>
                  </select>
                </div>

                <button
                  onClick={generateNewVisualArray}
                  className="px-4 py-2.5 bg-zinc-800 hover:bg-zinc-700 hover:text-white border border-zinc-700/60 text-xs font-semibold text-zinc-200 rounded-xl transition duration-150 cursor-pointer"
                >
                  Новый массив
                </button>
              </div>
            </div>

            {/* THE VISUAL CANVAS CONTAINER */}
            <div className="relative bg-[#09090b] rounded-2xl border border-zinc-900 h-[380px] w-full flex items-end px-4 py-8 overflow-hidden shadow-inner">
              {visualArray.map((val, idx) => {
                const isCompare = activeCompareIdx.includes(idx);
                const isSwap = activeSwapIdx.includes(idx);
                const isPivot = pivotIdx === idx;
                
                let barColor = "bg-gradient-to-t from-blue-700 to-blue-500 opacity-90";
                if (isPivot) {
                  barColor = "bg-gradient-to-t from-amber-600 to-amber-300 drop-shadow-[0_0_8px_rgba(245,158,11,0.4)]";
                } else if (isCompare) {
                  barColor = "bg-gradient-to-t from-red-600 to-red-450 drop-shadow-[0_0_8px_rgba(239,68,68,0.5)]";
                } else if (isSwap) {
                  barColor = "bg-gradient-to-t from-emerald-600 to-emerald-450 drop-shadow-[0_0_8px_rgba(16,185,129,0.5)]";
                }

                return (
                  <div 
                    key={idx} 
                    className="flex-1 flex flex-col justify-end items-center group relative h-full"
                    style={{ margin: "0 1px" }}
                  >
                    {/* Val tooltip on hover */}
                    <div className="absolute -top-7 opacity-0 group-hover:opacity-100 transition duration-150 bg-zinc-900 border border-zinc-800 text-white font-mono rounded-lg text-[10px] px-2 py-1 z-10 pointer-events-none shadow-xl">
                      Значение: {val}
                    </div>

                    {/* Vertical Bar */}
                    <div 
                      className={`w-full rounded-t-md transition-all duration-100 ${barColor}`}
                      style={{ height: `${(val / 320) * 100}%` }}
                    ></div>

                    {/* Key indicators beneath */}
                    {visualSize <= 32 && (
                      <span className="font-mono text-[9px] text-zinc-650 mt-2 font-bold select-none">{val}</span>
                    )}
                  </div>
                );
              })}

              {/* Status Overlay when empty */}
              {visualArray.length === 0 && (
                <div className="absolute inset-0 flex items-center justify-center text-zinc-500 text-xs text-center p-6">
                  Нет данных. Сгенерируйте новый массив для тестирования.
                </div>
              )}
            </div>



            {/* PLAYBACK CONTROL PANEL */}
            <div className="bg-[#0c0c0e] border border-zinc-850 rounded-2xl p-4 flex flex-col sm:flex-row items-center justify-between gap-4">
              
              <div className="flex items-center gap-3">
                <button
                  onClick={startAnimation}
                  className="p-3 bg-blue-600 hover:bg-blue-500 hover:scale-105 active:scale-95 text-white rounded-full shadow-lg hover:shadow-blue-500/25 transition-all flex items-center justify-center cursor-pointer"
                  title="Запустить алгоритм"
                >
                  <Play className="w-4 h-4 text-white fill-white" />
                </button>

                <button
                  onClick={pauseAnimation}
                  disabled={!isVisualRunning || isVisualPaused}
                  className="p-3 bg-zinc-800 hover:bg-zinc-700 text-zinc-300 hover:text-white rounded-full disabled:opacity-45 disabled:cursor-not-allowed active:scale-95 transition-all flex items-center justify-center cursor-pointer"
                  title="Поставить на паузу"
                >
                  <Pause className="w-4 h-4 fill-current" />
                </button>

                {/* STEP FORWARD STEPPER BUTTON */}
                <button
                  onClick={() => {
                    stepRequestedRef.current = true;
                    if (!isVisualRunning) {
                      isPausedRef.current = true;
                      setIsVisualPaused(true);
                      startAnimation();
                    } else if (isVisualPaused) {
                      isPausedRef.current = false;
                      setTimeout(() => {
                        isPausedRef.current = true;
                      }, 20);
                    } else {
                      isPausedRef.current = true;
                      setIsVisualPaused(true);
                      setVisualStatus("Остановлено. Пошаговый запуск...");
                    }
                  }}
                  className="p-3 bg-zinc-800 hover:bg-zinc-700 text-zinc-300 hover:text-white rounded-full active:scale-95 transition-all flex items-center justify-center cursor-pointer"
                  title="Сделать один шаг сортировки (Шаг вперед)"
                >
                  <SkipForward className="w-4 h-4" />
                </button>

                <button
                  onClick={abortAndResetVisual}
                  disabled={!isVisualRunning}
                  className="p-3 bg-red-950/20 hover:bg-red-900/40 text-red-400 hover:text-white border border-red-500/10 rounded-full disabled:opacity-44 disabled:cursor-not-allowed active:scale-95 transition-all flex items-center justify-center cursor-pointer"
                  title="Остановить и сбросить"
                >
                  <Trash2 className="w-4 h-4" />
                </button>
              </div>

              {/* Animator details */}
              <div className="flex items-center gap-3 bg-[#09090b] border border-zinc-800/80 px-4 py-2.5 rounded-xl text-xs font-mono font-medium max-w-full truncate">
                <span className="text-zinc-500 font-bold uppercase tracking-wider text-[10px]">Статус сортера:</span>
                <span className="text-emerald-400 font-bold tracking-tight">{visualStatus}</span>
              </div>

              {/* Speed slider */}
              <div className="flex items-center gap-3.5 w-full sm:w-auto">
                <span className="text-xs text-zinc-400 whitespace-nowrap font-bold uppercase tracking-wider">Интервал шага:</span>
                <input
                  type="range"
                  min="5"
                  max="100"
                  value={visualSpeed}
                  onChange={(e) => setVisualSpeed(Number(e.target.value))}
                  className="w-full sm:w-36 accent-blue-500 cursor-pointer h-1.5 bg-zinc-900 rounded-lg"
                />
                <span className="text-xs text-blue-400 font-mono font-bold w-10 text-right">{visualSpeed}%</span>
              </div>

            </div>

            {/* COLOR CODES LEGEND */}
            <div className="grid grid-cols-2 lg:grid-cols-4 gap-4 bg-[#09090b] p-4 rounded-xl border border-zinc-900 text-xs text-left">
              <div className="flex items-center gap-3">
                <div className="w-3.5 h-3.5 rounded bg-blue-600 shadow-[0_0_8px_rgba(59,130,246,0.2)]"></div>
                <div>
                  <div className="text-zinc-200 font-bold font-sans">Неотсортированный</div>
                  <div className="text-[10px] text-zinc-500 font-medium font-sans">Базовое состояние чисел</div>
                </div>
              </div>

              <div className="flex items-center gap-3">
                <div className="w-3.5 h-3.5 rounded bg-red-500 shadow-[0_0_8px_rgba(239,68,68,0.3)]"></div>
                <div>
                  <div className="text-zinc-200 font-bold font-sans">Операция сравнения</div>
                  <div className="text-[10px] text-zinc-500 font-medium font-mono">L1/L2 read; cmp instruction</div>
                </div>
              </div>

              <div className="flex items-center gap-3">
                <div className="w-3.5 h-3.5 rounded bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.3)]"></div>
                <div>
                  <div className="text-zinc-200 font-bold font-sans">Перестановка (Swap)</div>
                  <div className="text-[10px] text-zinc-505 font-medium font-mono">std::iter_swap write</div>
                </div>
              </div>

              <div className="flex items-center gap-3">
                <div className="w-3.5 h-3.5 rounded bg-amber-500 shadow-[0_0_8px_rgba(245,158,11,0.3)]"></div>
                <div>
                  <div className="text-zinc-200 font-bold font-sans">Опорный элемент (Pivot)</div>
                  <div className="text-[10px] text-zinc-505 font-medium font-sans">Разделитель поддиапазонов</div>
                </div>
              </div>
            </div>

          </div>
        )}

        {/* --- TAB 4: ARCHITECTURAL THEORY --- */}
        {activeTab === "theory" && (
          <div className="flex flex-col gap-8">
            <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
              
              {/* CPU Architecture Theory */}
              <div className="bg-[#111115] border border-zinc-900 p-6 rounded-2xl shadow-xl flex flex-col gap-4">
                <h2 className="text-sm font-bold text-white flex items-center gap-2.5 border-b border-zinc-850 pb-3">
                  <Cpu className="text-blue-500 w-5 h-5 font-bold" />
                  Локальность кэшей & Вычисления CPU
                </h2>

                <ul className="text-xs text-zinc-350 flex flex-col gap-4 leading-relaxed">
                  <li>
                    <strong className="text-zinc-205 block mb-1 font-bold">Фактор L1/L2/L3 Кэш-линий (Cache Lines)</strong>
                    Классические процессоры х86/ARM обладают многоуровневыми кэшами сверхбыстрой SRAM-памяти. Каждое чтение из RAM захватывает блок в 64 байта (кэш-линия). 
                    Алгоритмы типа <strong className="text-blue-400">QuickSort</strong> и подсегменты <strong className="text-blue-400">TimSort</strong> обладают высокой локальностью ссылок — они последовательно сканируют память, из-за чего промахов кэша (Cache Miss) почти не происходит.
                  </li>
                  <li>
                    <strong className="text-zinc-205 block mb-1 font-bold font-sans">Cache Miss на HeapSort (Бинарная куча)</strong>
                    Пирамидальная сортировка (HeapSort) вынуждена прыгать по индексам вида <code className="bg-zinc-950 text-amber-500 px-1.5 py-0.5 rounded text-[10px] select-all font-mono">2 * i + 1</code> и <code className="bg-zinc-950 text-amber-500 px-1.5 py-0.5 rounded text-[10px] select-all font-mono">2 * i + 2</code>. 
                    На объемах N &gt; 100k это приводит к постоянным промахам по системным кэшам CPU, заставляя процессор простаивать в ожидании медленной системной шины DDR RAM.
                  </li>
                  <li>
                    <strong className="text-zinc-205 block mb-1 font-bold">Влияние упорядоченности (TimSort)</strong>
                    Тимсорт разделяет массив на упорядоченные блоки (Run) и сливает их. На частично упорядоченной выборке TimSort задействует алгоритм вставки, требующий <code className="bg-[#09090b] px-1 rounded text-[10px] font-mono">O(N)</code> вычислений, оставляя позади все остальные алгоритмы.
                  </li>
                </ul>
              </div>

              {/* GPU & PCIe Architecture Theory */}
              <div className="bg-[#111115] border border-zinc-900 p-6 rounded-2xl shadow-xl flex flex-col gap-4">
                <h2 className="text-sm font-bold text-white flex items-center gap-2.5 border-b border-zinc-850 pb-3">
                  <Zap className="text-indigo-400 w-5 h-5 fill-indigo-400" />
                  Архитектура CUDA & Латентность шины PCIe
                </h2>

                <ul className="text-xs text-zinc-350 flex flex-col gap-4 leading-relaxed">
                  <li>
                    <strong className="text-zinc-205 block mb-1 font-bold">Бутылочное горлышко PCIe (PCI Express Overhead)</strong>
                    Оперативная память CPU (Host) и выделенная память GPU (Device VRAM) разделены физической системной шиной PCIe. 
                    Инструкция <code className="bg-zinc-950 text-indigo-400 px-1 rounded text-[10px] font-mono">cudaMemcpy</code> вынуждена инициировать DMA-передачи. Накладные задержки на инициализацию обмена составляют от 0.5 до 1.5 мс.
                    Именно поэтому для массивов маленького объема GPU сортировки всегда работают дольше, чем быстрый CPU.
                  </li>
                  <li>
                    <strong className="text-indigo-205 block mb-1 font-bold">Массовая параллелизация CUDA (Grids, Blocks, Warps)</strong>
                    Графический адаптер оперирует тысячами аппаратных ядер. GPU формирует сетку (Grid), делящую вызов на Блоки (Blocks) и Варпы (Warps по 32 потока). 
                    В битонической сортировке (<strong className="text-indigo-400">Bitonic Sort</strong>) ядра CUDA вычисляют независимые перестановки индексов по побитовой маске, сортируя миллионы чисел одновременно всего за мизерную долю миллисекунды.
                  </li>
                  <li>
                    <strong className="text-indigo-205 block mb-1 font-bold">Коалесцентность Global Memory Access</strong>
                    Широковещательный доступ к глобальной памяти GPU работает эффективно тогда, когда соседние потоки варпа считывают соседние 32/64-битные ячейки в RAM. 
                    Алгоритмы сортировки на CUDA проектируются так, чтобы избежать ветвлений внутри варпов (warp divergence) и снизить перерасход шины памяти.
                  </li>
                </ul>
              </div>

            </div>



          </div>
        )}




        {/* --- TAB: REPORTS & EXPORT --- */}
        {activeTab === "reports" && (
          <div className="flex flex-col gap-6">
            
            {/* Header info */}
            <div className="bg-zinc-900 border border-zinc-800 p-6 rounded-xl shadow-lg flex flex-col md:flex-row md:items-center justify-between gap-4">
              <div>
                <h2 className="text-lg font-bold text-white flex items-center gap-2">
                  <FileSpreadsheet className="text-emerald-400 w-5 h-5" />
                  Центр управления и экспорта отчетов
                </h2>
                <p className="text-xs text-zinc-400 mt-1">
                  Выгрузка результатов испытаний в структурированные форматы, проведение сравнительного анализа и очистка кэша замеров
                </p>
              </div>
              <div className="flex gap-2 shrink-0">
                <button
                  onClick={() => {
                    if (window.confirm("Вы уверены, что хотите полностью очистить историю замеров?")) {
                      setResults([]);
                    }
                  }}
                  disabled={results.length === 0}
                  className="px-4 py-2 bg-red-950/40 hover:bg-red-900/40 border border-red-500/20 text-red-200 text-xs font-semibold rounded-lg hover:text-white transition flex items-center gap-1.5 disabled:opacity-40 disabled:cursor-not-allowed"
                >
                  <Trash2 className="w-3.5 h-3.5" />
                  Очистить историю
                </button>
              </div>
            </div>

            {results.length === 0 ? (
              <div className="bg-zinc-900/50 border border-dashed border-zinc-800 rounded-xl p-12 text-center flex flex-col items-center justify-center gap-3">
                <FileSpreadsheet className="w-12 h-12 text-zinc-600 animate-pulse" />
                <h3 className="text-sm font-semibold text-zinc-300">История замеров пуста</h3>
                <p className="text-xs text-zinc-500 max-w-sm leading-relaxed">
                  Перейдите во вкладку <strong className="text-zinc-400">"Лаборатория бенчмарков"</strong>, чтобы выполнить кастомные тесты на CUDA и CPU архитектурах, после чего здесь появятся детальные отчеты.
                </p>
                <button
                  onClick={() => setActiveTab("bench")}
                  className="mt-2 bg-blue-600 hover:bg-blue-500 text-white text-xs font-semibold px-4 py-2 rounded-lg transition"
                >
                  Запустить бенчмарк
                </button>
              </div>
            ) : (
              <>
                {/* Stats Bento Grid */}
                <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
                  {/* Total runs */}
                  <div className="bg-zinc-900 border border-zinc-800/80 p-4.5 rounded-xl shadow-md flex items-center gap-4">
                    <div className="p-3 bg-blue-600/10 border border-blue-500/20 rounded-lg text-blue-400">
                      <BarChart3 className="w-5 h-5" />
                    </div>
                    <div>
                      <span className="text-[10px] text-zinc-500 uppercase font-bold tracking-wider">Всего испытаний</span>
                      <h4 className="text-xl font-bold font-mono text-white mt-0.5">{results.length}</h4>
                    </div>
                  </div>

                  {/* Fastest */}
                  {(() => {
                    const sortedResults = [...results].sort((a, b) => a.avgTimeMs - b.avgTimeMs);
                    const fastest = sortedResults[0];
                    return (
                      <div className="bg-zinc-900 border border-zinc-800/80 p-4.5 rounded-xl shadow-md flex items-center gap-4">
                        <div className="p-3 bg-emerald-600/10 border border-emerald-500/20 rounded-lg text-emerald-400">
                          <Zap className="w-5 h-5" />
                        </div>
                        <div className="flex-1 min-w-0">
                          <span className="text-[10px] text-zinc-500 uppercase font-bold tracking-wider block">Самый быстрый</span>
                          <span className="text-xs font-semibold text-white truncate block mt-0.5">{fastest?.algorithmName || "нет данных"}</span>
                          <span className="text-xs font-mono font-medium text-emerald-400 block">{fastest?.avgTimeMs.toFixed(3) || "0.000"} ms</span>
                        </div>
                      </div>
                    );
                  })()}

                  {/* Slowest */}
                  {(() => {
                    const sortedResults = [...results].sort((a, b) => b.avgTimeMs - a.avgTimeMs);
                    const slowest = sortedResults[0];
                    return (
                      <div className="bg-zinc-900 border border-zinc-800/80 p-4.5 rounded-xl shadow-md flex items-center gap-4">
                        <div className="p-3 bg-red-600/10 border border-red-500/20 rounded-lg text-red-400">
                          <Clock className="w-5 h-5" />
                        </div>
                        <div className="flex-1 min-w-0">
                          <span className="text-[10px] text-zinc-500 uppercase font-bold tracking-wider block">Самый медленный</span>
                          <span className="text-xs font-semibold text-white truncate block mt-0.5">{slowest?.algorithmName || "нет данных"}</span>
                          <span className="text-xs font-mono font-medium text-red-400 block">{slowest?.avgTimeMs.toFixed(3) || "0.000"} ms</span>
                        </div>
                      </div>
                    );
                  })()}

                  {/* Speedup */}
                  {(() => {
                    // Рассчитаем максимальное ускорение (CPU_time / GPU_time) для одинаковых N
                    let maxRatio = 1.0;
                    let bestPair: { cpu: string; gpu: string; n: number } | null = null;
                    
                    const cpuResults = results.filter(r => !r.isGPU);
                    const gpuResults = results.filter(r => r.isGPU);
                    
                    cpuResults.forEach(cpu => {
                      gpuResults.forEach(gpu => {
                        if (cpu.arraySize === gpu.arraySize && gpu.avgTimeMs > 0) {
                          const ratio = cpu.avgTimeMs / gpu.avgTimeMs;
                          if (ratio > maxRatio) {
                            maxRatio = ratio;
                            bestPair = { cpu: cpu.algorithmName, gpu: gpu.algorithmName, n: cpu.arraySize };
                          }
                        }
                      });
                    });

                    return (
                      <div className="bg-zinc-900 border border-zinc-800/80 p-4.5 rounded-xl shadow-md flex items-center gap-4">
                        <div className="p-3 bg-purple-600/10 border border-purple-500/20 rounded-lg text-purple-400">
                          <Cpu className="w-5 h-5" />
                        </div>
                        <div className="flex-1 min-w-0">
                          <span className="text-[10px] text-zinc-500 uppercase font-bold tracking-wider block">Пиковое ускорение GPU</span>
                          {bestPair ? (
                            <>
                              <h4 className="text-lg font-extrabold font-mono text-purple-400 mt-0.5">{(maxRatio).toFixed(1)}x</h4>
                              <span className="text-[9px] text-zinc-500 block truncate">
                                N={(bestPair as any).n.toLocaleString()} { (bestPair as any).gpu } vs { (bestPair as any).cpu }
                              </span>
                            </>
                          ) : (
                            <span className="text-xs font-medium text-zinc-500 block mt-1">Ожидает парных тестов</span>
                          )}
                        </div>
                      </div>
                    );
                  })()}
                </div>

                {/* Main section: formats generator & live table */}
                <div className="grid grid-cols-1 lg:grid-cols-12 gap-6 items-start">
                  
                  {/* Left Column: Generator controls */}
                  <div className="lg:col-span-5 bg-zinc-900 border border-zinc-800 rounded-xl p-5 flex flex-col gap-5 shadow-xl">
                    <div>
                      <h3 className="font-semibold text-white text-xs uppercase tracking-wider text-zinc-400">Инструменты генерации</h3>
                      <p className="text-[11px] text-zinc-500 mt-1">Выберите необходимый формат, чтобы скачать или скопировать сводные данные</p>
                    </div>

                    <div className="flex flex-col gap-3">
                      {/* CSV option */}
                      <div className="bg-zinc-950 p-4 rounded-xl border border-zinc-850 flex flex-col gap-3">
                        <div className="flex items-center justify-between">
                          <span className="font-semibold text-xs text-white flex items-center gap-1.5">
                            <FileSpreadsheet className="w-4 h-4 text-emerald-500" />
                            Табличный CSV Файл
                          </span>
                          <span className="text-[9px] bg-zinc-900 px-1.5 py-0.5 rounded text-zinc-500 font-mono">.csv</span>
                        </div>
                        <p className="text-[11px] text-zinc-400 leading-relaxed">
                          Идеально для импорта в Microsoft Excel, Google Таблицы или Python (Pandas). Содержит плоскую структуру со всеми временными сегментами.
                        </p>
                        <button
                          onClick={() => {
                            let csvContent = "\uFEFF"; // Добавляем BOM для корректного отображения кириллицы в Excel
                            csvContent += "Никнейм/Алгоритм;Архитектура;Размер_N;Среднее_ms;Медиана_ms;Дисперсия;PCIe_Upload_ms;CUDA_Kernel_ms;PCIe_Download_ms\r\n";
                            results.forEach(res => {
                              csvContent += `"${res.algorithmName}";"${res.isGPU ? "CUDA GPU" : "Intel/AMD CPU"}";${res.arraySize};${res.avgTimeMs.toFixed(6).replace('.', ',')};${res.medianTimeMs.toFixed(6).replace('.', ',')};${res.varianceMs.toFixed(6).replace('.', ',')};${res.avgUploadTimeMs.toFixed(6).replace('.', ',')};${res.avgKernelTimeMs.toFixed(6).replace('.', ',')};${res.avgDownloadTimeMs.toFixed(6).replace('.', ',')}\r\n`;
                            });
                            const value = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
                            const url = URL.createObjectURL(value);
                            const link = document.createElement("a");
                            link.setAttribute("href", url);
                            link.setAttribute("download", `SortBench_Comprehensive_Report_${Date.now()}.csv`);
                            document.body.appendChild(link);
                            link.click();
                            document.body.removeChild(link);
                          }}
                          className="w-full bg-emerald-600 hover:bg-emerald-500 text-white text-xs font-semibold py-2 rounded-lg transition text-center"
                        >
                          Скачать расширенный CSV (.csv)
                        </button>
                      </div>

                      {/* JSON Option */}
                      <div className="bg-zinc-950 p-4 rounded-xl border border-zinc-850 flex flex-col gap-3">
                        <div className="flex items-center justify-between">
                          <span className="font-semibold text-xs text-white flex items-center gap-1.5">
                            <Code className="w-4 h-4 text-blue-400" />
                            Программный JSON Сниппет
                          </span>
                          <span className="text-[9px] bg-zinc-900 px-1.5 py-0.5 rounded text-zinc-500 font-mono">.json</span>
                        </div>
                        <p className="text-[11px] text-zinc-400 leading-relaxed">
                          Полный структурированный массив объектов в формате JSON для интеграции во внешние веб-сервисы и графические утилиты.
                        </p>
                        <div className="flex gap-2">
                          <button
                            onClick={() => {
                              const jsonStr = JSON.stringify(results, null, 2);
                              const value = new Blob([jsonStr], { type: 'application/json' });
                              const url = URL.createObjectURL(value);
                              const link = document.createElement("a");
                              link.setAttribute("href", url);
                              link.setAttribute("download", `SortBench_JSON_Report_${Date.now()}.json`);
                              document.body.appendChild(link);
                              link.click();
                              document.body.removeChild(link);
                            }}
                            className="flex-1 bg-zinc-800 hover:bg-zinc-700 hover:text-white text-zinc-200 text-xs font-semibold py-2 rounded-lg transition text-center"
                          >
                            Скачать в JSON
                          </button>
                          <button
                            onClick={() => {
                              const jsonStr = JSON.stringify(results, null, 2);
                              navigator.clipboard.writeText(jsonStr);
                              triggerCopyNotification("Структурированный JSON отчет успешно скопирован!");
                            }}
                            className="bg-zinc-800 hover:bg-zinc-700 hover:text-white text-zinc-200 text-xs font-semibold px-3 rounded-lg transition"
                            title="Скопировать"
                          >
                            Copy
                          </button>
                        </div>
                      </div>

                      {/* Markdown option */}
                      <div className="bg-zinc-950 p-4 rounded-xl border border-zinc-850 flex flex-col gap-3">
                        <div className="flex items-center justify-between">
                          <span className="font-semibold text-xs text-white flex items-center gap-1.5">
                            <Info className="w-4 h-4 text-amber-500" />
                            Markdown-разметка (для GitHub)
                          </span>
                          <span className="text-[9px] bg-zinc-900 px-1.5 py-0.5 rounded text-zinc-500 font-mono">.md</span>
                        </div>
                        <p className="text-[11px] text-zinc-400 leading-relaxed">
                          Таблица со сравнительным анализом в Markdown формате, которую вы можете сразу опубликовать в Readme вашего проекта.
                        </p>
                        <button
                          onClick={() => {
                            let md = `## Сводная таблица производительности SortBench (CUDA vs CPU)\n\n| Алгоритм | Архитектура | Размер N | Среднее время (ms) | Медиана (ms) | Дисперсия |\n| :--- | :--- | :---: | :---: | :---: | :---: |\n`;
                            results.forEach(res => {
                              md += `| **${res.algorithmName}** | ${res.isGPU ? "NVIDIA CUDA GPU" : "x86 CPU Intel/AMD"} | ${res.arraySize.toLocaleString()} | \`${res.avgTimeMs.toFixed(3)} ms\` | \`${res.medianTimeMs.toFixed(3)} ms\` | ${res.varianceMs.toFixed(6)} |\n`;
                            });
                            navigator.clipboard.writeText(md);
                            triggerCopyNotification("Сравнительная Markdown-таблица успешно скопирована!");
                          }}
                          className="w-full bg-zinc-800 hover:bg-zinc-700 hover:text-white text-zinc-200 text-xs font-semibold py-2 rounded-lg transition text-center"
                        >
                          Скопировать Markdown в буфер
                        </button>
                      </div>

                    </div>
                  </div>

                  {/* Right Column: Live editable reports table */}
                  <div className="lg:col-span-7 flex flex-col gap-4">
                    
                    <div className="bg-zinc-900 border border-zinc-800/85 rounded-xl overflow-hidden shadow-2xl flex flex-col">
                      <div className="bg-zinc-900 px-5 py-3 border-b border-zinc-800 flex justify-between items-center bg-zinc-950/20">
                        <div className="flex items-center gap-2">
                          <span className="w-2.5 h-2.5 rounded-full bg-emerald-500 animate-pulse"></span>
                          <span className="font-semibold text-xs text-white uppercase tracking-wider">Интерактивный редактор отчета</span>
                        </div>
                        <span className="bg-zinc-950 px-2 py-0.5 border border-zinc-850 rounded text-[10px] font-mono text-zinc-500">
                          {results.length} записей
                        </span>
                      </div>

                      <div className="overflow-x-auto">
                        <table className="w-full text-left text-xs text-zinc-300">
                          <thead className="bg-[#18181b] text-zinc-400 text-[10px] font-bold uppercase tracking-wider border-b border-zinc-800">
                            <tr>
                              <th className="px-4 py-3">Алгоритм</th>
                              <th className="px-4 py-3">Размер N</th>
                              <th className="px-4 py-3">Среднее</th>
                              <th className="px-4 py-3">Архитектура</th>
                              <th className="px-4 py-3 text-right">Действия</th>
                            </tr>
                          </thead>
                          <tbody className="divide-y divide-zinc-850 bg-zinc-900/50">
                            {results.map((res, idx) => (
                              <tr key={idx} className="hover:bg-zinc-850/40 transition">
                                <td className="px-4 py-3 font-mono text-xs font-semibold text-white">{res.algorithmName}</td>
                                <td className="px-4 py-3 font-mono text-zinc-400 text-xs">{res.arraySize.toLocaleString()}</td>
                                <td className="px-4 py-3 font-mono text-emerald-400 font-bold">{res.avgTimeMs.toFixed(3)} ms</td>
                                <td className="px-4 py-3">
                                  <span className={`inline-flex items-center gap-1 px-1.5 py-0.5 rounded text-[9px] font-medium border ${
                                    res.isGPU 
                                      ? "bg-emerald-950/40 text-emerald-300 border-emerald-500/10" 
                                      : "bg-blue-950/40 text-blue-300 border-blue-500/10"
                                  }`}>
                                    {res.isGPU ? "CUDA GPU" : "x86 CPU"}
                                  </span>
                                </td>
                                <td className="px-4 py-3 text-right">
                                  <button
                                    onClick={() => {
                                      const updated = [...results];
                                      updated.splice(idx, 1);
                                      setResults(updated);
                                    }}
                                    className="p-1 px-2.5 text-red-400 hover:text-white bg-red-950/20 rounded hover:bg-red-800/80 border border-red-500/10 transition text-[10px]"
                                    title="Удалить из отчета"
                                  >
                                    Удалить
                                  </button>
                                </td>
                              </tr>
                            ))}
                          </tbody>
                        </table>
                      </div>

                      <div className="p-4 bg-zinc-950/40 border-t border-zinc-800 flex justify-between items-center text-[10px] text-zinc-500">
                        <span>* Изменения в списке применяются мгновенно и учитываются при скачивании CSV/JSON отчетов</span>
                        <span className="font-mono">Time: 2026-05-23</span>
                      </div>
                    </div>

                  </div>

                </div>
              </>
            )}

          </div>
        )}

      </main>

      {/* FOOTER */}
      <footer className="border-t border-zinc-900 bg-zinc-950 mt-12 py-6 px-6 text-center text-xs text-zinc-500">
        <div className="max-w-7xl mx-auto flex flex-col sm:flex-row items-center justify-between gap-4">
          <div className="text-left">
            <h4 className="text-sm font-bold font-mono text-zinc-300">SortBench Qt6/CUDA Workspace</h4>
            <p className="text-[11px] text-zinc-500 mt-1">Комбинированный стенд тестирования и архитектурного моделирования аппаратных архитектур.</p>
          </div>
          <div className="text-zinc-500 font-mono text-[10px] text-right">
            <span>Прецизионный таймер: Active 2026</span>
          </div>
        </div>
      </footer>

      {/* FLOATING TOAST REGISTRATION */}
      {copiedText && (
        <div className="fixed bottom-6 right-6 z-50 bg-zinc-950/95 backdrop-blur border border-zinc-850 text-zinc-100 rounded-2xl p-4 flex items-center gap-3.5 shadow-[0_20px_50px_rgba(0,0,0,0.85)] max-w-sm">
          <div className="p-2 bg-blue-600/10 border border-blue-500/20 rounded-xl text-blue-400">
            <Check className="w-5 h-5 stroke-[2.5px]" />
          </div>
          <div>
            <div className="text-xs font-bold text-white uppercase tracking-wider">Успешно</div>
            <div className="text-[11px] text-zinc-400 mt-0.5">{copiedText}</div>
          </div>
        </div>
      )}

    </div>
  );
}
