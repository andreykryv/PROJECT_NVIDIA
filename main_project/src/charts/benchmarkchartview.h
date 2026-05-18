#ifndef BENCHMARKCHARTVIEW_H
#define BENCHMARKCHARTVIEW_H

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QMenu>
#include <QAction>

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QWheelEvent;
class QContextMenuEvent;
QT_END_NAMESPACE

namespace SortBench {

class BenchmarkChartView : public QChartView
{
    Q_OBJECT

public:
    explicit BenchmarkChartView(QWidget *parent = nullptr);
    ~BenchmarkChartView() override = default;

    void resetZoom();
    void saveToFile(const QString &path);
    void applyTheme(bool isDark);
    void setTitle(const QString &title);

signals:
    void seriesClicked(const QString &seriesName);
    void pointHovered(QPointF point, const QString &seriesName);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void createActions();
    
    bool m_isPanning = false;
    QPoint m_lastPanPoint;
    QMenu *m_contextMenu = nullptr;
};

} // namespace SortBench

#endif // BENCHMARKCHARTVIEW_H
