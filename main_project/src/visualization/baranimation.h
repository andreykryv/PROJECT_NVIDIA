////////////////////////////////////////////////////////////////////////////////
// visualization/baranimation.h — контроллер анимации столбцов
//
// НАЗНАЧЕНИЕ:
//   BarAnimation управляет плавными переходами между кадрами (interpolation),
//   обеспечивая визуально непрерывную анимацию даже при нерегулярном
//   поступлении кадров от алгоритма.
//
// КЛАСС: BarAnimation : public QObject
//
//   СТРУКТУРА AnimatedBar:
//     float currentHeight     — текущая высота столбца (для интерполяции)
//     float targetHeight      — целевая высота
//     QColor currentColor
//     QColor targetColor
//     float velocity          — скорость анимации (spring physics)
//
//   ЧЛЕНЫ:
//     std::vector<AnimatedBar> bars
//     float springStiffness   — жёсткость пружины (default: 0.3)
//     float damping           — затухание (default: 0.7)
//     QTimer *updateTimer     — 16 мс (~60 FPS)
//
//   МЕТОДЫ:
//     setTargetFrame(VisFrame)  — устанавливает целевые значения для баров
//     update()                  — шаг симуляции пружин, вызывается таймером
//     getCurrentState() const   — возвращает текущее интерполированное состояние
//     setAnimationMode(Mode)    — SPRING (плавно), LINEAR, INSTANT
//
//   Spring physics:
//     velocity += (target - current) * stiffness
//     velocity *= damping
//     current += velocity
//   Это создаёт "упругий" эффект: столбцы прыгают к цели как на пружинах,
//   что выглядит динамично и приятно для глаза.
////////////////////////////////////////////////////////////////////////////////
