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
