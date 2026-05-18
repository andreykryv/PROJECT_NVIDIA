////////////////////////////////////////////////////////////////////////////////
// utils/threadpool.cpp — реализация пула потоков
//
// Конструктор: создаёт threadCount рабочих потоков.
// Каждый поток: while(!stopping) { lock; wait(cv); task = queue.front(); unlock; task(); }
// submit(): lock; tasks.push(wrapped_task); cv.notify_one(); return future.
// ~ThreadPool(): stopping=true; cv.notify_all(); join all workers.
////////////////////////////////////////////////////////////////////////////////
