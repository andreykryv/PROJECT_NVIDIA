////////////////////////////////////////////////////////////////////////////////
// charts/comparisonbarchart.cpp — реализация столбчатой диаграммы
//
// addResult():
//   Добавляет значения в три QBarSet.
//   Пересчитывает axisY->setMax(maxValue * 1.1).
//   Обновляет axisX категории.
//   QChart::setAnimationOptions(SeriesAnimations) — плавное появление.
//
// showValueLabels():
//   Создаёт QGraphicsTextItem над каждым баром с форматированным временем.
//   Позиция вычисляется через mapToPosition(QPointF(barIndex, value)).
////////////////////////////////////////////////////////////////////////////////
