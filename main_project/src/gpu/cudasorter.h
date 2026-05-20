#ifndef CUDASORTER_H
#define CUDASORTER_H

#include <QObject>
#include "cudasorter.cuh"

namespace SortBench {

// Wrapper class that combines QObject and CudaSorterBase for Qt integration
class CudaSorter : public QObject, public CudaSorterBase {
    Q_OBJECT

public:
    explicit CudaSorter(int deviceIndex = 0, QObject* parent = nullptr);
    ~CudaSorter();

    // Using CudaSorterBase implementation
    using CudaSorterBase::sort;
    using CudaSorterBase::allocateDeviceBuffers;
    using CudaSorterBase::freeDeviceBuffers;
    using CudaSorterBase::isReady;
    using CudaSorterBase::currentDeviceMemUsed;
};

} // namespace SortBench

#endif // CUDASORTER_H
