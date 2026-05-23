/**
 * @file sortingvisualizer.cpp
 * @brief Реализация виджета визуализации сортировки.
 */
#include "sortingvisualizer.h"
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include <algorithm>

SortingVisualizer::SortingVisualizer(QWidget* parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, &SortingVisualizer::onAnimationStep);
    generateNewArray(45);
}

SortingVisualizer::~SortingVisualizer() {
    m_timer->stop();
}

void SortingVisualizer::generateNewArray(int size) {
    m_timer->stop();
    m_isRunning = false;
    m_isPaused = false;
    m_data.clear();
    m_originalData.clear();
    m_activeIdx1 = -1;
    m_activeIdx2 = -1;
    m_pivotIdx = -1;
    m_sortedUpTo = -1;
    m_sortState = 0;
    m_opIndex = 0;
    m_operations.clear();
    
    for (int i = 0; i < size; ++i) {
        m_data.push_back(QRandomGenerator::global()->bounded(50, 280));
    }
    m_originalData = m_data;
    update();
}

void SortingVisualizer::startSort(const QString &algorithm, int speed) {
    m_currentAlgorithm = algorithm;
    setSpeed(speed);
    m_isRunning = true;
    m_isPaused = false;
    m_sortState = 0;
    m_opIndex = 0;
    m_operations.clear();
    
    // Генерация операций для выбранного алгоритма
    if (algorithm == "BubbleSort") {
        runBubbleSort();
    } else if (algorithm == "SelectionSort") {
        runSelectionSort();
    } else if (algorithm == "InsertionSort") {
        runInsertionSort();
    } else if (algorithm == "QuickSort") {
        runQuickSort();
    } else if (algorithm == "MergeSort") {
        runMergeSort();
    } else if (algorithm == "HeapSort") {
        runHeapSort();
    } else if (algorithm == "BitonicSort") {
        runBitonicSort();
    }
    
    if (!m_operations.isEmpty()) {
        m_timer->start(m_delay);
    }
}

void SortingVisualizer::pause() {
    m_isPaused = true;
    m_timer->stop();
}

void SortingVisualizer::resume() {
    if (m_isRunning && m_isPaused) {
        m_isPaused = false;
        m_timer->start(m_delay);
    }
}

void SortingVisualizer::setSpeed(int speed) {
    m_delay = qMax(2, 301 - speed * 3);
    if (m_isRunning && !m_isPaused) {
        m_timer->setInterval(m_delay);
    }
}

void SortingVisualizer::clear() {
    m_timer->stop();
    m_isRunning = false;
    m_activeIdx1 = -1;
    m_activeIdx2 = -1;
    m_pivotIdx = -1;
    update();
}

void SortingVisualizer::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int w = width();
    int h = height();
    int n = m_data.size();
    
    if (n == 0) return;
    
    int barWidth = (w - 20) / n;
    int maxVal = *std::max_element(m_data.begin(), m_data.end());
    
    for (int i = 0; i < n; ++i) {
        int barHeight = (m_data[i] * (h - 40)) / maxVal;
        int x = 10 + i * barWidth;
        int y = h - 20 - barHeight;
        
        // Цвет столбца
        QColor color;
        if (i == m_activeIdx1 || i == m_activeIdx2) {
            color = QColor(239, 68, 68); // Красный для сравниваемых
        } else if (i == m_pivotIdx) {
            color = QColor(251, 146, 60); // Оранжевый для опорного
        } else if (i <= m_sortedUpTo) {
            color = QColor(34, 197, 94); // Зеленый для отсортированных
        } else {
            color = QColor(59, 130, 246); // Синий по умолчанию
        }
        
        painter.setBrush(color);
        painter.setPen(color.darker(120));
        painter.drawRoundedRect(x + 1, y, barWidth - 2, barHeight, 3, 3);
    }
}

void SortingVisualizer::updateData(const std::vector<int>& newData, int active1, int active2, int pivot) {
    m_data = newData;
    m_activeIdx1 = active1;
    m_activeIdx2 = active2;
    m_pivotIdx = pivot;
    update();
}

void SortingVisualizer::onAnimationStep() {
    if (m_isPaused || m_opIndex >= m_operations.size()) {
        if (m_opIndex >= m_operations.size()) {
            m_timer->stop();
            m_isRunning = false;
            m_sortedUpTo = m_data.size() - 1;
            m_activeIdx1 = -1;
            m_activeIdx2 = -1;
            m_pivotIdx = -1;
            update();
        }
        return;
    }
    
    auto op = m_operations[m_opIndex++];
    int type = op.first;  // 0 = compare, 1 = swap
    int idx1 = op.second >> 16;
    int idx2 = op.second & 0xFFFF;
    
    if (type == 0) {
        // Сравнение
        m_activeIdx1 = idx1;
        m_activeIdx2 = idx2;
    } else if (type == 1) {
        // Обмен
        std::swap(m_data[idx1], m_data[idx2]);
        m_activeIdx1 = idx1;
        m_activeIdx2 = idx2;
    } else if (type == 2) {
        // Установка pivot
        m_pivotIdx = idx1;
    }
    
    update();
}

// Генерация операций для Bubble Sort
void SortingVisualizer::runBubbleSort() {
    int n = m_data.size();
    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - i - 1; ++j) {
            m_operations.append(qMakePair(0, (j << 16) | (j + 1))); // Compare
            if (m_data[j] > m_data[j + 1]) {
                m_operations.append(qMakePair(1, (j << 16) | (j + 1))); // Swap
            }
        }
    }
}

// Генерация операций для Selection Sort
void SortingVisualizer::runSelectionSort() {
    std::vector<int> temp = m_data;
    int n = temp.size();
    
    for (int i = 0; i < n - 1; ++i) {
        int minIdx = i;
        m_operations.append(qMakePair(2, (minIdx << 16) | 0)); // Pivot
        
        for (int j = i + 1; j < n; ++j) {
            m_operations.append(qMakePair(0, (j << 16) | (minIdx << 0)));
            if (temp[j] < temp[minIdx]) {
                minIdx = j;
                m_operations.append(qMakePair(2, (minIdx << 16) | 0));
            }
        }
        
        if (minIdx != i) {
            m_operations.append(qMakePair(1, (i << 16) | minIdx));
            std::swap(temp[i], temp[minIdx]);
        }
    }
    m_data = temp;
}

// Генерация операций для Insertion Sort
void SortingVisualizer::runInsertionSort() {
    std::vector<int> temp = m_data;
    int n = temp.size();
    
    for (int i = 1; i < n; ++i) {
        int key = temp[i];
        int j = i - 1;
        m_operations.append(qMakePair(2, (i << 16) | 0));
        
        while (j >= 0 && temp[j] > key) {
            m_operations.append(qMakePair(0, (j << 16) | (j + 1)));
            m_operations.append(qMakePair(1, (j << 16) | (j + 1)));
            temp[j + 1] = temp[j];
            j--;
        }
        temp[j + 1] = key;
    }
    m_data = temp;
}

// Генерация операций для Quick Sort
void SortingVisualizer::runQuickSort() {
    std::vector<int> temp = m_data;
    quickSortHelper(temp, 0, temp.size() - 1);
    m_data = temp;
}

void SortingVisualizer::quickSortHelper(std::vector<int>& arr, int low, int high) {
    if (low >= high) return;
    
    int pivot = arr[high];
    m_operations.append(qMakePair(2, (high << 16) | 0));
    
    int i = low - 1;
    for (int j = low; j < high; ++j) {
        m_operations.append(qMakePair(0, (j << 16) | (high << 0)));
        if (arr[j] < pivot) {
            i++;
            if (i != j) {
                m_operations.append(qMakePair(1, (i << 16) | j));
                std::swap(arr[i], arr[j]);
            }
        }
    }
    
    if (i + 1 != high) {
        m_operations.append(qMakePair(1, ((i + 1) << 16) | high));
        std::swap(arr[i + 1], arr[high]);
    }
    
    int pi = i + 1;
    quickSortHelper(arr, low, pi - 1);
    quickSortHelper(arr, pi + 1, high);
}

// Генерация операций для Merge Sort
void SortingVisualizer::runMergeSort() {
    std::vector<int> temp = m_data;
    mergeSortHelper(temp, 0, temp.size() - 1);
    m_data = temp;
}

void SortingVisualizer::mergeSortHelper(std::vector<int>& arr, int l, int r) {
    if (l >= r) return;
    
    int m = l + (r - l) / 2;
    mergeSortHelper(arr, l, m);
    mergeSortHelper(arr, m + 1, r);
    
    // Merge
    std::vector<int> L(arr.begin() + l, arr.begin() + m + 1);
    std::vector<int> R(arr.begin() + m + 1, arr.begin() + r + 1);
    
    int i = 0, j = 0, k = l;
    while (i < L.size() && j < R.size()) {
        m_operations.append(qMakePair(0, ((l + i) << 16) | (m + 1 + j)));
        if (L[i] <= R[j]) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
        m_operations.append(qMakePair(1, ((k - 1) << 16) | 0));
    }
    
    while (i < L.size()) {
        arr[k++] = L[i++];
        m_operations.append(qMakePair(1, ((k - 1) << 16) | 0));
    }
    
    while (j < R.size()) {
        arr[k++] = R[j++];
        m_operations.append(qMakePair(1, ((k - 1) << 16) | 0));
    }
}

// Генерация операций для Heap Sort
void SortingVisualizer::runHeapSort() {
    std::vector<int> temp = m_data;
    int n = temp.size();
    
    // Build heap
    for (int i = n / 2 - 1; i >= 0; --i) {
        heapify(temp, n, i);
    }
    
    // Extract elements
    for (int i = n - 1; i > 0; --i) {
        m_operations.append(qMakePair(1, (0 << 16) | i));
        std::swap(temp[0], temp[i]);
        heapify(temp, i, 0);
    }
    m_data = temp;
}

void SortingVisualizer::heapify(std::vector<int>& arr, int n, int i) {
    int largest = i;
    int l = 2 * i + 1;
    int r = 2 * i + 2;
    
    if (l < n) {
        m_operations.append(qMakePair(0, (l << 16) | (largest << 0)));
        if (arr[l] > arr[largest]) {
            largest = l;
        }
    }
    
    if (r < n) {
        m_operations.append(qMakePair(0, (r << 16) | (largest << 0)));
        if (arr[r] > arr[largest]) {
            largest = r;
        }
    }
    
    if (largest != i) {
        m_operations.append(qMakePair(1, (i << 16) | largest));
        std::swap(arr[i], arr[largest]);
        heapify(arr, n, largest);
    }
}

// Генерация операций для Bitonic Sort
void SortingVisualizer::runBitonicSort() {
    std::vector<int> temp = m_data;
    
    // Дополнить до степени 2
    int n = temp.size();
    int nextPow2 = 1;
    while (nextPow2 < n) nextPow2 *= 2;
    temp.resize(nextPow2, 300);
    
    bitonicSort(temp, 0, nextPow2, 1);
    
    temp.resize(n);
    m_data = temp;
}

void SortingVisualizer::bitonicSort(std::vector<int>& arr, int low, int cnt, int dir) {
    if (cnt <= 1) return;
    
    int k = cnt / 2;
    bitonicSort(arr, low, k, 1);
    bitonicSort(arr, low + k, k, 0);
    bitonicMerge(arr, low, cnt, dir);
}

void SortingVisualizer::bitonicMerge(std::vector<int>& arr, int low, int cnt, int dir) {
    if (cnt <= 1) return;
    
    int k = cnt / 2;
    for (int i = low; i < low + k; ++i) {
        m_operations.append(qMakePair(0, (i << 16) | (i + k)));
        if ((dir == 1 && arr[i] > arr[i + k]) || (dir == 0 && arr[i] < arr[i + k])) {
            m_operations.append(qMakePair(1, (i << 16) | (i + k)));
            std::swap(arr[i], arr[i + k]);
        }
    }
    
    bitonicMerge(arr, low, k, dir);
    bitonicMerge(arr, low + k, k, dir);
}
