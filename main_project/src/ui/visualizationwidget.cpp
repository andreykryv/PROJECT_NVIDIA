////////////////////////////////////////////////////////////////////////////////
// ui/visualizationwidget.cpp — реализация виджета анимации сортировки
//
// НАЗНАЧЕНИЕ:
//   Содержит реализацию VisualizationWidget: инициализация QPainter/OpenGL,
//   логика очереди кадров, интерполяция, рендеринг столбцов и оверлея.
//
// КЛЮЧЕВЫЕ ДЕТАЛИ РЕАЛИЗАЦИИ:
//
//   Конструктор:
//     — Устанавливает политику размера: Expanding по обоим осям.
//     — Создаёт animTimer (QTimer) с интервалом 1000/fps мс.
//     — Подключает animTimer::timeout → processNextFrame().
//     — Включает mouse tracking для tooltip по наведению.
//     — Создаёт overlayStatsLabel поверх виджета (setParent(this)) с
//       полупрозрачным фоном через QSS: background-color: rgba(0,0,0,120).
//
//   processNextFrame():
//     — Если frameQueue не пуста: берёт следующий кадр.
//     — Если isStepMode — ждёт сигнала stepForward().
//     — Запускает интерполяцию от currentFrame к nextFrame.
//     — Вызывает update() для перерисовки.
//     — Если очередь пуста и isReceivingFrames == false → emit queueEmpty().
//
//   drawBars():
//     — Вычисляет ширину столбца: (widget_width - 2*padding) / numElements.
//     — При ширине < 1px: переключается в "dot mode" (пиксельная строка).
//     — Высота столбца: value * (widget_height - topMargin - bottomMargin).
//     — Цвет столбца определяется ColorScheme::getColor(value, index, state).
//     — Для плавности использует antialiasing: painter.setRenderHint(Antialiasing).
//     — При зуме > 1: рисует только видимую область (viewport culling).
//
//   drawHighlights():
//     — Для каждого индекса из VisFrame::highlightedIdx рисует столбец
//       поверх базового с цветом по типу: COMPARE=жёлтый, SWAP=красный,
//       PIVOT=синий, SORTED=зелёный, WRITE=оранжевый.
//     — Мигание реализуется через sin(QDateTime::currentMSecsSinceEpoch() / 100.0)
//       для плавного fade-in/out подсветки.
//
//   drawOverlay():
//     — Обновляет overlayStatsLabel с текстом:
//       "Алгоритм: %1 | Сравнений: %2 | Перестановок: %3 | Обращений: %4"
//     — В правом верхнем углу: "FPS: %1 | Элементов: %2 | Очередь: %3 кадров"
//     — Рисует легенду цветов в нижнем левом углу (если showLegend == true).
//
//   mouseMoveEvent():
//     — Вычисляет индекс столбца под курсором.
//     — Показывает QToolTip::showText() с: "Индекс: %1\nЗначение: %2\nПозиция: %3".
//
//   wheelEvent():
//     — delta > 0: zoomFactor *= 1.1, пересчёт видимой области.
//     — delta < 0: zoomFactor /= 1.1, clamp к [1.0, 50.0].
//     — Вызывает update().
//
//   resizeEvent():
//     — Пересчитывает maxColumns на основе новой ширины и минимального
//       размера столбца (1px). Если numElements > maxColumns: включается
//       прореживание — отображается каждый (numElements/maxColumns)-й элемент.
////////////////////////////////////////////////////////////////////////////////
