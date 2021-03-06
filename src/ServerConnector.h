#ifndef SERVERCONNECTOR_H
#define SERVERCONNECTOR_H

#include <cstddef>
#include <cstdint>
#include <mutex>

#include <CharBuffer.h>

#include "Server.h"
#include "SignalHandler.h"
#include "GenAlloc.h"
#include "MsgHeader.h"
#include "Queue.h"
#include "WorkerPool.h"
#include "WorkerThreadInvocation.h"
#include "Shared.h"

extern "C"
{
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/select.h>
}

class WorkerThreadInvocation;

class ServerConnector
{
    friend class WorkerThreadInvocation;

  public:
    static const size_t MAX_CONNECTIONS;
    static const size_t MAX_CONNECTION_BACKLOG;

    // Locking order:
    //     1. com_queue_lock
    //     2. action_queue_lock
    std::mutex com_queue_lock;
    std::mutex action_queue_lock;

  private:
    class NetClient : public Queue<NetClient>::Node
    {
      public:
        static const size_t IO_BUFFER_SIZE;
        static const size_t FIELD_SIZE;
        static const size_t NODENAME_SIZE;

        enum class IoOp : uint8_t
        {
            NOOP    = 0,
            READ    = 1,
            WRITE   = 2
        };

        enum class Phase : uint8_t
        {
            RECV        = 0,
            SEND        = 1,
            PENDING     = 2,
            EXECUTING   = 3,
            CANCELED    = 4
        };

        std::unique_ptr<char[]> address_mgr;
        std::unique_ptr<char[]> io_buffer_mgr;

        CharBuffer          key_buffer;
        CharBuffer          value_buffer;

        CharBuffer          nodename;
        CharBuffer          secret;

        struct sockaddr*    address         = nullptr;
        socklen_t           address_length  = 0;
        int                 socket_domain   = AF_INET6;
        int                 socket_fd       = sys::FD_NONE;
        Phase               current_phase   = Phase::RECV;
        Phase               next_phase      = Phase::PENDING;
        IoOp                io_state        = IoOp::NOOP;
        MsgHeader           header;
        bool                have_header     = false;
        size_t              io_offset       = 0;
        char*               io_buffer       = nullptr;

        NetClient();
        virtual ~NetClient() noexcept;
        NetClient(const NetClient& orig) = delete;
        NetClient(NetClient&& orig) = default;
        virtual NetClient& operator=(const NetClient& orig) = delete;
        virtual NetClient& operator=(NetClient&& orig) = default;
        virtual void clear() noexcept;
        virtual void clear_io_buffer() noexcept;
    };

    using ClientAlloc = GenAlloc<NetClient>;

    Server* ufh_server;
    SignalHandler* stop_signal;

    std::unique_ptr<char[]> address_mgr;

    struct sockaddr*    address         = nullptr;
    socklen_t           address_length  = 0;
    int                 socket_domain   = AF_INET6;
    int                 socket_fd       = sys::FD_NONE;
    fd_set              *read_fd_set    = nullptr;
    fd_set              *write_fd_set   = nullptr;
    ClientAlloc         client_pool     = ClientAlloc(MAX_CONNECTIONS);

    int                 selector_trigger[2];

    Queue<NetClient>        com_queue;
    Queue<NetClient>        action_queue;

    std::unique_ptr<fd_set> read_fd_set_mgr;
    std::unique_ptr<fd_set> write_fd_set_mgr;

    std::unique_ptr<WorkerThreadInvocation> invocation_obj;

  public:
    // @throws std::bad_alloc, InetException
    ServerConnector(
        Server& server_ref,
        SignalHandler& stop_signal_ref,
        const CharBuffer& protocol_string,
        const CharBuffer& ip_string,
        const CharBuffer& port_string
    );
    virtual ~ServerConnector() noexcept;
    ServerConnector(const ServerConnector& orig) = delete;
    ServerConnector(ServerConnector&& orig) = delete;
    virtual ServerConnector& operator=(const ServerConnector& orig) = delete;
    virtual ServerConnector& operator=(ServerConnector&& orig) = delete;

    // @throws InetException, OsException
    virtual void run(WorkerPool& thread_pool);

    // Caller must have locked the client_queue_lock
    virtual void close_connection(NetClient* const current_client);

    virtual WorkerPool::WorkerPoolExecutor* get_worker_thread_invocation() noexcept;

  private:
    // Caller must have locked the action_queue_lock
    void process_action_queue() noexcept;

    // @throws InetException, OsException
    void init();

    // @throws InetException, OsException
    void selector_loop(WorkerPool& thread_pool);

    // Caller must hold the com_queue_lock to avoid concurrent close of the selector_trigger pipe
    // by the cleanup() method
    void wakeup_selector();

    void cleanup();

    void accept_connection();
    bool receive_message(NetClient* const current_client);
    bool send_message(NetClient* const current_client);

    // @throws ProtocolException
    void process_client_message(NetClient* client);

    void fence_action(Server::fence_action_method fence, NetClient* client);

    // @throws std::bad_alloc
    void clients_init_ipv4();
    // @throws std::bad_alloc
    void clients_init_ipv6();
};

#endif /* SERVERCONNECTOR_H */
