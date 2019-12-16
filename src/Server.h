#ifndef SERVER_H
#define SERVER_H

#include <cstddef>
#include <mutex>
#include <CharBuffer.h>

#include "plugin_loader.h"

class Server
{
  public:
    typedef bool (Server::*fence_action_method)(const CharBuffer& node_name, const CharBuffer& client_secret);

    static const char* const LABEL_POWER_OFF;
    static const char* const LABEL_POWER_ON;
    static const char* const LABEL_REBOOT;

    std::mutex stdio_lock;

    Server();
    virtual ~Server() noexcept;
    Server(const Server& other) = delete;
    Server(Server&& orig) = default;
    virtual Server& operator=(const Server& other) = delete;
    virtual Server& operator=(Server&& orig) = default;

    virtual int run(int argc, const char* const argv[]) noexcept;
    virtual bool fence_action_power_off(const CharBuffer& node_name, const CharBuffer& client_secret) noexcept;
    virtual bool fence_action_power_on(const CharBuffer& node_name, const CharBuffer& client_secret) noexcept;
    virtual bool fence_action_reboot(const CharBuffer& node_name, const CharBuffer& client_secret) noexcept;
    virtual const char* get_version() noexcept;
    virtual uint32_t get_version_code() noexcept;

  private:
    plugin::function_table plugin_functions;

    // @throws OsException
    void load_plugin(const char* const path);

    void report_fence_action(const char* action, const CharBuffer& node_name);
    void report_fence_action_result(const char* action, const CharBuffer& node_name, bool success_flag);
};

#endif /* SERVER_H */
