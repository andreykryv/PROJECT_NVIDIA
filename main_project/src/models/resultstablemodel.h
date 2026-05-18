#ifndef RESULTSTABLEMODEL_H
#define RESULTSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QColor>
#include "core/benchmarkresult.h"

namespace SortBench {

class ResultsTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Column {
        ColTimestamp = 0,
        ColCpuAlgo,
        ColGpuAlgo,
        ColArraySize,
        ColDataType,
        ColDistribution,
        ColCpuTime,
        ColGpuTotalTime,
        ColGpuKernelTime,
        ColH2DTime,
        ColD2HTime,
        ColSpeedup,
        ColCorrectness,
        ColumnCount
    };

    explicit ResultsTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    void addResult(const BenchmarkResult &result);
    void clearResults();
    BenchmarkResult resultAt(int row) const;
    const QList<BenchmarkResult>& allResults() const { return m_results; }

private:
    QList<BenchmarkResult> m_results;
    int m_sortColumn = 0;
    Qt::SortOrder m_sortOrder = Qt::DescendingOrder;
};

} // namespace SortBench

#endif // RESULTSTABLEMODEL_H
