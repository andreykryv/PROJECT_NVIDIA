////////////////////////////////////////////////////////////////////////////////
// core/algorithmregistry.h — реестр метаданных алгоритмов
//
// СТРУКТУРА AlgorithmInfo:
//   QString name, shortName, category ("CPU"/"GPU")
//   QString timeComplexity, spaceComplexity
//   QString description          — подробное описание на русском
//   bool    stable, inPlace, parallelizable
//   QColor  chartColor           — цвет серии на графике
//
// КЛАСС AlgorithmRegistry (Singleton):
//   static AlgorithmRegistry& instance()
//   AlgorithmInfo getInfo(CpuAlgorithm) const
//   AlgorithmInfo getInfo(GpuAlgorithm) const
//   QList<AlgorithmInfo> allCpuAlgorithms() const
//   QList<AlgorithmInfo> allGpuAlgorithms() const
//
// Реестр заполняется в приватном конструкторе.
// Хранение: QHash<int, AlgorithmInfo> (ключ — static_cast<int>(enum)).
////////////////////////////////////////////////////////////////////////////////
