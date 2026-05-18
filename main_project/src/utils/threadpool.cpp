////////////////////////////////////////////////////////////////////////////////
// utils/threadpool.cpp — реализация пула потоков
//
// Конструктор: создаёт threadCount рабочих потоков.
// Каждый поток: while(!stopping) { lock; wait(cv); task = queue.front(); unlock; task(); }
// submit(): lock; tasks.push(wrapped_task); cv.notify_one(); return future.
// ~ThreadPool(): stopping=true; cv.notify_all(); join all workers.
////////////////////////////////////////////////////////////////////////////////

#include "threadpool.h"
#include <QMutexLocker>
#include <QWaitCondition>
#include <QtConcurrent>
#include <QFuture>
#include <functional>

namespace SortBench {

class ThreadPool::Worker {
public:
    explicit Worker(ThreadPool *pool) : m_pool(pool) {}
    
    void operator()() {
        while (true) {
            std::function<void()> task;
            
            {
                QMutexLocker locker(&m_pool->m_mutex);
                
                while (m_pool->m_tasks.isEmpty() && !m_pool->m_stopping) {
                    m_pool->m_condition.wait(&m_pool->m_mutex);
                }
                
                if (m_pool->m_stopping && m_pool->m_tasks.isEmpty()) {
                    return;
                }
                
                if (!m_pool->m_tasks.isEmpty()) {
                    task = m_pool->m_tasks.dequeue();
                }
            }
            
            if (task) {
                task();
            }
        }
    }
    
private:
    ThreadPool *m_pool;
};

ThreadPool::ThreadPool(int threadCount, QObject *parent)
    : QObject(parent)
    , m_stopping(false)
    , m_maxThreads(threadCount <= 0 ? QThread::idealThreadCount() : threadCount)
{
    for (int i = 0; i < m_maxThreads; ++i) {
        QThread *thread = new QThread(this);
        Worker *worker = new Worker(this);
        
        worker->moveToThread(thread);
        
        connect(thread, &QThread::started, worker, [worker]() { (*worker)(); });
        connect(thread, &QThread::finished, worker, &QObject::deleteLater);
        
        m_threads.append(thread);
        thread->start();
    }
}

ThreadPool::~ThreadPool() {
    {
        QMutexLocker locker(&m_mutex);
        m_stopping = true;
    }
    
    m_condition.notify_all();
    
    for (QThread *thread : m_threads) {
        thread->quit();
        thread->wait(3000); // Ждём до 3 секунд
        if (thread->isFinished()) {
            delete thread;
        } else {
            qWarning() << "ThreadPool: Thread did not finish in time";
        }
    }
    m_threads.clear();
}

void ThreadPool::submit(std::function<void()> task) {
    {
        QMutexLocker locker(&m_mutex);
        if (m_stopping) {
            qWarning() << "ThreadPool: Submitting task while stopping";
            return;
        }
        m_tasks.enqueue([task = std::move(task)]() {
            try {
                task();
            } catch (const std::exception &e) {
                qCritical() << "ThreadPool: Exception in task:" << e.what();
            } catch (...) {
                qCritical() << "ThreadPool: Unknown exception in task";
            }
        });
    }
    m_condition.notify_one();
}

QFuture<void> ThreadPool::run(std::function<void()> task) {
    auto *watcher = new QFutureWatcher<void>();
    
    QFuture<void> future = QtConcurrent::run([this, task = std::move(task)]() {
        try {
            task();
        } catch (const std::exception &e) {
            qCritical() << "ThreadPool: Exception in task:" << e.what();
        } catch (...) {
            qCritical() << "ThreadPool: Unknown exception in task";
        }
    });
    
    watcher->setFuture(future);
    
    connect(watcher, &QFutureWatcher<void>::finished, watcher, &QObject::deleteLater);
    
    return future;
}

int ThreadPool::activeThreadCount() const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_maxThreads - m_threads.size();
}

int ThreadPool::pendingTaskCount() const {
    QMutexLocker locker(const_cast<QMutex*>(&m_mutex));
    return m_tasks.size();
}

void ThreadPool::waitForAll() {
    while (true) {
        {
            QMutexLocker locker(&m_mutex);
            if (m_tasks.isEmpty()) {
                break;
            }
        }
        QThread::msleep(10);
    }
}

void ThreadPool::clearQueue() {
    QMutexLocker locker(&m_mutex);
    m_tasks.clear();
}

} // namespace SortBench
