#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QQueue>
#include <QMutex>
#include <QMessageLogContext>

namespace SortBench {

class Logger : public QObject
{
    Q_OBJECT

public:
    enum class Level {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Fatal = 4
    };
    using LogLevel = Level;
    Q_ENUM(Level)

    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    static Logger& instance();

    void initialize(Level minLevel = Level::Info, const QString& logFilePath = "");

    void log(Level level, const QString &message, const QString &category = QString());
    void debug(const QString &msg, const QString &cat = QString());
    void info(const QString &msg, const QString &cat = QString());
    void warning(const QString &msg, const QString &cat = QString());
    void error(const QString &msg, const QString &cat = QString());
    void fatal(const QString &msg, const QString &cat = QString());

    void setMinLevel(Level level) { m_minLevel = level; }
    void setMaxFileSize(size_t bytes) { m_maxFileSize = bytes; }

    QStringList getRecentMessages(int count = 100) const;

    static void installMessageHandler();

signals:
    void messageLogged(const QString &formatted, int level);

private:
    static Logger* m_instance;
    static QMutex m_mutex;

    QFile *m_logFile;
    QQueue<QString> m_recentMessages;
    
    Level m_minLevel = Level::Debug;
    size_t m_maxFileSize;
    int m_maxRecentMessages;

    void rotateLog(const QString &fileName);
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
};

} // namespace SortBench

// Макросы для удобного логирования – должны быть вне класса и пространства имён
#define LOG_DEBUG(msg)  SortBench::Logger::instance().debug(msg)
#define LOG_INFO(msg)   SortBench::Logger::instance().info(msg)
#define LOG_WARN(msg)   SortBench::Logger::instance().warning(msg)
#define LOG_ERROR(msg)  SortBench::Logger::instance().error(msg)
#define LOG_FATAL(msg)  SortBench::Logger::instance().fatal(msg)

#endif // LOGGER_H