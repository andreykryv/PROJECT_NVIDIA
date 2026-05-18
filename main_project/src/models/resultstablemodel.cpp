#include "resultstablemodel.h"
#include <algorithm>

namespace SortBench {

ResultsTableModel::ResultsTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int ResultsTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_results.size();
}

int ResultsTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant ResultsTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_results.size())
        return QVariant();

    const BenchmarkResult &res = m_results.at(index.row());

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case ColTimestamp:
            return res.timestamp.toString("dd.MM.yyyy hh:mm:ss");
        case ColCpuAlgo:
            return toString(res.params.cpuAlgorithm);
        case ColGpuAlgo:
            return toString(res.params.gpuAlgorithm);
        case ColArraySize:
            return res.params.arraySize;
        case ColDataType:
            return toString(res.params.dataType);
        case ColDistribution:
            return toString(res.params.distribution);
        case ColCpuTime:
            return QString("%1").arg(res.cpuTimeMs, 0, 'f', 2);
        case ColGpuTotalTime:
            return QString("%1").arg(res.gpuTotalTimeMs, 0, 'f', 2);
        case ColGpuKernelTime:
            return QString("%1").arg(res.gpuKernelTimeMs, 0, 'f', 2);
        case ColH2DTime:
            return QString("%1").arg(res.gpuH2DTimeMs, 0, 'f', 3);
        case ColD2HTime:
            return QString("%1").arg(res.gpuD2HTimeMs, 0, 'f', 3);
        case ColSpeedup:
            return QString("%1x").arg(res.speedup(), 0, 'f', 2);
        case ColCorrectness:
            return res.isSorted ? "✓" : "✗";
        default:
            return QVariant();
        }
    }

    if (role == Qt::ForegroundRole && index.column() == ColSpeedup) {
        double speedup = res.speedup();
        if (speedup > 5.0)
            return QBrush(QColor(0, 150, 0));      // Зелёный
        else if (speedup >= 1.0)
            return QBrush(QColor(200, 150, 0));    // Жёлтый
        else
            return QBrush(QColor(200, 0, 0));       // Красный
    }

    if (role == Qt::ForegroundRole && index.column() == ColCorrectness) {
        if (!res.isSorted)
            return QBrush(QColor(200, 0, 0));
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() >= ColCpuTime && index.column() <= ColSpeedup)
            return Qt::AlignRight | Qt::AlignVCenter;
        return Qt::AlignLeft | Qt::AlignVCenter;
    }

    if (role == Qt::UserRole) {
        // Сырые данные для сортировки
        switch (index.column()) {
        case ColCpuTime: return res.cpuTimeMs;
        case ColGpuTotalTime: return res.gpuTotalTimeMs;
        case ColGpuKernelTime: return res.gpuKernelTimeMs;
        case ColH2DTime: return res.gpuH2DTimeMs;
        case ColD2HTime: return res.gpuD2HTimeMs;
        case ColSpeedup: return res.speedup();
        case ColArraySize: return res.params.arraySize;
        case ColTimestamp: return res.timestamp.toMSecsSinceEpoch();
        default: return QVariant();
        }
    }

    return QVariant();
}

QVariant ResultsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case ColTimestamp: return "Время";
        case ColCpuAlgo: return "CPU Алгоритм";
        case ColGpuAlgo: return "GPU Алгоритм";
        case ColArraySize: return "Размер";
        case ColDataType: return "Тип";
        case ColDistribution: return "Распределение";
        case ColCpuTime: return "CPU (мс)";
        case ColGpuTotalTime: return "GPU Total (мс)";
        case ColGpuKernelTime: return "GPU Kernel (мс)";
        case ColH2DTime: return "H2D (мс)";
        case ColD2HTime: return "D2H (мс)";
        case ColSpeedup: return "Ускорение";
        case ColCorrectness: return "Корректность";
        default: return QVariant();
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags ResultsTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void ResultsTableModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column >= ColumnCount)
        return;

    beginResetModel();
    m_sortColumn = column;
    m_sortOrder = order;

    std::sort(m_results.begin(), m_results.end(), [this, column, order](const BenchmarkResult &a, const BenchmarkResult &b) {
        bool less = false;
        
        switch (column) {
        case ColTimestamp:
            less = a.timestamp < b.timestamp;
            break;
        case ColCpuTime:
            less = a.cpuTimeMs < b.cpuTimeMs;
            break;
        case ColGpuTotalTime:
            less = a.gpuTotalTimeMs < b.gpuTotalTimeMs;
            break;
        case ColGpuKernelTime:
            less = a.gpuKernelTimeMs < b.gpuKernelTimeMs;
            break;
        case ColH2DTime:
            less = a.gpuH2DTimeMs < b.gpuD2HTimeMs;
            break;
        case ColD2HTime:
            less = a.gpuD2HTimeMs < b.gpuD2HTimeMs;
            break;
        case ColSpeedup:
            less = a.speedup() < b.speedup();
            break;
        case ColArraySize:
            less = a.params.arraySize < b.params.arraySize;
            break;
        default:
            less = a.timestamp < b.timestamp;
        }

        return order == Qt::AscendingOrder ? less : !less;
    });

    endResetModel();
}

void ResultsTableModel::addResult(const BenchmarkResult &result)
{
    beginInsertRows(QModelIndex(), 0, 0);
    m_results.prepend(result);
    endInsertRows();
}

void ResultsTableModel::clearResults()
{
    beginResetModel();
    m_results.clear();
    endResetModel();
}

BenchmarkResult ResultsTableModel::resultAt(int row) const
{
    if (row >= 0 && row < m_results.size())
        return m_results.at(row);
    return BenchmarkResult{};
}

} // namespace SortBench
