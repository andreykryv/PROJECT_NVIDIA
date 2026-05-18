#ifndef ANIMATIONCONTROLLER_H
#define ANIMATIONCONTROLLER_H

#include <QObject>
#include <QQueue>
#include "visualization/sortvisualizer.h"

namespace SortBench {

class AnimationController : public QObject
{
    Q_OBJECT
public:
    explicit AnimationController(QObject *parent = nullptr);
    
    void setAnimationSpeed(float speed);
    float animationSpeed() const;
    
    int frameDelayMs() const;
    
    void start();
    void pause();
    void resume();
    void stop();
    
    bool isRunning() const;
    int currentFrame() const;

public slots:
    void setCurrentFrame(int frame);
    void advanceFrame();

signals:
    void started();
    void paused();
    void resumed();
    void stopped();
    void animationSpeedChanged(float speed);
    void frameUpdated(int frame);

private:
    float m_animationSpeed;
    bool m_running = false;
    int m_currentFrame;
};

} // namespace SortBench

#endif // ANIMATIONCONTROLLER_H
