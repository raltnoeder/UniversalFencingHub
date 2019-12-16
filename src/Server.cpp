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

const char* const Server::VERSION_STRING = "1.0.0";
const uint32_t Server::VERSION_CODE = 0x00010000;

const char* const Server::LABEL_POWER_OFF    = "POWER OFF";
const char* const Server::LABEL_POWER_ON     = "POWER ON";
const char* const Server::LABEL_REBOOT       = "REBOOT";

Server::Server()
{
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
            "Version " << VERSION_STRING << ", Version code 0x" << std::hex << std::uppercase <<
            std::setw(8) << std::setfill('0') << VERSION_CODE << "\n\n" << std::flush;

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
            ServerConnector connector(this, protocol, ip_string, port_string);
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

bool Server::fence_action_power_off(const CharBuffer& node_name, const CharBuffer& client_secret) noexcept
{
    report_fence_action(LABEL_POWER_OFF, node_name);
    bool success_flag = plugin_functions.ufh_power_off(node_name.c_str(), node_name.length());
    report_fence_action_result(LABEL_POWER_OFF, node_name, success_flag);
    return success_flag;
}

bool Server::fence_action_power_on(const CharBuffer& node_name, const CharBuffer& client_secret) noexcept
{
    report_fence_action(LABEL_POWER_ON, node_name);
    bool success_flag = plugin_functions.ufh_power_on(node_name.c_str(), node_name.length());
    report_fence_action_result(LABEL_POWER_ON, node_name, success_flag);
    return success_flag;
}

bool Server::fence_action_reboot(const CharBuffer& node_name, const CharBuffer& client_secret) noexcept
{
    report_fence_action(LABEL_REBOOT, node_name);
    bool success_flag = plugin_functions.ufh_reboot(node_name.c_str(), node_name.length());
    report_fence_action_result(LABEL_REBOOT, node_name, success_flag);
    return success_flag;
}

const char* Server::get_version() noexcept
{
    return VERSION_STRING;
}

const uint32_t Server::get_version_code() noexcept
{
    return VERSION_CODE;
}

void Server::report_fence_action(const char* const action, const CharBuffer& node_name)
{
    std::unique_lock<std::mutex> scope_lock(stdio_lock);
    std::cout << "Fencing action " << action << " requested for node " << node_name.c_str() << std::endl;
}

void Server::report_fence_action_result(const char* const action, const CharBuffer& node_name, const bool success_flag)
{
    std::unique_lock<std::mutex> scope_lock(stdio_lock);
    std::cout << "Fencing action " << action << " for node " << node_name.c_str() <<
        (success_flag ? " SUCCEEDED" : " FAILED") << std::endl;
}

// @throws OsException
void Server::load_plugin(const char* const path)
{
    plugin::load_plugin(path, plugin_functions);
}

int main(int argc, char* argv[])
{
    Server instance;
    return instance.run(argc, argv);
}
