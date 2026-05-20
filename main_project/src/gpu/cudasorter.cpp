#include "cudasorter.h"

namespace SortBench {

CudaSorter::CudaSorter(int deviceIndex, QObject* parent)
    : QObject(parent), CudaSorterBase(deviceIndex)
{
}

CudaSorter::~CudaSorter() {
}

} // namespace SortBench
