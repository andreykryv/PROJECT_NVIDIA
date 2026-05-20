////////////////////////////////////////////////////////////////////////////////
// utils/threadpool.cpp — реализация пула потоков
//
// Конструктор: создаёт threadCount рабочих потоков.
// Каждый поток: while(!stopping) { lock; wait(cv); task = queue.front(); unlock; task(); }
// submit(): lock; tasks.push(wrapped_task); cv.notify_one(); return future.
// ~ThreadPool(): stopping=true; cv.notify_all(); join all workers.
////////////////////////////////////////////////////////////////////////////////

#include "threadpool.h"

ThreadPool::ThreadPool(size_t threadCount)
    : m_stopping(false)
{
    const size_t count = threadCount > 0 ? threadCount : std::thread::hardware_concurrency();

    for (size_t i = 0; i < count; ++i) {
        m_workers.emplace_back([this] { workerFunc(); });
    }
}

ThreadPool::~ThreadPool() {
    m_stopping.store(true);
    m_cv.notify_all();

    for (std::thread &worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::workerFunc() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            m_cv.wait(lock, [this] {
                return m_stopping.load() || !m_tasks.empty();
            });

            if (m_stopping.load() && m_tasks.empty()) {
                return;
            }

            if (!m_tasks.empty()) {
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
        }

        if (task) {
            m_activeCount.fetch_add(1, std::memory_order_relaxed);
            try {
                task();
            } catch (...) {
                // Игнорируем исключения в задачах
            }
            m_activeCount.fetch_sub(1, std::memory_order_relaxed);
            m_pendingCount.fetch_sub(1, std::memory_order_relaxed);
            m_finishedCv.notify_all();
        }
    }
}

void ThreadPool::waitAll() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_finishedCv.wait(lock, [this] {
        return m_tasks.empty() && m_pendingCount.load() == 0;
    });
}

int ThreadPool::pendingCount() const {
    return m_pendingCount.load(std::memory_order_relaxed);
}