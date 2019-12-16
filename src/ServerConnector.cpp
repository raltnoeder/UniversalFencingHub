#include <memory>
#include <iostream>
#include <algorithm>
#include <limits>

#include "ServerConnector.h"
#include "Shared.h"
#include "ip_parse.h"
#include "zero_memory.h"
#include "exceptions.h"
#include "socket_setup.h"

extern "C"
{
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <sys/select.h>
    #include <sys/types.h>
    #include <sys/socket.h>
}

const size_t ServerConnector::MAX_CONNECTIONS               = 24;
const size_t ServerConnector::MAX_CONNECTION_BACKLOG        = 24;

const size_t ServerConnector::NetClient::IO_BUFFER_SIZE     = 1024;
const size_t ServerConnector::NetClient::FIELD_SIZE         = 1024;
const size_t ServerConnector::NetClient::NODENAME_SIZE     = 255;

// @throws std::bad_alloc, InetException
ServerConnector::ServerConnector(
    Server& server_ref,
    SignalHandler& stop_signal_ref,
    const CharBuffer& protocol_string,
    const CharBuffer& ip_string,
    const CharBuffer& port_string
)
{
    ufh_server = &server_ref;
    stop_signal = &stop_signal_ref;

    selector_trigger[sys::PIPE_READ_END] = sys::FD_NONE;
    selector_trigger[sys::PIPE_WRITE_END] = sys::FD_NONE;

    init_socket_address(protocol_string, ip_string, port_string, socket_domain, address_mgr, address, address_length);

    if (socket_domain == AF_INET6)
    {
        clients_init_ipv6();
    }
    else
    if (socket_domain == AF_INET)
    {
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
    sys::close_fd(socket_fd);
}

// @throws InetException, OsException
void ServerConnector::run(WorkerPool& thread_pool)
{
    try
    {
        init();
        std::cout << "Initialization complete, ready to process requests" << std::endl;
        selector_loop(thread_pool);
    }
    catch (std::exception&)
    {
        cleanup();
        throw;
    }

    cleanup();
}

// @throws InetException, OsException
void ServerConnector::init()
{
    socket_fd = socket(socket_domain, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        throw InetException(InetException::ErrorId::SOCKET_ERROR);
    }

    socket_setup::set_no_linger(socket_fd);

    if (bind(socket_fd, address, address_length) != 0)
    {
        throw InetException(InetException::ErrorId::BIND_FAILED);
    }

    if (fcntl(socket_fd, F_SETFL, O_NONBLOCK) != 0)
    {
        throw OsException(OsException::ErrorId::NBLK_IO_ERROR);
    }

    if (listen(socket_fd, ServerConnector::MAX_CONNECTION_BACKLOG) != 0)
    {
        throw InetException(InetException::ErrorId::LISTEN_ERROR);
    }

    if (pipe(selector_trigger) != 0)
    {
        throw OsException(OsException::ErrorId::IPC_ERROR);
    }
    if (fcntl(selector_trigger[sys::PIPE_READ_END], F_SETFL, O_NONBLOCK) != 0 ||
        fcntl(selector_trigger[sys::PIPE_WRITE_END], F_SETFL, O_NONBLOCK) != 0)
    {
        throw OsException(OsException::ErrorId::NBLK_IO_ERROR);
    }
}

// @throws InetException, OsException
void ServerConnector::selector_loop(WorkerPool& thread_pool)
{
    stop_signal->enable_wakeup_fd(selector_trigger[sys::PIPE_WRITE_END]);
    while (!stop_signal->is_signaled())
    {
        FD_ZERO(read_fd_set);
        FD_ZERO(write_fd_set);

        int max_fd = 0;
        {
            FD_SET(selector_trigger[sys::PIPE_READ_END], read_fd_set);
            max_fd = std::max(max_fd, selector_trigger[sys::PIPE_READ_END]);

            std::unique_lock<std::mutex> com_lock(com_queue_lock);

            // Add the server socket to the read-selectable set
            if (com_queue.get_size() < MAX_CONNECTIONS)
            {
                FD_SET(socket_fd, read_fd_set);
                max_fd = std::max(max_fd, socket_fd);
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

        // Select ready file descriptors
        if (max_fd >= std::numeric_limits<int>::max())
        {
            throw OsException(OsException::ErrorId::INVALID_SELECT_FD);
        }
        ++max_fd;
        int select_rc = select(max_fd, read_fd_set, write_fd_set, nullptr, nullptr);
        if (select_rc >= 0)
        {
            // Accept pending connections
            if (FD_ISSET(socket_fd, read_fd_set) != 0)
            {
                accept_connection();
            }

            // Clear the selector trigger pipe
            if (FD_ISSET(selector_trigger[sys::PIPE_READ_END], read_fd_set) != 0)
            {
                char trigger_byte;
                ssize_t read_count = 0;
                do
                {
                    read_count = read(selector_trigger[sys::PIPE_READ_END], &trigger_byte, 1);
                }
                while (read_count >= 0 || (read_count == -1 && errno == EINTR));
            }

            // Perform pending client I/O operations
            {
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
                        close_connection(client);
                    }
                    else
                    if (FD_ISSET(client->socket_fd, read_fd_set) != 0)
                    {
                        // Client ready for receive
                        bool recv_complete = receive_message(client);
                        if (recv_complete)
                        {
                            client->current_phase = client->next_phase;

                            if (client->current_phase == NetClient::Phase::CANCELED)
                            {
                                close_connection(client);
                            }
                            else
                            if (client->current_phase == NetClient::Phase::PENDING)
                            {
                                com_queue.remove(client);
                                client->io_state = NetClient::IoOp::NOOP;

                                std::unique_lock<std::mutex> action_lock(action_queue_lock);
                                action_queue.add_last(client);
                                thread_pool.notify();
                            }
                        }
                    }
                    else
                    if (FD_ISSET(client->socket_fd, write_fd_set) != 0)
                    {
                        // Client ready for send
                        bool send_complete = send_message(client);
                        if (send_complete)
                        {
                            client->current_phase = client->next_phase;

                            if (client->current_phase == NetClient::Phase::CANCELED)
                            {
                                close_connection(client);
                            }
                            else
                            if (client->current_phase == NetClient::Phase::RECV)
                            {
                                client->clear_io_buffer();
                                client->next_phase = NetClient::Phase::PENDING;
                                client->io_state = NetClient::IoOp::READ;
                            }
                        }
                    }

                    client = next_client;
                }
            }
        }
        else
        if (errno != EINTR)
        {
            throw OsException(OsException::ErrorId::IO_ERROR);
        }
    }
}

// Caller must hold the com_queue_lock to avoid concurrent close of the selector_trigger pipe
// by the cleanup() method
void ServerConnector::wakeup_selector()
{
    if (selector_trigger[sys::PIPE_WRITE_END] != sys::FD_NONE)
    {
        // Write a byte to the pipe to wake up the selector
        // Failure to write because there is no space in the pipe is ignored, because then the selector will
        // wake up anyway. Failure to write due to an interrupted system call causes a retry.
        ssize_t write_length = 0;
        do
        {
            write_length = write(selector_trigger[sys::PIPE_WRITE_END], &ufh::WAKEUP_TRIGGER_BYTE, 1);
        }
        while (write_length == -1 && errno == EINTR);
    }
}

void ServerConnector::cleanup()
{
    // Close the server socket
    sys::close_fd(socket_fd);

    // Close connections of clients on the action queue
    {
        std::unique_lock<std::mutex> action_lock(action_queue_lock);
        for (NetClient* client = action_queue.remove_first(); client != nullptr; client = action_queue.remove_first())
        {
            close_connection(client);
        }
    }

    // Close connections of clients on the com queue
    // Notify worker threads to close connections of clients that are currently being processed
    {
        std::unique_lock<std::mutex> com_lock(com_queue_lock);
        stop_signal->disable_wakeup_fd();
        stop_signal->signal();

        for (NetClient* client = com_queue.remove_first(); client != nullptr; client = com_queue.remove_first())
        {
            close_connection(client);
        }

        // Close the selector trigger pipe
        sys::close_fd(selector_trigger[sys::PIPE_READ_END]);
        sys::close_fd(selector_trigger[sys::PIPE_WRITE_END]);
    }
}

void ServerConnector::accept_connection()
{
    try
    {
        ClientAlloc::scope_ptr new_client = client_pool.allocate_scope();
        NetClient* new_client_ptr = new_client.get();
        if (new_client_ptr == nullptr)
        {
            throw std::bad_alloc();
        }

        new_client_ptr->clear();
        new_client_ptr->socket_fd = accept(socket_fd, new_client_ptr->address, &(new_client_ptr->address_length));
        new_client_ptr->socket_domain = socket_domain;
        new_client_ptr->io_state = NetClient::IoOp::READ;
        new_client_ptr->current_phase = NetClient::Phase::RECV;
        new_client_ptr->next_phase = NetClient::Phase::PENDING;

        {
            std::unique_lock<std::mutex> lock(com_queue_lock);
            com_queue.add_last(new_client_ptr);
        }

        new_client.release();
    }
    catch (std::bad_alloc&)
    {
        // This section should not be unreachable, since MAX_CONNECTIONS == client_pool size
        std::cerr << "Unexpected error: ServerConnector: accept_connection: Client object allocation failed" <<
            std::endl;
    }
}

// Caller must have locked the client_queue_lock
void ServerConnector::close_connection(NetClient* const client)
{
    sys::close_fd(client->socket_fd);

    com_queue.remove(client);

    client->clear();
    client_pool.deallocate(client);
}

bool ServerConnector::receive_message(NetClient* const client)
{
    bool recv_complete_flag = false;

    const size_t req_read_size = client->io_offset < MsgHeader::HEADER_SIZE ?
        MsgHeader::HEADER_SIZE - client->io_offset :
        client->header.data_length - client->io_offset;
    const ssize_t read_size = recv(
        client->socket_fd,
        &(client->io_buffer[client->io_offset]),
        req_read_size,
        0 // Flags
    );
    if (read_size > 0)
    {
        client->io_offset += static_cast<size_t> (read_size);

        if (client->have_header)
        {
            if (client->io_offset >= client->header.data_length)
            {
                recv_complete_flag = true;
            }
        }
        else
        if (client->io_offset >= MsgHeader::HEADER_SIZE)
        {
            client->header.deserialize(client->io_buffer);
            if (client->header.data_length > NetClient::IO_BUFFER_SIZE)
            {
                client->header.data_length = NetClient::IO_BUFFER_SIZE;
            }
            client->have_header = true;

            if (client->header.data_length <= MsgHeader::HEADER_SIZE)
            {
                recv_complete_flag = true;
            }
        }
    }
    else
    {
        // read_size == 0: End of stream
        // read_size <= 0: I/O error
        close_connection(client);
    }

    return recv_complete_flag;
}

bool ServerConnector::send_message(NetClient* const client)
{
    bool send_complete_flag = false;

    if (!client->have_header)
    {
        if (client->header.data_length < MsgHeader::HEADER_SIZE)
        {
            client->header.data_length = static_cast<uint16_t> (MsgHeader::HEADER_SIZE);
        }
        else
        if (client->header.data_length > NetClient::IO_BUFFER_SIZE)
        {
            client->header.data_length = static_cast<uint16_t> (NetClient::IO_BUFFER_SIZE);
        }
        client->header.serialize(client->io_buffer);
        client->have_header = true;
    }

    const size_t req_write_size = std::min(
        static_cast<uint16_t> (NetClient::IO_BUFFER_SIZE),
        static_cast<uint16_t> (client->header.data_length - client->io_offset)
    );
    const ssize_t write_size = send(
        client->socket_fd,
        &(client->io_buffer[client->io_offset]),
        req_write_size,
        0 // Flags
    );
    if (write_size == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        close_connection(client);
    }
    else
    {
        if (write_size > 0)
        {
            client->io_offset += static_cast<size_t> (write_size);
        }

        if (client->io_offset >= client->header.data_length)
        {
            send_complete_flag = true;
        }
    }

    return send_complete_flag;
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

            client->current_phase = NetClient::Phase::EXECUTING;
            process_client_message(client);

            if (client->current_phase == NetClient::Phase::RECV || client->current_phase == NetClient::Phase::SEND)
            {
                // Continue client I/O
                std::unique_lock<std::mutex> com_lock(com_queue_lock);
                if (!stop_signal->is_signaled())
                {
                    com_queue.add_last(client);

                    wakeup_selector();
                }
                else
                {
                    // The selector loop is stopped (shutdown is in progress), end client communication
                    close_connection(client);
                }
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
        std::cerr << "Error: Unhandled exception caught in class ServerConnector, method process_action_queue" <<
            std::endl;
    }
}

// @throws ProtocolException
void ServerConnector::process_client_message(NetClient* const client)
{
    switch (static_cast<protocol::MsgType> (client->header.msg_type))
    {
        case protocol::MsgType::ECHO_REQUEST:
            client->clear_io_buffer();
            client->header.msg_type = static_cast<uint16_t> (protocol::MsgType::ECHO_REPLY);
            client->header.data_length = MsgHeader::HEADER_SIZE;
            client->current_phase = NetClient::Phase::SEND;
            client->next_phase = NetClient::Phase::RECV;
            client->io_state = NetClient::IoOp::WRITE;
            break;
        case protocol::MsgType::VERSION_REQUEST:
            // TODO: Implement version request
            break;
        case protocol::MsgType::FENCE_OFF:
            fence_action(&Server::fence_action_off, client);
            break;
        case protocol::MsgType::FENCE_ON:
            fence_action(&Server::fence_action_off, client);
            break;
        case protocol::MsgType::FENCE_REBOOT:
            fence_action(&Server::fence_action_off, client);
            break;
        case protocol::MsgType::FENCE_SUCCESS:
            // fall-through
        case protocol::MsgType::FENCE_FAIL:
            // fall-through
        case protocol::MsgType::ECHO_REPLY:
            // fall-through
        default:
            std::cerr << "Warning: Invalid request from client with socket_fd = " << client->socket_fd <<
                ", unknwon msg_type = " << client->header.msg_type << std::endl;
            // Protocol error, kick the client out
            client->current_phase = NetClient::Phase::CANCELED;
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
            if (client->key_buffer == protocol::NODENAME)
            {
                client->nodename = client->value_buffer;
            }
            else
            if (client->key_buffer == protocol::SECRET)
            {
                client->secret = client->value_buffer;
            }
        }

        client->clear_io_buffer();
        client->header.clear();

        if (client->nodename.length() > 0)
        {
            bool success_flag = (ufh_server->*fence)(client->nodename, client->secret);

            client->header.msg_type = success_flag ?
                static_cast<uint16_t> (protocol::MsgType::FENCE_SUCCESS) :
                static_cast<uint16_t> (protocol::MsgType::FENCE_FAIL);
            client->header.data_length = MsgHeader::HEADER_SIZE;
            client->current_phase = NetClient::Phase::SEND;
            // FIXME: For debugging, disconnect after replying; should probably go back to RECV for production release
            client->next_phase = NetClient::Phase::CANCELED;
            client->io_state = NetClient::IoOp::WRITE;
        }
    }
    catch (ProtocolException&)
    {
        std::cerr << "Warning: Protocol error, client socket_fd = " << client->socket_fd << std::endl;
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

// @throws std::bad_alloc
ServerConnector::NetClient::NetClient():
    key_buffer(FIELD_SIZE),
    value_buffer(FIELD_SIZE),
    nodename(NODENAME_SIZE),
    secret(protocol::MAX_SECRET_LENGTH)
{
    io_buffer_mgr = std::unique_ptr<char[]>(new char[IO_BUFFER_SIZE]);
    io_buffer = io_buffer_mgr.get();
}

ServerConnector::NetClient::~NetClient() noexcept
{
}

void ServerConnector::NetClient::clear() noexcept
{
    zero_memory(reinterpret_cast<char*> (address), static_cast<size_t> (address_length));
    socket_fd       = sys::FD_NONE;
    current_phase   = Phase::RECV;
    next_phase      = Phase::PENDING;
    io_state        = IoOp::NOOP;
    header.clear();
    nodename.wipe();
    secret.wipe();
    key_buffer.wipe();
    value_buffer.wipe();
    clear_io_buffer();
}

void ServerConnector::NetClient::clear_io_buffer() noexcept
{
    io_offset = 0;
    have_header = false;
    zero_memory(io_buffer, IO_BUFFER_SIZE);
}
