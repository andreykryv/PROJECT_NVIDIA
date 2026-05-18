#include "progresspanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolTip>

namespace SortBench {

// === CpuGpuSplitBar ===

CpuGpuSplitBar::CpuGpuSplitBar(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(40);
}

void CpuGpuSplitBar::setCpuGpuTimes(double cpuMs, double gpuMs)
{
    m_cpuMs = cpuMs;
    m_gpuMs = gpuMs;
    update();
}

void CpuGpuSplitBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    
    // Если нет данных - рисуем серый прямоугольник
    if (m_cpuMs <= 0.0 && m_gpuMs <= 0.0) {
        painter.fillRect(rect(), QColor(200, 200, 200));
        painter.setPen(Qt::black);
        painter.drawText(rect(), Qt::AlignCenter, tr("Нет данных"));
        return;
    }
    
    double total = m_cpuMs + m_gpuMs;
    double ratio = m_cpuMs / total;
    
    int cpuWidth = static_cast<int>(ratio * w);
    int gpuWidth = w - cpuWidth;
    
    // Синий прямоугольник для CPU
    QRect cpuRect(0, 0, cpuWidth, h);
    painter.fillRect(cpuRect, QColor(52, 152, 219));  // синий
    
    // Зелёный прямоугольник для GPU
    QRect gpuRect(cpuWidth, 0, gpuWidth, h);
    painter.fillRect(gpuRect, QColor(39, 174, 96));   // зелёный
    
    // Граница между участками
    painter.setPen(QPen(Qt::white, 2));
    painter.drawLine(cpuWidth, 0, cpuWidth, h);
    
    // Подписи внутри участков
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setBold(true);
    painter.setFont(font);
    
    // CPU текст
    QString cpuText = QString("CPU: %1 мс").arg(QString::number(m_cpuMs, 'f', 1));
    QRect cpuTextRect = cpuRect.adjusted(5, 0, -5, 0);
    painter.drawText(cpuTextRect, Qt::AlignLeft | Qt::AlignVCenter, cpuText);
    
    // GPU текст
    QString gpuText = QString("GPU: %1 мс").arg(QString::number(m_gpuMs, 'f', 1));
    QRect gpuTextRect = gpuRect.adjusted(5, 0, -5, 0);
    painter.drawText(gpuTextRect, Qt::AlignRight | Qt::AlignVCenter, gpuText);
}

// === ProgressPanel ===

ProgressPanel::ProgressPanel(QWidget *parent)
    : QWidget(parent)
{
    createUI();
    
    m_elapsedTimer = new QElapsedTimer(this);
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(100);
    
    connect(m_updateTimer, &QTimer::timeout, this, &ProgressPanel::updateElapsed);
}

void ProgressPanel::createUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    
    // Прогресс-бар
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressBar);
    
    // Фаза
    m_phaseLabel = new QLabel(tr("Готов к запуску"), this);
    QFont phaseFont = m_phaseLabel->font();
    phaseFont.setBold(true);
    m_phaseLabel->setFont(phaseFont);
    mainLayout->addWidget(m_phaseLabel);
    
    // Строка с elapsed и ETA
    auto *timeLayout = new QHBoxLayout();
    m_elapsedLabel = new QLabel(tr("Elapsed: 0.000 s"), this);
    m_etaLabel = new QLabel(tr("ETA: --"), this);
    timeLayout->addWidget(m_elapsedLabel);
    timeLayout->addWidget(m_etaLabel);
    timeLayout->addStretch();
    mainLayout->addLayout(timeLayout);
    
    // Скорость
    m_speedLabel = new QLabel(tr("Скорость: --"), this);
    mainLayout->addWidget(m_speedLabel);
    
    // CPU/GPU Split Bar
    m_cpuGpuSplitBar = new CpuGpuSplitBar(this);
    m_cpuGpuSplitBar->setMinimumHeight(50);
    mainLayout->addWidget(m_cpuGpuSplitBar);
    
    mainLayout->addStretch();
}

void ProgressPanel::setArraySize(int size)
{
    m_arraySize = size;
}

void ProgressPanel::setProgress(int percent)
{
    m_progressBar->setValue(percent);
}

void ProgressPanel::setPhase(const QString &phase)
{
    m_phaseLabel->setText(phase);
}

void ProgressPanel::setBenchmarkStarted()
{
    m_elapsedTimer->restart();
    m_updateTimer->start();
    m_cpuMs = 0.0;
    m_gpuMs = 0.0;
    m_cpuGpuSplitBar->setCpuGpuTimes(0.0, 0.0);
    updateElapsed();
}

void ProgressPanel::setBenchmarkFinished(double cpuMs, double gpuMs)
{
    m_updateTimer->stop();
    m_cpuMs = cpuMs;
    m_gpuMs = gpuMs;
    m_cpuGpuSplitBar->setCpuGpuTimes(cpuMs, gpuMs);
    
    // Финальное обновление elapsed
    double elapsedSec = m_elapsedTimer->elapsed() / 1000.0;
    m_elapsedLabel->setText(tr("Elapsed: %1 s").arg(QString::number(elapsedSec, 'f', 3)));
    
    // Расчёт скорости
    long long elementsPerSec = static_cast<long long>(m_arraySize / elapsedSec);
    m_speedLabel->setText(tr("Скорость: %1").arg(formatSpeed(elementsPerSec)));
    
    m_etaLabel->setText(tr("ETA: завершено"));
}

void ProgressPanel::reset()
{
    m_progressBar->setValue(0);
    m_phaseLabel->setText(tr("Готов к запуску"));
    m_elapsedLabel->setText(tr("Elapsed: 0.000 s"));
    m_etaLabel->setText(tr("ETA: --"));
    m_speedLabel->setText(tr("Скорость: --"));
    m_cpuGpuSplitBar->setCpuGpuTimes(0.0, 0.0);
    m_updateTimer->stop();
    m_cpuMs = 0.0;
    m_gpuMs = 0.0;
}

void ProgressPanel::updateElapsed()
{
    double elapsedSec = m_elapsedTimer->elapsed() / 1000.0;
    m_elapsedLabel->setText(tr("Elapsed: %1 s").arg(QString::number(elapsedSec, 'f', 3)));
    
    // Обновление скорости в реальном времени
    if (elapsedSec > 0.0) {
        long long elementsPerSec = static_cast<long long>(m_arraySize / elapsedSec);
        m_speedLabel->setText(tr("Скорость: %1").arg(formatSpeed(elementsPerSec)));
    }
}

QString ProgressPanel::formatSpeed(long long elementsPerSec) const
{
    const long long K = 1000;
    const long long M = K * 1000;
    const long long G = M * 1000;
    
    if (elementsPerSec >= G) {
        return QString("%1 G элем/с").arg(QString::number(static_cast<double>(elementsPerSec) / G, 'f', 2));
    } else if (elementsPerSec >= M) {
        return QString("%1 M элем/с").arg(QString::number(static_cast<double>(elementsPerSec) / M, 'f', 2));
    } else if (elementsPerSec >= K) {
        return QString("%1 K элем/с").arg(QString::number(static_cast<double>(elementsPerSec) / K, 'f', 1));
    } else {
        return QString("%1 элем/с").arg(elementsPerSec);
    }
}

} // namespace SortBench
