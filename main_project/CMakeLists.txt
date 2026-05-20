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

cmake_minimum_required(VERSION 3.24)

# Сначала проверяем опции до объявления проекта
option(ENABLE_CUDA "Включить поддержку CUDA" OFF)

if(ENABLE_CUDA)
    project(SortBench VERSION 1.0.0 LANGUAGES CXX CUDA)
else()
    project(SortBench VERSION 1.0.0 LANGUAGES CXX)
endif()

# Стандарты языков
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)

# Целевые архитектуры GPU
set(CMAKE_CUDA_ARCHITECTURES "75;86;89")

# Опции сборки
option(SORTBENCH_ENABLE_PROFILING "Включить профилировщик CUDA Nsight" OFF)
option(SORTBENCH_DEBUG_ANIMATION "Подробное логирование анимационных кадров" OFF)
set(SORTBENCH_MAX_ARRAY_SIZE 100000000 CACHE STRING "Максимальный размер массива")

# Поиск Qt (предпочтение Qt6, затем Qt5)
set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/cmake;/usr")
find_package(Qt6 COMPONENTS Widgets Charts Concurrent Svg REQUIRED)

# Поиск CUDA Toolkit (только если включена поддержка CUDA)
if(ENABLE_CUDA)
    find_package(CUDAToolkit REQUIRED)
endif()

# Выходной каталог для исполняемых файлов
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Добавление подпроекта src
add_subdirectory(src)

# Настройки оптимизации для Release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-O3>
        $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math;-O3>
    )
endif()

# Настройки отладки для Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:-g;-G>
    )
endif()
# Разрешаем constexpr __host__ функции в __global__ функциях CUDA
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>
)
# Конфигурация CPack для создания инсталляторов
set(CPACK_PACKAGE_NAME "SortBench")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Benchmark CUDA vs CPU Sorting Algorithms")
set(CPACK_PACKAGE_VENDOR "SortBench Team")
set(CPACK_PACKAGE_CONTACT "sortbench@example.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# Генераторы для разных платформ
if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_MODIFY_PATH ON)
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base-dev, qt6-charts-dev")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase-devel, qt6-qtcharts-devel")
endif()

include(CPack)
