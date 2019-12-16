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

        std::cout << "Loading fencing plugin, path = " << plugin_path.c_str() << std::endl;
        load_plugin(plugin_path.c_str());

        std::cout << "Initializing fencing plugin" << std::endl;
        if (plugin_functions.ufh_plugin_init())
        {
            std::cout << "Initializing ServerConnector" << std::endl;
            ServerConnector connector(*this, *stop_signal, protocol, ip_string, port_string);
            std::cout << "Initializing WorkerPool" << std::endl;
            WorkerPool thread_pool(
                &(connector.action_queue_lock),
                ServerConnector::MAX_CONNECTIONS,
                connector.get_worker_thread_invocation()
            );
            std::cout << "Starting WorkerPool" << std::endl;
            thread_pool.start();

            std::cout << "Starting ServerConnector" << std::endl;
            connector.run(thread_pool);

            rc = EXIT_SUCCESS;
        }
        else
        {
            std::cout << "Error: Plugin initialization failed" << std::endl;
        }
    }
    catch (std::bad_alloc&)
    {
        std::cerr << "Initialization failed: Out of memory" << std::endl;
    }
    catch (OsException& os_exc)
    {
        std::cerr << "System error: " << os_exc.get_error_description() << std::endl;
    }
    catch (InetException& inet_exc)
    {
        std::cerr << "Network server intialization failed: " << inet_exc.get_error_description() << std::endl;
    }
    catch (std::logic_error& log_err)
    {
        std::cerr << "Error: Logic error caught by class Server, method run: " << log_err.what() << std::endl;
    }
    catch (std::exception&)
    {
        std::cerr << "Error: Unhandled exception caught in class Server, method run: terminating" << std::endl;
    }
    std::cout << "End application" << std::endl;
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
    std::cout << "Fencing action " << action << " requested for node " << nodename.c_str() << std::endl;
}

void Server::report_fence_action_result(const char* const action, const CharBuffer& nodename, const bool success_flag)
{
    std::unique_lock<std::mutex> scope_lock(stdio_lock);
    std::cout << "Fencing action " << action << " for node " << nodename.c_str() <<
        (success_flag ? " SUCCEEDED" : " FAILED") << std::endl;
}

// @throws OsException
void Server::load_plugin(const char* const path)
{
    plugin::load_plugin(path, plugin_functions);
}
