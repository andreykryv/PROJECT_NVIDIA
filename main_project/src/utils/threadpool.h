////////////////////////////////////////////////////////////////////////////////
// utils/threadpool.h — пул потоков для параллельного CPU-тестирования
//
// НАЗНАЧЕНИЕ:
//   Используется в batch-режиме для параллельного запуска нескольких
//   CPU-тестов одновременно (каждый на своём ядре).
//   В обычном режиме не используется (тесты последовательны для чистоты замеров).
//
// КЛАСС: ThreadPool
//
//   ThreadPool(int threadCount = QThread::idealThreadCount())
//   ~ThreadPool()                      — ожидает завершения всех задач
//
//   template<typename F, typename... Args>
//   std::future<std::invoke_result_t<F, Args...>> submit(F&& f, Args&&... args)
//     — Добавляет задачу в очередь. Возвращает std::future для получения результата.
//
//   void waitAll()                     — блокирует до завершения всех задач
//   int pendingCount() const           — число задач в очереди
//   int activeCount() const            — число работающих потоков
//
// РЕАЛИЗАЦИЯ:
//   std::vector<std::thread> workers   — рабочие потоки
//   std::queue<std::function<void()>> tasks — очередь задач
//   std::mutex mutex, std::condition_variable cv
//   std::atomic<bool> stopping
//
// NOTE: Используется только для батч-тестирования. Основной движок (SortBenchEngine)
//   использует один QThread для детерминированных и воспроизводимых замеров.
////////////////////////////////////////////////////////////////////////////////
