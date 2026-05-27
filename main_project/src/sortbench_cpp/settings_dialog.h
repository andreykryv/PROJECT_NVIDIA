/**
 * @file settings_dialog.h
 * @brief Окно настроек SortBench.
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void onSave();
    void onRestoreDefaults();

private:
    void setupUI();
    void loadSettings();

    QTabWidget* m_tabs = nullptr;

    // Вкладка: Общие
    QSpinBox*   m_telemetrySpin = nullptr;
    QComboBox*  m_csvSeparatorCombo = nullptr;

    // Вкладка: Бенчмарк
    QLineEdit*  m_sweepSizesEdit = nullptr;
    QSpinBox*   m_defaultRunsSpin = nullptr;

    // Вкладка: Визуализация
    QComboBox*  m_themeCombo = nullptr;
    QCheckBox*  m_antialiasingCheck = nullptr;
};