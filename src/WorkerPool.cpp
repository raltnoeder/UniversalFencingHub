#include "WorkerPool.h"

WorkerPool::WorkerPool(std::mutex* const lock, const size_t worker_count, WorkerPoolExecutor* const executor)
{
    pool_lock = lock;
    pool_size = worker_count;
    pool_threads_mgr = std::unique_ptr<std::thread[]>(new std::thread[pool_size]);
    pool_threads = pool_threads_mgr.get();
    pool_executor = executor;
}

WorkerPool::~WorkerPool() noexcept
{
    stop_threads();
}

WorkerPool::WorkerPool(WorkerPool&& orig)
{
    std::unique_lock<std::mutex> lock(*(orig.pool_lock));

    pool_lock = orig.pool_lock;
    pool_threads_mgr = std::move(orig.pool_threads_mgr);
    pool_threads = orig.pool_threads;
    pool_size = orig.pool_size;
    pool_executor = orig.pool_executor;
    orig.pool_lock = nullptr;
    orig.pool_threads = nullptr;
    orig.pool_executor = nullptr;
}

WorkerPool::WorkerPoolExecutor::~WorkerPoolExecutor() noexcept
{
}

// @throws std::system_error
void WorkerPool::start()
{
    std::unique_lock<std::mutex> lock(*pool_lock);
    try
    {
        for (size_t idx = 0; idx < pool_size; ++idx)
        {
            pool_threads[idx] = std::thread(&WorkerPool::worker_loop, this);
        }
    }
    catch (std::system_error&)
    {
        stop_workers = true;
        pool_condition.notify_all();
        lock.unlock();

        await_threads_termination();
        throw;
    }
}

void WorkerPool::stop() noexcept
{
    stop_threads();
}

void WorkerPool::notify() noexcept
{
    pool_condition.notify_one();
}

void WorkerPool::worker_loop() noexcept
{
    std::unique_lock<std::mutex> lock(*pool_lock);
    while (!stop_workers)
    {
        // The pool_lock must be released while work is being done
        // and must be reacquired afterwards before returning into
        // this loop
        pool_executor->run();

        if (!stop_workers)
        {
            pool_condition.wait(lock);
        }
    }
}

void WorkerPool::stop_threads() noexcept
{
    {
        std::unique_lock<std::mutex> lock(*pool_lock);
        stop_workers = true;
        pool_condition.notify_all();
    }
    await_threads_termination();
}

void WorkerPool::await_threads_termination() noexcept
{
    for (size_t idx = 0; idx < pool_size; ++idx)
    {
        if (pool_threads[idx].joinable())
        {
            try
            {
                pool_threads[idx].join();
            }
            catch (std::system_error& ignored)
            {
            }
        }
    }
}
