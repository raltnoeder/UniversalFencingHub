#include <memory>
#include <iostream>
#include <algorithm>
#include <limits>

#include "ServerConnector.h"
#include "Shared.h"
#include "ip_parse.h"
#include "zero_memory.h"
#include "exceptions.h"

extern "C"
{
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <sys/select.h>
    #include <sys/types.h>
    #include <sys/socket.h>
}

constexpr const size_t   ServerConnector::MAX_CONNECTIONS            = 24;
constexpr const size_t   ServerConnector::MAX_CONNECTION_BACKLOG     = 24;
constexpr const int      ServerConnector::SOCKET_FD_NONE             = -1;

constexpr const size_t ServerConnector::NetClient::IO_BUFFER_SIZE    = 256;

// @throws std::bad_alloc, InetException
ServerConnector::ServerConnector(
    const CharBuffer& protocol_string,
    const CharBuffer& ip_string,
    const CharBuffer& port_string
)
{
    if (protocol_string == keyword::PROTO_IPV6)
    {
        socket_domain = AF_INET6;
        address_length = sizeof (struct sockaddr_in6);
    }
    else
    if (protocol_string == keyword::PROTO_IPV4)
    {
        socket_domain = AF_INET;
        address_length = sizeof (struct sockaddr_in);
    }
    else
    {
        throw InetException(InetException::ErrorId::UNKNOWN_AF);
    }
    address_mgr = std::unique_ptr<char[]>(new char[address_length]);
    address = reinterpret_cast<struct sockaddr*> (address_mgr.get());

    if (socket_domain == AF_INET6)
    {
        struct sockaddr_in6* inet_address = reinterpret_cast<struct sockaddr_in6*> (address);
        parse_ipv6(ip_string, port_string, *inet_address);
    }
    else
    if (socket_domain == AF_INET)
    {
        struct sockaddr_in* inet_address = reinterpret_cast<struct sockaddr_in*> (address);
        parse_ipv4(ip_string, port_string, *inet_address);
    }

    read_fd_set_mgr = std::unique_ptr<fd_set>(new fd_set);
    write_fd_set_mgr = std::unique_ptr<fd_set>(new fd_set);
    read_fd_set = read_fd_set_mgr.get();
    write_fd_set = write_fd_set_mgr.get();

    invocation_obj = std::unique_ptr<WorkerThreadInvocation>(new WorkerThreadInvocation(this));
}

ServerConnector::~ServerConnector() noexcept
{
    close_fd(socket_fd);
}

ServerConnector::NetClient::NetClient()
{
}

ServerConnector::NetClient::~NetClient()
{
}

void ServerConnector::NetClient::clear() noexcept
{
    zero_memory(reinterpret_cast<char*> (address), static_cast<size_t> (address_length));
    socket_fd       = SOCKET_FD_NONE;
    current_phase   = Phase::RECV;
    io_state        = IoOp::NOOP;
    header.clear();
    have_header     = false;
    clear_io_buffer();
}

void ServerConnector::NetClient::clear_io_buffer() noexcept
{
    io_offset = 0;
    zero_memory(io_buffer, IO_BUFFER_SIZE);
}

ServerConnector::WorkerThreadInvocation::WorkerThreadInvocation(ServerConnector* const connector)
{
    invocation_target = connector;
}

ServerConnector::WorkerThreadInvocation::~WorkerThreadInvocation() noexcept
{
}

void ServerConnector::WorkerThreadInvocation::run() noexcept
{
    invocation_target->process_action_queue();
}

// @throws InetException
void ServerConnector::run(WorkerPool& thread_pool)
{
    std::cout << "ServerConnector: Initializing server socket" << std::endl;
    socket_fd = socket(socket_domain, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        throw InetException(InetException::ErrorId::SOCKET_ERROR);
    }

    std::cout << "ServerConnector: Binding server socket" << std::endl;
    if (bind(socket_fd, address, address_length) != 0)
    {
        throw InetException(InetException::ErrorId::BIND_FAILED);
    }

    std::cout << "ServerConnector: Setting nonblocking I/O mode" << std::endl;
    if (fcntl(socket_fd, F_SETFL, O_NONBLOCK) != 0)
    {
        throw InetException(InetException::ErrorId::NIO_ERROR);
    }

    std::cout << "ServerConnector: Listening on the server socket" << std::endl;
    if (listen(socket_fd, ServerConnector::MAX_CONNECTION_BACKLOG) != 0)
    {
        throw InetException(InetException::ErrorId::LISTEN_ERROR);
    }

    std::cout << "ServerConnector: Entering server loop" << std::endl;
    while (!stop_flag)
    {
        std::cout << "ServerConnector: Zeroing file descriptor sets" << std::endl;
        FD_ZERO(read_fd_set);
        FD_ZERO(write_fd_set);

        std::cout << "ServerConnector: Initializing file descriptor sets" << std::endl;
        int max_fd = 0;
        {
            std::unique_lock<std::mutex> com_lock(com_queue_lock);

            // Add the server socket to the read-selectable set
            if (com_queue.get_size() < MAX_CONNECTIONS)
            {
                FD_SET(socket_fd, read_fd_set);
                max_fd = socket_fd;
            }

            // Add active clients to the read/write sets
            for (NetClient* client = com_queue.get_first(); client != nullptr; client = client->get_next_node())
            {
                if (client->io_state == NetClient::IoOp::READ)
                {
                    FD_SET(client->socket_fd, read_fd_set);
                    max_fd = std::max(max_fd, client->socket_fd);
                }
                else
                if (client->io_state == NetClient::IoOp::WRITE)
                {
                    FD_SET(client->socket_fd, write_fd_set);
                    max_fd = std::max(max_fd, client->socket_fd);
                }
            }
        }

        std::cout << "ServerConnector: Selecting ready channels" << std::endl;
        // Select ready file descriptors
        if (max_fd >= std::numeric_limits<int>::max())
        {
            throw OsException(OsException::ErrorId::INVALID_SELECT_FD);
        }
        ++max_fd;
        int select_rc = select(max_fd, read_fd_set, write_fd_set, nullptr, nullptr);
        if (select_rc == -1)
        {
            throw OsException(OsException::ErrorId::IO_ERROR);
        }

        // Accept pending connections
        if (FD_ISSET(socket_fd, read_fd_set) != 0)
        {
            std::cout << "ServerConnector: Accepting pending connections" << std::endl;
            accept_connection();
        }

        // Perform pending client I/O operations
        {
            std::cout << "ServerConnector: Performing pending client actions" << std::endl;
            std::unique_lock<std::mutex> lock(com_queue_lock);
            NetClient* client = com_queue.get_first();
            NetClient* next_client = nullptr;
            while (client != nullptr)
            {
                // The client may be removed from the queue during I/O operations,
                // so get_next_node() must be executed before any I/O operations
                next_client = client->get_next_node();

                if (client->current_phase == NetClient::Phase::CANCELED)
                {
                    std::cout << "ServerConnector: Client with socket_fd = " << client->socket_fd << ": " <<
                        "Connection canceled" << std::endl;
                    close_connection(client);
                }
                else
                if (FD_ISSET(client->socket_fd, read_fd_set) != 0)
                {
                    std::cout << "ServerConnector: Client with socket_fd = " << client->socket_fd << ": " <<
                        "Receiving data" << std::endl;
                    // Client ready for receive
                    bool pending = receive_message(client);
                    if (pending)
                    {
                        com_queue.remove(client);
                        client->io_state = NetClient::IoOp::NOOP;
                        client->current_phase = NetClient::Phase::PENDING;

                        std::unique_lock<std::mutex> action_lock(action_queue_lock);
                        action_queue.add_last(client);
                        thread_pool.notify();
                    }
                }
                else
                if (FD_ISSET(client->socket_fd, write_fd_set) != 0)
                {
                    std::cout << "ServerConnector: Client with socket_fd = " << client->socket_fd << ": " <<
                        "Sending data" << std::endl;
                    // Client ready for send
                    send_message(client);
                }

                client = next_client;
            }
            std::cout << "Continuing server loop" << std::endl;
        }
    }

    close_fd(socket_fd);
}

void ServerConnector::accept_connection()
{
    try
    {
        ClientAlloc::scope_ptr new_client = client_pool.allocate_scope();
        NetClient* new_client_ptr = new_client.get();
        new_client_ptr->clear();
        new_client_ptr->socket_fd = accept(socket_fd, new_client_ptr->address, &(new_client_ptr->address_length));
        new_client_ptr->socket_domain = socket_domain;
        new_client_ptr->io_state = NetClient::IoOp::READ;
        new_client_ptr->current_phase = NetClient::Phase::RECV;

        {
            std::unique_lock<std::mutex> lock(com_queue_lock);
            com_queue.add_last(new_client_ptr);
        }

        new_client.release();
    }
    catch (std::bad_alloc&)
    {
        // No free NetClient objects
        // TODO: Log implementation error, this section should not be reached,
        //       since MAX_CONNECTIONS == client_pool size
    }
}

// Caller must have locked the client_queue_lock
void ServerConnector::close_connection(NetClient* const client)
{
    close_fd(client->socket_fd);
    client->clear();

    com_queue.remove(client);

    client_pool.deallocate(client);
}

bool ServerConnector::receive_message(NetClient* const client)
{
    bool pending_flag = false;

    const size_t req_read_size = client->io_offset < MsgHeader::HEADER_SIZE ?
        MsgHeader::HEADER_SIZE - MsgHeader::HEADER_SIZE :
        NetClient::IO_BUFFER_SIZE - client->io_offset;
    const ssize_t read_size = recv(
        client->socket_fd,
        &(client->io_buffer[client->io_offset]),
        req_read_size,
        0 // Flags
    );
    if (read_size == -1)
    {
        // IO Error
        close_connection(client);
    }
    else
    if (read_size == 0)
    {
        // End of stream
        close_connection(client);
    }
    else
    {
        client->io_offset += static_cast<size_t> (read_size);
    }

    if (client->have_header)
    {
        if (client->io_offset >= client->header.data_length)
        {
            pending_flag = true;
        }
    }
    else
    if (client->io_offset >= MsgHeader::HEADER_SIZE)
    {
        client->header.msg_type = MsgHeader::bytes_to_field_value(
            client->io_buffer, MsgHeader::MSG_TYPE_OFFSET
        );
        client->header.data_length = std::min(
            static_cast<uint16_t> (NetClient::IO_BUFFER_SIZE),
            MsgHeader::bytes_to_field_value(
                client->io_buffer, MsgHeader::DATA_LENGTH_OFFSET
            )
        );
        client->have_header = true;
    }
    return pending_flag;
}

void ServerConnector::send_message(NetClient* const client)
{
    if (!client->have_header)
    {
        MsgHeader::field_value_to_bytes(
            client->header.msg_type,
            client->io_buffer,
            MsgHeader::MSG_TYPE_OFFSET
        );
        MsgHeader::field_value_to_bytes(
            client->header.data_length,
            client->io_buffer,
            MsgHeader::DATA_LENGTH_OFFSET
        );
        client->have_header = true;
    }

    const size_t req_write_size = std::min(
        static_cast<uint16_t> (NetClient::IO_BUFFER_SIZE),
        client->header.data_length
    );
    const ssize_t write_size = send(
        client->socket_fd,
        &(client->io_buffer[client->io_offset]),
        req_write_size,
        0 // Flags
    );
    if (write_size > 0)
    {
        client->io_offset += static_cast<size_t> (write_size);
    }
    else
    if (write_size == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        close_connection(client);
    }
}

WorkerPool::WorkerPoolExecutor* ServerConnector::get_worker_thread_invocation() noexcept
{
    return invocation_obj.get();
}

// Caller must have locked the action_queue_lock
void ServerConnector::process_action_queue() noexcept
{
    try
    {
        NetClient* client = action_queue.remove_first();
        while (client != nullptr)
        {
            action_queue_lock.unlock();

            // TODO: Perform the action requested by the client
            std::cout << "Thread " << std::this_thread::get_id() << " is handling client with socket_fd = " <<
                client->socket_fd << std::endl;

            client->current_phase = NetClient::Phase::CANCELED;
            client->io_state = NetClient::IoOp::NOOP;

            // Continue client I/O
            {
                std::unique_lock<std::mutex> com_lock(com_queue_lock);
                com_queue.add_last(client);
            }

            action_queue_lock.lock();
            client = action_queue.remove_first();
        }
    }
    catch (std::exception&)
    {
        // TODO: Log unhandled exception
    }
}

void ServerConnector::test()
{
    const int test_count = 5;

    for (int idx = 0; idx < test_count; ++idx)
    {
        ClientAlloc::scope_ptr current_client = client_pool.allocate_scope();
        current_client->socket_fd = idx;
        std::cout << "Queueing client with socket_fd = " << current_client->socket_fd << std::endl;
        com_queue.add_last(current_client.get());
        current_client.release();
    }
    std::cout << '\n';
    for (int idx = 0; idx < test_count; ++idx)
    {
        ClientAlloc::scope_ptr current_client = client_pool.make_scope_ptr(com_queue.remove_first());
        std::cout << "Dequeued client with socket_fd = " << current_client->socket_fd << std::endl;
    }
    std::cout << '\n';

    std::cout << "ServerConnector socket_domain = " << socket_domain << ", " <<
        "address_length = " << address_length << '\n' <<
        std::endl;
}

void ServerConnector::close_fd(int& fd) noexcept
{
    if (fd >= 0)
    {
        int rc = 0;
        do
        {
            rc = close(fd);
        }
        while (rc != 0 && errno == EINTR);
        fd = SOCKET_FD_NONE;
    }
}
