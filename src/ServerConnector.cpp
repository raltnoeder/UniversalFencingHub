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

    read_fd_set_mgr = std::unique_ptr<fd_set>(new fd_set);
    write_fd_set_mgr = std::unique_ptr<fd_set>(new fd_set);
    read_fd_set = read_fd_set_mgr.get();
    write_fd_set = write_fd_set_mgr.get();
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

// @throws InetException
void ServerConnector::run()
{
    socket_fd = socket(socket_domain, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        throw InetException(InetException::ErrorId::SOCKET_ERROR);
    }

    if (bind(socket_fd, address, address_length) != 0)
    {
        throw InetException(InetException::ErrorId::BIND_FAILED);
    }

    if (fcntl(socket_fd, F_SETFL, O_NONBLOCK) != 0)
    {
        throw InetException(InetException::ErrorId::NIO_ERROR);
    }

    if (listen(socket_fd, ServerConnector::MAX_CONNECTION_BACKLOG) != 0)
    {
        throw InetException(InetException::ErrorId::LISTEN_ERROR);
    }

    while (!stop_flag)
    {
        FD_ZERO(read_fd_set);
        FD_ZERO(write_fd_set);

        int max_fd = 0;
        {
            std::unique_lock<std::mutex> lock(client_queue_lock);

            // Add the server socket to the read-selectable set
            if (client_queue.get_size() < MAX_CONNECTIONS)
            {
                FD_SET(socket_fd, read_fd_set);
                max_fd = socket_fd;
            }

            // Add active clients to the read/write sets
            for (NetClient* client = client_queue.get_first(); client != nullptr; client = client->get_next_node())
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
        if (select_rc == -1)
        {
            throw OsException(OsException::ErrorId::IO_ERROR);
        }
        if (FD_ISSET(socket_fd, read_fd_set) != 0)
        {
            accept_connection();
        }
        // TODO: Check active clients for operations
        {
            std::unique_lock<std::mutex> lock(client_queue_lock);
            for (NetClient* client = client_queue.get_first(); client != nullptr; client = client->get_next_node())
            {
                // TODO
            }
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
            std::unique_lock<std::mutex> lock(client_queue_lock);
            client_queue.add_last(new_client_ptr);
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

void ServerConnector::close_connection(NetClient* const current_client)
{
    close_fd(current_client->socket_fd);
    current_client->clear();

    std::unique_lock<std::mutex> lock(client_queue_lock);
    client_queue.remove(current_client);

    client_pool.deallocate(current_client);
}

void ServerConnector::receive_message(NetClient* const current_client)
{
    const size_t req_read_size = current_client->io_offset < MsgHeader::HEADER_SIZE ?
        MsgHeader::HEADER_SIZE - MsgHeader::HEADER_SIZE :
        NetClient::IO_BUFFER_SIZE - current_client->io_offset;
    const ssize_t read_size = recv(
        current_client->socket_fd,
        &(current_client->io_buffer[current_client->io_offset]),
        req_read_size,
        0 // Flags
    );
    if (read_size == -1)
    {
        // IO Error
        close_connection(current_client);
    }
    else
    if (read_size == 0)
    {
        // End of stream
        close_connection(current_client);
    }
    else
    {
        current_client->io_offset += static_cast<size_t> (read_size);
    }

    if (current_client->have_header)
    {
        if (current_client->io_offset >= current_client->header.data_length)
        {
            std::unique_lock<std::mutex> lock(client_queue_lock);
            current_client->io_state = NetClient::IoOp::NOOP;
            current_client->current_phase = NetClient::Phase::PENDING;

            // TODO: Notify the worker pool to pick up pending tasks
        }
    }
    else
    if (current_client->io_offset >= MsgHeader::HEADER_SIZE)
    {
        current_client->header.msg_type = MsgHeader::bytes_to_field_value(
            current_client->io_buffer, MsgHeader::MSG_TYPE_OFFSET
        );
        current_client->header.data_length = std::min(
            static_cast<uint16_t> (NetClient::IO_BUFFER_SIZE),
            MsgHeader::bytes_to_field_value(
                current_client->io_buffer, MsgHeader::DATA_LENGTH_OFFSET
            )
        );
        current_client->have_header = true;
    }
}

void ServerConnector::send_message(NetClient* const current_client)
{
    if (!current_client->have_header)
    {
        MsgHeader::field_value_to_bytes(
            current_client->header.msg_type,
            current_client->io_buffer,
            MsgHeader::MSG_TYPE_OFFSET
        );
        MsgHeader::field_value_to_bytes(
            current_client->header.data_length,
            current_client->io_buffer,
            MsgHeader::DATA_LENGTH_OFFSET
        );
        current_client->have_header = true;
    }

    const size_t req_write_size = std::min(
        static_cast<uint16_t> (NetClient::IO_BUFFER_SIZE),
        current_client->header.data_length
    );
    const ssize_t write_size = send(
        current_client->socket_fd,
        &(current_client->io_buffer[current_client->io_offset]),
        req_write_size,
        0 // Flags
    );
    if (write_size > 0)
    {
        current_client->io_offset += static_cast<size_t> (write_size);
    }
    else
    if (write_size == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        close_connection(current_client);
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
        client_queue.add_last(current_client.get());
        current_client.release();
    }
    std::cout << '\n';
    for (int idx = 0; idx < test_count; ++idx)
    {
        ClientAlloc::scope_ptr current_client = client_pool.make_scope_ptr(client_queue.remove_first());
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
