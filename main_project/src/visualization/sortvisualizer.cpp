#include "sortvisualizer.h"
#include <algorithm>
#include <cmath>

namespace SortBench {

SortVisualizer::SortVisualizer(int size)
    : m_arraySize(size), m_highlightedIndex(-1), m_highlightedIndex2(-1)
{
    m_array.resize(size);
    m_colors.resize(size, Qt::white);
}

void SortVisualizer::initialize(const QVector<int>& data)
{
    m_array = data;
    m_arraySize = data.size();
    m_colors.fill(Qt::white, m_arraySize);
    m_highlightedIndex = -1;
    m_highlightedIndex2 = -1;
}

void SortVisualizer::updateFromCallback(const int* data, int size, int idx1, int idx2, 
                                         HighlightType type, long long comparisons, long long swaps)
{
    Q_UNUSED(comparisons);
    Q_UNUSED(swaps);
    
    for (int i = 0; i < size && i < m_arraySize; ++i) {
        m_array[i] = data[i];
    }
    
    m_highlightedIndex = idx1;
    m_highlightedIndex2 = idx2;
    
    for (int i = 0; i < m_arraySize; ++i) {
        m_colors[i] = Qt::white;
    }
    
    switch (type) {
    case HighlightType::Compare:
        if (idx1 >= 0 && idx1 < m_arraySize) m_colors[idx1] = QColor(255, 200, 0);
        if (idx2 >= 0 && idx2 < m_arraySize) m_colors[idx2] = QColor(255, 200, 0);
        break;
    case HighlightType::Swap:
        if (idx1 >= 0 && idx1 < m_arraySize) m_colors[idx1] = QColor(255, 0, 0);
        if (idx2 >= 0 && idx2 < m_arraySize) m_colors[idx2] = QColor(255, 0, 0);
        break;
    case HighlightType::Sorted:
        if (idx1 >= 0 && idx1 < m_arraySize) m_colors[idx1] = QColor(0, 200, 0);
        break;
    case HighlightType::Pivot:
        if (idx1 >= 0 && idx1 < m_arraySize) m_colors[idx1] = QColor(0, 0, 255);
        break;
     case HighlightType::Write:
        if (idx1 >= 0 && idx1 < m_arraySize) m_colors[idx1] = QColor(200, 0, 200);
        break;
          case HighlightType::None:
        break;
    }
}

QVector<int> SortVisualizer::getArray() const
{
    return m_array;
}

QVector<QColor> SortVisualizer::getColors() const
{
    return m_colors;
}

int SortVisualizer::getHighlightedIndex() const
{
    return m_highlightedIndex;
}

int SortVisualizer::getHighlightedIndex2() const
{
    return m_highlightedIndex2;
}

int SortVisualizer::getArraySize() const
{
    return m_arraySize;
}

bool SortVisualizer::isSorted() const
{
    for (int i = 0; i < m_arraySize - 1; ++i) {
        if (m_array[i] > m_array[i + 1])
            return false;
    }
    return true;
}

QColor SortVisualizer::getColorForValue(int value, int maxValue) const
{
    if (maxValue == 0) return Qt::white;
    
    float ratio = static_cast<float>(value) / maxValue;
    int hue = static_cast<int>(240.0f * (1.0f - ratio));
    return QColor::fromHsl(hue, 200, 150);
}

} // namespace SortBench
