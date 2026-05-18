#ifndef BENCHMARKRESULT_H
#define BENCHMARKRESULT_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QUuid>
#include "sortparams.h"

namespace SortBench {

struct BenchmarkResult {
    // Идентификация
    QString id;
    QDateTime timestamp;
    SortParams params;

    // Временные метрики (миллисекунды)
    double cpuTimeMs = 0.0;
    double gpuTotalTimeMs = 0.0;
    double gpuKernelTimeMs = 0.0;
    double gpuH2DTimeMs = 0.0;
    double gpuD2HTimeMs = 0.0;
    double gpuSyncOverheadMs = 0.0;
    double arrayGenerationTimeMs = 0.0;

    // Операционные счётчики
    long long cpuComparisons = 0;
    long long cpuSwaps = 0;
    long long cpuArrayAccesses = 0;
    long long gpuKernelLaunches = 0;

    // Корректность
    bool isSorted = false;
    bool cpuGpuMatch = false;

    // Системная информация
    QString cpuName;
    QString gpuName;
    int gpuComputeCapMajor = 0;
    int gpuComputeCapMinor = 0;
    size_t gpuVramUsedBytes = 0;
    QString cudaVersion;
    QString qtVersion;

    BenchmarkResult() {
        id = QUuid::createUuid().toString();
        timestamp = QDateTime::currentDateTime();
    }

    // Производные методы
    double speedup() const {
        if (gpuTotalTimeMs <= 0.0) return 0.0;
        return cpuTimeMs / gpuTotalTimeMs;
    }

    double gpuOnlySpeedup() const {
        if (gpuKernelTimeMs <= 0.0) return 0.0;
        return cpuTimeMs / gpuKernelTimeMs;
    }

    double throughputCPU() const {
        if (cpuTimeMs <= 0.0) return 0.0;
        return (static_cast<double>(params.arraySize) / cpuTimeMs) * 1000.0; // M elem/s
    }

    double throughputGPU() const {
        if (gpuTotalTimeMs <= 0.0) return 0.0;
        return (static_cast<double>(params.arraySize) / gpuTotalTimeMs) * 1000.0; // M elem/s
    }

    QString summary() const {
        QString cpuAlgo = toString(params.cpuAlgorithm);
        QString gpuAlgo = toString(params.gpuAlgorithm);
        QString sizeStr;
        if (params.arraySize >= 1000000) {
            sizeStr = QString::number(params.arraySize / 1000000) + "M";
        } else if (params.arraySize >= 1000) {
            sizeStr = QString::number(params.arraySize / 1000) + "K";
        } else {
            sizeStr = QString::number(params.arraySize);
        }
        return QString("%1(CPU) %2мс vs %3(GPU) %4мс → %5x @ %6 %7")
            .arg(cpuAlgo)
            .arg(QString::number(cpuTimeMs, 'f', 1))
            .arg(gpuAlgo)
            .arg(QString::number(gpuTotalTimeMs, 'f', 1))
            .arg(QString::number(speedup(), 'f', 2))
            .arg(sizeStr)
            .arg(toString(params.dataType));
    }

    // Сериализация
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["timestamp"] = timestamp.toString(Qt::ISODate);
        obj["cpuTimeMs"] = cpuTimeMs;
        obj["gpuTotalTimeMs"] = gpuTotalTimeMs;
        obj["gpuKernelTimeMs"] = gpuKernelTimeMs;
        obj["gpuH2DTimeMs"] = gpuH2DTimeMs;
        obj["gpuD2HTimeMs"] = gpuD2HTimeMs;
        obj["gpuSyncOverheadMs"] = gpuSyncOverheadMs;
        obj["arrayGenerationTimeMs"] = arrayGenerationTimeMs;
        obj["cpuComparisons"] = static_cast<qint64>(cpuComparisons);
        obj["cpuSwaps"] = static_cast<qint64>(cpuSwaps);
        obj["cpuArrayAccesses"] = static_cast<qint64>(cpuArrayAccesses);
        obj["gpuKernelLaunches"] = static_cast<qint64>(gpuKernelLaunches);
        obj["isSorted"] = isSorted;
        obj["cpuGpuMatch"] = cpuGpuMatch;
        obj["cpuName"] = cpuName;
        obj["gpuName"] = gpuName;
        obj["gpuComputeCapMajor"] = gpuComputeCapMajor;
        obj["gpuComputeCapMinor"] = gpuComputeCapMinor;
        obj["gpuVramUsedBytes"] = static_cast<qint64>(gpuVramUsedBytes);
        obj["cudaVersion"] = cudaVersion;
        obj["qtVersion"] = qtVersion;
        obj["arraySize"] = params.arraySize;
        obj["cpuAlgorithm"] = static_cast<int>(params.cpuAlgorithm);
        obj["gpuAlgorithm"] = static_cast<int>(params.gpuAlgorithm);
        obj["dataType"] = static_cast<int>(params.dataType);
        obj["distribution"] = static_cast<int>(params.distribution);
        return obj;
    }

    static BenchmarkResult fromJson(const QJsonObject& json) {
        BenchmarkResult result;
        result.id = json["id"].toString();
        result.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
        result.cpuTimeMs = json["cpuTimeMs"].toDouble(0.0);
        result.gpuTotalTimeMs = json["gpuTotalTimeMs"].toDouble(0.0);
        result.gpuKernelTimeMs = json["gpuKernelTimeMs"].toDouble(0.0);
        result.gpuH2DTimeMs = json["gpuH2DTimeMs"].toDouble(0.0);
        result.gpuD2HTimeMs = json["gpuD2HTimeMs"].toDouble(0.0);
        result.gpuSyncOverheadMs = json["gpuSyncOverheadMs"].toDouble(0.0);
        result.arrayGenerationTimeMs = json["arrayGenerationTimeMs"].toDouble(0.0);
        result.cpuComparisons = json["cpuComparisons"].toVariant().toLongLong();
        result.cpuSwaps = json["cpuSwaps"].toVariant().toLongLong();
        result.cpuArrayAccesses = json["cpuArrayAccesses"].toVariant().toLongLong();
        result.gpuKernelLaunches = json["gpuKernelLaunches"].toVariant().toLongLong();
        result.isSorted = json["isSorted"].toBool(false);
        result.cpuGpuMatch = json["cpuGpuMatch"].toBool(false);
        result.cpuName = json["cpuName"].toString();
        result.gpuName = json["gpuName"].toString();
        result.gpuComputeCapMajor = json["gpuComputeCapMajor"].toInt(0);
        result.gpuComputeCapMinor = json["gpuComputeCapMinor"].toInt(0);
        result.gpuVramUsedBytes = json["gpuVramUsedBytes"].toVariant().toULongLong();
        result.cudaVersion = json["cudaVersion"].toString();
        result.qtVersion = json["qtVersion"].toString();
        result.params.arraySize = json["arraySize"].toInt(100000);
        result.params.cpuAlgorithm = static_cast<CpuAlgorithm>(json["cpuAlgorithm"].toInt(1));
        result.params.gpuAlgorithm = static_cast<GpuAlgorithm>(json["gpuAlgorithm"].toInt(1));
        result.params.dataType = static_cast<DataType>(json["dataType"].toInt(0));
        result.params.distribution = static_cast<Distribution>(json["distribution"].toInt(0));
        return result;
    }

    QStringList toCsvRow() const {
        return QStringList{
            id,
            timestamp.toString(Qt::ISODate),
            toString(params.cpuAlgorithm),
            toString(params.gpuAlgorithm),
            QString::number(params.arraySize),
            toString(params.dataType),
            toString(params.distribution),
            QString::number(cpuTimeMs, 'f', 6),
            QString::number(gpuTotalTimeMs, 'f', 6),
            QString::number(gpuKernelTimeMs, 'f', 6),
            QString::number(gpuH2DTimeMs, 'f', 6),
            QString::number(gpuD2HTimeMs, 'f', 6),
            QString::number(gpuSyncOverheadMs, 'f', 6),
            QString::number(arrayGenerationTimeMs, 'f', 6),
            QString::number(cpuComparisons),
            QString::number(cpuSwaps),
            QString::number(cpuArrayAccesses),
            QString::number(gpuKernelLaunches),
            isSorted ? "true" : "false",
            cpuGpuMatch ? "true" : "false",
            cpuName,
            gpuName,
            QString::number(gpuComputeCapMajor) + "." + QString::number(gpuComputeCapMinor),
            QString::number(gpuVramUsedBytes),
            cudaVersion,
            qtVersion,
            QString::number(speedup(), 'f', 6),
            QString::number(throughputCPU(), 'f', 6),
            QString::number(throughputGPU(), 'f', 6)
        };
    }

    static QStringList csvHeaders() {
        return QStringList{
            "ID", "Timestamp", "CPU Algorithm", "GPU Algorithm", "Array Size",
            "Data Type", "Distribution", "CPU Time (ms)", "GPU Total Time (ms)",
            "GPU Kernel Time (ms)", "GPU H2D Time (ms)", "GPU D2H Time (ms)",
            "GPU Sync Overhead (ms)", "Array Gen Time (ms)", "CPU Comparisons",
            "CPU Swaps", "CPU Array Accesses", "GPU Kernel Launches",
            "Is Sorted", "CPU-GPU Match", "CPU Name", "GPU Name",
            "GPU Compute Capability", "GPU VRAM Used", "CUDA Version", "Qt Version",
            "Speedup", "Throughput CPU (M elem/s)", "Throughput GPU (M elem/s)"
        };
    }
};

} // namespace SortBench

#endif // BENCHMARKRESULT_H
