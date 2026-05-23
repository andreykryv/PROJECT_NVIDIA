cmake_minimum_required(VERSION 3.20)
project(SortBenchQt6 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

qt_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/sortingvisualizer.cpp
    src/benchmarksimulator.cpp
)

set(HEADERS
    src/mainwindow.h
    src/sortingvisualizer.h
    src/benchmarksimulator.h
)

add_executable(SortBenchQt6
    ${SOURCES}
    ${HEADERS}
)

target_link_libraries(SortBenchQt6 PRIVATE
    Qt6::Widgets
    Qt6::Charts
)

target_include_directories(SortBenchQt6 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
