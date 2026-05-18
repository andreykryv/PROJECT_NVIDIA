#include "baranimation.h"
#include <QPainter>
#include <QRect>
#include <QtMath>

namespace SortBench {

BarAnimation::BarAnimation(QObject *parent)
    : QObject(parent), m_value(0.0), m_targetValue(0.0)
{
}

void BarAnimation::setValue(qreal value)
{
    m_value = qBound(0.0, value, 1.0);
    emit valueChanged(m_value);
}

void BarAnimation::setTargetValue(qreal target)
{
    m_targetValue = qBound(0.0, target, 1.0);
}

qreal BarAnimation::value() const
{
    return m_value;
}

qreal BarAnimation::targetValue() const
{
    return m_targetValue;
}

void BarAnimation::update(qreal deltaTime)
{
    Q_UNUSED(deltaTime);
    // Простая линейная интерполяция
    qreal diff = m_targetValue - m_value;
    if (qAbs(diff) > 0.01) {
        m_value += diff * 0.3;  // Коэффициент сглаживания
        emit valueChanged(m_value);
    } else {
        m_value = m_targetValue;
        emit valueChanged(m_value);
    }
}

void BarAnimation::reset()
{
    m_value = 0.0;
    m_targetValue = 0.0;
    emit valueChanged(m_value);
}

} // namespace SortBench
