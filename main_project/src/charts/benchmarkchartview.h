////////////////////////////////////////////////////////////////////////////////
// charts/benchmarkchartview.h — базовый класс для всех графиков
//
// НАЗНАЧЕНИЕ:
//   Расширяет QChartView стандартным поведением: zoom, pan, контекстное меню,
//   экспорт в файл, настройка темы. Все конкретные графики наследуют от него.
//
// КЛАСС: BenchmarkChartView : public QChartView
//
//   ПЕРЕОПРЕДЕЛЁННЫЕ МЕТОДЫ:
//     mousePressEvent()        — начало pan (средняя кнопка) или выделения
//     mouseMoveEvent()         — pan при зажатой средней кнопке
//     mouseReleaseEvent()      — завершение zoom-выделения (RubberBand)
//     wheelEvent()             — zoom in/out
//     contextMenuEvent()       — меню: Сохранить PNG, Сохранить SVG, Сбросить zoom,
//                                 Копировать в буфер обмена, Полноэкранный режим
//
//   МЕТОДЫ:
//     void resetZoom()         — QChart::zoomReset()
//     void saveToFile(QString) — сохранение в PNG или SVG по расширению
//     void applyTheme(bool isDark)
//     void setTitle(QString)   — обновить заголовок с форматированием
//
//   СИГНАЛЫ:
//     seriesClicked(QString seriesName)  — клик по серии данных
//     pointHovered(QPointF, QString)     — наведение на точку
////////////////////////////////////////////////////////////////////////////////
