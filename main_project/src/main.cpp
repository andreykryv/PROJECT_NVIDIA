#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QStyleFactory>
#include <QFile>
#include <QDir>

#include "mainwindow.h"
#include "utils/logger.h"
#include "utils/settingsmanager.h"
#include "gpu/cudadeviceinfo.h"

int main(int argc, char *argv[])
{
    // 1. Инициализация QApplication
    QApplication app(argc, argv);
    app.setApplicationName("SortBench");
    app.setOrganizationName("CUDA Lab");
    app.setApplicationVersion("1.0.0");
    app.setWindowIcon(QIcon(":icons/app_icon.png"));
    
    // 2. Разбор аргументов командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription("Бенчмарк алгоритмов сортировки CPU vs GPU");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption noCudaOption(QStringList() << "no-cuda", 
        "Запустить только CPU-режим (без CUDA)");
    parser.addOption(noCudaOption);
    
    QCommandLineOption darkOption(QStringList() << "dark", 
        "Принудительно тёмная тема");
    parser.addOption(darkOption);
    
    QCommandLineOption lightOption(QStringList() << "light", 
        "Принудительно светлая тема");
    parser.addOption(lightOption);
    
    QCommandLineOption logLevelOption(QStringList() << "log-level", 
        "Уровень логирования (0-3)", "level", "2");
    parser.addOption(logLevelOption);
    
    QCommandLineOption benchmarkOption(QStringList() << "benchmark", 
        "Файл конфигурации авто-бенчмарка", "file");
    parser.addOption(benchmarkOption);
    
    QCommandLineOption exportOption(QStringList() << "export", 
        "Экспорт результатов в файл и выход", "file");
    parser.addOption(exportOption);
    
    parser.process(app);
    
    // 3. Инициализация логгера
    int logLevel = parser.value(logLevelOption).toInt();
    Logger::instance().initialize(static_cast<Logger::LogLevel>(logLevel), 
                                  QDir::homePath() + "/.sortbench/sortbench.log");
    
    // 4. Проверка CUDA
    bool cudaAvailable = true;
    if (!parser.isSet(noCudaOption)) {
        try {
            int deviceCount = CudaDeviceInfo::queryAllDevices();
            if (deviceCount == 0) {
                cudaAvailable = false;
                LOG_WARN("CUDA устройства не найдены");
            } else {
                LOG_INFO("Найдено CUDA устройств: %d", deviceCount);
            }
        } catch (const std::exception& e) {
            cudaAvailable = false;
            LOG_ERROR("Ошибка инициализации CUDA: %s", e.what());
        }
        
        if (!cudaAvailable) {
            QMessageBox::warning(nullptr, "CUDA недоступна",
                "CUDA не обнаружена на этом устройстве.\n"
                "Приложение будет работать в CPU-only режиме.");
        }
    }
    
    // 5. Загрузка настроек
    bool useDarkTheme = parser.isSet(darkOption) || 
                       (!parser.isSet(lightOption) && 
                        SettingsManager::instance().value("theme/dark", true).toBool());
    
    // 6. Применение темы
    QString themeFile = useDarkTheme ? ":/styles/darktheme.qss" : ":/styles/lighttheme.qss";
    QFile styleFile(themeFile);
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString stylesheet = QString::fromUtf8(styleFile.readAll());
        app.setStyleSheet(stylesheet);
        styleFile.close();
    }
    
    // Применение стиля Fusion для кроссплатформенности
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // 7. Создание и показ главного окна
    MainWindow mainWindow;
    mainWindow.show();
    
    // 8. Запуск цикла событий
    return app.exec();
}
