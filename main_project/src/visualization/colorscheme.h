#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include <QColor>
#include <QString>
#include "../core/sortparams.h"

namespace SortBench {

enum class ElementState {
    Normal,
    Comparing,
    Swapping,
    Pivot,
    Sorted,
    Writing
};

class ColorScheme {
public:
    virtual ~ColorScheme() = default;
    
    virtual QColor getColor(float normalizedValue, int index, ElementState state) = 0;
    virtual QString name() const = 0;
    void setBrightness(float b) { m_brightness = b; }
    float brightness() const { return m_brightness; }
    
    static ColorScheme* create(ColorSchemeType type);

protected:
    float m_brightness = 1.0f;
};

class RainbowScheme : public ColorScheme {
public:
    QColor getColor(float normalizedValue, int index, ElementState state) override;
    QString name() const override { return "Радуга"; }
};

class HeatmapScheme : public ColorScheme {
public:
    QColor getColor(float normalizedValue, int index, ElementState state) override;
    QString name() const override { return "Тепловая карта"; }
};

class MonochromeScheme : public ColorScheme {
public:
    QColor getColor(float normalizedValue, int index, ElementState state) override;
    QString name() const override { return "Монохром"; }
};

class StatusColorsScheme : public ColorScheme {
public:
    QColor getColor(float normalizedValue, int index, ElementState state) override;
    QString name() const override { return "Статусные цвета"; }
};

} // namespace SortBench

#endif // COLORSCHEME_H
