/**
 * @file main.cpp
 * @brief Точка входа в приложение SortBench.
 * Инициализирует QApplication, накладывает стилизацию темной темы и открывает главное окно.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    // Поддержка High-DPI экранов
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName("SortBench: CUDA vs CPU");
    app.setApplicationVersion("1.0.0");

    // Стилизация современного темного интерфейса (Fusion с кастомной палитрой)
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(24, 24, 27));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(39, 39, 42));
    darkPalette.setColor(QPalette::AlternateBase, QColor(24, 24, 27));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(39, 39, 42));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(59, 130, 246));
    
    // Акцентный цвет - синий (tailwindcss-like blue-500)
    darkPalette.setColor(QPalette::Highlight, QColor(59, 130, 246));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));

    app.setPalette(darkPalette);

    // Дополнительный QSS стиль для скругленных углов, элегантных границ и слайдеров
    app.setStyleSheet(
        "QMainWindow { background-color: #18181b; }"
        "QWidget { font-family: 'Segoe UI', Arial, sans-serif; font-size: 13px; }"
        "QGroupBox { font-weight: bold; border: 1px solid #3f3f46; border-radius: 6px; margin-top: 12px; padding: 12px; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 5px; color: #a1a1aa; }"
        "QPushButton { background-color: #27272a; border: 1px solid #3f3f46; border-radius: 4px; padding: 6px 16px; color: white; min-width: 80px; }"
        "QPushButton:hover { background-color: #3f3f46; border-color: #52525b; }"
        "QPushButton:pressed { background-color: #1a1a1a; }"
        "QPushButton:disabled { background-color: #18181b; border-color: #27272a; color: #71717a; }"
        "QPushButton#startBtn { background-color: #15803d; border-color: #166534; }"
        "QPushButton#startBtn:hover { background-color: #166534; }"
        "QPushButton#stopBtn { background-color: #b91c1c; border-color: #991b1b; }"
        "QPushButton#stopBtn:hover { background-color: #991b1b; }"
        "QProgressBar { border: 1px solid #3f3f46; border-radius: 4px; text-align: center; background-color: #18181b; }"
        "QProgressBar::chunk { background-color: #3b82f6; width: 10px; }"
        "QSlider::groove:horizontal { border: 1px solid #3f3f46; height: 6px; background: #27272a; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #3b82f6; border: 1px solid #2563eb; width: 14px; margin: -4px 0; border-radius: 7px; }"
        "QSlider::handle:horizontal:hover { background: #60a5fa; }"
        "QComboBox { background-color: #27272a; border: 1px solid #3f3f46; border-radius: 4px; padding: 4px 8px; color: white; min-width: 120px; }"
        "QComboBox::drop-down { border-left: 1px solid #3f3f46; width: 20px; }"
        "QComboBox QAbstractItemView { background-color: #27272a; border: 1px solid #3f3f46; selection-background-color: #3b82f6; }"
        "QTextEdit, QTableWidget { background-color: #18181b; border: 1px solid #3f3f46; border-radius: 6px; color: #f4f4f5; gridline-color: #27272a; }"
        "QHeaderView::section { background-color: #27272a; color: white; border: 1px solid #3f3f46; padding: 4px; }"
    );

    MainWindow w;
    w.showMaximized();
    return app.exec();
}
