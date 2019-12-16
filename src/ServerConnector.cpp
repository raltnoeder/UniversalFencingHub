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

const size_t   ServerConnector::MAX_CONNECTIONS             = 24;
const size_t   ServerConnector::MAX_CONNECTION_BACKLOG      = 24;
const int      ServerConnector::SOCKET_FD_NONE              = -1;

const size_t ServerConnector::NetClient::IO_BUFFER_SIZE     = 1024;
const size_t ServerConnector::NetClient::FIELD_SIZE         = 1024;
const size_t ServerConnector::NetClient::NODE_NAME_SIZE     = 255;

// @throws std::bad_alloc, InetException
ServerConnector::ServerConnector(
    Server* const server_ref,
    const CharBuffer& protocol_string,
    const CharBuffer& ip_string,
    const CharBuffer& port_string
)
{
    ufh_server = server_ref;

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

        clients_init_ipv6();
    }
    else
    if (socket_domain == AF_INET)
    {
        struct sockaddr_in* inet_address = reinterpret_cast<struct sockaddr_in*> (address);
        parse_ipv4(ip_string, port_string, *inet_address);

        clients_init_ipv4();
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
                std::cout << "ServerConnector: Adding server with socket_fd = " << socket_fd <<
                    " to read_fd_set" << std::endl;
                FD_SET(socket_fd, read_fd_set);
                max_fd = socket_fd;
            }
            else
            {
                std::cout << "ServerConnector: com_queue is full, size = " << com_queue.get_size() << std::endl;
            }

            // Add active clients to the read/write sets
            for (NetClient* client = com_queue.get_first(); client != nullptr; client = client->get_next_node())
            {
                if (client->io_state == NetClient::IoOp::READ)
                {
                    std::cout << "ServerConnector: Adding client with socket_fd = " << client->socket_fd <<
                        " to read_fd_set" << std::endl;
                    FD_SET(client->socket_fd, read_fd_set);
                    max_fd = std::max(max_fd, client->socket_fd);
                }
                else
                if (client->io_state == NetClient::IoOp::WRITE)
                {
                    std::cout << "ServerConnector: Adding client with socket_fd = " << client->socket_fd <<
                        " to write_fd_set" << std::endl;
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
        std::cout << "ServerConnector: select() triggered" << std::endl;

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
        std::cout << "ServerConnector: Allocating client object" << std::endl;
        ClientAlloc::scope_ptr new_client = client_pool.allocate_scope();
        NetClient* new_client_ptr = new_client.get();
        if (new_client_ptr == nullptr)
        {
            throw std::bad_alloc();
        }
        std::cout << "ServerConnector: Initializing client object" << std::endl;
        std::cout << "ServerConnector: Client clear()" << std::endl;
        new_client_ptr->clear();
        std::cout << "ServerConnector: Client accept()" << std::endl;
        new_client_ptr->socket_fd = accept(socket_fd, new_client_ptr->address, &(new_client_ptr->address_length));
        std::cout << "ServerConnector: Client assign socket_domain" << std::endl;
        new_client_ptr->socket_domain = socket_domain;
        std::cout << "ServerConnector: Client assign io_state" << std::endl;
        new_client_ptr->io_state = NetClient::IoOp::READ;
        std::cout << "ServerConnector: Client assign current_phase" << std::endl;
        new_client_ptr->current_phase = NetClient::Phase::RECV;

        std::cout << "ServerConnector: Queueing client object" << std::endl;
        {
            std::unique_lock<std::mutex> lock(com_queue_lock);
            com_queue.add_last(new_client_ptr);
        }

        new_client.release();
    }
    catch (std::bad_alloc&)
    {
        std::cout << "ServerConnector: Client object allocation failed" << std::endl;
        // No free NetClient objects
        // TODO: Log implementation error, this section should not be reached,
        //       since MAX_CONNECTIONS == client_pool size
    }
}

// Caller must have locked the client_queue_lock
void ServerConnector::close_connection(NetClient* const client)
{
    std::cout << "ServerConnector: close_connection() client socket_fd = " << client->socket_fd << std::endl;
    close_fd(client->socket_fd);

    com_queue.remove(client);

    client->clear();
    client_pool.deallocate(client);
}

bool ServerConnector::receive_message(NetClient* const client)
{
    bool pending_flag = false;

    const size_t req_read_size = client->io_offset < MsgHeader::HEADER_SIZE ?
        MsgHeader::HEADER_SIZE - client->io_offset :
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
        std::cout << "ServerConnector: receive_message(): I/O error" << std::endl;
        close_connection(client);
    }
    else
    if (read_size == 0)
    {
        // End of stream
        std::cout << "ServerConnector: receive_message(): End of stream" << std::endl;
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
        std::cout << "ServerConnector: Message type = " << client->header.msg_type << ", data length = " <<
            client->header.data_length << std::endl;
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
        std::cout << "ServerConnector: send_message(): I/O error" << std::endl;
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

            client->current_phase = NetClient::Phase::EXECUTING;
            process_client_message(client);

            if (client->current_phase == NetClient::Phase::RECV || client->current_phase == NetClient::Phase::SEND)
            {
                // Continue client I/O
                std::unique_lock<std::mutex> com_lock(com_queue_lock);
                com_queue.add_last(client);
            }
            else
            {
                // End client communication
                close_connection(client);
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

// @throws ProtocolException
void ServerConnector::process_client_message(NetClient* const client)
{
    switch (static_cast<msgtype> (client->header.msg_type))
    {
        case msgtype::ECHO_REQUEST:
            break;
        case msgtype::VERSION_REQUEST:
            break;
        case msgtype::POWER_OFF:
            fence_action(&Server::action_power_off, client);
        case msgtype::POWER_ON:
            fence_action(&Server::action_power_off, client);
        case msgtype::REBOOT:
            fence_action(&Server::action_power_off, client);
        case msgtype::FENCE_SUCCESS:
            // fall-through
        case msgtype::FENCE_FAIL:
            // fall-through
        case msgtype::ECHO_REPLY:
            // fall-through
        default:
            throw ProtocolException();
            break;
    }
}

void ServerConnector::fence_action(const Server::fence_action_method fence, NetClient* const client)
{
    try
    {
        size_t field_offset = client->header.HEADER_SIZE;
        while (field_offset < client->io_offset)
        {
            protocol::read_field(client->io_buffer, client->io_offset, field_offset, client->key_buffer);
            protocol::split_key_value_pair(client->key_buffer, client->value_buffer);
            if (client->key_buffer == protocol::NODE_NAME)
            {
                client->node_name = client->value_buffer;
            }
        }

        client->clear_io_buffer();
        client->header.clear();
        if (client->node_name.length() > 0)
        {
            bool success_flag = (ufh_server->*fence)(client->node_name);

            client->header.msg_type = success_flag ?
                static_cast<uint16_t> (msgtype::FENCE_SUCCESS) :
                static_cast<uint16_t> (msgtype::FENCE_FAIL);
            client->header.field_value_to_bytes(
                client->header.msg_type, client->io_buffer, MsgHeader::MSG_TYPE_OFFSET
            );
            client->header.field_value_to_bytes(
                client->header.data_length, client->io_buffer, MsgHeader::DATA_LENGTH_OFFSET
            );
            client->current_phase = NetClient::Phase::SEND;
            client->io_state = NetClient::IoOp::WRITE;
        }
    }
    catch (ProtocolException&)
    {
        client->current_phase = NetClient::Phase::CANCELED;
        client->io_state = NetClient::IoOp::NOOP;
    }
}

// @throws std::bad_alloc
void ServerConnector::clients_init_ipv4()
{
    const size_t pool_size = client_pool.get_pool_size();
    const size_t alloc_size = sizeof (struct sockaddr_in);
    for (size_t idx = 0; idx < pool_size; ++idx)
    {
        NetClient* client = client_pool.object_at_index(idx);
        if (client != nullptr)
        {
            client->socket_domain = AF_INET;
            client->address_mgr = std::unique_ptr<char[]>(new char[alloc_size]);
            client->address = reinterpret_cast<struct sockaddr*> (client->address_mgr.get());
            client->address_length = alloc_size;
        }
        else
        {
            throw std::logic_error("client_init_ipv4(): Implementation error: client_pool returned nullptr");
        }
    }
}

// @throws std::bad_alloc
void ServerConnector::clients_init_ipv6()
{
    const size_t pool_size = client_pool.get_pool_size();
    const size_t alloc_size = sizeof (struct sockaddr_in6);
    for (size_t idx = 0; idx < pool_size; ++idx)
    {
        NetClient* client = client_pool.object_at_index(idx);
        if (client != nullptr)
        {
            client->socket_domain = AF_INET6;
            client->address_mgr = std::unique_ptr<char[]>(new char[alloc_size]);
            client->address = reinterpret_cast<struct sockaddr*> (client->address_mgr.get());
            client->address_length = alloc_size;
        }
        else
        {
            throw std::logic_error("client_init_ipv6(): Implementation error: client_pool returned nullptr");
        }
    }
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

// @throws std::bad_alloc
ServerConnector::NetClient::NetClient():
    key_buffer(FIELD_SIZE),
    value_buffer(FIELD_SIZE),
    node_name(NODE_NAME_SIZE)
{
    io_buffer_mgr = std::unique_ptr<char[]>(new char[IO_BUFFER_SIZE]);
    io_buffer = io_buffer_mgr.get();
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
    node_name.wipe();
    key_buffer.wipe();
    value_buffer.wipe();
    clear_io_buffer();
}

void ServerConnector::NetClient::clear_io_buffer() noexcept
{
    io_offset = 0;
    zero_memory(io_buffer, IO_BUFFER_SIZE);
}
