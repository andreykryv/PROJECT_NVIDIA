////////////////////////////////////////////////////////////////////////////////
// utils/logger.h — система логирования приложения
//
// НАЗНАЧЕНИЕ:
//   Singleton-логгер, перехватывает qDebug/qWarning/qCritical,
//   пишет в файл и испускает сигналы для обновления UI (панель лога).
//
// КЛАСС: Logger : public QObject (Singleton)
//
//   enum Level { Debug=0, Info=1, Warning=2, Error=3, Critical=4 }
//
//   МЕТОДЫ:
//     static Logger& instance()
//     void initialize(Level minLevel, QString filePath, bool timestamped)
//     void log(Level, QString message, QString category = "")
//     void debug(QString msg)    → log(Debug, msg)
//     void info(QString msg)
//     void warning(QString msg)
//     void error(QString msg)
//     void setMinLevel(Level)
//     void setMaxFileSize(size_t bytes)  — ротация лог-файла
//     QStringList getRecentMessages(int n) const
//
//   СИГНАЛЫ:
//     messageLogged(QString formatted, int level)  — для UI
//
//   ФОРМАТ СТРОКИ ЛОГА:
//     [2024-01-15 12:34:56.789] [INFO ] [Engine  ] Тест запущен: QuickSort 100K
//
//   THREAD SAFETY:
//     QMutex защищает запись в файл и список последних сообщений.
//     emit messageLogged() автоматически доставляется в UI-поток через Qt::AutoConnection.
//
//   Устанавливается как qInstallMessageHandler в main.cpp.
////////////////////////////////////////////////////////////////////////////////

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
    Q_ENUM(Level)

    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    // Singleton access
    static Logger* instance();

    // Initialization
    void initialize();

    // Logging methods
    void log(Level level, const QString &message, const QString &category = QString());
    void debug(const QString &msg, const QString &cat = QString());
    void info(const QString &msg, const QString &cat = QString());
    void warning(const QString &msg, const QString &cat = QString());
    void error(const QString &msg, const QString &cat = QString());
    void fatal(const QString &msg, const QString &cat = QString());

    // Configuration
    void setMinLevel(Level level) { m_minLevel = level; }
    void setMaxFileSize(size_t bytes) { m_maxFileSize = bytes; }

    // Retrieve recent messages
    QStringList getRecentMessages(int count = 100) const;

    // Install as global Qt message handler
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

#endif // LOGGER_H
