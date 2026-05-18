#ifndef BARANIMATION_H
#define BARANIMATION_H

#include <QObject>
#include <QColor>

namespace SortBench {

class BarAnimation : public QObject
{
    Q_OBJECT
public:
    explicit BarAnimation(QObject *parent = nullptr);
    
    void setValue(qreal value);
    void setTargetValue(qreal target);
    
    qreal value() const;
    qreal targetValue() const;
    
    void update(qreal deltaTime);
    void reset();

signals:
    void valueChanged(qreal value);

private:
    qreal m_value;
    qreal m_targetValue;
};

} // namespace SortBench

#endif // BARANIMATION_H
