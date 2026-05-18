////////////////////////////////////////////////////////////////////////////////
// utils/settingsmanager.cpp — реализация менеджера настроек
//
// Внутри: QSettings settings("CUDA Lab", "SortBench").
// Каждый геттер: settings.value(key, defaultValue).toXxx().
// Каждый сеттер: settings.setValue(key, value); emit settingChanged(key).
// load(): вызывается один раз в main.cpp. Нет необходимости в явном чтении —
//   QSettings читает из системного хранилища при первом value() вызове.
// save(): settings.sync() — гарантирует запись на диск немедленно.
// resetToDefaults(): settings.clear(); settings.sync().
////////////////////////////////////////////////////////////////////////////////
