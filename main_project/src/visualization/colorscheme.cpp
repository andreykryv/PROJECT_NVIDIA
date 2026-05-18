#include "colorscheme.h"

namespace SortBench {

ColorScheme* ColorScheme::create(ColorSchemeType type)
{
    switch (type) {
    case ColorSchemeType::Rainbow:
        return new RainbowScheme();
    case ColorSchemeType::Heatmap:
        return new HeatmapScheme();
    case ColorSchemeType::Monochrome:
        return new MonochromeScheme();
    case ColorSchemeType::StatusColors:
        return new StatusColorsScheme();
    default:
        return new RainbowScheme();
    }
}

QColor RainbowScheme::getColor(float normalizedValue, int index, ElementState state)
{
    Q_UNUSED(index);
    
    // Цвета для состояний
    switch (state) {
    case ElementState::Comparing:
        return QColor(255, 200, 0);
    case ElementState::Swapping:
        return QColor(255, 0, 0);
    case ElementState::Pivot:
        return QColor(0, 0, 255);
    case ElementState::Sorted:
        return QColor(0, 200, 0);
    case ElementState::Writing:
        return QColor(200, 0, 200);
    default:
        break;
    }
    
    // Радужный градиент по значению
    int hue = static_cast<int>(240.0f * (1.0f - normalizedValue));
    int saturation = static_cast<int>(200 * m_brightness);
    int lightness = static_cast<int>(150 * m_brightness);
    
    return QColor::fromHsl(hue, saturation, lightness);
}

QColor HeatmapScheme::getColor(float normalizedValue, int index, ElementState state)
{
    Q_UNUSED(index);
    
    switch (state) {
    case ElementState::Comparing:
        return QColor(255, 200, 0);
    case ElementState::Swapping:
        return QColor(255, 0, 0);
    case ElementState::Pivot:
        return QColor(0, 0, 255);
    case ElementState::Sorted:
        return QColor(0, 200, 0);
    case ElementState::Writing:
        return QColor(200, 0, 200);
    default:
        break;
    }
    
    // Тепловая карта: синий -> зелёный -> жёлтый -> красный
    float r, g, b;
    if (normalizedValue < 0.25f) {
        r = 0.0f;
        g = normalizedValue * 4.0f;
        b = 1.0f;
    } else if (normalizedValue < 0.5f) {
        r = 0.0f;
        g = 1.0f;
        b = 1.0f - (normalizedValue - 0.25f) * 4.0f;
    } else if (normalizedValue < 0.75f) {
        r = (normalizedValue - 0.5f) * 4.0f;
        g = 1.0f;
        b = 0.0f;
    } else {
        r = 1.0f;
        g = 1.0f - (normalizedValue - 0.75f) * 4.0f;
        b = 0.0f;
    }
    
    return QColor(
        static_cast<int>(r * 255 * m_brightness),
        static_cast<int>(g * 255 * m_brightness),
        static_cast<int>(b * 255 * m_brightness)
    );
}

QColor MonochromeScheme::getColor(float normalizedValue, int index, ElementState state)
{
    Q_UNUSED(index);
    
    switch (state) {
    case ElementState::Comparing:
        return QColor(200, 200, 0);
    case ElementState::Swapping:
        return QColor(255, 0, 0);
    case ElementState::Pivot:
        return QColor(0, 0, 200);
    case ElementState::Sorted:
        return QColor(0, 150, 0);
    case ElementState::Writing:
        return QColor(200, 0, 200);
    default:
        break;
    }
    
    // Оттенки серого
    int gray = static_cast<int>(255.0f * (1.0f - normalizedValue) * m_brightness);
    return QColor(gray, gray, gray);
}

QColor StatusColorsScheme::getColor(float normalizedValue, int index, ElementState state)
{
    Q_UNUSED(normalizedValue);
    Q_UNUSED(index);
    
    // Все элементы одного цвета, только состояния влияют на цвет
    switch (state) {
    case ElementState::Comparing:
        return QColor(255, 200, 0);
    case ElementState::Swapping:
        return QColor(255, 0, 0);
    case ElementState::Pivot:
        return QColor(0, 0, 255);
    case ElementState::Sorted:
        return QColor(0, 200, 0);
    case ElementState::Writing:
        return QColor(200, 0, 200);
    default:
        return QColor(100, 149, 237);  // Cornflower blue
    }
}

} // namespace SortBench
