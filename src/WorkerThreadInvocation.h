#ifndef WORKERTHREADINVOCATION_H
#define WORKERTHREADINVOCATION_H

#include <ServerConnector.h>

class ServerConnector;

class WorkerThreadInvocation : public WorkerPool::WorkerPoolExecutor
{
  private:
    ServerConnector* invocation_target;

  public:
    WorkerThreadInvocation(ServerConnector* connector);
    virtual ~WorkerThreadInvocation() noexcept;
    WorkerThreadInvocation(const WorkerThreadInvocation& other) = default;
    WorkerThreadInvocation(WorkerThreadInvocation&& orig) = default;
    virtual WorkerThreadInvocation& operator=(const WorkerThreadInvocation& other) = default;
    virtual WorkerThreadInvocation& operator=(WorkerThreadInvocation&& orig) = default;

    virtual void run() noexcept;
};

#endif /* WORKERTHREADINVOCATION_H */
