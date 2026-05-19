////////////////////////////////////////////////////////////////////////////////
// utils/csvexporter.h — экспортёр результатов в CSV и JSON
//
// НАЗНАЧЕНИЕ:
//   Сохраняет список BenchmarkResult в файлы CSV или JSON для дальнейшего
//   анализа в Excel, Python/pandas, R.
//
// КЛАСС: CsvExporter
//
//   СТАТИЧЕСКИЕ МЕТОДЫ:
//
//   bool exportCsv(QList<BenchmarkResult> results, QString filePath):
//     — Открывает файл, записывает BenchmarkResult::csvHeaders() как первую строку.
//     — Для каждого result: записывает result.toCsvRow().join(",") + "\n".
//     — Экранирует поля с запятыми в кавычки: "Intel Core i9, 13900K".
//     — Возвращает false при ошибке записи.
//
//   bool exportJson(QList<BenchmarkResult> results, QString filePath):
//     — QJsonArray из result.toJson() для каждого результата.
//     — Сохраняет QJsonDocument::Indented для читаемости.
//
//   bool exportMarkdownTable(QList<BenchmarkResult>, QString):
//     — Создаёт таблицу Markdown для вставки в README/документацию.
//     — Столбцы: Algorithm | Size | CPU(ms) | GPU(ms) | Speedup.
//
//   QList<BenchmarkResult> importCsv(QString filePath):
//     — Обратная операция: читает CSV и восстанавливает результаты.
//
//   ДИАЛОГ ЭКСПОРТА:
//   static bool exportWithDialog(QWidget *parent, QList<BenchmarkResult> results):
//     — Открывает QFileDialog для выбора пути и формата.
//     — Показывает QProgressDialog при большом числе записей.
////////////////////////////////////////////////////////////////////////////////

#ifndef CSV_EXPORTER_H
#define CSV_EXPORTER_H

#include <QString>
#include <QList>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QProgressDialog>
#include <QWidget>

namespace SortBench {
class BenchmarkResult;
}

class CsvExporter {
public:
    // Экспорт в CSV
    static bool exportCsv(const QList<SortBench::BenchmarkResult>& results, const QString& filePath);
    
    // Экспорт в JSON
    static bool exportJson(const QList<SortBench::BenchmarkResult>& results, const QString& filePath);
    
    // Экспорт в Markdown таблицу
    static bool exportMarkdownTable(const QList<SortBench::BenchmarkResult>& results, const QString& filePath);
    
    // Импорт из CSV
    static QList<SortBench::BenchmarkResult> importCsv(const QString& filePath);
    
    // Диалог экспорта
    static bool exportWithDialog(QWidget* parent, const QList<SortBench::BenchmarkResult>& results);
    
    // Форматирование числа с разделителями
    static QString formatNumber(double value, int precision = 3);
    
    // Экранирование CSV поля
    static QString escapeCsvField(const QString& field);
    
private:
    // Парсинг одной строки CSV
    static SortBench::BenchmarkResult parseCsvLine(const QString& line);
};

#endif // CSV_EXPORTER_H
