################################################################################
# CMakeLists.txt — корневой файл системы сборки проекта SortBench CUDA vs CPU
#
# НАЗНАЧЕНИЕ:
#   Главный конфигурационный файл CMake. Задаёт версию CMake, имя проекта,
#   стандарты C++ и CUDA, настраивает путь к Qt6 и CUDA Toolkit,
#   подключает все необходимые пакеты и добавляет дочерние подпроекты.
#
# СОДЕРЖИМОЕ:
#
#   cmake_minimum_required(VERSION 3.24)
#     — Минимальная версия CMake, необходимая для полноценной поддержки
#       языков CUDA и интеграции с современными версиями Qt6.
#
#   project(SortBench VERSION 1.0.0 LANGUAGES CXX CUDA)
#     — Имя проекта и явное включение языков C++ и CUDA.
#       Это заставляет CMake искать и настраивать nvcc (компилятор CUDA).
#
#   CMAKE_CXX_STANDARD 20
#     — Используем C++20 для поддержки концептов, std::span, structured
#       bindings, ranges и других современных возможностей языка.
#
#   CMAKE_CUDA_STANDARD 17
#     — Стандарт CUDA C++. CUDA 17 обеспечивает поддержку лямбд в device-коде,
#       constexpr if, structured bindings в ядрах GPU.
#
#   CMAKE_CUDA_ARCHITECTURES "75;86;89"
#     — Целевые архитектуры GPU: Turing (RTX 20xx), Ampere (RTX 30xx),
#       Ada Lovelace (RTX 40xx). Можно расширить под конкретный стенд.
#
#   find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Concurrent)
#     — Подключение Qt6: виджеты GUI, модуль QtCharts для графиков,
#       QtConcurrent для асинхронных задач без ручного управления потоками.
#
#   find_package(CUDAToolkit REQUIRED)
#     — Поиск CUDA Toolkit: заголовки, библиотеки (cublas, cufft, thrust).
#       Без него CUDA-ядра не скомпилируются.
#
#   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#     — Все исполняемые файлы собираются в единый каталог bin/.
#
#   add_subdirectory(src)
#     — Включение дочернего CMakeLists.txt из каталога src/.
#
#   Опции сборки:
#     SORTBENCH_ENABLE_PROFILING  — включает профилировщик CUDA Nsight
#     SORTBENCH_DEBUG_ANIMATION   — подробное логирование анимационных кадров
#     SORTBENCH_MAX_ARRAY_SIZE    — максимальный поддерживаемый размер массива
#                                   (по умолчанию 100 000 000 элементов)
#
#   Настройки оптимизации:
#     Release-сборка: -O3 для CPU, --use_fast_math и -O3 для nvcc.
#     Debug-сборка: -g -G для CUDA (device-отладка через cuda-gdb).
#
#   CPack-конфигурация:
#     Задаёт параметры для генерации инсталляторов под Windows (NSIS),
#     Linux (DEB/RPM) и macOS (DMG) командой cmake --build . --target package.
################################################################################
