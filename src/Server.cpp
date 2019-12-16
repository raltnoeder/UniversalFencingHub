#include "Server.h"

#include <cstdint>
#include <new>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <iostream>
#include <iomanip>
#include <chrono>

#include <CharBuffer.h>

#include "ServerConnector.h"
#include "WorkerPool.h"
#include "exceptions.h"
#include "version.h"
#include "SignalHandler.h"

const char* const Server::LABEL_OFF     = "OFF";
const char* const Server::LABEL_ON      = "ON";
const char* const Server::LABEL_REBOOT  = "REBOOT";

Server::Server(SignalHandler& signal_handler_ref)
{
    stop_signal = &signal_handler_ref;
}

Server::~Server() noexcept
{
}

int Server::run(const int argc, const char* const argv[]) noexcept
{
    int rc = EXIT_FAILURE;
    try
    {
        std::cout << "Universal Fencing Hub Server\n"
            "Version " << ufh::VERSION_STRING << ", Version code 0x" << std::hex << std::uppercase <<
            std::setw(8) << std::setfill('0') << ufh::VERSION_CODE << "\n\n" << std::flush;

        const CharBuffer protocol("IPV6");
        const CharBuffer ip_string("::1");
        const CharBuffer port_string("2111");
        const CharBuffer plugin_path("./debug_plugin.so");

        std::cout << ufh::LOGPFX_START << "Loading fencing plugin: Plugin path = " << plugin_path.c_str() << std::endl;
        load_plugin(plugin_path.c_str());

        std::cout << ufh::LOGPFX_START << "Executing fencing plugin initialization" << std::endl;
        if (plugin_functions.ufh_plugin_init())
        {
            std::cout << ufh::LOGPFX_START << "Initializing network connector" << std::endl;
            std::unique_ptr<ServerConnector> connector(
                new ServerConnector(*this, *stop_signal, protocol, ip_string, port_string)
            );

            std::cout << ufh::LOGPFX_START << "Initializing thread pool" << std::endl;
            std::unique_ptr<WorkerPool> thread_pool(
                new WorkerPool(
                    &(connector->action_queue_lock),
                    ServerConnector::MAX_CONNECTIONS,
                    connector->get_worker_thread_invocation()
                )
            );

            std::cout << ufh::LOGPFX_START << "Starting worker threads" << std::endl;
            thread_pool->start();

            std::cout << ufh::LOGPFX_START << "Starting network connector" << std::endl;
            connector->run(*thread_pool);
            // Newline after possible "^C" output caused by Ctrl-C being entered on the console, purely cosmetic
            std::cout << std::endl;

            std::cout << ufh::LOGPFX_STOP << "Stopping worker threads" << std::endl;
            thread_pool->stop();

            std::cout << ufh::LOGPFX_STOP << "Uninitializing worker pool" << std::endl;
            thread_pool = nullptr;

            std::cout << ufh::LOGPFX_STOP << "Uninitializing network connector" << std::endl;
            connector = nullptr;

            rc = EXIT_SUCCESS;
        }
        else
        {
            std::cout << ufh::LOGPFX_ERROR << "Fencing plugin initialization failed" << std::endl;
        }
    }
    catch (std::bad_alloc&)
    {
        std::cerr << ufh::LOGPFX_ERROR << "Initialization failed: Out of memory" << std::endl;
    }
    catch (OsException& os_exc)
    {
        std::cerr << ufh::LOGPFX_ERROR << "System error: " << os_exc.get_error_description() << std::endl;
    }
    catch (InetException& inet_exc)
    {
        std::cerr << ufh::LOGPFX_ERROR << "Network server intialization failed: " << inet_exc.get_error_description() << std::endl;
    }
    catch (std::logic_error& log_err)
    {
        std::cerr << ufh::LOGPFX_ERROR << "Error: Logic error caught by class Server, method run: " << log_err.what() << std::endl;
    }
    catch (std::exception&)
    {
        std::cerr << ufh::LOGPFX_ERROR << "Error: Unhandled exception caught in class Server, method run: terminating" << std::endl;
    }
    std::cout << ufh::LOGPFX_STOP << "End application" << std::endl;
    return rc;
}

bool Server::fence_action_off(const CharBuffer& nodename, const CharBuffer& client_secret) noexcept
{
    report_fence_action(LABEL_OFF, nodename);
    bool success_flag = plugin_functions.ufh_fence_off(nodename.c_str(), nodename.length());
    report_fence_action_result(LABEL_OFF, nodename, success_flag);
    return success_flag;
}

bool Server::fence_action_power_on(const CharBuffer& nodename, const CharBuffer& client_secret) noexcept
{
    report_fence_action(LABEL_ON, nodename);
    bool success_flag = plugin_functions.ufh_fence_on(nodename.c_str(), nodename.length());
    report_fence_action_result(LABEL_ON, nodename, success_flag);
    return success_flag;
}

bool Server::fence_action_reboot(const CharBuffer& nodename, const CharBuffer& client_secret) noexcept
{
    report_fence_action(LABEL_REBOOT, nodename);
    bool success_flag = plugin_functions.ufh_fence_reboot(nodename.c_str(), nodename.length());
    report_fence_action_result(LABEL_REBOOT, nodename, success_flag);
    return success_flag;
}

const char* Server::get_version() noexcept
{
    return ufh::VERSION_STRING;
}

uint32_t Server::get_version_code() noexcept
{
    return ufh::VERSION_CODE;
}

void Server::report_fence_action(const char* const action, const CharBuffer& nodename)
{
    std::unique_lock<std::mutex> scope_lock(stdio_lock);
    std::cout << ufh::LOGPFX_FENCE << "Executing fencing action \"" << action <<
        "\" affecting node \"" << nodename.c_str() << "\"" << std::endl;
}

void Server::report_fence_action_result(const char* const action, const CharBuffer& nodename, const bool success_flag)
{
    std::unique_lock<std::mutex> scope_lock(stdio_lock);
    std::cout << ufh::LOGPFX_FENCE << "Fencing action \"" << action <<
        "\" affecting node \"" << nodename.c_str() << (success_flag ? "\" SUCCEEDED" : " FAILED") << std::endl;
}

// @throws OsException
void Server::load_plugin(const char* const path)
{
    plugin::load_plugin(path, plugin_functions);
}
