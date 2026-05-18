#include "statspanel.h"
#include <QColor>
#include <QFont>
#include <QScrollArea>

namespace SortBench {

StatsPanel::StatsPanel(QWidget *parent)
    : QWidget(parent)
{
    createUI();
    connectSignals();
}

void StatsPanel::createUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // === Секция "Последний результат" ===
    m_lastResultGroup = new QGroupBox(tr("Последний результат"), this);
    auto *lastResultLayout = new QVBoxLayout(m_lastResultGroup);

    m_cpuTimeLabel = new QLabel(tr("CPU время: -- мс"), this);
    m_gpuTimeLabel = new QLabel(tr("GPU время: -- мс"), this);
    m_gpuKernelLabel = new QLabel(tr("GPU kernel: -- мс"), this);
    m_gpuH2DLabel = new QLabel(tr("Передача H→D: -- мс"), this);
    m_gpuD2HLabel = new QLabel(tr("Передача D→H: -- мс"), this);
    m_speedupLabel = new QLabel(tr("Ускорение GPU: --"), this);
    m_speedupLabel->setFont(QFont(font().family(), font().pointSize(), QFont::Bold));
    m_algorithmLabel = new QLabel(tr("Алгоритм: --"), this);
    m_arraySizeLabel = new QLabel(tr("Размер массива: --"), this);

    lastResultLayout->addWidget(m_cpuTimeLabel);
    lastResultLayout->addWidget(m_gpuTimeLabel);
    lastResultLayout->addWidget(m_gpuKernelLabel);
    lastResultLayout->addWidget(m_gpuH2DLabel);
    lastResultLayout->addWidget(m_gpuD2HLabel);
    lastResultLayout->addWidget(m_speedupLabel);
    lastResultLayout->addWidget(m_algorithmLabel);
    lastResultLayout->addWidget(m_arraySizeLabel);
    lastResultLayout->addStretch();

    mainLayout->addWidget(m_lastResultGroup);

    // === Секция "Память GPU" ===
    m_gpuMemoryGroup = new QGroupBox(tr("Память GPU"), this);
    auto *gpuMemoryLayout = new QVBoxLayout(m_gpuMemoryGroup);

    m_vramProgressBar = new QProgressBar(this);
    m_vramProgressBar->setRange(0, 100);
    m_vramProgressBar->setValue(0);
    m_vramProgressBar->setFormat("%p%");

    m_vramUsageLabel = new QLabel(tr("Использовано: -- МБ / -- МБ"), this);
    m_deviceNameLabel = new QLabel(tr("Устройство: --"), this);

    gpuMemoryLayout->addWidget(m_vramProgressBar);
    gpuMemoryLayout->addWidget(m_vramUsageLabel);
    gpuMemoryLayout->addWidget(m_deviceNameLabel);
    gpuMemoryLayout->addStretch();

    mainLayout->addWidget(m_gpuMemoryGroup);

    // === Секция "История" ===
    m_historyGroup = new QGroupBox(tr("История"), this);
    auto *historyLayout = new QVBoxLayout(m_historyGroup);

    m_historyList = new QListWidget(this);
    m_historyList->setAlternatingRowColors(true);
    m_historyList->setSelectionMode(QAbstractItemView::SingleSelection);

    historyLayout->addWidget(m_historyList);

    mainLayout->addWidget(m_historyGroup);
}

void StatsPanel::connectSignals()
{
    connect(m_historyList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        QVariant data = item->data(Qt::UserRole);
        if (data.canConvert<BenchmarkResult>()) {
            emit resultSelected(data.value<BenchmarkResult>());
        }
    });
}

void StatsPanel::updateResult(const BenchmarkResult &result)
{
    // Обновление меток последнего результата
    m_cpuTimeLabel->setText(tr("CPU время: %1").arg(formatTime(result.cpuTimeMs)));
    m_gpuTimeLabel->setText(tr("GPU время: %1").arg(formatTime(result.gpuTotalTimeMs)));
    m_gpuKernelLabel->setText(tr("GPU kernel: %1").arg(formatTime(result.gpuKernelTimeMs)));
    m_gpuH2DLabel->setText(tr("Передача H→D: %1").arg(formatTime(result.gpuH2DTimeMs)));
    m_gpuD2HLabel->setText(tr("Передача D→H: %1").arg(formatTime(result.gpuD2HTimeMs)));

    double speedup = result.speedup();
    m_speedupLabel->setText(tr("Ускорение GPU: %1").arg(formatSpeedup(speedup)));
    m_speedupLabel->setStyleSheet(QString("color: %1; font-weight: bold;").arg(speedupColor(speedup).name()));

    QString cpuAlgo = toString(result.params.cpuAlgorithm);
    QString gpuAlgo = toString(result.params.gpuAlgorithm);
    m_algorithmLabel->setText(tr("Алгоритм: %1 (CPU) vs %2 (GPU)").arg(cpuAlgo, gpuAlgo));

    size_t arrayBytes = static_cast<size_t>(result.params.arraySize) * elementSize(result.params.dataType);
    double arrayMB = static_cast<double>(arrayBytes) / (1024.0 * 1024.0);
    m_arraySizeLabel->setText(tr("Размер массива: %1 элементов (%2 МБ)")
        .arg(result.params.arraySize)
        .arg(QString::number(arrayMB, 'f', 2)));

    // Добавление в историю
    QString historyText = QString("%1(CPU) %2мс vs %3(GPU) %4мс (%5x)")
        .arg(toString(result.params.cpuAlgorithm))
        .arg(QString::number(result.cpuTimeMs, 'f', 1))
        .arg(toString(result.params.gpuAlgorithm))
        .arg(QString::number(result.gpuTotalTimeMs, 'f', 1))
        .arg(QString::number(speedup, 'f', 1));

    auto *item = new QListWidgetItem(historyText, m_historyList);
    item->setData(Qt::UserRole, QVariant::fromValue(result));
    m_historyList->addItem(item);

    // Ограничение истории
    while (m_historyList->count() > m_maxHistory) {
        delete m_historyList->takeItem(0);
    }
}

void StatsPanel::updateGPUMemory(size_t used, size_t total)
{
    if (total == 0) {
        m_vramProgressBar->setValue(0);
        m_vramUsageLabel->setText(tr("Использовано: -- МБ / -- МБ"));
        return;
    }

    double usedMB = static_cast<double>(used) / (1024.0 * 1024.0);
    double totalMB = static_cast<double>(total) / (1024.0 * 1024.0);
    int percent = static_cast<int>((static_cast<double>(used) / total) * 100.0);

    m_vramProgressBar->setValue(percent);
    m_vramUsageLabel->setText(tr("Использовано: %1 МБ / %2 МБ")
        .arg(QString::number(usedMB, 'f', 1))
        .arg(QString::number(totalMB, 'f', 1)));
}

void StatsPanel::clearHistory()
{
    m_historyList->clear();
}

QString StatsPanel::formatTime(double ms) const
{
    if (ms >= 1000.0) {
        return QString("%1 с").arg(QString::number(ms / 1000.0, 'f', 1));
    } else if (ms >= 1.0) {
        return QString("%1 мс").arg(QString::number(ms, 'f', 3));
    } else if (ms > 0.0) {
        return QString("%1 мкс").arg(QString::number(ms * 1000.0, 'f', 1));
    } else {
        return tr("-- мс");
    }
}

QString StatsPanel::formatBytes(size_t bytes) const
{
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;

    if (bytes >= static_cast<size_t>(GB)) {
        return QString("%1 ГБ").arg(QString::number(bytes / GB, 'f', 2));
    } else if (bytes >= static_cast<size_t>(MB)) {
        return QString("%1 МБ").arg(QString::number(bytes / MB, 'f', 2));
    } else if (bytes >= static_cast<size_t>(KB)) {
        return QString("%1 КБ").arg(QString::number(bytes / KB, 'f', 1));
    } else {
        return QString("%1 Б").arg(bytes);
    }
}

QString StatsPanel::formatSpeedup(double x) const
{
    if (x <= 0.0) {
        return tr("--");
    }
    return QString("%1x").arg(QString::number(x, 'f', 2));
}

QColor StatsPanel::speedupColor(double x) const
{
    if (x > 10.0) {
        return QColor(0, 200, 0);   // ярко-зелёный
    } else if (x > 2.0) {
        return QColor(0, 150, 0);   // зелёный
    } else if (x > 1.0) {
        return QColor(200, 200, 0); // жёлтый
    } else {
        return QColor(200, 0, 0);   // красный
    }
}

} // namespace SortBench
