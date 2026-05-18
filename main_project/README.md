# SortBench — CUDA vs CPU Sorting Benchmark

<!--
  README.md — документация проекта SortBench CUDA vs CPU
  
  СОДЕРЖИМОЕ ЭТОГО ФАЙЛА:
  
  ## Описание
  Интерактивное Qt6-приложение для сравнения производительности алгоритмов
  сортировки на CPU (C++) и GPU (CUDA). Визуализирует процесс сортировки
  в реальном времени и строит сравнительные графики.

  ## Возможности
  - 6 CPU-алгоритмов: Bubble, Quick (3-way), Merge, Heap, Radix (LSD), std::sort
  - 4 GPU-алгоритма: Bitonic Sort, Thrust Radix Sort, GPU QuickSort, CUB DeviceSort
  - Типы данных: int32, int64, float, double
  - Распределения: случайное, почти отсортированное, обратное, много дубликатов,
    пилообразное, ступенчатый шум, нормальное
  - Анимация сортировки с подсветкой сравнений/перестановок
  - Пошаговый режим и регулировка скорости анимации (0–144 FPS)
  - 4 цветовые схемы: Rainbow, Heatmap, Monochrome, Status Colors
  - Spring-physics анимация столбцов
  - Детальные метрики GPU: H2D / kernel / D2H / sync overhead
  - QtCharts-графики: столбчатый, scatter, ускорение, детали GPU
  - Серийное тестирование (batch mode) по матрице параметров
  - Экспорт результатов в CSV, JSON, Markdown
  - Тёмная и светлая темы (Catppuccin Mocha / Material Light)

  ## Требования
  - CMake >= 3.24
  - Qt 6.5+ (Widgets, Charts, Concurrent)
  - CUDA Toolkit 12.0+ (nvcc, Thrust, CUB)
  - GPU: NVIDIA с Compute Capability >= 7.5 (Turing+)
  - Компилятор: GCC 11+ / MSVC 2022 / Clang 14+
  - C++20

  ## Сборка
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --parallel
    ./bin/SortBench

  ## Сборка без GPU (CPU-only режим)
    cmake .. -DSORTBENCH_ENABLE_CUDA=OFF

  ## Структура проекта
    CMakeLists.txt          — корневой CMake
    src/
      main.cpp              — точка входа
      mainwindow.*          — главное окно
      ui/                   — виджеты панелей управления
      core/                 — движок, параметры, результаты
      cpu/                  — CPU-алгоритмы
      gpu/                  — CUDA-алгоритмы (.cu/.cuh)
      visualization/        — анимация, цветовые схемы
      charts/               — QtCharts-графики
      utils/                — логгер, экспорт, настройки
      models/               — Qt-модели данных
    resources/
      styles/               — QSS-темы
      icons/                — иконки приложения

  ## Горячие клавиши
    F5         — Запустить тест
    Esc        — Остановить тест
    Пробел     — Пауза / Возобновить анимацию
    →          — Один шаг (в пошаговом режиме)
    Ctrl+E     — Экспорт CSV
    Ctrl+S     — Сохранить график
    Ctrl+,     — Настройки
    F11        — Полный экран
    Ctrl+D     — Переключить тему

  ## Лицензия
  MIT License. Copyright (c) 2024 CUDA Lab.
-->
