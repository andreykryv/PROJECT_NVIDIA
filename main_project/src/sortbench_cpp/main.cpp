/**
 * @file main.cpp
 * @brief Точка входа в приложение SortBench.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SortBench: CUDA vs CPU");
    app.setApplicationVersion("2.1.0");

    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(18, 18, 22));
    darkPalette.setColor(QPalette::WindowText, QColor(240, 240, 245));
    darkPalette.setColor(QPalette::Base, QColor(28, 28, 34));
    darkPalette.setColor(QPalette::AlternateBase, QColor(24, 24, 28));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(45, 45, 55));
    darkPalette.setColor(QPalette::ToolTipText, QColor(240, 240, 245));
    darkPalette.setColor(QPalette::Text, QColor(240, 240, 245));
    darkPalette.setColor(QPalette::Button, QColor(38, 38, 45));
    darkPalette.setColor(QPalette::ButtonText, QColor(240, 240, 245));
    darkPalette.setColor(QPalette::BrightText, QColor(255, 100, 100));
    darkPalette.setColor(QPalette::Link, QColor(96, 165, 250));
    darkPalette.setColor(QPalette::Highlight, QColor(59, 130, 246));
    darkPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 130));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 130));
    app.setPalette(darkPalette);

    app.setStyleSheet(
        "QWidget { font-family: 'Segoe UI', 'Inter', sans-serif; font-size: 13px; outline: none; }"
        "QMainWindow { background-color: #121216; }"
        "QGroupBox { background-color: #18181e; border: 1px solid #2a2a32; border-radius: 16px; margin-top: 18px; padding-top: 12px; font-weight: 600; color: #cbd5e6; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 18px; padding: 0 10px; background-color: #18181e; color: #60a5fa; }"
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2a2a35, stop:1 #1f1f28); border: 1px solid #3a3a44; border-radius: 10px; padding: 8px 18px; color: #eef2ff; font-weight: 500; transition: all 0.2s ease-in-out; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3a3a48, stop:1 #2a2a35); border-color: #5a5a6e; color: white; }"
        "QPushButton:pressed { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1f1f28, stop:1 #18181e); }"
        "QPushButton:disabled { background: #18181e; border-color: #2a2a32; color: #6a6a78; }"
        "QProgressBar { background-color: #1a1a22; border: 1px solid #2a2a32; border-radius: 12px; text-align: center; color: white; font-weight: bold; height: 18px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2563eb, stop:1 #4f46e5); border-radius: 10px; }"
    );

    MainWindow w;
    w.showMaximized();
    return app.exec();
}