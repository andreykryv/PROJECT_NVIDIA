/**
 * @file benchmark_runner.h
 * @brief Описание потокового класса-раннера бенчмарков.
 * Наследуется от QThread. Выполняет генерацию массивов и поочередный безопасный запуск
 * выбранных алгоритмов во вспомогательном потоке, собирая статистику.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <QThread>
#include <QString>
#include <vector>
#include <atomic>
#include <mutex>

namespace Benchmark {

    enum class Distribution {
        Uniform,            // Равномерное случайное
        Normal,             // Нормальное (Гауссово)
        ReverseSorted,      // Обратная сортировка
        AlmostSorted,       // Почти отсортировано
        AllEqual            // Все элементы равны
    };

    /**
     * @brief Полная статистика прогона бенчмарка.
     */
    struct StatResults {
        QString algorithmName;
        bool isGPU;
        int arraySize;
        double minTimeMs;
        double maxTimeMs;
        double avgTimeMs;
        double medianTimeMs;
        double varianceMs;
        double avgUploadTimeMs;   // Для GPU
        double avgDownloadTimeMs; // Для GPU
        double avgKernelTimeMs;   // Для GPU
        bool success;
        QString errorMsg;
    };

    /**
     * @brief Основная конфигурация запуска тестов.
     */
    struct Config {
        std::vector<QString> selectedAlgorithms; // Имена выбранных алгоритмов
        int arraySize = 1000;
        Distribution dist = Distribution::Uniform;
        int runsCount = 5;                       // Кол-во повторных запусков для усреднения
        bool isDoublePrecision = true;
    };

} // namespace Benchmark

class BenchmarkRunner : public QThread {
    Q_OBJECT

public:
    explicit BenchmarkRunner(QObject* parent = nullptr);
    ~BenchmarkRunner();

    /**
     * @brief Настройка конфигурации тестов.
     */
    void setConfig(const Benchmark::Config& cfg);

    /**
     * @brief Запрос прерывания работы.
     */
    void requestStop();

signals:
    /**
     * @brief Сигнал отправки результатов одного завершенного алгоритма.
     */
    void algorithmCompleted(const Benchmark::StatResults& results);

    /**
     * @brief Сигнал изменения общего прогресса сборки бенчмарков (0-100%).
     */
    void progressUpdated(int percent);

    /**
     * @brief Сигнал полного завершения всех тестов бенчмарка.
     */
    void finishedAll();

protected:
    void run() override;

private:
    Benchmark::Config m_config;
    std::atomic<bool> m_stopRequested;
    std::mutex m_mutex;

    /**
     * @brief Метод генерации исходного массива по заданному закону распределения.
     */
    std::vector<double> generateData(int size, Benchmark::Distribution dist);
};
