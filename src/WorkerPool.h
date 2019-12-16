#ifndef WORKERPOOL_H
#define WORKERPOOL_H

#include <cstddef>
#include <thread>
#include <mutex>
#include <condition_variable>

class WorkerPool
{
  public:
    class WorkerPoolExecutor
    {
      public:
        virtual ~WorkerPoolExecutor() noexcept;
        virtual void run() noexcept = 0;
    };

    WorkerPool(std::mutex* lock, size_t worker_count, WorkerPoolExecutor* executor);
    virtual ~WorkerPool() noexcept;
    WorkerPool(const WorkerPool& other) = delete;
    WorkerPool(WorkerPool&& orig);
    virtual WorkerPool& operator=(const WorkerPool& other) = delete;
    virtual WorkerPool& operator=(WorkerPool&& orig) = delete;

    virtual void start();
    virtual void stop() noexcept;
    virtual void notify() noexcept;

  private:
    std::mutex*                     pool_lock;
    std::condition_variable         pool_condition;
    std::unique_ptr<std::thread[]>  pool_threads_mgr;
    std::thread*                    pool_threads        = nullptr;
    size_t                          pool_size           = 0;
    WorkerPoolExecutor*             pool_executor       = nullptr;
    volatile bool                   stop_workers        = false;

    void stop_threads() noexcept;
    void await_threads_termination() noexcept;
    void worker_loop() noexcept;
};

#endif /* WORKERPOOL_H */
