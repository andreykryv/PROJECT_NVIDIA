#ifndef SORTBENCHENGINE_H
#define SORTBENCHENGINE_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>
#include <QAtomicInteger>
#include <QPointer>
#include <vector>
#include <memory>
#include "sortparams.h"
#include "benchmarkresult.h"
#include "cpusorter.h"
#include "arraygenerator.h"

#ifdef USE_CUDA
#include "cudasorter.h"
#endif

namespace SortBench {

// Forward declarations
#ifndef USE_CUDA
class CudaSorter;
#endif

// GpuTimings forward declaration for non-CUDA builds
#ifndef USE_CUDA
struct GpuTimings {
    double h2dMs = 0.0;
    double kernelMs = 0.0;
    double d2hMs = 0.0;
    double syncMs = 0.0;
    double totalMs() const { return h2dMs + kernelMs + d2hMs + syncMs; }
};
#endif

// Кадр визуализации
struct VisFrame {
    std::vector<float> normalizedValues;  // значения в [0, 1]
    std::vector<float> values;            // алиас для обратной совместимости
    QList<int> highlightedIdx;             // индексы сравниваемых элементов
    HighlightType highlightType;           // тип подсветки
    long long comparisons;                 // счётчик сравнений
    long long swaps;                       // счётчик перестановок
    long long arrayAccesses;               // счётчик обращений к массиву
    int totalElements;                     // всего элементов
    QString algoName;                      // имя алгоритма

    VisFrame() : highlightType(HighlightType::None), comparisons(0), swaps(0), arrayAccesses(0), totalElements(0) {}
};

class SortBenchEngine : public QObject {
    Q_OBJECT


public:
    enum class State {
        Idle,
        GeneratingArray,
        SortingCPU,
        SortingGPU,
        TransferringH2D,
        TransferringD2H,
        Verifying,
        Paused,
        Finished,
        Stopped
    };
    Q_ENUM(State)

    explicit SortBenchEngine(QObject *parent = nullptr);
    ~SortBenchEngine();

    SortParams currentParams() const { return m_currentParams; }
    State state() const { return m_state; }
    bool isRunning() const { return m_state != State::Idle && m_state != State::Finished && m_state != State::Stopped; }

public slots:
    void startBenchmark(const SortParams& params);
    void stopBenchmark();
    void pauseBenchmark();
    void resumeBenchmark();

signals:
    void progressUpdated(int percent, const QString& phase);
    void frameReady(const VisFrame& frame);
    void benchmarkFinished(const BenchmarkResult& result);
    void benchmarkStopped();
    void logMessage(const QString& message, int level);  // 0=info, 1=warn, 2=error
    void gpuMemoryUpdated(size_t used, size_t total);
    void stateChanged(State state);

private slots:
    void pollGpuMemory();

private:
    void setState(State newState);
    void generateArray();
    void runCpuSort();
    void runGpuSort();
    void verifyResults();
    void collectSystemInfo();

    template<typename T>
    void emitFrame(const std::vector<T>& data, const QList<int>& highlighted,
                   HighlightType type, long long cmp, long long swaps);

    void checkPausePoint();

    SortParams m_currentParams;
    State m_state = State::Idle;
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_pauseRequested{false};

    CpuSorter* m_cpuSorter = nullptr;
    CudaSorter* m_gpuSorter = nullptr;

    QElapsedTimer m_stageTimer;
    BenchmarkResult m_partialResult;
    QTimer* m_gpuMemPollTimer = nullptr;

    // Данные для сортировки
    std::vector<uint8_t> m_originalData;  // сырые данные
    std::vector<uint8_t> m_cpuResult;
    std::vector<uint8_t> m_gpuResult;
    size_t m_elementSize = 0;

    // Для анимации
    qint64 m_lastFrameTime = 0;
    int m_frameIntervalMs = 16;  // ~60 FPS
};

} // namespace SortBench

#endif // SORTBENCHENGINE_H