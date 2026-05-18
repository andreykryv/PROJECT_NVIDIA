////////////////////////////////////////////////////////////////////////////////
// core/benchmarkrunner.cpp — реализация серийного запуска
//
// startBatch(): декартово произведение всех списков BatchConfig → QList<SortParams>.
//   При randomizeOrder: std::shuffle с std::random_device.
//   Последовательный запуск через connect(engine, &SortBenchEngine::benchmarkFinished,
//   this, &BenchmarkRunner::onTestFinished, Qt::QueuedConnection).
//   onTestFinished(): сохраняет результат, увеличивает счётчик, запускает следующий.
//   По завершении всех тестов: emit batchFinished(allResults).
////////////////////////////////////////////////////////////////////////////////
