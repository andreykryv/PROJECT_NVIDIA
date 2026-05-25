/**
 * @file interactive_visualizer.h
 * @brief Отдельное окно для интерактивной визуализации всех алгоритмов сортировки.
 * 
 * Поддерживает 20 CPU алгоритмов и 6 GPU алгоритмов (с ограничением – GPU визуализация недоступна).
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <QDialog>
#include <QThread>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <atomic>
#include <vector>
#include "sorting_visualizer.h"

class InteractiveVisualizerWindow : public QDialog {
    Q_OBJECT

public:
    explicit InteractiveVisualizerWindow(QWidget *parent = nullptr);
    ~InteractiveVisualizerWindow();

private slots:
    void onGenerateArray();
    void onStartSort();
    void onPauseResume();
    void onStopSort();
    void onSpeedChanged(int value);
    void onAlgorithmChanged(int index);

private:
    void setupUI();
    void loadAlgorithms();                     // Загрузить все CPU + GPU алгоритмы
    void runCpuSort(const QString& algKey);    // Запуск CPU сортировки с визуализацией
    void disableControlsDuringSort(bool disable);
    void updateStatus(const QString& text, bool isError = false);

    // Виджеты
    SortingVisualizer* m_visualizer = nullptr;
    QComboBox*        m_algCombo = nullptr;
    QSpinBox*         m_sizeSpin = nullptr;
    QPushButton*      m_genBtn = nullptr;
    QPushButton*      m_startBtn = nullptr;
    QPushButton*      m_pauseBtn = nullptr;
    QPushButton*      m_stopBtn = nullptr;
    QSlider*          m_speedSlider = nullptr;
    QLabel*           m_statusLabel = nullptr;

    // Состояние
    std::vector<double> m_arrayData;            // Текущий массив для визуализации
    std::atomic<bool>   m_stopRequested;
    std::atomic<bool>   m_isPaused;
    std::atomic<int>    m_delayMs;              // Задержка для анимации
    QThread*            m_sortThread = nullptr;

    // Флаг, указывающий, что выбран GPU алгоритм (визуализация не поддерживается)
    bool m_isGpuAlgorithm = false;
};