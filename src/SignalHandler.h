#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <mutex>

extern "C"
{
    #include <signal.h>
}

class SignalHandler
{
  private:
    // @throws OsException
    SignalHandler();

  public:
    // @throws std::bad_alloc, OsException
    static SignalHandler* get_instance();

    virtual ~SignalHandler() noexcept;
    SignalHandler(const SignalHandler& other) = delete;
    SignalHandler(SignalHandler&& orig) = delete;
    virtual SignalHandler& operator=(const SignalHandler& other) = delete;
    virtual SignalHandler& operator=(SignalHandler&& orig) = delete;

    virtual void signal() noexcept;
    virtual bool is_signaled() noexcept;

    // Enable writing to a wakeup pipe
    // The pipe must be in non-blocking mode to avoid becoming stuck in the signal handler
    virtual void enable_wakeup_fd(int fd) noexcept;

    // Disable writing to a wakeup pipe
    virtual void disable_wakeup_fd() noexcept;

    static bool signal_flag;
    static std::mutex signal_lock;
    static struct sigaction signal_action;
    // Wakeup pipe (write side file descriptor)
    static int wakeup_fd;

  private:
    static SignalHandler* instance;
    static std::mutex class_lock;
};

extern "C"
{
    void ufh_signal_handler(int signal_nr) noexcept;
}

#endif /* SIGNALHANDLER_H */
