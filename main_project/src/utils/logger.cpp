////////////////////////////////////////////////////////////////////////////////
// utils/logger.cpp — реализация логгера
//
// initialize(): QFile::open(QIODevice::Append), записывает заголовок сессии.
// log(): форматирует строку, записывает в QFile и emit messageLogged.
//   Если файл превысил maxFileSize: ротация (переименовать в .old, создать новый).
// messageHandler(): qInstallMessageHandler callback. Маппит QtMsgType на Level.
// getRecentMessages(): возвращает последние N из QQueue<QString> recentMessages.
////////////////////////////////////////////////////////////////////////////////

#include "logger.h"
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMutexLocker>
#include <QDir>
#include <QFileInfo>
#include <iostream>
#include <queue>

namespace SortBench {

Logger* Logger::m_instance = nullptr;
QMutex Logger::m_mutex;

Logger::Logger(QObject *parent) 
    : QObject(parent)
    , m_logFile(nullptr)
    , m_maxFileSize(10 * 1024 * 1024) // 10 MB
    , m_maxRecentMessages(1000) 
{
    initialize();
}

Logger::~Logger() {
    if (m_logFile && m_logFile->isOpen()) {
        m_logFile->close();
    }
    delete m_logFile;
}

Logger& Logger::instance() {
    QMutexLocker locker(&m_mutex);
    if (!m_instance) {
        m_instance = new Logger();
    }
    return *m_instance;
}

void Logger::initialize(Level minLevel, const QString& logFilePath) {
    m_minLevel = minLevel;
    QString logDir = QDir::homePath() + "/.sortbench/logs";
    QDir().mkpath(logDir);
    
    QString baseName = "sortbench.log";
    QString fileName = logDir + "/" + baseName;
    
    m_logFile = new QFile(fileName);
    
    // Проверка размера для ротации
    QFileInfo fileInfo(fileName);
    if (fileInfo.exists() && fileInfo.size() > m_maxFileSize) {
        rotateLog(fileName);
        m_logFile = new QFile(fileName);
    }
    
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        if (m_logFile->size() == 0) {
            log(Level::Info, "=== SortBench Session Started ===");
        } else {
            log(Level::Info, "=== Log Continued ===");
        }
    } else {
        std::cerr << "Failed to open log file: " << fileName.toStdString() << std::endl;
    }
}

void Logger::rotateLog(const QString &fileName) {
    QString oldFileName = fileName + ".old";
    QFile::remove(oldFileName);
    QFile::rename(fileName, oldFileName);
}

void Logger::log(Level level, const QString &message, const QString &category) {
    QMutexLocker locker(&m_mutex);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr;
    
    switch (level) {
        case Level::Debug:   levelStr = "DEBUG"; break;
        case Level::Info:    levelStr = "INFO";  break;
        case Level::Warning: levelStr = "WARN";  break;
        case Level::Error:   levelStr = "ERROR"; break;
        case Level::Fatal:   levelStr = "FATAL"; break;
    }
    
    QString logLine;
    if (category.isEmpty()) {
        logLine = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);
    } else {
        logLine = QString("[%1] [%2] [%3] %4").arg(timestamp, levelStr, category, message);
    }
    
    // Сохранение в очередь последних сообщений
    m_recentMessages.enqueue(logLine);
    while (m_recentMessages.size() > m_maxRecentMessages) {
        m_recentMessages.dequeue();
    }
    
    // Вывод в консоль/stderr
    if (level == Level::Error || level == Level::Fatal) {
        std::cerr << logLine.toStdString() << std::endl;
    } else if (level == Level::Warning) {
        std::cout << "\033[93m" << logLine.toStdString() << "\033[0m" << std::endl;
    } else if (level == Level::Debug) {
        // Debug можно выводить только в debug сборке
#ifdef QT_DEBUG
        std::cout << logLine.toStdString() << std::endl;
#endif
    } else {
        std::cout << logLine.toStdString() << std::endl;
    }
    
    // Запись в файл
    if (m_logFile && m_logFile->isOpen()) {
        QTextStream out(m_logFile);
        out << logLine << "\n";
        out.flush();
        
        // Проверка размера после записи
        if (m_logFile->size() > m_maxFileSize) {
            locker.unlock();
            rotateLog(m_logFile->fileName());
            locker.relock();
            
            delete m_logFile;
            m_logFile = new QFile(m_logFile->fileName());
            if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                log(Level::Info, "Log rotated");
            }
        }
    }
    
    emit messageLogged(logLine, static_cast<int>(level));
}

void Logger::debug(const QString &msg, const QString &cat) { log(Level::Debug, msg, cat); }
void Logger::info(const QString &msg, const QString &cat)  { log(Level::Info, msg, cat); }
void Logger::warning(const QString &msg, const QString &cat) { log(Level::Warning, msg, cat); }
void Logger::error(const QString &msg, const QString &cat)   { log(Level::Error, msg, cat); }
void Logger::fatal(const QString &msg, const QString &cat)   { log(Level::Fatal, msg, cat); }

QStringList Logger::getRecentMessages(int count) const {
    QMutexLocker locker(&m_mutex);
    int actualCount = qMin(count, m_recentMessages.size());
    
    QStringList result;
    auto it = m_recentMessages.end() - actualCount;
    while (it != m_recentMessages.end()) {
        result.append(*it);
        ++it;
    }
    return result;
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    Logger *logger = instance();
    
    Level level = Level::Info;
    switch (type) {
        case QtDebugMsg:    level = Level::Debug; break;
        case QtInfoMsg:     level = Level::Info; break;
        case QtWarningMsg:  level = Level::Warning; break;
        case QtCriticalMsg: level = Level::Error; break;
        case QtFatalMsg:    level = Level::Fatal; break;
    }
    
    QString category = context.category ? QString::fromUtf8(context.category) : QString();
    logger->log(level, msg, category);
}

void Logger::installMessageHandler() {
    qInstallMessageHandler(Logger::messageHandler);
}

} // namespace SortBench
