////////////////////////////////////////////////////////////////////////////////
// ui/visualizationwidget.h — заголовок виджета анимации сортировки
//
// НАЗНАЧЕНИЕ:
//   VisualizationWidget — центральный виджет для визуального отображения
//   процесса сортировки в реальном времени. Рисует столбчатую диаграмму,
//   где каждый столбец = один элемент массива, высота = значение элемента.
//   Подсвечивает сравниваемые, переставляемые и уже отсортированные элементы.
//
// КЛАСС: VisualizationWidget : public QOpenGLWidget (или QWidget с QPainter)
//
//   РЕЖИМ РЕНДЕРИНГА:
//     — Основной: QOpenGLWidget для аппаратного ускорения при большом числе
//       столбцов (> 10 000). Использует OpenGL vertex buffers.
//     — Fallback: QPainter на QPixmap при отсутствии OpenGL или < 1000 элементов.
//       QPainter достаточно быстр и даёт лучшее качество субпиксельного
//       рендеринга при малом числе столбцов.
//     — Автоматическое переключение режима по количеству элементов.
//
//   СТРУКТУРА VisFrame (описание одного кадра анимации):
//     std::vector<float>  values          — нормализованные значения [0..1]
//     std::vector<int>    highlightedIdx  — индексы подсвеченных элементов
//     HighlightType       highlightType   — COMPARE/SWAP/PIVOT/SORTED/WRITE
//     int                 pivotIndex      — индекс опорного элемента (для QuickSort)
//     int                 sortedBoundary  — граница уже отсортированной части
//     long long           comparisons     — счётчик сравнений
//     long long           swaps           — счётчик перестановок
//     long long           arrayAccesses   — счётчик обращений к массиву
//     QString             algoName        — имя текущего алгоритма
//     bool                isGPU           — кадр от GPU-алгоритма?
//
//   ЧЛЕНЫ-ДАННЫЕ:
//     VisFrame currentFrame              — текущий отображаемый кадр
//     VisFrame nextFrame                 — следующий кадр (интерполяция)
//     float interpolationT               — прогресс интерполяции [0..1]
//     QTimer *animTimer                  — таймер перехода между кадрами
//     ColorScheme *colorScheme           — текущая цветовая схема
//     int animationSpeedFPS              — целевой FPS анимации
//     bool isPaused                      — флаг паузы
//     QQueue<VisFrame> frameQueue        — очередь кадров от алгоритма
//     std::atomic<bool> isReceivingFrames — алгоритм ещё генерирует кадры?
//     QLabel *overlayStatsLabel          — overlay-метки: сравнения, перестановки
//     QPushButton *stepButton            — кнопка "Один шаг" в пошаговом режиме
//     int maxColumns                     — макс. число столбцов на экране
//     bool isStepMode                    — пошаговый режим (animSpeed == 0)
//
//   СЛОТЫ:
//     renderFrame(VisFrame)              — добавляет кадр в очередь
//     setAnimationSpeed(int fps)         — устанавливает целевой FPS
//     pause()                            — ставит анимацию на паузу
//     resume()                           — возобновляет анимацию
//     reset()                            — очищает очередь и сбрасывает состояние
//     setColorScheme(ColorScheme *)      — смена цветовой схемы без перезапуска
//     stepForward()                      — один шаг в пошаговом режиме
//
//   МЕТОДЫ РЕНДЕРИНГА:
//     paintEvent(QPaintEvent *)          — основной метод отрисовки (QPainter)
//     initializeGL()                     — инициализация OpenGL (если QOpenGLWidget)
//     paintGL()                          — отрисовка OpenGL-кадра
//     drawBars(QPainter &, VisFrame &)   — отрисовка столбцов
//     drawHighlights(QPainter &, VisFrame &) — цветные подсветки
//     drawOverlay(QPainter &)            — наложение статистики и подписей
//     drawGradientBackground(QPainter &) — фоновый градиент
//     interpolateFrames()                — плавный переход между кадрами
//     calculateBarWidth()                — адаптивная ширина столбца
//
//   СИГНАЛЫ:
//     frameRendered(int frameIndex)      — кадр отрисован
//     queueEmpty()                       — очередь кадров исчерпана
//     fpsUpdated(int actualFPS)          — реально достигнутый FPS
//
//   ИНТЕРАКТИВНОСТЬ:
//     — Наведение мыши на столбец показывает tooltip: индекс, значение,
//       позиция в текущем массиве.
//     — Клик правой кнопкой — контекстное меню: "Сохранить кадр как PNG",
//       "Копировать в буфер обмена", "Показать в отдельном окне".
//     — Колёсико мыши — масштабирование (zoom-in/out по горизонтали).
//     — Перетаскивание — прокрутка при zoom > 1.
////////////////////////////////////////////////////////////////////////////////
