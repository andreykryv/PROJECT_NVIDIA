////////////////////////////////////////////////////////////////////////////////
// models/resultstablemodel.cpp — реализация модели таблицы результатов
//
// data(role==DisplayRole): форматирует поле согласно номеру столбца.
// data(role==ForegroundRole): возвращает QBrush для цветового кодирования.
// data(role==UserRole): возвращает сырое double значение для сортировки.
// sort(): std::sort на internal QList<BenchmarkResult> с компаратором по столбцу.
// addResult(): beginInsertRows(0,0) → prepend (новые сверху) → endInsertRows.
////////////////////////////////////////////////////////////////////////////////
