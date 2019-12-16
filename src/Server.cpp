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
#include "server_exceptions.h"
#include "version.h"
#include "SignalHandler.h"
#include "ServerParameters.h"

const char* const Server::LABEL_OFF     = "OFF";
const char* const Server::LABEL_ON      = "ON";
const char* const Server::LABEL_REBOOT  = "REBOOT";

Server::Server(SignalHandler& signal_handler_ref)
{
    stop_signal = &signal_handler_ref;
    plugin_context = nullptr;
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

        std::unique_ptr<PluginMgr> plugin;
        std::unique_ptr<ServerConnector> connector;
        {
            std::unique_ptr<ServerParameters> params(new ServerParameters());
            params->initialize();
            params->read_parameters(argc, argv);
            params->check_required();

            const CharBuffer& protocol = params->get_value(ServerParameters::KEY_PROTOCOL);
            const CharBuffer& bind_address = params->get_value(ServerParameters::KEY_BIND_ADDRESS);
            const CharBuffer& port = params->get_value(ServerParameters::KEY_TCP_PORT);
            const CharBuffer& fence_module = params->get_value(ServerParameters::KEY_FENCE_MODULE);

            plugin = std::unique_ptr<PluginMgr>(new PluginMgr(fence_module.c_str(), this));
            connector = std::unique_ptr<ServerConnector>(
                new ServerConnector(*this, *stop_signal, protocol, bind_address, port)
            );
        }

        std::unique_ptr<WorkerPool> thread_pool(
            new WorkerPool(
                &(connector->action_queue_lock),
                ServerConnector::MAX_CONNECTIONS,
                connector->get_worker_thread_invocation()
            )
        );

        thread_pool->start();

        connector->run(*thread_pool);
        // Newline after possible "^C" output caused by Ctrl-C being entered on the console, purely cosmetic
        std::cout << std::endl;

        rc = EXIT_SUCCESS;
    }
    catch (std::bad_alloc&)
    {
        std::cerr << ufh::LOGPFX_ERROR << "Initialization failed: Out of memory" << std::endl;
    }
    catch (Arguments::ArgumentsException& args_exc)
    {
        std::cerr << ufh::LOGPFX_ERROR << "Server initialization failed due to incorrect arguments" << std::endl;
        std::cerr << args_exc.what() << "\n\n" << std::flush;
    }
    catch (OsException& os_exc)
    {
        std::cerr << ufh::LOGPFX_ERROR << "System error: " << os_exc.get_error_description() << std::endl;
    }
    catch (InetException& inet_exc)
    {
        std::cerr << ufh::LOGPFX_ERROR << "Network server intialization failed: " <<
            inet_exc.get_error_description() << std::endl;
    }
    catch (std::logic_error& log_err)
    {
        std::cerr << ufh::LOGPFX_ERROR << "Error: Logic error caught by class Server, method run: " <<
            log_err.what() << std::endl;
    }
    catch (std::exception&)
    {
        std::cerr << ufh::LOGPFX_ERROR << "Error: Unhandled exception caught in class Server, "
            "method run: terminating" << std::endl;
    }
    std::cout << ufh::LOGPFX_STOP << "End application" << std::endl;
    return rc;
}

bool Server::fence_action_off(const CharBuffer& nodename, const CharBuffer& client_secret) noexcept
{
    report_fence_action(LABEL_OFF, nodename);
    bool success_flag = plugin_functions.ufh_fence_off(plugin_context, nodename.c_str(), nodename.length());
    report_fence_action_result(LABEL_OFF, nodename, success_flag);
    return success_flag;
}

bool Server::fence_action_on(const CharBuffer& nodename, const CharBuffer& client_secret) noexcept
{
    report_fence_action(LABEL_ON, nodename);
    bool success_flag = plugin_functions.ufh_fence_on(plugin_context, nodename.c_str(), nodename.length());
    report_fence_action_result(LABEL_ON, nodename, success_flag);
    return success_flag;
}

bool Server::fence_action_reboot(const CharBuffer& nodename, const CharBuffer& client_secret) noexcept
{
    report_fence_action(LABEL_REBOOT, nodename);
    bool success_flag = plugin_functions.ufh_fence_reboot(plugin_context, nodename.c_str(), nodename.length());
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
        "\" affecting node \"" << nodename.c_str() << "\" " << (success_flag ? "SUCCEEDED" : "FAILED") << std::endl;
}


// @throws OsException, PluginException
Server::PluginMgr::PluginMgr(const char* path, Server* srv_ref)
{
    std::cout << ufh::LOGPFX_START << "Loading fencing plugin" << std::endl;
    std::cout << ufh::LOGPFX_CONT << "Plugin path = " << path << std::endl;
    srv = srv_ref;
    plugin_handle = nullptr;
    have_plugin_init = false;

    plugin_handle = plugin::load_plugin(path, srv->plugin_functions);

    plugin::init_rc rc = srv->plugin_functions.ufh_plugin_init();
    if (rc.init_successful)
    {
        have_plugin_init = true;
        srv->plugin_context = rc.context;
    }
    else
    {
        std::cerr << ufh::LOGPFX_ERROR << "Plugin initialization failed" << std::endl;
        throw PluginException();
    }
}

Server::PluginMgr::~PluginMgr() noexcept
{
    if (have_plugin_init)
    {
        std::cout << ufh::LOGPFX_STOP << "Uninitializing fencing plugin" << std::endl;
        srv->plugin_functions.ufh_plugin_destroy(srv->plugin_context);
        srv->plugin_context = nullptr;
        have_plugin_init = false;
    }
    std::cout << ufh::LOGPFX_STOP << "Unloading fencing plugin" << std::endl;

    if (plugin_handle != nullptr)
    {
        plugin::unload_plugin(plugin_handle, srv->plugin_functions);
    }
    plugin_handle = nullptr;
}
