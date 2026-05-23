import express from "express";
import path from "path";
import fs from "fs";
import JSZip from "jszip";
import { createServer as createViteServer } from "vite";

async function startServer() {
  const app = express();
  const PORT = 3000;

  app.use(express.json());

  // API 1: Генерация и скачивание ZIP-архива проекта со всеми исходными кодами C++/CUDA/Qt6
  app.get("/api/download-zip", async (req, res) => {
    try {
      const cppDir = path.join(process.cwd(), "src", "sortbench_cpp");
      
      if (!fs.existsSync(cppDir)) {
        return res.status(404).json({ error: "Каталог с C++ исходниками не найден на сервере." });
      }

      const zip = new JSZip();
      
      // Рекурсивное чтение файлов из каталога /src/sortbench_cpp/
      const addFilesToZip = (dirPath: string, zipFolder: JSZip) => {
        const files = fs.readdirSync(dirPath);
        for (const file of files) {
          const filePath = path.join(dirPath, file);
          const stat = fs.statSync(filePath);
          
          if (stat.isDirectory()) {
            const nestedFolder = zipFolder.folder(file);
            if (nestedFolder) {
              addFilesToZip(filePath, nestedFolder);
            }
          } else {
            const fileContent = fs.readFileSync(filePath);
            zipFolder.file(file, fileContent);
          }
        }
      };

      addFilesToZip(cppDir, zip);

      // Генерируем буфер архива
      const buffer = await zip.generateAsync({ type: "nodebuffer" });

      res.setHeader("Content-Disposition", "attachment; filename=SortBench_CUDA_vs_CPU.zip");
      res.setHeader("Content-Type", "application/zip");
      res.send(buffer);
    } catch (err: any) {
      console.error("Ошибка при создании ZIP-архива:", err);
      res.status(500).json({ error: "Не удалось сформировать ZIP-архив: " + err.message });
    }
  });

  // API 2: Математический симулятор бенчмарков с физически верной точностью
  // Моделирует задержки шины PCI-e, пропускную способность памяти GPU и одноядерные/многоядерные CPU
  app.post("/api/benchmark-simulate", (req, res) => {
    const { arraySize, distribution, algorithms, isDouble, pcieGen, gpuThrottling } = req.body;
    
    const N = Number(arraySize) || 1000;
    const precisionMultiplier = isDouble ? 2.0 : 1.0; // double весит 8 байт, float 4 байта

    const results = [];

    for (const alg of algorithms) {
      let avgTimeMs = 0;
      let minTimeMs = 0;
      let maxTimeMs = 0;
      let medianTimeMs = 0;
      let varianceMs = 0;
      let isGPU = alg.startsWith("GPU_");
      
      // Вспомогательные тайминги GPU
      let uploadTimeMs = 0;
      let kernelTimeMs = 0;
      let downloadTimeMs = 0;

      // Моделируем характеристики производительности
      if (isGPU) {
        // PCIe Gen4 x16 пропускает около 20 Гбайт/с на запись
        // Объем памяти: N * size (8 байт для double или 4 для float)
        const bytesCount = N * (isDouble ? 8 : 4);
        
        // Имитируем пропускную способность шины PCIe в зависимости от выбранного поколения
        let bandwidthGBs = 16.0; 
        let latencyMultiplier = 1.0;
        if (pcieGen === "Gen3") {
          bandwidthGBs = 8.0;
          latencyMultiplier = 1.6;
        } else if (pcieGen === "Gen4") {
          bandwidthGBs = 16.0;
          latencyMultiplier = 1.0;
        } else if (pcieGen === "Gen5") {
          bandwidthGBs = 32.0;
          latencyMultiplier = 0.65;
        }
        
        // PCIe Upload (Host to Device) - латентность старта около 0.2 мс + время передачи данных
        uploadTimeMs = (0.18 * latencyMultiplier) + (bytesCount / (bandwidthGBs * 1e9)) * 1000.0;
        
        // PCIe Download (Device to Host)
        downloadTimeMs = (0.12 * latencyMultiplier) + (bytesCount / (bandwidthGBs * 1e9)) * 1000.0;

        // Моделирование вычислений ядер на GPU
        if (alg === "GPU_Bitonic") {
          // Битоническая сортировка на CUDA выполняется за O(log^2(N)) шагов, имея колоссальный параллелизм.
          // Имитируем RTX 4090: 16384 ядер CUDA
          // Накладные расходы на вызов ядер - около 0.05мс
          kernelTimeMs = 0.04 + (N * Math.log2(N) * Math.log2(N) * 2.5e-10) * precisionMultiplier;
        } else if (alg === "GPU_Radix") {
          // Radix Sort в GPU очень быстрая: O(N) проходов по байтам
          kernelTimeMs = 0.02 + (N * 1.2e-9) * precisionMultiplier;
        } else if (alg === "GPU_OddEven") {
          // Odd-Even сортировка на GPU медленная (занимает O(N) шагов синхронизации)
          kernelTimeMs = 0.1 + (N * N * 1.5e-9);
        } else if (alg === "GPU_QuickSort") {
          // Высокопараллельный блочный QuickSort в CUDA
          kernelTimeMs = 0.035 + (N * Math.log2(N) * 2.8e-10) * precisionMultiplier;
        } else if (alg === "GPU_MergeSort") {
          // Параллельный Merge Sort на GPU с использованием общей памяти (shared memory)
          kernelTimeMs = 0.045 + (N * Math.log2(N) * 3.1e-10) * precisionMultiplier;
        } else if (alg === "GPU_BogoSort") {
          // Параллельный BogoSort на GPU (миллиард потоков генерируют случайные перестановки)
          if (N > 11) {
            kernelTimeMs = 999999.0; // Таймаут TDR
          } else {
            // Факториал N делить на миллиард параллельных потоков в секунду
            let fact = 1;
            for (let i = 2; i <= N; i++) fact *= i;
            kernelTimeMs = 0.1 + (fact * 1e-11) * 1000.0;
          }
        } else {
          // Дефолтный fallback для неизвестных GPU
          kernelTimeMs = 0.05 + (N * Math.log2(N) * 5e-10);
        }

        // Учитываем температурный троттлинг GPU (снижение тактовых частот из-за перегрева)
        if (gpuThrottling) {
          kernelTimeMs *= 2.45;
        }

        avgTimeMs = uploadTimeMs + kernelTimeMs + downloadTimeMs;
      } 
      else {
        // Моделирование вычислений CPU (Intel i9-14900K или Ryzen 7 7800X3D)
        // Частота ~4.8 ГГц, отличный кэш L3, но кэш-промахи на больших N
        const cacheMissRatio = N > 32768 ? Math.log(N / 32768) * 0.15 : 0;
        const penalty = 1.0 + cacheMissRatio;

        if (alg === "CPU_std::sort") {
          // Высокооптимизированный IntroSort на CPU
          avgTimeMs = (N * Math.log2(N) * 1.6e-7) * penalty;
        } else if (alg === "CPU_QuickSort") {
          // Кастомный quicksort с чуть меньшей эффективностью
          avgTimeMs = (N * Math.log2(N) * 2.2e-7) * penalty;
        } else if (alg === "CPU_MergeSort") {
          // Слияние требует O(N) дополнительной памяти, что вызывает кэш-промахи раньше
          avgTimeMs = (N * Math.log2(N) * 2.7e-7) * (penalty * 1.3);
        } else if (alg === "CPU_HeapSort") {
          // Куча провоцирует частые промахи по кэшу из-за скачков во вложенных индексах
          avgTimeMs = (N * Math.log2(N) * 3.2e-7) * (penalty * 1.6);
        } else if (alg === "CPU_TimSort") {
          // Timsort гибридный, эффективен на упорядоченных блоках
          let orderBonus = 1.0;
          if (distribution === "AlmostSorted") orderBonus = 0.3; // Ускорение в несколько раз
          else if (distribution === "AllEqual") orderBonus = 0.05; // Почти моментально
          
          avgTimeMs = (N * Math.log2(N) * 2.1e-7) * penalty * orderBonus;
        } else if (alg === "CPU_BubbleSort") {
          // Пузырьковая сортировка O(N^2)
          let orderBonus = 1.0;
          if (distribution === "AlmostSorted") orderBonus = 0.15;
          else if (distribution === "AllEqual") orderBonus = 0.01;
          avgTimeMs = (N * N * 1.3e-7) * penalty * orderBonus;
        } else if (alg === "CPU_SelectionSort") {
          // Сортировка выбором O(N^2) - всегда квадрат числа сравнений
          avgTimeMs = (N * N * 1.4e-7) * penalty;
        } else if (alg === "CPU_InsertionSort") {
          // Сортировка вставками O(N^2)
          let orderBonus = 1.0;
          if (distribution === "AlmostSorted") orderBonus = 0.05;
          else if (distribution === "AllEqual") orderBonus = 0.005;
          avgTimeMs = (N * N * 0.8e-7) * penalty * orderBonus;
        } else if (alg === "CPU_ShellSort") {
          // Сортировка Шелла O(N log^2 N)
          avgTimeMs = (N * Math.log2(N) * Math.log2(N) * 1.8e-7) * penalty;
        } else if (alg === "CPU_CocktailSort") {
          // Шейкерная сортировка O(N^2)
          let orderBonus = 1.0;
          if (distribution === "AlmostSorted") orderBonus = 0.2;
          avgTimeMs = (N * N * 1.4e-7) * penalty * orderBonus;
        } else if (alg === "CPU_GnomeSort") {
          // Гномья сортировка O(N^2)
          let orderBonus = 1.0;
          if (distribution === "AlmostSorted") orderBonus = 0.1;
          avgTimeMs = (N * N * 1.6e-7) * penalty * orderBonus;
        } else if (alg === "CPU_CombSort") {
          // Сортировка расческой O(N log N)
          avgTimeMs = (N * Math.log2(N) * 2.9e-7) * penalty;
        } else if (alg === "CPU_RadixSortLSD") {
          // Поразрядная LSD O(N)
          avgTimeMs = (N * 8 * 1.2e-7) * penalty;
        } else if (alg === "CPU_CountingSort") {
          // Сортировка подсчетом O(N + K)
          avgTimeMs = (N * 1.8e-7 + 10000 * 1.5e-7) * penalty;
        } else if (alg === "CPU_BucketSort") {
          // Блочная сортировка O(N)
          avgTimeMs = (N * 2.6e-7) * penalty;
        } else if (alg === "CPU_PancakeSort") {
          // Блинная сортировка O(N^2)
          avgTimeMs = (N * N * 1.9e-7) * penalty;
        } else if (alg === "CPU_BogoSort") {
          // Богосорт O(N * N!)
          if (N > 11) {
            avgTimeMs = 3600000.0 * 24; // 24 часа (или бесконечность)
          } else {
            let fact = 1;
            for (let i = 2; i <= N; i++) fact *= i;
            avgTimeMs = (fact * 1.1e-7) * penalty;
          }
        } else if (alg === "CPU_StoogeSort") {
          // Сортировка Студжа O(N^2.7)
          avgTimeMs = (Math.pow(N, 2.709) * 2.2e-8) * penalty;
        } else if (alg === "CPU_OddEvenSort") {
          // Чет-нечетная сортировка CPU O(N^2)
          avgTimeMs = (N * N * 1.5e-7) * penalty;
        } else if (alg === "CPU_CycleSort") {
          // Циклическая сортировка O(N^2)
          avgTimeMs = (N * N * 1.7e-7) * penalty;
        } else {
          avgTimeMs = (N * Math.log2(N) * 2.5e-7) * penalty;
        }
      }

      // Добавим стохастический гауссов шум для имитации реальной погрешности операционной системы
      const noiseSigma = avgTimeMs * 0.04 + 0.005; 
      const noise = (Math.random() - 0.5) * 2 * noiseSigma;
      avgTimeMs = Math.max(0.0001, avgTimeMs + noise);

      minTimeMs = avgTimeMs * 0.92;
      maxTimeMs = avgTimeMs * 1.14;
      medianTimeMs = avgTimeMs + (Math.random() - 0.5) * noiseSigma * 0.3;
      varianceMs = noiseSigma * noiseSigma;

      results.push({
        algorithmName: alg,
        isGPU,
        arraySize: N,
        avgTimeMs,
        minTimeMs,
        maxTimeMs,
        medianTimeMs,
        varianceMs,
        avgUploadTimeMs: isGPU ? uploadTimeMs : 0,
        avgKernelTimeMs: isGPU ? kernelTimeMs : 0,
        avgDownloadTimeMs: isGPU ? downloadTimeMs : 0,
        success: true,
        errorMsg: ""
      });
    }

    res.json({ results });
  });

  // Настройка Vite middleware в режиме разработки
  if (process.env.NODE_ENV !== "production") {
    const vite = await createViteServer({
      server: { middlewareMode: true },
      appType: "spa",
    });
    app.use(vite.middlewares);
  } else {
    // В продакшене отдаем готовые файлы из папки /dist
    const distPath = path.join(process.cwd(), "dist");
    app.use(express.static(distPath));
    app.get("*", (req, res) => {
      res.sendFile(path.join(distPath, "index.html"));
    });
  }

  app.listen(PORT, "0.0.0.0", () => {
    console.log(`Server SortBench up and running on http://0.0.0.0:${PORT}`);
  });
}

startServer();
