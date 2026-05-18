////////////////////////////////////////////////////////////////////////////////
// models/resultstablemodel.h — Qt-модель таблицы результатов
//
// НАЗНАЧЕНИЕ:
//   QAbstractTableModel для QTableView на вкладке "Таблица результатов".
//   Каждая строка = один BenchmarkResult, столбцы = метрики.
//
// КЛАСС: ResultsTableModel : public QAbstractTableModel
//
//   СТОЛБЦЫ:
//     0  — Время/Дата
//     1  — CPU Алгоритм
//     2  — GPU Алгоритм
//     3  — Размер массива
//     4  — Тип данных
//     5  — Распределение
//     6  — CPU время (мс)
//     7  — GPU total (мс)
//     8  — GPU kernel (мс)
//     9  — H2D (мс)
//     10 — D2H (мс)
//     11 — Ускорение GPU
//     12 — Корректность
//
//   МЕТОДЫ:
//     void addResult(BenchmarkResult)
//     void clearResults()
//     BenchmarkResult resultAt(int row) const
//     void sortByColumn(int col, Qt::SortOrder) — переопределяет QAbstractTableModel::sort
//     Qt::ItemFlags flags(const QModelIndex&) const override
//         — ItemIsSelectable | ItemIsEnabled (строки только для чтения)
//
//   ФОРМАТИРОВАНИЕ:
//     — Время: 2 знака после запятой мс.
//     — Ускорение: цветовое кодирование через Qt::ForegroundRole:
//       > 5x → зелёный, 1–5x → жёлтый, < 1x → красный.
//     — Булевы: "✓" / "✗".
////////////////////////////////////////////////////////////////////////////////
