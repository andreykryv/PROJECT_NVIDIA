////////////////////////////////////////////////////////////////////////////////
// models/algorithmsmodel.h — Qt-модель списка алгоритмов
//
// НАЗНАЧЕНИЕ:
//   QAbstractListModel для QComboBox и QListView. Отображает список алгоритмов
//   с иконками, именами, цветовыми метками и информацией о сложности.
//
// КЛАСС: AlgorithmsModel : public QAbstractListModel
//
//   Роли:
//     Qt::DisplayRole      — имя алгоритма ("Quick Sort")
//     Qt::DecorationRole   — цветная иконка (QPixmap 16x16 заполненный цветом)
//     Qt::ToolTipRole      — "O(n log n) avg | Стабильный: нет"
//     AlgorithmNameRole    — полное имя
//     AlgorithmColorRole   — QColor серии на графике
//     AlgorithmTypeRole    — "CPU" или "GPU"
//     ComplexityRole       — строка сложности
//
//   МЕТОДЫ:
//     int rowCount(const QModelIndex&) const override
//     QVariant data(const QModelIndex&, int role) const override
//     void setCpuMode(bool)          — показывать только CPU или только GPU
//     void setEnabledAlgorithms(QSet<QString>) — какие доступны
////////////////////////////////////////////////////////////////////////////////
