#ifndef SERVERCONNECTOR_H
#define SERVERCONNECTOR_H

#include <cstddef>
#include <cstdint>
#include <mutex>

#include <CharBuffer.h>

#include "GenAlloc.h"
#include "MsgHeader.h"
#include "Queue.h"

extern "C"
{
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/select.h>
}

class ServerConnector
{
  public:
    static const size_t MAX_CONNECTIONS;
    static const size_t MAX_CONNECTION_BACKLOG;
    static const int SOCKET_FD_NONE;

  private:
    class NetClient : public Queue<NetClient>::Node
    {
      public:
        static const size_t IO_BUFFER_SIZE;

        enum class IoOp : uint8_t
        {
            NOOP    = 0,
            READ    = 1,
            WRITE   = 2
        };

        enum class Phase : uint8_t
        {
            RECV_HEADER = 0,
            RECV_DATA   = 1,
            SEND        = 2,
            PENDING     = 3,
            EXECUTING   = 4,
            CANCELED    = 5
        };

        struct sockaddr*    address         = nullptr;
        socklen_t           address_length  = 0;
        int                 socket_domain   = AF_INET6;
        int                 socket_fd       = SOCKET_FD_NONE;
        Phase               current_phase   = Phase::RECV_HEADER;
        IoOp                io_state        = IoOp::NOOP;
        size_t              io_offset       = 0;
        MsgHeader           header;
        char*               io_buffer       = nullptr;

        NetClient();
        virtual ~NetClient() noexcept;
        NetClient(const NetClient& orig) = delete;
        NetClient(NetClient&& orig) = default;
        virtual NetClient& operator=(const NetClient& orig) = delete;
        virtual NetClient& operator=(NetClient&& orig) = default;
        virtual void clear() noexcept;
    };

    using ClientAlloc = GenAlloc<NetClient>;

    std::unique_ptr<char[]> address_mgr;

    volatile bool       stop_flag       = false;
    struct sockaddr*    address         = nullptr;
    socklen_t           address_length  = 0;
    int                 socket_domain   = AF_INET6;
    int                 socket_fd       = SOCKET_FD_NONE;
    fd_set              *read_fd_set    = nullptr;
    fd_set              *write_fd_set   = nullptr;
    ClientAlloc         client_pool     = ClientAlloc(MAX_CONNECTIONS);

    std::mutex              client_queue_lock;
    Queue<NetClient>        client_queue;

    std::unique_ptr<fd_set> read_fd_set_mgr;
    std::unique_ptr<fd_set> write_fd_set_mgr;

  public:
    ServerConnector(
        const CharBuffer& protocol_string,
        const CharBuffer& ip_string,
        const CharBuffer& port_string
    );
    virtual ~ServerConnector() noexcept;
    ServerConnector(const ServerConnector& orig) = delete;
    ServerConnector(ServerConnector&& orig) = delete;
    virtual ServerConnector& operator=(const ServerConnector& orig) = delete;
    virtual ServerConnector& operator=(ServerConnector&& orig) = delete;

    virtual void run();
    virtual void test();

  private:
    void close_fd(int& fd) noexcept;
};

#endif /* SERVERCONNECTOR_H */
