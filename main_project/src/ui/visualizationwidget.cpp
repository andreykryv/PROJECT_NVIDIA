#include "visualizationwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QToolTip>
#include <QVBoxLayout>
#include <cmath>

VisualizationWidget::VisualizationWidget(QWidget *parent)
    : QWidget(parent)
    , animTimer(new QTimer(this))
    , colorScheme(nullptr)
    , overlayStatsLabel(new QLabel(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    
    // Настройка таймера анимации
    connect(animTimer, &QTimer::timeout, this, &VisualizationWidget::processNextFrame);
    setAnimationSpeed(60);
    
    // Настройка overlay-метки
    overlayStatsLabel->setStyleSheet(
        "background-color: rgba(0,0,0,120); color: white; "
        "padding: 5px; border-radius: 3px; font-size: 11px;"
    );
    overlayStatsLabel->hide();
    
    // Цветовая схема по умолчанию
    colorScheme = new ColorScheme(this);
}

VisualizationWidget::~VisualizationWidget() {
    delete colorScheme;
}

void VisualizationWidget::setAnimationSpeed(int fps) {
    animationSpeedFPS = fps;
    isStepMode = (fps == 0);
    
    if (!isStepMode && !isPaused) {
        animTimer->setInterval(1000 / fps);
        animTimer->start();
    } else {
        animTimer->stop();
    }
}

void VisualizationWidget::pause() {
    isPaused = true;
    animTimer->stop();
}

void VisualizationWidget::resume() {
    isPaused = false;
    if (!isStepMode) {
        animTimer->setInterval(1000 / animationSpeedFPS);
        animTimer->start();
    }
}

void VisualizationWidget::reset() {
    frameQueue.clear();
    currentFrame = VisFrame();
    nextFrame = VisFrame();
    interpolationT = 0.0f;
    isReceivingFrames = false;
    isPaused = false;
    zoomFactor = 1.0;
    scrollOffset = 0;
    frameIndex = 0;
    overlayStatsLabel->hide();
    update();
}

void VisualizationWidget::setColorScheme(ColorScheme *scheme) {
    colorScheme = scheme;
    update();
}

void VisualizationWidget::stepForward() {
    if (isStepMode && !frameQueue.isEmpty()) {
        processNextFrame();
    }
}

void VisualizationWidget::renderFrame(const VisFrame& frame) {
    frameQueue.enqueue(frame);
    isReceivingFrames = true;
    
    if (!overlayStatsLabel->isVisible()) {
        overlayStatsLabel->show();
    }
}

void VisualizationWidget::processNextFrame() {
    if (frameQueue.isEmpty()) {
        if (!isReceivingFrames) {
            emit queueEmpty();
        }
        return;
    }
    
    if (isStepMode && !isPaused) {
        return; // Ждём явного шага
    }
    
    nextFrame = frameQueue.dequeue();
    interpolationT = 0.0f;
    
    // Запуск интерполяции
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (lastFrameTime > 0) {
        int actualFPS = static_cast<int>(1000.0 / (currentTime - lastFrameTime));
        emit fpsUpdated(actualFPS);
    }
    lastFrameTime = currentTime;
    
    update();
    emit frameRendered(frameIndex++);
}

void VisualizationWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    drawGradientBackground(painter);
    
    if (!currentFrame.values.empty()) {
        drawBars(painter, currentFrame);
        drawHighlights(painter, currentFrame);
    }
    
    drawOverlay(painter);
}

void VisualizationWidget::drawBars(QPainter &painter, const VisFrame &frame) {
    if (frame.values.empty()) return;
    
    int padding = 10;
    int topMargin = 20;
    int bottomMargin = 30;
    
    int availableWidth = width() - 2 * padding;
    int availableHeight = height() - topMargin - bottomMargin;
    
    int numElements = static_cast<int>(frame.values.size());
    int visibleElements = static_cast<int>(numElements / zoomFactor);
    visibleElements = qMax(1, qMin(visibleElements, maxColumns));
    
    int barWidth = calculateBarWidth();
    int gap = qMax(0, (availableWidth / visibleElements) - barWidth);
    
    painter.setPen(Qt::NoPen);
    
    for (int i = scrollOffset; i < qMin(scrollOffset + visibleElements, numElements); ++i) {
        float value = frame.values[i];
        int barHeight = static_cast<int>(value * availableHeight);
        
        int x = padding + (i - scrollOffset) * (barWidth + gap);
        int y = height() - bottomMargin - barHeight;
        
        QColor color = colorScheme ? 
            colorScheme->getColor(value, i, HighlightType::None) : 
            QColor::fromHsvF(value, 1.0, 1.0);
        
        painter.setBrush(color);
        painter.drawRect(x, y, barWidth, barHeight);
    }
}

void VisualizationWidget::drawHighlights(QPainter &painter, const VisFrame &frame) {
    if (frame.highlightedIdx.empty()) return;
    
    int padding = 10;
    int bottomMargin = 30;
    int availableHeight = height() - 20 - bottomMargin;
    
    int barWidth = calculateBarWidth();
    int gap = qMax(0, (width() - 2 * padding) / qMin(static_cast<int>(frame.values.size()), maxColumns) - barWidth);
    
    // Цвет подсветки по типу
    QColor highlightColor;
    switch (frame.highlightType) {
        case HighlightType::Compare: highlightColor = QColor(255, 255, 0); break;   // Жёлтый
        case HighlightType::Swap:    highlightColor = QColor(255, 0, 0); break;     // Красный
        case HighlightType::Pivot:   highlightColor = QColor(0, 0, 255); break;     // Синий
        case HighlightType::Sorted:  highlightColor = QColor(0, 255, 0); break;     // Зелёный
        case HighlightType::Write:   highlightColor = QColor(255, 165, 0); break;   // Оранжевый
        default: return;
    }
    
    // Мигание
    float alpha = 0.7f + 0.3f * std::sin(QDateTime::currentMSecsSinceEpoch() / 100.0);
    highlightColor.setAlphaF(alpha);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(highlightColor);
    
    for (int idx : frame.highlightedIdx) {
        if (idx < scrollOffset || idx >= scrollOffset + maxColumns) continue;
        
        float value = frame.values[idx];
        int barHeight = static_cast<int>(value * availableHeight);
        
        int x = 10 + (idx - scrollOffset) * (barWidth + gap);
        int y = height() - bottomMargin - barHeight;
        
        painter.drawRect(x, y, barWidth, barHeight);
    }
}

void VisualizationWidget::drawOverlay(QPainter &painter) {
    QString statsText = QString(
        "Алгоритм: %1 | Сравнений: %2 | Перестановок: %3 | Обращений: %4"
    ).arg(currentFrame.algoName)
     .arg(currentFrame.comparisons)
     .arg(currentFrame.swaps)
     .arg(currentFrame.arrayAccesses);
    
    overlayStatsLabel->setText(statsText);
    overlayStatsLabel->adjustSize();
    overlayStatsLabel->move(10, 10);
    
    // FPS и очередь в правом верхнем углу
    QString fpsText = QString("FPS: %1 | Элементов: %2 | Очередь: %3")
        .arg(animationSpeedFPS)
        .arg(currentFrame.values.size())
        .arg(frameQueue.size());
    
    painter.setPen(QColor(200, 200, 200));
    painter.setFont(QFont("Arial", 10));
    painter.drawText(width() - 200, 20, fpsText);
}

void VisualizationWidget::drawGradientBackground(QPainter &painter) {
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, QColor(30, 30, 40));
    gradient.setColorAt(1, QColor(50, 50, 60));
    painter.fillRect(rect(), gradient);
}

void VisualizationWidget::interpolateFrames() {
    if (interpolationT >= 1.0f) {
        currentFrame = nextFrame;
        interpolationT = 1.0f;
        return;
    }
    
    interpolationT += animationSpeedFPS / 1000.0f;
    interpolationT = qMin(1.0f, interpolationT);
    
    // Линейная интерполяция значений
    if (!currentFrame.values.empty() && !nextFrame.values.empty()) {
        size_t n = qMin(currentFrame.values.size(), nextFrame.values.size());
        for (size_t i = 0; i < n; ++i) {
            currentFrame.values[i] = 
                currentFrame.values[i] * (1.0f - interpolationT) +
                nextFrame.values[i] * interpolationT;
        }
    }
}

int VisualizationWidget::calculateBarWidth() const {
    int padding = 10;
    int availableWidth = width() - 2 * padding;
    int visibleElements = qMin(maxColumns, static_cast<int>(currentFrame.values.size() / zoomFactor));
    
    if (visibleElements <= 0) return 1;
    
    int barWidth = availableWidth / visibleElements;
    return qMax(1, qMin(barWidth, 50));
}

void VisualizationWidget::mouseMoveEvent(QMouseEvent *event) {
    if (currentFrame.values.empty()) return;
    
    int padding = 10;
    int barWidth = calculateBarWidth();
    int gap = 1;
    
    int idx = (event->x() - padding) / (barWidth + gap);
    idx += scrollOffset;
    
    if (idx >= 0 && idx < static_cast<int>(currentFrame.values.size())) {
        QString tooltip = QString("Индекс: %1\nЗначение: %2\nПозиция: %3")
            .arg(idx)
            .arg(currentFrame.values[idx], 0, 'f', 4)
            .arg(idx);
        QToolTip::showText(event->globalPos(), tooltip, this);
    } else {
        QToolTip::hideText();
    }
    
    QWidget::mouseMoveEvent(event);
}

void VisualizationWidget::wheelEvent(QWheelEvent *event) {
    double delta = event->angleDelta().y();
    
    if (delta > 0) {
        zoomFactor *= 1.1;
    } else {
        zoomFactor /= 1.1;
    }
    
    zoomFactor = qBound(1.0, zoomFactor, 50.0);
    update();
    
    QWidget::wheelEvent(event);
}

void VisualizationWidget::resizeEvent(QResizeEvent *event) {
    int minBarWidth = 1;
    int padding = 20;
    maxColumns = qMax(10, (width() - padding) / minBarWidth);
    
    // Прореживание при большом числе элементов
    if (currentFrame.values.size() > static_cast<size_t>(maxColumns)) {
        // Включаем скролл
    }
    
    QWidget::resizeEvent(event);
}
