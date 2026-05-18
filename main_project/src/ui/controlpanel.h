////////////////////////////////////////////////////////////////////////////////
// ui/controlpanel.h — заголовок панели управления тестами
//
// НАЗНАЧЕНИЕ:
//   ControlPanel — левая панель приложения с элементами выбора алгоритмов,
//   настройки параметров массива и кнопками управления запуском теста.
//   Это главный инструмент взаимодействия пользователя с приложением.
//
// КЛАСС: ControlPanel : public QWidget
//
//   СЕКЦИЯ "Алгоритм":
//     QGroupBox "Алгоритм сортировки"
//       — QComboBox *cpuAlgorithmCombo   : выбор CPU-алгоритма
//           Пункты: "Bubble Sort", "Quick Sort", "Merge Sort", "Heap Sort",
//                   "Radix Sort (LSD)", "std::sort (intro sort)"
//       — QComboBox *gpuAlgorithmCombo   : выбор GPU-алгоритма
//           Пункты: "Bitonic Sort", "Thrust Radix Sort", "GPU Quick Sort",
//                   "CUB Device Sort", "Отключён"
//       — QCheckBox *enableCPU           : включить/выключить CPU-ветку
//       — QCheckBox *enableGPU           : включить/выключить GPU-ветку
//       — QPushButton *algoInfoBtn       : кнопка "?" — показывает popup
//                                          с описанием алгоритма и его сложностью
//
//   СЕКЦИЯ "Массив":
//     QGroupBox "Параметры массива"
//       — QSpinBox *arraySizeSpinBox     : размер массива (1 000 – 100 000 000)
//       — QSlider *arraySizeSlider       : логарифмический слайдер (1K – 100M)
//                                          синхронизирован с arraySizeSpinBox
//       — QComboBox *dataTypeCombo       : тип данных
//           Пункты: "int32", "int64", "float", "double"
//       — QComboBox *distributionCombo   : начальное распределение элементов
//           Пункты: "Случайное равномерное", "Почти отсортированное",
//                   "Обратный порядок", "Много повторов", "Пилообразное",
//                   "Шагающий шум", "Случайное нормальное"
//       — QSpinBox *randomSeedSpinBox    : зерно генератора (для воспроизводимости)
//       — QCheckBox *autoSeedCheck       : автоматическое зерно (random_device)
//
//   СЕКЦИЯ "Анимация":
//     QGroupBox "Настройки анимации"
//       — QSlider *animSpeedSlider       : скорость (0=пошаговая, 100=максимальная)
//       — QLabel  *animSpeedLabel        : текстовое значение скорости
//       — QCheckBox *showComparisonsCheck : подсвечивать сравниваемые элементы
//       — QCheckBox *showAccessCountCheck : показывать счётчик обращений к массиву
//       — QComboBox *colorSchemeCombo    : цветовая схема столбцов
//           Пункты: "Радужная", "Тепловая карта", "Монохром", "Статус-цвета"
//       — QSpinBox *maxVisElementsSpin   : макс. число отображаемых столбцов
//                                          (при большом массиве — прореживание)
//
//   СЕКЦИЯ "Серийное тестирование":
//     QGroupBox "Авто-серия"
//       — QCheckBox *batchModeCheck      : режим серии тестов
//       — QPushButton *configureBatchBtn : открыть диалог настройки серии
//       — QLabel *batchStatusLabel       : "0 / N тестов выполнено"
//
//   КНОПКИ УПРАВЛЕНИЯ:
//     QPushButton *runBtn               : "▶ Запустить" (зелёная, F5)
//     QPushButton *stopBtn              : "■ Стоп" (красная, Esc, disabled)
//     QPushButton *pauseResumeBtn       : "⏸ Пауза" (жёлтая, Пробел, disabled)
//     QPushButton *resetBtn             : "↺ Сброс" (серая)
//
//   СИГНАЛЫ:
//     parametersChanged(SortParams)     — эмитируется при любом изменении параметров
//     runRequested()                    — нажатие "Запустить"
//     stopRequested()                   — нажатие "Стоп"
//     pauseResumeRequested()            — нажатие "Пауза/Возобновить"
//     resetRequested()                  — нажатие "Сброс"
//     animationSpeedChanged(int)        — изменение скорости анимации
//
//   МЕТОДЫ:
//     SortParams getCurrentParams() const — возвращает текущие параметры
//     void setRunning(bool)              — блокирует/разблокирует элементы UI
//     void setPaused(bool)               — меняет текст кнопки пауза/возобновить
//     void updateBatchProgress(int, int) — обновляет batchStatusLabel
//     void loadFromSettings()            — восстанавливает значения из QSettings
//     void saveToSettings() const        — сохраняет значения в QSettings
////////////////////////////////////////////////////////////////////////////////
