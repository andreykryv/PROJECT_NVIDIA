#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QStyleFactory>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QScreen>
#include <QPalette>
#include <QColor>

#include "mainwindow.h"
// Подключаем заголовки, но не используем их тяжелые методы до показа окна
#include "utils/logger.h"
#include "utils/settingsmanager.h"
#include "gpu/cudadeviceinfo.h"

using namespace SortBench;

int main(int argc, char *argv[])
{
    // 1. Создаем приложение
    QApplication app(argc, argv);
    
    app.setApplicationName("SortBench");
    app.setOrganizationName("CUDA Lab");
    app.setApplicationVersion("1.0.0");
    app.setFont(QFont("Segoe UI", 10));

    // 2. Парсинг аргументов (легкая операция)
    QCommandLineParser parser;
    parser.setApplicationDescription("Бенчмарк алгоритмов сортировки CPU vs GPU");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption noCudaOption(QStringList() << "no-cuda", "Запустить только CPU-режим");
    parser.addOption(noCudaOption);

    QCommandLineOption darkOption(QStringList() << "dark", "Тёмная тема");
    parser.addOption(darkOption);

    QCommandLineOption lightOption(QStringList() << "light", "Светлая тема");
    parser.addOption(lightOption);

    QCommandLineOption logLevelOption(QStringList() << "log-level", "Уровень логирования", "level", "2");
    parser.addOption(logLevelOption);

    parser.process(app);

    // 3. Применение стиля (только файлы ресурсов, без тяжелых системных вызовов)
    bool useDarkTheme = parser.isSet(darkOption) || 
                       (!parser.isSet(lightOption) && SettingsManager::instance().theme() == "dark");

    QString themeFile = useDarkTheme ? ":/styles/styles/darktheme.qss" : ":/styles/styles/lighttheme.qss";
    QFile styleFile(themeFile);
    
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        styleFile.close();
    } else {
        // Fallback стиль, если файл темы не найден
        app.setStyle(QStyleFactory::create("Fusion"));
        if (useDarkTheme) {
            QPalette darkPalette;
            darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
            darkPalette.setColor(QPalette::WindowText, Qt::white);
            darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
            darkPalette.setColor(QPalette::Text, Qt::white);
            darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
            darkPalette.setColor(QPalette::ButtonText, Qt::white);
            darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
            app.setPalette(darkPalette);
        }
    }

    // 4. СОЗДАНИЕ И ПОКАЗ ОКНА (Самое важное: делаем это ДО любой тяжелой логики)
    MainWindow mainWindow;
    
    // Гарантируем геометрию
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->availableGeometry();
        mainWindow.setGeometry(geo.x() + 50, geo.y() + 50, qMin(1280, geo.width()-100), qMin(800, geo.height()-100));
    } else {
        mainWindow.resize(1280, 800);
    }

    mainWindow.show();
    mainWindow.raise();
    mainWindow.activateWindow();
    
    // Принудительно обрабатываем события отрисовки окна ПРЯМО СЕЙЧАС
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    // 5. Инициализация логгера (после показа окна)
    int logLevel = parser.value(logLevelOption).toInt();
    QString logPath = QDir::homePath() + "/.sortbench/sortbench.log";
    QDir().mkpath(QDir::homePath() + "/.sortbench");
    Logger::instance().initialize(static_cast<Logger::Level>(logLevel), logPath);
    
    LOG_INFO("Приложение запущено. Окно отображено.");
    LOG_INFO(QString("Platform: %1").arg(QGuiApplication::platformName()));

    // 6. Асинхронная проверка CUDA и остальных тяжелых вещей
    // Используем таймер с задержкой 0, чтобы код выполнился в следующем цикле event loop
    QTimer::singleShot(0, &mainWindow, [&mainWindow, &parser]() {
        bool cudaAvailable = true;
        
        if (!parser.isSet("no-cuda")) {
            try {
                // Эта операция может быть тяжелой или блокирующей, поэтому она здесь
                int deviceCount = CudaDeviceInfo::deviceCount();
                if (deviceCount == 0) {
                    cudaAvailable = false;
                    LOG_WARN("CUDA устройства не найдены");
                } else {
                    LOG_INFO(QString("Найдено CUDA устройств: %1").arg(deviceCount));
                }
            } catch (const std::exception& e) {
                cudaAvailable = false;
                LOG_ERROR(QString("Ошибка CUDA: %1").arg(e.what()));
            }
        } else {
            cudaAvailable = false;
        }

        if (!cudaAvailable) {
            // Показываем сообщение уже после того, как окно точно видно
            QMessageBox::warning(&mainWindow, "CUDA недоступна",
                "CUDA не обнаружена.\nРабота в режиме CPU-only.");
        }
        
        // Здесь можно вызвать метод mainWindow.initialize() если нужна загрузка данных
    });

    return app.exec();
}