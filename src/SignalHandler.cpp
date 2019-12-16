#include "SignalHandler.h"
#include "exceptions.h"
#include "Shared.h"

bool                SignalHandler::signal_flag      = false;
std::mutex          SignalHandler::signal_lock;
struct sigaction    SignalHandler::signal_action;
// Wakeup pipe (write side file descriptor)
int                 SignalHandler::wakeup_fd        = sys::FD_NONE;

SignalHandler*      SignalHandler::instance         = nullptr;
std::mutex          SignalHandler::class_lock;

// @throws OsException
SignalHandler::SignalHandler()
{
    SignalHandler::signal_flag = false;

    SignalHandler::signal_action.sa_handler = &ufh_signal_handler;
    SignalHandler::signal_action.sa_flags = 0;
    {
        int rc = 0;
        rc |= sigemptyset(&SignalHandler::signal_action.sa_mask);
        rc |= sigaddset(&SignalHandler::signal_action.sa_mask, SIGHUP);
        rc |= sigaddset(&SignalHandler::signal_action.sa_mask, SIGINT);
        rc |= sigaddset(&SignalHandler::signal_action.sa_mask, SIGTERM);
        if (rc != 0)
        {
            throw OsException(OsException::ErrorId::SIGNAL_HND_ERROR);
        }
    }

    {
        int rc = 0;
        rc |= sigaction(SIGHUP, &SignalHandler::signal_action, nullptr);
        rc |= sigaction(SIGINT, &SignalHandler::signal_action, nullptr);
        rc |= sigaction(SIGTERM, &SignalHandler::signal_action, nullptr);
        if (rc != 0)
        {
            throw OsException(OsException::ErrorId::SIGNAL_HND_ERROR);
        }
    }
}

// @throws std::bad_alloc, OsException
SignalHandler* SignalHandler::get_instance()
{
    std::unique_lock<std::mutex> section_lock(SignalHandler::class_lock);
    if (instance == nullptr)
    {
        instance = new SignalHandler();
    }
    return instance;
}

SignalHandler::~SignalHandler() noexcept
{
    sigaction(SIGHUP, reinterpret_cast<const struct sigaction*> (SIG_DFL), nullptr);
    sigaction(SIGINT, reinterpret_cast<const struct sigaction*> (SIG_DFL), nullptr);
    sigaction(SIGTERM, reinterpret_cast<const struct sigaction*> (SIG_DFL), nullptr);
}

void SignalHandler::signal() noexcept
{
    sigprocmask(SIG_BLOCK, &SignalHandler::signal_action.sa_mask, nullptr);
    SignalHandler::signal_lock.lock();
    SignalHandler::signal_flag = true;
    SignalHandler::signal_lock.unlock();
    sigprocmask(SIG_UNBLOCK, &SignalHandler::signal_action.sa_mask, nullptr);
}

bool SignalHandler::is_signaled() noexcept
{
    sigprocmask(SIG_BLOCK, &SignalHandler::signal_action.sa_mask, nullptr);
    SignalHandler::signal_lock.lock();
    bool local_flag = SignalHandler::signal_flag;
    SignalHandler::signal_lock.unlock();
    sigprocmask(SIG_UNBLOCK, &SignalHandler::signal_action.sa_mask, nullptr);
    return local_flag;
}

void SignalHandler::enable_wakeup_fd(const int fd) noexcept
{
    sigprocmask(SIG_BLOCK, &SignalHandler::signal_action.sa_mask, nullptr);
    SignalHandler::signal_lock.lock();
    SignalHandler::wakeup_fd = fd;
    SignalHandler::signal_lock.unlock();
    sigprocmask(SIG_UNBLOCK, &SignalHandler::signal_action.sa_mask, nullptr);
}

void SignalHandler::disable_wakeup_fd() noexcept
{
    sigprocmask(SIG_BLOCK, &SignalHandler::signal_action.sa_mask, nullptr);
    SignalHandler::signal_lock.lock();
    SignalHandler::wakeup_fd = sys::FD_NONE;
    SignalHandler::signal_lock.unlock();
    sigprocmask(SIG_UNBLOCK, &SignalHandler::signal_action.sa_mask, nullptr);
}

extern "C"
void ufh_signal_handler(const int signal_nr) noexcept
{
    SignalHandler::signal_lock.lock();
    SignalHandler::signal_flag = true;
    if (SignalHandler::wakeup_fd != sys::FD_NONE)
    {
        // Write a trigger byte to the wakeup pipe
        // Failure to write due to insufficient pipe capacity is ignored, to avoid becoming deadlocked
        // in the signal handler (since the thread that executes the signal handler may be the same thread
        // that is supposed to read the wakeup pipe).
        // Insufficient capacity in the wakeup pipe means that the wakeup pipe has already been
        // triggered anyway.
        ssize_t write_length = 0;
        do
        {
            write_length = write(SignalHandler::wakeup_fd, &ufh::WAKEUP_TRIGGER_BYTE, 1);
        }
        while (write_length == -1 && errno == EINTR);
    }
    SignalHandler::signal_lock.unlock();
}
