/**
 * @file settings_dialog.cpp
 * @brief Реализация окна настроек SortBench с тёмной стилизацией.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSettings>
#include <QMessageBox>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Настройки SortBench");
    resize(480, 360);
    setMinimumSize(450, 320);
    setupUI();
    loadSettings();
}

void SettingsDialog::setupUI() {
    QVBoxLayout* mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(16, 16, 16, 16);
    mainLay->setSpacing(12);

    m_tabs = new QTabWidget(this);

    // --- Вкладка: Общие ---
    QWidget* genTab = new QWidget(m_tabs);
    QGridLayout* genLay = new QGridLayout(genTab);
    genLay->setContentsMargins(16, 16, 16, 16);
    genLay->setSpacing(10);

    auto makeLabel = [&](const QString& text) {
        QLabel* l = new QLabel(text, this);
        l->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: 600;");
        return l;
    };

    genLay->addWidget(makeLabel("Интервал обновления телеметрии GPU (мс):"), 0, 0);
    m_telemetrySpin = new QSpinBox(genTab);
    m_telemetrySpin->setRange(200, 5000);
    m_telemetrySpin->setSingleStep(100);
    m_telemetrySpin->setValue(1000);
    genLay->addWidget(m_telemetrySpin, 0, 1);

    genLay->addWidget(makeLabel("Разделитель экспорта CSV:"), 1, 0);
    m_csvSeparatorCombo = new QComboBox(genTab);
    m_csvSeparatorCombo->addItem(";");
    m_csvSeparatorCombo->addItem(",");
    genLay->addWidget(m_csvSeparatorCombo, 1, 1);
    genLay->setRowStretch(2, 1);

    m_tabs->addTab(genTab, "Общие");

    // --- Вкладка: Бенчмарк ---
    QWidget* benchTab = new QWidget(m_tabs);
    QGridLayout* benchLay = new QGridLayout(benchTab);
    benchLay->setContentsMargins(16, 16, 16, 16);
    benchLay->setSpacing(10);

    benchLay->addWidget(makeLabel("Сетка размеров N для режима Sweep (через запятую):"), 0, 0, 1, 2);
    m_sweepSizesEdit = new QLineEdit(benchTab);
    m_sweepSizesEdit->setPlaceholderText("100,500,1000,5000,10000");
    benchLay->addWidget(m_sweepSizesEdit, 1, 0, 1, 2);

    benchLay->addWidget(makeLabel("Повторов запусков по умолчанию:"), 2, 0);
    m_defaultRunsSpin = new QSpinBox(benchTab);
    m_defaultRunsSpin->setRange(1, 50);
    m_defaultRunsSpin->setValue(5);
    benchLay->addWidget(m_defaultRunsSpin, 2, 1);
    benchLay->setRowStretch(3, 1);

    m_tabs->addTab(benchTab, "Бенчмарк");

    // --- Вкладка: Визуализация ---
    QWidget* visTab = new QWidget(m_tabs);
    QGridLayout* visLay = new QGridLayout(visTab);
    visLay->setContentsMargins(16, 16, 16, 16);
    visLay->setSpacing(10);

    visLay->addWidget(makeLabel("Цветовая палитра визуализации:"), 0, 0);
    m_themeCombo = new QComboBox(visTab);
    m_themeCombo->addItem("Classic Blue");
    m_themeCombo->addItem("Emerald Green");
    m_themeCombo->addItem("Synthwave Pink");
    m_themeCombo->addItem("Monochrome");
    visLay->addWidget(m_themeCombo, 0, 1);

    visLay->addWidget(makeLabel("Использовать сглаживание (Antialiasing):"), 1, 0);
    m_antialiasingCheck = new QCheckBox(visTab);
    m_antialiasingCheck->setChecked(true);
    visLay->addWidget(m_antialiasingCheck, 1, 1);
    visLay->setRowStretch(2, 1);

    m_tabs->addTab(visTab, "Визуализация");

    mainLay->addWidget(m_tabs, 1);

    // Кнопки управления снизу
    QHBoxLayout* btnLay = new QHBoxLayout();
    btnLay->setSpacing(10);

    QPushButton* defaultsBtn = new QPushButton("Сбросить", this);
    connect(defaultsBtn, &QPushButton::clicked, this, &SettingsDialog::onRestoreDefaults);
    btnLay->addWidget(defaultsBtn);

    btnLay->addStretch();

    QPushButton* cancelBtn = new QPushButton("Отмена", this);
    connect(cancelBtn, &QPushButton::clicked, this, &SettingsDialog::reject);
    btnLay->addWidget(cancelBtn);

    QPushButton* saveBtn = new QPushButton("Сохранить", this);
    saveBtn->setObjectName("saveBtn");
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    btnLay->addWidget(saveBtn);

    mainLay->addLayout(btnLay);

    // Применяем тёмную Dashboard стилизацию
    setStyleSheet(R"(
        QDialog { background-color: #0d0d1b; color: #f1f5f9; font-family: 'Segoe UI', sans-serif; }
        QTabWidget::panel { border: 1px solid #232342; background: #11112b; border-radius: 8px; }
        QTabBar::tab { background: #191929; border: 1px solid #232342; padding: 8px 16px; color: #94a3b8; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }
        QTabBar::tab:selected { background: #11112b; color: #6366f1; border-bottom-color: #11112b; font-weight: bold; }
        QSpinBox, QComboBox, QLineEdit { background: #141428; border: 1px solid #232342; border-radius: 6px; padding: 4px 8px; color: #f1f5f9; min-height: 20px; }
        QSpinBox:hover, QComboBox:hover, QLineEdit:hover { border-color: #6366f1; }
        QComboBox::drop-down { border: none; width: 18px; }
        QCheckBox::indicator { width: 14px; height: 14px; }
        QPushButton { background: #1c1c24; border: 1px solid #232342; color: #e4e4e7; border-radius: 8px; padding: 6px 14px; font-weight: 600; min-width: 70px; }
        QPushButton:hover { background: #272730; border-color: #6366f1; }
        QPushButton#saveBtn { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #6366f1, stop:1 #4f46e5); border: none; color: white; }
        QPushButton#saveBtn:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #818cf8, stop:1 #6366f1); }
    )");
}

void SettingsDialog::loadSettings() {
    QSettings s;
    m_telemetrySpin->setValue(s.value("telemetry/interval", 1000).toInt());
    m_csvSeparatorCombo->setCurrentText(s.value("csv/separator", ";").toString());
    m_sweepSizesEdit->setText(s.value("benchmark/sweep_sizes", "100,500,1000,5000,10000,25000").toString());
    m_defaultRunsSpin->setValue(s.value("benchmark/default_runs", 5).toInt());
    m_themeCombo->setCurrentText(s.value("visual/theme", "Classic Blue").toString());
    m_antialiasingCheck->setChecked(s.value("visual/antialiasing", true).toBool());
}

void SettingsDialog::onSave() {
    QString rawSweep = m_sweepSizesEdit->text().trimmed();
    QStringList parts = rawSweep.split(",");
    bool valid = true;
    for (const QString& part : parts) {
        bool ok;
        int val = part.trimmed().toInt(&ok);
        if (!ok || val <= 0) {
            valid = false;
            break;
        }
    }

    if (!valid || parts.isEmpty()) {
        QMessageBox::warning(this, "Ошибка валидации", "Сетка Sweep должна состоять из целых чисел, разделенных запятой!");
        return;
    }

    QSettings s;
    s.setValue("telemetry/interval", m_telemetrySpin->value());
    s.setValue("csv/separator", m_csvSeparatorCombo->currentText());
    s.setValue("benchmark/sweep_sizes", rawSweep);
    s.setValue("benchmark/default_runs", m_defaultRunsSpin->value());
    s.setValue("visual/theme", m_themeCombo->currentText());
    s.setValue("visual/antialiasing", m_antialiasingCheck->isChecked());

    accept();
}

void SettingsDialog::onRestoreDefaults() {
    m_telemetrySpin->setValue(1000);
    m_csvSeparatorCombo->setCurrentText(";");
    m_sweepSizesEdit->setText("100,500,1000,5000,10000,25000");
    m_defaultRunsSpin->setValue(5);
    m_themeCombo->setCurrentText("Classic Blue");
    m_antialiasingCheck->setChecked(true);
}