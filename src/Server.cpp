#include "Server.h"

#include <cstdint>
#include <new>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <iostream>
#include <chrono>

#include <CharBuffer.h>

#include "ServerConnector.h"
#include "WorkerPool.h"
#include "exceptions.h"

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
        std::cout << "Start application" << std::endl;

        std::cout << "Initializing parameter buffers" << std::endl;
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
    catch (std::exception&)
    {
        std::cerr << "Server::run() caught an unhandled exception, terminating" << std::endl;
    }
    std::cout << "End application" << std::endl;
    return rc;
}

bool Server::fence_action_power_off(const CharBuffer& node_name, const CharBuffer& client_secret) noexcept
{
    std::cout << "Server: action_power_off(...) with node_name = " << node_name.c_str() << std::endl;
    std::cout << "Server: secret = " << client_secret.c_str() << std::endl;
    return plugin_functions.ufh_power_off(node_name.c_str(), node_name.length());
}

bool Server::fence_action_power_on(const CharBuffer& node_name, const CharBuffer& client_secret) noexcept
{
    std::cout << "Server: action_power_on(...) with node_name = " << node_name.c_str() << std::endl;
    std::cout << "Server: secret = " << client_secret.c_str() << std::endl;
    return plugin_functions.ufh_power_on(node_name.c_str(), node_name.length());
}

bool Server::fence_action_reboot(const CharBuffer& node_name, const CharBuffer& client_secret) noexcept
{
    std::cout << "Server: action_reboot(...) with node_name = " << node_name.c_str() << std::endl;
    std::cout << "Server: secret = " << client_secret.c_str() << std::endl;
    return plugin_functions.ufh_reboot(node_name.c_str(), node_name.length());
}

const char* Server::get_version() noexcept
{
    return ""; // TODO: Return version
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
