#include "WorkerThreadInvocation.h"

WorkerThreadInvocation::WorkerThreadInvocation(ServerConnector* const connector)
{
    invocation_target = connector;
}

WorkerThreadInvocation::~WorkerThreadInvocation() noexcept
{
}

void WorkerThreadInvocation::run() noexcept
{
    invocation_target->process_action_queue();
}
