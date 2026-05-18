////////////////////////////////////////////////////////////////////////////////
// models/algorithmsmodel.cpp — реализация модели алгоритмов
//
// Конструктор: заполняет QList<AlgorithmInfo> из AlgorithmRegistry::instance().
// data(): switch(role) возвращает нужное поле из AlgorithmInfo.
// DecorationRole: QPixmap(16, 16) с fill(info.chartColor).
// ToolTipRole: форматированная HTML строка с таблицей сложности.
////////////////////////////////////////////////////////////////////////////////
