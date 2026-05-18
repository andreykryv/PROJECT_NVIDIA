#ifndef SORTVISUALIZER_H
#define SORTVISUALIZER_H

#include <QObject>
#include <QVector>
#include <QColor>
#include "visualization/colorscheme.h"

namespace SortBench {

enum class HighlightType {
    None,
    Compare,
    Swap,
    Sorted,
    Pivot,
    Partition
};

struct VisFrame {
    QVector<int> values;
    QVector<QColor> colors;
    int highlightedIndex = -1;
    int highlightedIndex2 = -1;
    long long comparisons = 0;
    long long swaps = 0;
    bool isSorted = false;
};

class SortVisualizer : public QObject
{
    Q_OBJECT
public:
    explicit SortVisualizer(int size = 0);
    
    void initialize(const QVector<int>& data);
    void updateFromCallback(const int* data, int size, int idx1, int idx2,
                            HighlightType type, long long comparisons, long long swaps);
    
    QVector<int> getArray() const;
    QVector<QColor> getColors() const;
    int getHighlightedIndex() const;
    int getHighlightedIndex2() const;
    int getArraySize() const;
    bool isSorted() const;
    
    QColor getColorForValue(int value, int maxValue) const;
    
    void setColorScheme(ColorScheme scheme) { m_colorScheme = scheme; }
    ColorScheme colorScheme() const { return m_colorScheme; }

private:
    QVector<int> m_array;
    QVector<QColor> m_colors;
    int m_arraySize;
    int m_highlightedIndex;
    int m_highlightedIndex2;
    ColorScheme m_colorScheme = ColorScheme::Rainbow;
};

} // namespace SortBench

#endif // SORTVISUALIZER_H
