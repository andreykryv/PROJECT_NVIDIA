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

using namespace SortBench;   // позволяет писать Logger, SettingsManager без SortBench::

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SortBench");
    app.setOrganizationName("CUDA Lab");
    app.setApplicationVersion("1.0.0");
    app.setWindowIcon(QIcon(":icons/app_icon.png"));
    
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
    
    int logLevel = parser.value(logLevelOption).toInt();
    Logger::instance().initialize(static_cast<Logger::Level>(logLevel), 
                                  QDir::homePath() + "/.sortbench/sortbench.log");
    
    bool cudaAvailable = true;
    if (!parser.isSet(noCudaOption)) {
        try {
            int deviceCount = CudaDeviceInfo::deviceCount();
            if (deviceCount == 0) {
                cudaAvailable = false;
                LOG_WARN("CUDA устройства не найдены");
            } else {
                LOG_INFO(QString("Найдено CUDA устройств: %1").arg(deviceCount));
            }
        } catch (const std::exception& e) {
            cudaAvailable = false;
            LOG_ERROR(QString("Ошибка инициализации CUDA: %1").arg(e.what()));
        }
        
        if (!cudaAvailable) {
            QMessageBox::warning(nullptr, "CUDA недоступна",
                "CUDA не обнаружена на этом устройстве.\n"
                "Приложение будет работать в CPU-only режиме.");
        }
    }
    
    bool useDarkTheme = parser.isSet(darkOption) || 
                       (!parser.isSet(lightOption) && 
                        SettingsManager::instance().theme() == "dark");
    
    QString themeFile = useDarkTheme ? ":/styles/darktheme.qss" : ":/styles/lighttheme.qss";
    QFile styleFile(themeFile);
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString stylesheet = QString::fromUtf8(styleFile.readAll());
        app.setStyleSheet(stylesheet);
        styleFile.close();
    }
    
    app.setStyle(QStyleFactory::create("Fusion"));
    
   SortBench::MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}