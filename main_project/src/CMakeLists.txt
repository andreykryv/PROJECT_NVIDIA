################################################################################
# src/CMakeLists.txt — CMake-файл исходного кода приложения
#
# НАЗНАЧЕНИЕ:
#   Собирает все исходные файлы проекта в единый исполняемый файл SortBench.
#   Здесь описываются: список источников, связывание библиотек, правила для
#   Qt6 (автоматический MOC/UIC/RCC), интеграция CUDA-файлов с nvcc.
#
# СОДЕРЖИМОЕ:
#
#   Список исходных файлов (переменная SORTBENCH_SOURCES):
#     — все .cpp файлы из подкаталогов ui/, core/, cpu/, gpu/, visualization/,
#       charts/, utils/, models/
#     — отдельный список CUDA_SOURCES для .cu файлов (компилируются nvcc)
#
#   set_source_files_properties(*.cu PROPERTIES LANGUAGE CUDA)
#     — Явно указываем CMake, что .cu файлы — это CUDA-источники, а не C++.
#       Иначе g++ попытается их скомпилировать и выдаст ошибки.
#
#   add_executable(SortBench WIN32 ...)
#     — WIN32 убирает консольное окно под Windows при Release-сборке.
#       В Debug-режиме консоль оставляется для вывода диагностики.
#
#   Свойства CUDA:
#     CUDA_SEPARABLE_COMPILATION ON  — включает раздельную компиляцию .cu файлов,
#       необходимую для вызовов device-функций из разных единиц трансляции.
#     CUDA_RESOLVE_DEVICE_SYMBOLS ON — финальная линковка device-символов.
#
#   target_link_libraries(SortBench PRIVATE ...):
#     Qt6::Widgets       — основные виджеты Qt
#     Qt6::Charts        — модуль QtCharts для графиков
#     Qt6::Concurrent    — асинхронные задачи
#     CUDA::cudart       — runtime CUDA
#     CUDA::cuda_driver  — низкоуровневый CUDA Driver API
#     CUDA::cublas       — CUBLAS (опционально, для будущих GPU-операций)
#     CUDA::cufft        — cuFFT (не используется напрямую, для расширений)
#
#   Thrust подключается через заголовки (header-only), явная линковка не нужна.
#
#   qt_add_resources(SortBench "resources" ...)
#     — Компилирует файл resources/resources.qrc в .cpp и встраивает ресурсы
#       (иконки, QSS-стили, шрифты) прямо в исполняемый файл.
#
#   target_include_directories:
#     — Добавляет все подкаталоги src/ в пути поиска заголовков, чтобы
#       #include "core/sortparams.h" работал из любого файла проекта.
#
#   Автоматические правила Qt:
#     set_target_properties(SortBench PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
#     — MOC обрабатывает макросы Q_OBJECT, UIC генерирует ui_*.h из .ui файлов,
#       RCC компилирует ресурсы. Всё это происходит автоматически при сборке.
#
#   Установка (install):
#     — install(TARGETS SortBench DESTINATION bin) — целевой каталог
#     — install(FILES ... DESTINATION share/sortbench) — документация и примеры
#     — На Windows дополнительно вызывается windeployqt для сборки DLL Qt.
################################################################################

# Список всех CPP файлов
set(SORTBENCH_SOURCES
    # Main
    main.cpp
    mainwindow.cpp

    # Core
    core/algorithmregistry.cpp
    core/arraygenerator.cpp
    core/benchmarkresult.cpp
    core/benchmarkrunner.cpp
    core/sortbenchengine.cpp

    # CPU сортировки
    cpu/cpusorter.cpp
    cpu/bubblesort.cpp
    cpu/quicksort.cpp
    cpu/mergesort.cpp
    cpu/heapsort.cpp
    cpu/radixsort.cpp
    cpu/stdsort.cpp

    # Charts
    charts/benchmarkchartview.cpp
    charts/comparisonbarchart.cpp
    charts/scatterplotchart.cpp
    charts/timelinechart.cpp

    # Models
    models/algorithmsmodel.cpp
    models/resultstablemodel.cpp

    # UI
    ui/aboutdialog.cpp
    ui/chartwidget.cpp
    ui/controlpanel.cpp
    ui/progresspanel.cpp
    ui/settingsdialog.cpp
    ui/statspanel.cpp
    ui/visualizationwidget.cpp

    # Utils
    utils/csvexporter.cpp
    utils/logger.cpp
    utils/settingsmanager.cpp
    utils/threadpool.cpp

    # Visualization
    visualization/animationcontroller.cpp
    visualization/baranimation.cpp
    visualization/colorscheme.cpp
    visualization/sortvisualizer.cpp
)

# GPU файлы добавляются только если включена поддержка CUDA
if(ENABLE_CUDA)
    list(APPEND SORTBENCH_SOURCES
        # GPU сортировки
        gpu/cudasorter.cpp
        gpu/cudamemorymanager.cpp

        # CUDA файлы
        gpu/bitonicsort.cu
        gpu/cudadeviceinfo.cu
        gpu/cudaquicksort.cu
        gpu/cudautils.cu
        gpu/thrustradixsort.cu
    )
endif()

# Список заголовочных файлов
set(SORTBENCH_HEADERS
    mainwindow.h

    core/algorithmregistry.h
    core/arraygenerator.h
    core/benchmarkresult.h
    core/benchmarkrunner.h
    core/sortbenchengine.h
    core/sortparams.h

    cpu/cpusorter.h
    cpu/bubblesort.h
    cpu/quicksort.h
    cpu/mergesort.h
    cpu/heapsort.h
    cpu/radixsort.h
    cpu/stdsort.h

    charts/benchmarkchartview.h
    charts/comparisonbarchart.h
    charts/scatterplotchart.h
    charts/timelinechart.h

    models/algorithmsmodel.h
    models/resultstablemodel.h

    ui/aboutdialog.h
    ui/chartwidget.h
    ui/controlpanel.h
    ui/progresspanel.h
    ui/settingsdialog.h
    ui/statspanel.h
    ui/visualizationwidget.h

    utils/csvexporter.h
    utils/logger.h
    utils/settingsmanager.h
    utils/threadpool.h

    visualization/animationcontroller.h
    visualization/baranimation.h
    visualization/colorscheme.h
    visualization/sortvisualizer.h
)

# GPU заголовки добавляются только если включена поддержка CUDA
if(ENABLE_CUDA)
    list(APPEND SORTBENCH_HEADERS
        gpu/cudasorter.h
        gpu/cudamemorymanager.h
        gpu/bitonicsort.cuh
        gpu/cudadeviceinfo.cuh
        gpu/cudadeviceinfo.h
        gpu/cudaquicksort.cuh
        gpu/cudautils.cuh
        gpu/thrustradixsort.cuh
    )
endif()

# Указываем CMake, что .cu файлы — это CUDA (только если ENABLE_CUDA)
if(ENABLE_CUDA)
    set_source_files_properties(
        gpu/bitonicsort.cu
        gpu/cudadeviceinfo.cu
        gpu/cudaquicksort.cu
        gpu/cudautils.cu
        gpu/thrustradixsort.cu
        PROPERTIES LANGUAGE CUDA
    )
endif()

# Создание исполняемого файла
if(WIN32)
    add_executable(SortBench WIN32 ${SORTBENCH_SOURCES} ${SORTBENCH_HEADERS})
else()
    add_executable(SortBench ${SORTBENCH_SOURCES} ${SORTBENCH_HEADERS})
endif()

# Настройки CUDA для целевого файла
set_target_properties(SortBench PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
)

# Подключение библиотек
if(Qt6_FOUND)
    target_link_libraries(SortBench PRIVATE
        Qt6::Widgets
        Qt6::Charts
        Qt6::Concurrent
        Qt6::Svg
    )
else()
    target_link_libraries(SortBench PRIVATE
        Qt5::Widgets
        Qt5::Charts
        Qt5::Concurrent
        Qt5::Svg
    )
endif()

if(ENABLE_CUDA)
    target_link_libraries(SortBench PRIVATE
        CUDA::cudart
        CUDA::cuda_driver
        CUDA::cublas
        CUDA::cufft
    )
endif()

# Добавление ресурсов Qt
if(TARGET Qt6::Core)
    qt_add_resources(SortBench "resources"
        PREFIX "/"
        FILES
            ../resources/resources.qrc
            ../resources/styles/darktheme.qss
            ../resources/styles/lighttheme.qss
    )
else()
    qt5_add_resources(SortBench_SRC
        ../resources/resources.qrc
        ../resources/styles/darktheme.qss
        ../resources/styles/lighttheme.qss
    )
    target_sources(SortBench PRIVATE ${SortBench_SRC})
endif()

# Пути поиска заголовков
target_include_directories(SortBench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/cpu
    ${CMAKE_CURRENT_SOURCE_DIR}/gpu
    ${CMAKE_CURRENT_SOURCE_DIR}/charts
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/visualization
)

# Автоматические правила Qt (MOC, UIC, RCC)
set_target_properties(SortBench PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Определение макросов компиляции
target_compile_definitions(SortBench PRIVATE
    SORTBENCH_VERSION="${PROJECT_VERSION}"
    QT_USE_QSTRINGBUILDER
    $<$<CONFIG:Debug>:SORTBENCH_DEBUG>
    $<$<BOOL:${SORTBENCH_ENABLE_PROFILING}>:SORTBENCH_PROFILING_ENABLED>
    $<$<BOOL:${SORTBENCH_DEBUG_ANIMATION}>:SORTBENCH_DEBUG_ANIMATION_ENABLED>
    SORTBENCH_MAX_ARRAY_SIZE=${SORTBENCH_MAX_ARRAY_SIZE}
    $<$<BOOL:${ENABLE_CUDA}>:USE_CUDA>
)

# Правила установки
install(TARGETS SortBench
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Установка дополнительных файлов
install(FILES
    ../README.md
    ../LICENSE
    DESTINATION share/sortbench
)

# Windows-specific: windeployqt
if(WIN32)
    install(CODE [[
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES $<TARGET_FILE:SortBench>
            RESOLVED_DEPENDENCIES_VAR _r_deps
            UNRESOLVED_DEPENDENCIES_VAR _u_deps
            DIRECTORIES
        )
        foreach(_file ${_r_deps})
            file(INSTALL
                DESTINATION "${CMAKE_INSTALL_PREFIX}/bin"
                TYPE SHARED_LIBRARY
                FOLLOW_SYMLINK_CHAIN
                FILES "${_file}"
            )
        endforeach()
        list(LENGTH _u_deps _u_len)
        if("${_u_len}" GREATER 0)
            message(WARNING "Unresolved dependencies: ${_u_deps}")
        endif()
    ]])
endif()
