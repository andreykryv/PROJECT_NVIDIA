cmake_minimum_required(VERSION 3.20)
project(SortBench LANGUAGES CXX CUDA)

# Настройка стандартов C++ и CUDA
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
# Явный поиск CUDA Toolkit (создаст цель CUDA::cudart)
find_package(CUDAToolkit REQUIRED)

# Оптимальные флаги оптимизации и архитектуры CUDA
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -arch=sm_86 -O3 --use_fast_math")

# Включение автоматической генерации MOC/UIC/RCC для Qt6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Поиск необходимых модулей Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

# Объявление исполняемого файла со всеми исходными исходниками
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
    settings_dialog.h
    settings_dialog.cpp
)

# Настройка линковки
target_link_libraries(SortBench PRIVATE
    Qt6::Widgets
    Qt6::Charts
    CUDA::cudart
)

# Определение инклудов
target_include_directories(SortBench PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
)