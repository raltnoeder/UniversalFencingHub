#ifndef CLIENT_H
#define CLIENT_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>

#include "ClientConnector.h"
#include "ClientParameters.h"

class Client
{
  public:
    enum class ExitCode : int
    {
        FENCING_SUCCESS = 0,
        FENCING_FAILURE = 1
    };

    static const char* const DEFAULT_APP_NAME;

    const char* pgm_call_path;

    Client(const char* const pgm_call_path_ref);
    virtual ~Client() noexcept;
    Client(const Client& other) = delete;
    Client(Client&& orig) = default;
    virtual Client& operator=(const Client& other) = delete;
    virtual Client& operator=(Client&& orig) = default;

    // @throws std::bad_alloc, ClientException
    virtual ExitCode run();

    virtual uint32_t get_version_code() noexcept;
    virtual const char* get_version() noexcept;

  private:
    // @throws std::bad_alloc, OsException, InetException, ClientException, ArgumentsException
    ExitCode dispatch_request(ClientParameters& params);

    // @throws std::bad_alloc, OsException, InetException, ClientException, ArgumentsException
    std::unique_ptr<ClientConnector> init_connector(ClientParameters& params);

    // @throws std::bad_alloc, OsException, InetException, ClientException, ProtocolException, ArgumentsException
    bool check_server_connection(ClientParameters& params);

    // @throws std::bad_alloc, OsException, InetException, ClientException, ProtocolException, ArgumentsException
    bool fence_action(ClientParameters& params);

    void output_metadata();
};

#endif /* CLIENT_H */
