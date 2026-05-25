/**
 * @file main.cpp
 * @brief Точка входа в приложение SortBench.
 * Инициализирует QApplication, накладывает премиальную темную тему с профессиональным дизайном
 * всех элементов управления: стрелок, ползунков, выпадающих списков, вкладок и кнопок.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SortBench: CUDA vs CPU");
    app.setApplicationVersion("2.0.0");

    // Применяем современный стиль Fusion как основу
    app.setStyle(QStyleFactory::create("Fusion"));

    // --- Премиальная темная палитра (глубокий charcoal + акцентный градиент) ---
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

    // --- ПРЕМИАЛЬНЫЙ ПРОФЕССИОНАЛЬНЫЙ СТИЛЬ (все элементы: стрелки, ползунки, выпадашки, вкладки) ---
    app.setStyleSheet(
        // Глобальные настройки
        "QWidget {"
        "  font-family: 'Segoe UI', 'Inter', 'Roboto', sans-serif;"
        "  font-size: 13px;"
        "  outline: none;"
        "}"
        
        // Главное окно и фон
        "QMainWindow {"
        "  background-color: #121216;"
        "}"
        
        // Группы с элегантной обводкой и скруглением
        "QGroupBox {"
        "  background-color: #18181e;"
        "  border: 1px solid #2a2a32;"
        "  border-radius: 16px;"
        "  margin-top: 18px;"
        "  padding-top: 12px;"
        "  font-weight: 600;"
        "  color: #cbd5e6;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 18px;"
        "  padding: 0 10px;"
        "  background-color: #18181e;"
        "  color: #60a5fa;"
        "}"
        
        // Кнопки с градиентом и плавной анимацией
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #2a2a35, stop:1 #1f1f28);"
        "  border: 1px solid #3a3a44;"
        "  border-radius: 10px;"
        "  padding: 8px 18px;"
        "  color: #eef2ff;"
        "  font-weight: 500;"
        "  transition: all 0.2s ease-in-out;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #3a3a48, stop:1 #2a2a35);"
        "  border-color: #5a5a6e;"
        "  color: white;"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #1f1f28, stop:1 #18181e);"
        "}"
        "QPushButton:disabled {"
        "  background: #18181e;"
        "  border-color: #2a2a32;"
        "  color: #6a6a78;"
        "}"
        
        // Специальные кнопки (Старт / Стоп) с яркими градиентами
        "QPushButton#startBtn, QPushButton[text=\"Старт тест\"], QPushButton[text=\"Старт\"], QPushButton[text=\"Генерировать\"] {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #2563eb, stop:1 #4f46e5);"
        "  border: none;"
        "  color: white;"
        "  font-weight: bold;"
        "}"
        "QPushButton#startBtn:hover, QPushButton[text=\"Старт тест\"]:hover,"
        "QPushButton[text=\"Старт\"]:hover, QPushButton[text=\"Генерировать\"]:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #3b82f6, stop:1 #6366f1);"
        "}"
        
        "QPushButton#stopBtn, QPushButton[text=\"Стоп\"], QPushButton[text=\"Сброс\"] {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #b91c1c, stop:1 #9b2c2c);"
        "  border: none;"
        "  color: white;"
        "}"
        "QPushButton#stopBtn:hover, QPushButton[text=\"Стоп\"]:hover, QPushButton[text=\"Сброс\"]:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #dc2626, stop:1 #be123c);"
        "}"
        
        // --- ВЫПАДАЮЩИЕ СПИСКИ (QComboBox) с кастомной стрелкой ---
        "QComboBox {"
        "  background-color: #1f1f28;"
        "  border: 1px solid #3a3a44;"
        "  border-radius: 10px;"
        "  padding: 6px 12px;"
        "  padding-right: 28px;"
        "  color: #eef2ff;"
        "  selection-background-color: #2563eb;"
        "}"
        "QComboBox:hover {"
        "  border-color: #5a5a6e;"
        "  background-color: #2a2a35;"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 24px;"
        "  border-left: 1px solid #3a3a44;"
        "  border-top-right-radius: 9px;"
        "  border-bottom-right-radius: 9px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: url('data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%239ca3af\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"><polyline points=\"6 9 12 15 18 9\"></polyline></svg>');"
        "  width: 14px;"
        "  height: 14px;"
        "  margin-right: 6px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: #1f1f28;"
        "  border: 1px solid #3a3a44;"
        "  border-radius: 12px;"
        "  selection-background-color: #2563eb;"
        "  selection-color: white;"
        "  padding: 6px;"
        "}"
        
        // --- СПИСКИ (QListWidget) с премиальным скроллом ---
        "QListWidget {"
        "  background-color: #1a1a22;"
        "  border: 1px solid #2a2a32;"
        "  border-radius: 14px;"
        "  padding: 6px;"
        "  outline: none;"
        "}"
        "QListWidget::item {"
        "  border-radius: 10px;"
        "  padding: 8px 12px;"
        "  margin: 2px 0px;"
        "  color: #cbd5e6;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #2a2a36;"
        "  color: white;"
        "}"
        "QListWidget::item:selected {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #2563eb, stop:1 #4f46e5);"
        "  color: white;"
        "}"
        
        // --- ВКЛАДКИ (QTabWidget) с премиальными индикаторами ---
        "QTabWidget::pane {"
        "  background-color: #121216;"
        "  border: 1px solid #2a2a32;"
        "  border-radius: 16px;"
        "  top: -1px;"
        "}"
        "QTabBar::tab {"
        "  background-color: #1a1a22;"
        "  border: 1px solid #2a2a32;"
        "  border-bottom: none;"
        "  border-top-left-radius: 12px;"
        "  border-top-right-radius: 12px;"
        "  padding: 10px 24px;"
        "  margin-right: 6px;"
        "  color: #9ca3af;"
        "  font-weight: 600;"
        "}"
        "QTabBar::tab:hover {"
        "  background-color: #242430;"
        "  color: #eef2ff;"
        "}"
        "QTabBar::tab:selected {"
        "  background-color: #121216;"
        "  border-bottom: 2px solid #3b82f6;"
        "  color: #60a5fa;"
        "}"
        
        // --- СЛАЙДЕРЫ (QSlider) с гладкими ручками и треками ---
        "QSlider::groove:horizontal {"
        "  border: none;"
        "  height: 6px;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #2a2a35, stop:1 #3a3a48);"
        "  border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: qradialgradient(cx:0.5, cy:0.5, radius:0.5,"
        "    stop:0 #60a5fa, stop:1 #2563eb);"
        "  border: none;"
        "  width: 16px;"
        "  height: 16px;"
        "  margin: -5px 0;"
        "  border-radius: 8px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "  background: qradialgradient(cx:0.5, cy:0.5, radius:0.5,"
        "    stop:0 #93c5fd, stop:1 #3b82f6);"
        "  transform: scale(1.1);"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #2563eb, stop:1 #4f46e5);"
        "  border-radius: 3px;"
        "}"
        
        // --- ПОЛЯ ВВОДА (QSpinBox) с аккуратными стрелками ---
        "QSpinBox {"
        "  background-color: #1f1f28;"
        "  border: 1px solid #3a3a44;"
        "  border-radius: 10px;"
        "  padding: 5px 8px;"
        "  color: #eef2ff;"
        "}"
        "QSpinBox:hover {"
        "  border-color: #5a5a6e;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button {"
        "  background-color: #2a2a35;"
        "  border: none;"
        "  width: 20px;"
        "  border-radius: 4px;"
        "}"
        "QSpinBox::up-button:hover, QSpinBox::down-button:hover {"
        "  background-color: #3b82f6;"
        "}"
        "QSpinBox::up-arrow {"
        "  image: url('data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%239ca3af\" stroke-width=\"2\"><polyline points=\"18 15 12 9 6 15\"></polyline></svg>');"
        "  width: 10px;"
        "  height: 10px;"
        "}"
        "QSpinBox::down-arrow {"
        "  image: url('data:image/svg+xml;utf8,<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"%239ca3af\" stroke-width=\"2\"><polyline points=\"6 9 12 15 18 9\"></polyline></svg>');"
        "  width: 10px;"
        "  height: 10px;"
        "}"
        
        // --- ПРОГРЕСС-БАР с анимированным градиентом ---
        "QProgressBar {"
        "  background-color: #1a1a22;"
        "  border: 1px solid #2a2a32;"
        "  border-radius: 12px;"
        "  text-align: center;"
        "  color: white;"
        "  font-weight: bold;"
        "  height: 18px;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #2563eb, stop:1 #4f46e5);"
        "  border-radius: 10px;"
        "}"
        
        // --- ТАБЛИЦЫ с гладкими заголовками и ячейками ---
        "QTableWidget {"
        "  background-color: #0f0f13;"
        "  border: 1px solid #2a2a32;"
        "  border-radius: 16px;"
        "  gridline-color: #23232b;"
        "}"
        "QTableWidget::item {"
        "  padding: 8px;"
        "  color: #e2e8f0;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #2563eb;"
        "  color: white;"
        "}"
        "QHeaderView::section {"
        "  background-color: #18181e;"
        "  color: #94a3b8;"
        "  padding: 10px;"
        "  border: none;"
        "  border-bottom: 2px solid #2a2a32;"
        "  font-weight: 600;"
        "}"
        
        // --- СКРОЛЛБАРЫ (вертикальные и горизонтальные) с премиальным дизайном ---
        "QScrollBar:vertical {"
        "  background: #121216;"
        "  width: 10px;"
        "  border-radius: 5px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #3a3a48;"
        "  border-radius: 5px;"
        "  min-height: 30px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: #5a5a6e;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
        
        "QScrollBar:horizontal {"
        "  background: #121216;"
        "  height: 10px;"
        "  border-radius: 5px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: #3a3a48;"
        "  border-radius: 5px;"
        "  min-width: 30px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "  background: #5a5a6e;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0px;"
        "}"
        
        // --- TOOLTIP с мягкой тенью ---
        "QToolTip {"
        "  background-color: #1f1f28;"
        "  border: 1px solid #3a3a44;"
        "  border-radius: 8px;"
        "  color: #e2e8f0;"
        "  padding: 6px 12px;"
        "  font-size: 12px;"
        "}"
        
        // --- СТАТУС БАР (индикаторы) ---
        "QLabel#statusLabel {"
        "  color: #94a3b8;"
        "  background: transparent;"
        "  padding: 4px 8px;"
        "}"
        
        // --- ЧАРТЫ (фон) ---
        "QChartView {"
        "  background: transparent;"
        "  border: none;"
        "}"
    );

    MainWindow w;
    w.showMaximized();
    return app.exec();
}