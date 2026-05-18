#ifndef ALGORITHMSMODEL_H
#define ALGORITHMSMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QSet>
#include <QString>
#include <QColor>
#include "core/algorithmregistry.h"

namespace SortBench {

class AlgorithmsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        AlgorithmNameRole = Qt::UserRole + 1,
        AlgorithmColorRole,
        AlgorithmTypeRole,
        ComplexityRole,
        EnabledRole
    };

    explicit AlgorithmsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void setCpuMode(bool cpuMode);
    void setEnabledAlgorithms(const QSet<QString> &enabled);
    
    AlgorithmInfo getAlgorithm(int index) const;
    int findAlgorithmByName(const QString &name) const;

private:
    void reloadAlgorithms();
    
    QList<AlgorithmInfo> m_algorithms;
    bool m_cpuMode;
    QSet<QString> m_enabledAlgorithms;
};

} // namespace SortBench

#endif // ALGORITHMSMODEL_H
