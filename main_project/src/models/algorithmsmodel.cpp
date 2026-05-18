#include "algorithmsmodel.h"
#include "core/algorithmregistry.h"
#include <QPixmap>
#include <QPainter>
#include <QColor>

namespace SortBench {

AlgorithmsModel::AlgorithmsModel(QObject *parent)
    : QAbstractListModel(parent), m_cpuMode(true)
{
    reloadAlgorithms();
}

int AlgorithmsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_algorithms.size();
}

QVariant AlgorithmsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_algorithms.size())
        return QVariant();

    const AlgorithmInfo &info = m_algorithms.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return info.displayName;

    case Qt::DecorationRole: {
        QPixmap pixmap(16, 16);
        pixmap.fill(info.chartColor);
        return pixmap;
    }

    case Qt::ToolTipRole:
        return QString("<b>%1</b><br>%2<br>Стабильный: %3<br>Лучший: %4<br>Средний: %5<br>Худший: %6")
            .arg(info.displayName)
            .arg(info.description)
            .arg(info.isStable ? "Да" : "Нет")
            .arg(info.bestComplexity)
            .arg(info.avgComplexity)
            .arg(info.worstComplexity);

    case AlgorithmNameRole:
        return info.name;

    case AlgorithmColorRole:
        return info.chartColor;

    case AlgorithmTypeRole:
        return info.type;

    case ComplexityRole:
        return info.avgComplexity;

    case EnabledRole:
        return m_enabledAlgorithms.isEmpty() || m_enabledAlgorithms.contains(info.name);

    default:
        return QVariant();
    }
}

Qt::ItemFlags AlgorithmsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    
    const AlgorithmInfo &info = m_algorithms.at(index.row());
    if (m_enabledAlgorithms.isEmpty() || m_enabledAlgorithms.contains(info.name))
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    return Qt::NoItemFlags;
}

void AlgorithmsModel::setCpuMode(bool cpuMode)
{
    beginResetModel();
    m_cpuMode = cpuMode;
    reloadAlgorithms();
    endResetModel();
}

void AlgorithmsModel::setEnabledAlgorithms(const QSet<QString> &enabled)
{
    beginResetModel();
    m_enabledAlgorithms = enabled;
    reloadAlgorithms();
    endResetModel();
}

void AlgorithmsModel::reloadAlgorithms()
{
    m_algorithms.clear();
   auto& registry = AlgorithmRegistry::instance();
QList<AlgorithmInfo> allAlgos;
for (const auto& a : registry.allCpuAlgorithms()) allAlgos.append(a);
for (const auto& a : registry.allGpuAlgorithms()) allAlgos.append(a);
    
    for (const auto &algo : allAlgos) {
        bool isCpu = algo.type == "CPU";
        if (m_cpuMode == isCpu) {
            m_algorithms.append(algo);
        }
    }
}

AlgorithmInfo AlgorithmsModel::getAlgorithm(int index) const
{
    if (index >= 0 && index < m_algorithms.size())
        return m_algorithms.at(index);
    return AlgorithmInfo{};
}

int AlgorithmsModel::findAlgorithmByName(const QString &name) const
{
    for (int i = 0; i < m_algorithms.size(); ++i) {
        if (m_algorithms.at(i).name == name)
            return i;
    }
    return -1;
}

} // namespace SortBench
