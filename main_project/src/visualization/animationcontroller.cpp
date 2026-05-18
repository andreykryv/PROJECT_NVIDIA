////////////////////////////////////////////////////////////////////////////////
// visualization/animationcontroller.cpp — реализация контроллера анимации
//
// playbackTimer::timeout → onTimerTick():
//   Если буфер не пуст и !paused: берём frameBuffer.dequeue() и emit frameReady.
//   Считаем реальный FPS: delta между последними N вызовами, emit fpsActual.
//
// Адаптивный буфер:
//   onFrameReceived(): если буфер достиг maxBufferSize → emit bufferFull.
//     Движок получит сигнал и приостановит генерацию кадров.
//   onTimerTick(): если буфер < 20% && mode==Buffered → emit bufferEmpty.
//     Движок возобновит генерацию.
//
// StepByStep mode: playbackTimer остановлен. step() вручную выдаёт один кадр.
////////////////////////////////////////////////////////////////////////////////
