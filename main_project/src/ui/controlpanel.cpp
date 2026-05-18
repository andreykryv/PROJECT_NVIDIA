////////////////////////////////////////////////////////////////////////////////
// ui/controlpanel.cpp — реализация панели управления
//
// НАЗНАЧЕНИЕ:
//   Реализует логику ControlPanel: построение layout, обработка изменений
//   элементов управления, синхронизация слайдера с SpinBox, формирование
//   объекта SortParams и его эмитирование через сигнал parametersChanged.
//
// ПОСТРОЕНИЕ ИНТЕРФЕЙСА (конструктор):
//   — QVBoxLayout с тремя QGroupBox-секциями и блоком кнопок внизу.
//   — Между секциями добавляются QFrame::HLine разделители.
//   — Для каждого QGroupBox создаётся collapsible-поведение: клик по заголовку
//     сворачивает/разворачивает содержимое (анимировано через QPropertyAnimation
//     на maximumHeight), что экономит место при маленьком окне.
//   — arraySizeSlider использует логарифмическую шкалу: значение слайдера N
//     преобразуется в реальный размер как pow(10, 3 + N/100 * 5) — от 1 000
//     до 100 000 000. Это обеспечивает удобный охват 5 порядков величины.
//   — SpinBox и Slider синхронизированы двусторонне через блокировку сигналов
//     во избежание рекурсивных вызовов.
//
// ЛОГИКА алгоритма "?" tooltip:
//   — При нажатии algoInfoBtn определяется текущий выбор combo.
//   — Из AlgorithmRegistry::instance().getInfo(name) запрашивается структура
//     AlgorithmInfo { name, timeComplexity, spaceComplexity, description,
//                     stable, parallelizable }.
//   — Показывается QToolTip::showText() или кастомный QDialog с таблицей
//     O-нотации и текстовым описанием особенностей алгоритма.
//
// СИНХРОНИЗАЦИЯ СОСТОЯНИЯ:
//   — При setRunning(true): все комбобоксы, спинбоксы и слайдеры отключаются
//     (setEnabled(false)). Кнопка Run недоступна, Stop доступна.
//   — При setRunning(false): всё возвращается в рабочее состояние.
//   — При setPaused(true): текст pauseResumeBtn меняется на "▶ Продолжить".
//
// ФОРМИРОВАНИЕ SortParams:
//   — При каждом изменении любого контрола вызывается buildParams(),
//     который собирает все значения в структуру SortParams и эмитирует
//     сигнал parametersChanged(params).
//   — SortParams содержит: cpuAlgorithm, gpuAlgorithm, arraySize, dataType,
//     distribution, randomSeed, enableCPU, enableGPU, animationSpeed,
//     showComparisons, colorScheme, maxVisElements.
//
// БЛОКИРОВКА GPU-контролов:
//   — Если при запуске приложения CudaDeviceInfo::deviceCount() == 0,
//     все GPU-элементы управления блокируются и показывается подсказка
//     "CUDA недоступна на этом устройстве".
//   — gpuAlgorithmCombo.setCurrentIndex(4) — "Отключён" по умолчанию.
//
// СОХРАНЕНИЕ/ВОССТАНОВЛЕНИЕ:
//   — loadFromSettings(): QSettings -> setCurrentIndex/setValue для всех контролов.
//   — saveToSettings(): обратный процесс. Вызывается из destructor и при closeEvent.
////////////////////////////////////////////////////////////////////////////////
