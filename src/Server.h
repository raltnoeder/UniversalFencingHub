#ifndef SERVER_H
#define SERVER_H

#include <cstddef>
#include <mutex>
#include <CharBuffer.h>

#include "SignalHandler.h"
#include "plugin_loader.h"

class Server
{
  public:
    typedef bool (Server::*fence_action_method)(const CharBuffer& nodename, const CharBuffer& client_secret);

    static const char* const LABEL_OFF;
    static const char* const LABEL_ON;
    static const char* const LABEL_REBOOT;

    std::mutex stdio_lock;

    Server(SignalHandler& signal_handler_ref);
    virtual ~Server() noexcept;
    Server(const Server& other) = delete;
    Server(Server&& orig) = default;
    virtual Server& operator=(const Server& other) = delete;
    virtual Server& operator=(Server&& orig) = default;

    virtual int run(int argc, const char* const argv[]) noexcept;
    virtual bool fence_action_off(const CharBuffer& nodename, const CharBuffer& client_secret) noexcept;
    virtual bool fence_action_on(const CharBuffer& nodename, const CharBuffer& client_secret) noexcept;
    virtual bool fence_action_reboot(const CharBuffer& nodename, const CharBuffer& client_secret) noexcept;
    virtual const char* get_version() noexcept;
    virtual uint32_t get_version_code() noexcept;

  private:
    class PluginMgr
    {
      private:
        Server* srv;
        void* plugin_handle;
        bool have_plugin_init;

      public:
        // @throws OsException, ClientException
        PluginMgr(const char* path, Server* srv_ref);
        virtual ~PluginMgr() noexcept;
        PluginMgr(const PluginMgr& other) = delete;
        PluginMgr(PluginMgr&& orig) = delete;
        virtual PluginMgr& operator=(const PluginMgr& other) = default;
        virtual PluginMgr& operator=(PluginMgr&& orig) = default;
    };

    SignalHandler* stop_signal;

    plugin::function_table plugin_functions;
    void* plugin_context;

    void report_fence_action(const char* action, const CharBuffer& nodename);
    void report_fence_action_result(const char* action, const CharBuffer& nodename, bool success_flag);
};

#endif /* SERVER_H */
