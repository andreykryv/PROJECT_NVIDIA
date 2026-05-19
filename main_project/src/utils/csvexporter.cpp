#include "csvexporter.h"
#include "core/benchmarkresult.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QProgressDialog>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QDebug>

namespace SortBench {

static QString escapeCsvField(const QString &field) {
    if (field.contains(',') || field.contains('"') || field.contains('\n') || field.contains('\r')) {
        QString escaped = field;
        escaped.replace("\"", "\"\"");
        return "\"" + escaped + "\"";
    }
    return field;
}

bool CsvExporter::exportCsv(const QList<SortBench::BenchmarkResult> &results, const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << SortBench::BenchmarkResult::csvHeaders().join(",") << "\n";

    for (const auto &result : results) {
        QStringList row = result.toCsvRow();
        for (int i = 0; i < row.size(); ++i) {
            row[i] = escapeCsvField(row[i]);
        }
        out << row.join(",") << "\n";
    }

    file.close();
    return true;
}

bool CsvExporter::exportJson(const QList<SortBench::BenchmarkResult> &results, const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QJsonArray array;
    for (const auto &result : results) {
        array.append(result.toJson());
    }

    QJsonDocument doc(array);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool CsvExporter::exportMarkdownTable(const QList<SortBench::BenchmarkResult> &results, const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QTextStream out(&file);

    out << "| Algorithm | Size | Distribution | CPU Time (ms) | GPU Time (ms) | Speedup |\n";
    out << "|-----------|------|--------------|---------------|---------------|--------|\n";

    for (const auto &result : results) {
        QString algo = toString(result.params.cpuAlgorithm) + " / " +
                       toString(result.params.gpuAlgorithm);
        QString size = QString::number(result.params.arraySize);
        QString dist = toString(result.params.distribution);
        // FIX: объявлена переменная cpuTime (ранее была не объявлена)
        QString cpuTime = QString::number(result.cpuTimeMs, 'f', 3);
        // FIX: было result.gpuTimeMs — поле называется gpuTotalTimeMs
        QString gpuTime = result.gpuTotalTimeMs >= 0.0
                          ? QString::number(result.gpuTotalTimeMs, 'f', 3) : "N/A";

        QString speedup = "N/A";
        // FIX: было result.gpuTimeMs
        if (result.cpuTimeMs > 0 && result.gpuTotalTimeMs > 0) {
            speedup = QString::number(result.cpuTimeMs / result.gpuTotalTimeMs, 'f', 2) + "x";
        }

        out << "| " << algo << " | " << size << " | " << dist
            << " | " << cpuTime << " | " << gpuTime << " | " << speedup << " |\n";
    }

    file.close();
    return true;
}

QList<SortBench::BenchmarkResult> CsvExporter::importCsv(const QString &filePath) {
    QList<SortBench::BenchmarkResult> results;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for reading:" << filePath;
        return results;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    // Пропуск заголовка
    if (!in.atEnd()) {
        in.readLine();
    }

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        // parseCsvLine — вспомогательный приватный метод
        QStringList fields = parseCsvLine(line);
        if (fields.size() >= SortBench::BenchmarkResult::csvHeaders().size()) {
            // Полноценный парсинг через BenchmarkResult::fromJson не применим к CSV;
            // оставляем заглушку — при необходимости реализовать fromCsvRow.
        }
    }

    file.close();
    return results;
}

QStringList CsvExporter::parseCsvLine(const QString &line) {
    QStringList fields;
    QString currentField;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        QChar ch = line[i];

        if (inQuotes) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    currentField += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                currentField += ch;
            }
        } else {
            if (ch == '"') {
                inQuotes = true;
            } else if (ch == ',') {
                fields.append(currentField.trimmed());
                currentField.clear();
            } else {
                currentField += ch;
            }
        }
    }

    fields.append(currentField.trimmed());
    return fields;
}

bool CsvExporter::exportWithDialog(QWidget *parent, QList<SortBench::BenchmarkResult> results) {
    if (results.isEmpty()) {
        qWarning() << "No results to export";
        return false;
    }

    QFileDialog dialog(parent);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter("CSV Files (*.csv);;JSON Files (*.json);;Markdown Tables (*.md);;All Files (*)");
    dialog.setDefaultSuffix("csv");
    dialog.selectFile("sortbench_results_" +
                      QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".csv");

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    QString filePath = dialog.selectedFiles().first();
    QString filter = dialog.selectedNameFilter();

    QProgressDialog *progress = nullptr;
    if (results.size() > 100) {
        progress = new QProgressDialog("Exporting results...", "Cancel", 0, results.size(), parent);
        progress->setWindowModality(Qt::WindowModal);
        progress->show();
    }

    bool success = false;
    if (filter.contains("CSV")) {
        success = exportCsv(results, filePath);
    } else if (filter.contains("JSON")) {
        success = exportJson(results, filePath);
    } else if (filter.contains("Markdown")) {
        success = exportMarkdownTable(results, filePath);
    }

    delete progress;

    return success;
}

QString CsvExporter::formatNumber(double value, int precision) {
    return QString::number(value, 'f', precision);
}

QString CsvExporter::escapeCsvField(const QString &field) {
    return ::SortBench::escapeCsvField(field);
}

} // namespace SortBench
