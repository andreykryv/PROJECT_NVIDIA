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
//   ThreadPool(int threadCount = std::thread::hardware_concurrency())
//   ~ThreadPool()                      — ожидает завершения всех задач
//
//   template<typename F, typename... Args>
//   auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
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

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>
#include <stdexcept>

class ThreadPool {
public:
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    // Запрет копирования и перемещения
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    
    // Добавить задачу в очередь
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) 
        -> std::future<std::invoke_result_t<F, Args...>>;
    
    // Дождаться завершения всех задач
    void waitAll();
    
    // Число задач в очереди
    int pendingCount() const;
    
    // Число активных потоков
    int activeCount() const { return m_activeCount.load(); }
    
    // Получить число рабочих потоков
    size_t threadCount() const { return m_workers.size(); }
    
    // Проверить, остановлен ли пул
    bool isStopped() const { return m_stopping.load(); }

private:
    void workerFunc();
    
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::condition_variable m_finishedCv;
    
    std::atomic<bool> m_stopping{false};
    std::atomic<int> m_activeCount{0};
    std::atomic<int> m_pendingCount{0};
};

// Шаблонная реализация submit
template<typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>
{
    using ReturnType = std::invoke_result_t<F, Args...>;
    
    if (m_stopping.load()) {
        throw std::runtime_error("Cannot submit task to stopped ThreadPool");
    }
    
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<ReturnType> result = task->get_future();
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.emplace([task]() { (*task)(); });
        m_pendingCount.fetch_add(1, std::memory_order_relaxed);
    }
    
    m_cv.notify_one();
    return result;
}

#endif // THREAD_POOL_H
