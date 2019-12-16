#ifndef CLIENTCONNECTOR_H
#define CLIENTCONNECTOR_H

#include <CharBuffer.h>

#include "MsgHeader.h"
#include "Shared.h"

extern "C"
{
    #include <sys/types.h>
    #include <sys/socket.h>
}

class ClientConnector
{
  public:
    static const size_t IO_BUFFER_SIZE;

    // @throws std::bad_alloc, InetException
    ClientConnector(
        const CharBuffer& protocol_string,
        const CharBuffer& ip_string,
        const CharBuffer& port_string
    );
    virtual ~ClientConnector() noexcept;
    ClientConnector(const ClientConnector& other) = default;
    ClientConnector(ClientConnector&& orig) = default;
    virtual ClientConnector& operator=(const ClientConnector& other) = default;
    virtual ClientConnector& operator=(ClientConnector&& orig) = default;

    // @throws InetException, OsException
    virtual void connect_to_server();

    virtual void disconnect_from_server() noexcept;

    virtual void clear_io_buffer() noexcept;

    // @throws InetException, OsException
    virtual bool check_connection();

    // @throws InetException, OsException, ProtocolException
    virtual bool fence_action_off(const std::string& nodename, const std::string& secret);

    // @throws InetException, OsException, ProtocolException
    virtual bool fence_action_on(const std::string& nodename, const std::string& secret);

    // @throws InetException, OsException, ProtocolException
    virtual bool fence_action_reboot(const std::string& nodename, const std::string& secret);

    // @throws InetException, OsException
    virtual void send_message();

    // @throws InetExeption, OsException
    virtual void receive_message();

  private:
    std::unique_ptr<char[]> address_mgr;
    std::unique_ptr<char[]> io_buffer_mgr;

    char*               io_buffer       = nullptr;
    MsgHeader           header;

    struct sockaddr*    address         = nullptr;
    socklen_t           address_length  = 0;
    int                 socket_domain   = AF_INET6;
    int                 socket_fd       = sys::FD_NONE;

    // @throws InetException, OsException, ProtocolException
    bool fence_action_impl(
        const protocol::MsgType& msg_type,
        const std::string& nodename,
        const std::string& secret
    );
};

#endif /* CLIENTCONNECTOR_H */
