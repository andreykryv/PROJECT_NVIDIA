#include "animationcontroller.h"
#include <QTimeLine>
#include <QPropertyAnimation>

namespace SortBench {

AnimationController::AnimationController(QObject *parent)
    : QObject(parent), m_animationSpeed(1.0), m_currentFrame(0)
{
}

void AnimationController::setAnimationSpeed(float speed)
{
    m_animationSpeed = qBound(0.1f, speed, 10.0f);
    emit animationSpeedChanged(m_animationSpeed);
}

int AnimationController::frameDelayMs() const
{
    // Базовая задержка 50мс, делённая на скорость
    return static_cast<int>(50.0f / m_animationSpeed);
}

void AnimationController::start()
{
    m_running = true;
    emit started();
}

void AnimationController::pause()
{
    m_running = false;
    emit paused();
}

void AnimationController::resume()
{
    m_running = true;
    emit resumed();
}

void AnimationController::stop()
{
    m_running = false;
    m_currentFrame = 0;
    emit stopped();
}

bool AnimationController::isRunning() const
{
    return m_running;
}

float AnimationController::animationSpeed() const
{
    return m_animationSpeed;
}

int AnimationController::currentFrame() const
{
    return m_currentFrame;
}

void AnimationController::setCurrentFrame(int frame)
{
    if (m_currentFrame != frame) {
        m_currentFrame = frame;
        emit frameUpdated(frame);
    }
}

void AnimationController::advanceFrame()
{
    ++m_currentFrame;
    emit frameUpdated(m_currentFrame);
}

} // namespace SortBench
