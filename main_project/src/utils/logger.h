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
