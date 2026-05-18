////////////////////////////////////////////////////////////////////////////////
// visualization/sortvisualizer.h — координатор визуализации
//
// НАЗНАЧЕНИЕ:
//   SortVisualizer — промежуточный слой между движком и VisualizationWidget.
//   Принимает сырые данные кадров от SortBenchEngine, нормализует их,
//   применяет цветовую схему и передаёт готовые кадры в VisualizationWidget.
//
// КЛАСС: SortVisualizer : public QObject
//
//   МЕТОДЫ:
//     void setColorScheme(ColorScheme cs)    — смена схемы
//     void setMaxElements(int n)             — макс. столбцов на экране
//     VisFrame processFrame(RawFrame)        — преобразует сырой кадр в готовый
//
//   processFrame():
//     1. Если rawFrame.size > maxElements: прореживание (subsampling).
//        Выбирается каждый (size/maxElements)-й элемент.
//     2. Нормализация значений: (val - minVal) / (maxVal - minVal) → [0..1].
//        minVal/maxVal кешируются и обновляются раз в 100 кадров.
//     3. Перекодирование highlightedIdx с учётом прореживания.
//     4. Применение цветовой схемы: colorScheme->getColor(normalizedVal) → QColor.
//     5. Возвращает VisFrame с готовыми данными для рендеринга.
//
//   СИГНАЛЫ:
//     frameProcessed(VisFrame)              — кадр обработан, готов к рисованию
////////////////////////////////////////////////////////////////////////////////
