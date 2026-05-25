/**
 * @file sorting_visualizer.h
 * @brief Кастомный виджет отображения процесса сортировки в реальном времени.
 * Использует QPainter для рисования элементов массива в виде вертикальных столбцов.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <QWidget>
#include <QPainter>
#include <QColor>
#include <vector>
#include <mutex>

class SortingVisualizer : public QWidget {
    Q_OBJECT

public:
    explicit SortingVisualizer(QWidget* parent = nullptr);

    /**
     * @brief Обновить данные для отрисовки.
     * @param newData Новый массив элементов.
     * @param active1 Индекс первого активного элемента (подсветка красным/синим).
     * @param active2 Индекс второго активного элемента.
     * @param pivot Индекс опорного элемента (подсветка золотистым).
     */
    void updateData(const std::vector<double>& newData, int active1 = -1, int active2 = -1, int pivot = -1);

    /**
     * @brief Сбросить состояние визуалайзера.
     */
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<double> m_data;
    int m_activeIdx1 = -1;
    int m_activeIdx2 = -1;
    int m_pivotIdx = -1;
    
    std::mutex m_mutex; // Синхронизация между потоком вычислений и GUI-потоком отрисовки
    
    double m_minValue = 0.0;
    double m_maxValue = 1.0;
};
