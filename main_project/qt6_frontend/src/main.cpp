/**
 * @file main.cpp
 * @brief Точка входа в приложение SortBench Qt6.
 * Инициализирует QApplication, накладывает стилизацию темной темы и открывает главное окно.
 */
#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SortBench: CUDA vs CPU");
    app.setApplicationVersion("1.0.0");

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
    darkPalette.setColor(QPalette::Highlight, QColor(59, 130, 246));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);

    app.setPalette(darkPalette);
    app.setStyleSheet(
        "QMainWindow { background-color: #18181b; }"
        "QWidget { font-family: 'Segoe UI', sans-serif; font-size: 13px; }"
        "QTabWidget::pane { border: 1px solid #3f3f46; }"
        "QTabBar::tab { background-color: #27272a; color: white; padding: 8px 16px; }"
        "QTabBar::tab:selected { background-color: #3b82f6; }"
        "QPushButton { background-color: #3b82f6; color: white; border: none; padding: 6px 12px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton:disabled { background-color: #3f3f46; color: #a1a1aa; }"
        "QCheckBox { color: white; }"
        "QLabel { color: white; }"
        "QComboBox { background-color: #27272a; color: white; border: 1px solid #3f3f46; padding: 4px; }"
        "QSpinBox { background-color: #27272a; color: white; border: 1px solid #3f3f46; padding: 4px; }"
        "QSlider::groove:horizontal { background: #3f3f46; height: 8px; border-radius: 4px; }"
        "QSlider::handle:horizontal { background: #3b82f6; width: 16px; margin: -4px 0; border-radius: 8px; }"
        "QTableWidget { background-color: #18181b; color: white; gridline-color: #3f3f46; }"
        "QTableWidget::item:selected { background-color: #3b82f6; }"
        "QHeaderView::section { background-color: #27272a; color: white; padding: 4px; border: none; }"
    );

    MainWindow w;
    w.showMaximized();
    return app.exec();
}
