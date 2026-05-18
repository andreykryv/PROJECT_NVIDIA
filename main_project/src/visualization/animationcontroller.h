////////////////////////////////////////////////////////////////////////////////
// visualization/animationcontroller.h — главный контроллер анимации
//
// НАЗНАЧЕНИЕ:
//   AnimationController управляет жизненным циклом анимации:
//   скоростью воспроизведения, паузой, пошаговым режимом,
//   буферизацией кадров и синхронизацией с движком.
//
// КЛАСС: AnimationController : public QObject
//
//   РЕЖИМЫ:
//     enum class Mode { RealTime, Buffered, StepByStep, FastForward }
//
//   ЧЛЕНЫ:
//     QQueue<VisFrame> frameBuffer          — очередь входящих кадров
//     int maxBufferSize                     — ограничение буфера (default: 300 кадров)
//     QTimer *playbackTimer                 — таймер воспроизведения
//     Mode mode
//     int targetFPS                         — желаемый FPS
//     bool paused
//     int  frameIndex                       — текущий номер кадра
//
//   СЛОТЫ:
//     onFrameReceived(VisFrame)             — добавляет кадр в буфер
//     setTargetFPS(int fps)                 — меняет интервал таймера
//     pause(), resume(), step()
//     setMode(Mode)
//     reset()                               — очистить буфер, сброс счётчика
//
//   СИГНАЛЫ:
//     frameReady(VisFrame)                  — кадр готов к рендерингу
//     bufferFull()                          — буфер переполнен (замедлить движок)
//     bufferEmpty()                         — буфер пуст (ускорить движок)
//     fpsActual(int)                        — реально достигнутый FPS
//
//   АДАПТИВНЫЙ FPS:
//     — В режиме Buffered: если буфер > 80% → увеличить FPS воспроизведения.
//       Если < 20% → уменьшить FPS, чтобы ждать движок.
//     — Отправляет обратную связь движку через сигналы bufferFull/bufferEmpty.
////////////////////////////////////////////////////////////////////////////////
