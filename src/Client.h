#ifndef CLIENT_H
#define CLIENT_H

#include <cstddef>
#include <cstdint>
#include <string>

class Client
{
  public:
    enum class ExitCode : int
    {
        FENCING_SUCCESS = 0,
        FENCING_FAILURE = 1
    };

    static const char* const DEFAULT_APP_NAME;

    static const std::string KEY_ACTION;
    static const std::string KEY_IPADDR;
    static const std::string KEY_PORT;
    static const std::string KEY_SECRET;
    static const std::string KEY_NODENAME;

    static const std::string ACTION_OFF;
    static const std::string ACTION_ON;
    static const std::string ACTION_REBOOT;
    static const std::string ACTION_METADATA;
    static const std::string ACTION_STATUS;
    static const std::string ACTION_LIST;
    static const std::string ACTION_MONITOR;
    static const std::string ACTION_START;
    static const std::string ACTION_STOP;

    static const char KEY_VALUE_SEPARATOR;

    static const size_t LINE_BUFFER_SIZE;

    const char* pgm_call_path;

    Client(const char* const pgm_call_path_ref);
    virtual ~Client() noexcept;
    Client(const Client& other) = delete;
    Client(Client&& orig) = default;
    virtual Client& operator=(const Client& other) = delete;
    virtual Client& operator=(Client&& orig) = default;

    // @throws std::bad_alloc
    virtual ExitCode run();

    virtual uint32_t get_version_code() noexcept;
    virtual const char* get_version() noexcept;

  private:
    struct FenceParameters
    {
        std::string action;
        bool have_action = false;

        std::string ip_address;
        bool have_ip_address = false;

        std::string tcp_port;
        bool have_tcp_port = false;

        std::string secret;
        bool have_secret = false;

        std::string nodename;
        bool have_nodename = false;
    };

    // @throws std::bad_alloc, OsException
    void read_parameters(FenceParameters& params);
};

#endif /* CLIENT_H */
