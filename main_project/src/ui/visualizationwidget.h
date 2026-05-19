#ifndef VISUALIZATIONWIDGET_H
#define VISUALIZATIONWIDGET_H

#include <QWidget>
#include <QVector>
#include <QTimer>
#include <QQueue>
#include <QLabel>
#include <atomic>
#include "../core/sortparams.h"

namespace SortBench {
enum class HighlightType;
class ColorScheme;
}

struct VisFrame {
    std::vector<float> values;
    std::vector<int> highlightedIdx;
    SortBench::HighlightType highlightType = SortBench::HighlightType::None;
    int pivotIndex = -1;
    int sortedBoundary = -1;
    long long comparisons = 0;
    long long swaps = 0;
    long long arrayAccesses = 0;
    QString algoName;
    bool isGpu = false;
};

class VisualizationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VisualizationWidget(QWidget *parent = nullptr);
    ~VisualizationWidget() override;

    void setAnimationSpeed(int fps);
    void pause();
    void resume();
    void reset();
    int currentFPS() const;
    void setColorScheme(SortBench::ColorScheme *scheme);
    void stepForward();

public slots:
    void renderFrame(const VisFrame& frame);

signals:
    void frameRendered(int frameIndex);
    void queueEmpty();
    void fpsUpdated(int actualFPS);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void processNextFrame();

private:
    void drawBars(QPainter &painter, const VisFrame &frame);
    void drawHighlights(QPainter &painter, const VisFrame &frame);
    void drawOverlay(QPainter &painter);
    void drawGradientBackground(QPainter &painter);
    void interpolateFrames();
    int calculateBarWidth() const;

    VisFrame currentFrame;
    VisFrame nextFrame;
    float interpolationT = 0.0f;
    
    QTimer *animTimer;
    SortBench::ColorScheme *colorScheme;
    int animationSpeedFPS = 60;
    bool isPaused = false;
    
    QQueue<VisFrame> frameQueue;
    std::atomic<bool> isReceivingFrames{false};
    
    QLabel *overlayStatsLabel;
    int maxColumns = 1000;
    bool isStepMode = false;
    
    // Zoom/scroll
    double zoomFactor = 1.0;
    int scrollOffset = 0;
    
    int frameIndex = 0;
    qint64 lastFrameTime = 0;
};

#endif // VISUALIZATIONWIDGET_H
