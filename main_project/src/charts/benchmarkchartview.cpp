////////////////////////////////////////////////////////////////////////////////
// charts/benchmarkchartview.cpp — реализация базового класса графиков
//
// wheelEvent(): QChart::zoom(1.1) или zoom(0.9) в зависимости от delta.
// contextMenuEvent(): QMenu с QAction для сохранения и копирования.
// saveToFile(): для PNG — grab().save(path), для SVG — QSvgGenerator.
// applyTheme(): QChart::setTheme(ChartThemeDark/Light) + ручная настройка цветов.
////////////////////////////////////////////////////////////////////////////////
