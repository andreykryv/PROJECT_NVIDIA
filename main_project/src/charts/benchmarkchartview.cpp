#include "benchmarkchartview.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QSvgGenerator>
#include <QApplication>
#include <QClipboard>

namespace SortBench {

BenchmarkChartView::BenchmarkChartView(QWidget *parent)
    : QChartView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    setRubberBand(QChartView::RectangleRubberBand);
    
    createActions();
}

void BenchmarkChartView::createActions()
{
    m_contextMenu = new QMenu(this);
    
    QAction *savePngAction = new QAction(tr("Сохранить как PNG"), this);
    connect(savePngAction, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, tr("Сохранить график"), 
                                                     QString(), tr("PNG Files (*.png)"));
        if (!path.isEmpty()) {
            if (!path.endsWith(".png", Qt::CaseInsensitive)) {
                path += ".png";
            }
            grab().save(path);
        }
    });
    
    QAction *saveSvgAction = new QAction(tr("Сохранить как SVG"), this);
    connect(saveSvgAction, &QAction::triggered, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, tr("Сохранить график"),
                                                     QString(), tr("SVG Files (*.svg)"));
        if (!path.isEmpty()) {
            if (!path.endsWith(".svg", Qt::CaseInsensitive)) {
                path += ".svg";
            }
            QSvgGenerator generator;
            generator.setFileName(path);
            generator.setSize(size());
            generator.setViewBox(rect());
            
            QPainter painter(&generator);
            render(&painter);
        }
    });
    
    QAction *copyAction = new QAction(tr("Копировать в буфер обмена"), this);
    connect(copyAction, &QAction::triggered, this, [this]() {
        QPixmap pixmap = grab();
        QApplication::clipboard()->setPixmap(pixmap);
    });
    
    QAction *resetZoomAction = new QAction(tr("Сбросить zoom"), this);
    connect(resetZoomAction, &QAction::triggered, this, &BenchmarkChartView::resetZoom);
    
    QAction *fullscreenAction = new QAction(tr("Полноэкранный режим"), this);
    connect(fullscreenAction, &QAction::triggered, this, [this]() {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    });
    
    m_contextMenu->addAction(savePngAction);
    m_contextMenu->addAction(saveSvgAction);
    m_contextMenu->addAction(copyAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(resetZoomAction);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction(fullscreenAction);
}

void BenchmarkChartView::resetZoom()
{
    if (chart()) {
        chart()->zoomReset();
    }
}

void BenchmarkChartView::saveToFile(const QString &path)
{
    if (path.endsWith(".svg", Qt::CaseInsensitive)) {
        QSvgGenerator generator;
        generator.setFileName(path);
        generator.setSize(size());
        generator.setViewBox(rect());
        
        QPainter painter(&generator);
        render(&painter);
    } else {
        grab().save(path);
    }
}

void BenchmarkChartView::applyTheme(bool isDark)
{
    if (!chart()) return;
    
    if (isDark) {
        chart()->setTheme(QChart::ChartThemeDark);
    } else {
        chart()->setTheme(QChart::ChartThemeLight);
    }
}

void BenchmarkChartView::setTitle(const QString &title)
{
    if (chart()) {
        chart()->setTitle(title);
    }
}

void BenchmarkChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QChartView::mousePressEvent(event);
    }
}

void BenchmarkChartView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning && chart()) {
        QPoint delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();
        
        // Панорамирование через сдвиг осей
        for (auto *axis : chart()->axes()) {
            if (auto *valueAxis = qobject_cast<QValueAxis*>(axis)) {
                double min = valueAxis->min();
                double max = valueAxis->max();
                double range = max - min;
                
                if (axis->orientation() == Qt::Horizontal) {
                    double shift = -delta.x() / static_cast<double>(width()) * range;
                    valueAxis->setRange(min + shift, max + shift);
                } else {
                    double shift = delta.y() / static_cast<double>(height()) * range;
                    valueAxis->setRange(min + shift, max + shift);
                }
            }
        }
        event->accept();
    } else {
        QChartView::mouseMoveEvent(event);
    }
}

void BenchmarkChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QChartView::mouseReleaseEvent(event);
    }
}

void BenchmarkChartView::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {
        // Zoom in
        if (chart()) {
            chart()->zoom(1.1);
        }
    } else {
        // Zoom out
        if (chart()) {
            chart()->zoom(0.9);
        }
    }
    event->accept();
}

void BenchmarkChartView::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_contextMenu) {
        m_contextMenu->exec(event->globalPos());
    }
    event->accept();
}

} // namespace SortBench
