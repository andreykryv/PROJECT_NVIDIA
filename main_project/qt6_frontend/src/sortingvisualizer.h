/**
 * @file sortingvisualizer.h
 * @brief Кастомный QWidget для отрисовки массива вертикальными столбцами с анимацией.
 */
#pragma once

#include <QWidget>
#include <vector>
#include <QTimer>
#include <QString>

class SortingVisualizer : public QWidget {
    Q_OBJECT

public:
    explicit SortingVisualizer(QWidget* parent = nullptr);
    ~SortingVisualizer();
    
    void generateNewArray(int size);
    void startSort(const QString &algorithm, int speed);
    void pause();
    void resume();
    void setSpeed(int speed);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onAnimationStep();

private:
    void updateData(const std::vector<int>& newData, int active1 = -1, int active2 = -1, int pivot = -1);
    void swapElements(int i, int j);
    
    // Алгоритмы сортировки
    void runBubbleSort();
    void runSelectionSort();
    void runInsertionSort();
    void runQuickSort();
    void runMergeSort();
    void runHeapSort();
    void runBitonicSort();
    
    // Вспомогательные функции для алгоритмов
    void quickSortHelper(std::vector<int>& arr, int low, int high);
    void mergeSortHelper(std::vector<int>& arr, int l, int r);
    void heapify(std::vector<int>& arr, int n, int i);
    void bitonicSort(std::vector<int>& arr, int low, int cnt, int dir);
    void bitonicMerge(std::vector<int>& arr, int low, int cnt, int dir);
    
    std::vector<int> m_data;
    std::vector<int> m_originalData;
    int m_activeIdx1 = -1;
    int m_activeIdx2 = -1;
    int m_pivotIdx = -1;
    int m_sortedUpTo = -1;
    
    QTimer *m_timer;
    int m_delay = 30;
    bool m_isRunning = false;
    bool m_isPaused = false;
    QString m_currentAlgorithm;
    
    // Для итеративных алгоритмов
    int m_sortState = 0;
    int m_i = 0, m_j = 0, m_key = 0;
    QVector<QPair<int, int>> m_operations;
    int m_opIndex = 0;
};
