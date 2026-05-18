////////////////////////////////////////////////////////////////////////////////
// utils/csvexporter.cpp — реализация экспорта результатов
//
// exportCsv(): QTextStream с UTF-8 кодировкой.
//   Экранирование: если поле содержит ',', '"', '\n' — обернуть в кавычки,
//   кавычки внутри удвоить ("").
// exportJson(): QJsonDocument(array).toJson(Indented).
// importCsv(): построчное чтение, парсинг через простой CSV-парсер
//   (учитывает кавычки и escape). BenchmarkResult::fromCsvRow().
////////////////////////////////////////////////////////////////////////////////

#include "csvexporter.h"
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

// Экранирование CSV-поля
static QString escapeCsvField(const QString &field) {
    if (field.contains(',') || field.contains('"') || field.contains('\n') || field.contains('\r')) {
        QString escaped = field.replace("\"", "\"\"");
        return "\"" + escaped + "\"";
    }
    return field;
}

bool CsvExporter::exportCsv(const QList<BenchmarkResult> &results, const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Заголовки
    out << BenchmarkResult::csvHeaders().join(",") << "\n";

    // Данные
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

bool CsvExporter::exportJson(const QList<BenchmarkResult> &results, const QString &filePath) {
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

bool CsvExporter::exportMarkdownTable(const QList<BenchmarkResult> &results, const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QTextStream out(&file);
    
    // Заголовок таблицы
    out << "| Algorithm | Size | Distribution | CPU Time (ms) | GPU Time (ms) | Speedup |\n";
    out << "|-----------|------|--------------|---------------|---------------|--------|\n";

    // Данные
    for (const auto &result : results) {
     QString algo = toString(result.params.cpuAlgorithm) + " / " +
               toString(result.params.gpuAlgorithm);
QString size = QString::number(result.params.arraySize);
QString dist = toString(result.params.distribution);
QString gpuTime = result.gpuTotalTimeMs >= 0
                  ? QString::number(result.gpuTotalTimeMs, 'f', 3) : "N/A";
        
        QString speedup = "N/A";
        if (result.cpuTimeMs > 0 && result.gpuTimeMs > 0) {
            speedup = QString::number(result.cpuTimeMs / result.gpuTimeMs, 'f', 2) + "x";
        } else if (result.cpuTimeMs >= 0 && result.gpuTimeMs < 0) {
            speedup = "CPU only";
        } else if (result.gpuTimeMs >= 0 && result.cpuTimeMs < 0) {
            speedup = "GPU only";
        }

        out << "| " << algo << " | " << size << " | " << dist 
            << " | " << cpuTime << " | " << gpuTime << " | " << speedup << " |\n";
    }

    file.close();
    return true;
}

QList<BenchmarkResult> CsvExporter::importCsv(const QString &filePath) {
    QList<BenchmarkResult> results;
    
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

        QStringList fields = parseCsvLine(line);
        if (fields.size() >= BenchmarkResult::csvHeaders().size()) {
            
           
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
                    // Экранированная кавычка
                    currentField += '"';
                    ++i;
                } else {
                    // Конец цитирования
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

    // Добавляем последнее поле
    fields.append(currentField.trimmed());
    return fields;
}

bool CsvExporter::exportWithDialog(QWidget *parent, QList<BenchmarkResult> results) {
    if (results.isEmpty()) {
        qWarning() << "No results to export";
        return false;
    }

    QFileDialog dialog(parent);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter("CSV Files (*.csv);;JSON Files (*.json);;Markdown Tables (*.md);;All Files (*)");
    dialog.setDefaultSuffix("csv");
    dialog.selectFile("sortbench_results_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".csv");

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

    if (progress) {
        delete progress;
    }

    if (success) {
        qDebug() << "Results exported to:" << filePath;
    } else {
        qWarning() << "Failed to export results to:" << filePath;
    }

    return success;
}

} // namespace SortBench
