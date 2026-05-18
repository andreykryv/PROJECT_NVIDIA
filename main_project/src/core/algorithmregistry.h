#ifndef ALGORITHMREGISTRY_H
#define ALGORITHMREGISTRY_H

#include <QString>
#include <QColor>
#include <QList>
#include <QHash>
#include "sortparams.h"

namespace SortBench {

struct AlgorithmInfo {
    QString name;
    QString shortName;
    QString category;  // "CPU" or "GPU"
    QString timeComplexity;
    QString spaceComplexity;
    QString description;
    bool stable = false;
    bool inPlace = false;
    bool parallelizable = false;
    QColor chartColor;
};

class AlgorithmRegistry {
public:
    static AlgorithmRegistry& instance();
    
    AlgorithmInfo getInfo(CpuAlgorithm algo) const;
    AlgorithmInfo getInfo(GpuAlgorithm algo) const;
    QList<AlgorithmInfo> allCpuAlgorithms() const;
    QList<AlgorithmInfo> allGpuAlgorithms() const;
    
private:
    AlgorithmRegistry();
    QHash<int, AlgorithmInfo> m_cpuAlgorithms;
    QHash<int, AlgorithmInfo> m_gpuAlgorithms;
};

} // namespace SortBench

#endif // ALGORITHMREGISTRY_H
