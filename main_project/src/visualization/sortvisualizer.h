#ifndef SORTVISUALIZER_H
#define SORTVISUALIZER_H

#include <QObject>
#include <QVector>
#include <QColor>
#include "../core/sortparams.h"

namespace SortBench {

// Используем HighlightType из sortbenchengine.h через forward declaration
enum class HighlightType;



class ColorScheme;

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
    
    void setColorScheme(ColorScheme *scheme) { m_colorScheme = scheme; }
    ColorScheme* colorScheme() const { return m_colorScheme; }

private:
    QVector<int> m_array;
    QVector<QColor> m_colors;
    int m_arraySize;
    int m_highlightedIndex;
    int m_highlightedIndex2;
    ColorScheme *m_colorScheme = nullptr;
};

} // namespace SortBench

#endif // SORTVISUALIZER_H
