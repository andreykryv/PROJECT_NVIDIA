/**
 * @file sorting_visualizer.cpp
 * @brief Реализация custom-виджета визуализации.
 * Вычисляет пропорции столбцов, осуществляет отрисовку с учетом подсветки активных зон.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sorting_visualizer.h"
#include <algorithm>
#include <QSettings>

SortingVisualizer::SortingVisualizer(QWidget* parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void SortingVisualizer::updateData(const std::vector<double>& newData, int active1, int active2, int pivot) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data = newData;
    m_activeIdx1 = active1;
    m_activeIdx2 = active2;
    m_pivotIdx = pivot;

    if (!m_data.empty()) {
        auto [minIt, maxIt] = std::minmax_element(m_data.begin(), m_data.end());
        m_minValue = *minIt;
        m_maxValue = *maxIt;
        if (m_minValue == m_maxValue) {
            m_minValue -= 1.0;
            m_maxValue += 1.0;
        }
    }
    
    update();
}

void SortingVisualizer::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.clear();
    m_activeIdx1 = -1;
    m_activeIdx2 = -1;
    m_pivotIdx = -1;
    update();
}

void SortingVisualizer::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    
    painter.fillRect(rect(), QColor(15, 15, 18));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_data.empty()) {
        painter.setPen(QColor(113, 113, 122));
        painter.drawText(rect(), Qt::AlignCenter, "Массив пуст. Сгенерируйте данные и запустите сортировку.");
        return;
    }

    // Читаем настройки визуализации
    QSettings s;
    QString theme = s.value("visual/theme", "Classic Blue").toString();
    bool antialiasing = s.value("visual/antialiasing", true).toBool();
    painter.setRenderHint(QPainter::Antialiasing, antialiasing);

    int n = m_data.size();
    double w = width();
    double h = height();
    double barWidth = w / n;

    for (int i = 0; i < n; ++i) {
        double val = m_data[i];
        
        double normVal = (val - m_minValue) / (m_maxValue - m_minValue);
        double barHeight = normVal * (h - 20) + 5;
        
        double x = i * barWidth;
        double y = h - barHeight;

        QColor barColor;
        if (i == m_pivotIdx) {
            barColor = QColor(234, 179, 8); // Золотисто-желтый для Pivot
        } else if (i == m_activeIdx1 || i == m_activeIdx2) {
            barColor = QColor(239, 68, 68); // Красный для сравниваемых элементов
        } else {
            // Динамический выбор цвета в зависимости от выбранной темы в настройках
            if (theme == "Emerald Green") {
                int factor = static_cast<int>(normVal * 100);
                barColor = QColor(16, 185 - factor, 129, 210 + factor / 4);
            } else if (theme == "Synthwave Pink") {
                int factor = static_cast<int>(normVal * 120);
                barColor = QColor(236, 72, 153 - factor, 210 + factor / 4);
            } else if (theme == "Monochrome") {
                int factor = static_cast<int>(normVal * 150);
                barColor = QColor(60 + factor, 60 + factor, 60 + factor);
            } else { // Classic Blue
                int factor = static_cast<int>(normVal * 120);
                barColor = QColor(37, 99, 235 - factor, 200 + factor / 4);
            }
        }

        QRectF barRect(x, y, std::max(1.0, barWidth - 1.0), barHeight);
        painter.fillRect(barRect, barColor);

        if (n < 60) {
            painter.setPen(QColor(241, 245, 249, 100));
            painter.drawLine(QPointF(x, y), QPointF(x + std::max(1.0, barWidth - 1.0), y));
        }
    }
}