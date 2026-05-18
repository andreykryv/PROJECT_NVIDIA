////////////////////////////////////////////////////////////////////////////////
// utils/logger.cpp — реализация логгера
//
// initialize(): QFile::open(QIODevice::Append), записывает заголовок сессии.
// log(): форматирует строку, записывает в QFile и emit messageLogged.
//   Если файл превысил maxFileSize: ротация (переименовать в .old, создать новый).
// messageHandler(): qInstallMessageHandler callback. Маппит QtMsgType на Level.
// getRecentMessages(): возвращает последние N из QQueue<QString> recentMessages.
////////////////////////////////////////////////////////////////////////////////
