#include "ClientConnector.h"

#include "ip_parse.h"
#include "zero_memory.h"
#include "exceptions.h"

const size_t ClientConnector::IO_BUFFER_SIZE = 1024;

// @throws std::bad_alloc, InetException
ClientConnector::ClientConnector(
    const CharBuffer& protocol_string,
    const CharBuffer& ip_string,
    const CharBuffer& port_string
)
{
    init_socket_address(protocol_string, ip_string, port_string, socket_domain, address_mgr, address, address_length);

    io_buffer_mgr = std::unique_ptr<char[]>(new char[IO_BUFFER_SIZE]);
    io_buffer = io_buffer_mgr.get();
}

ClientConnector::~ClientConnector() noexcept
{
    sys::close_fd(socket_fd);
}

// @throws InetException, OsException
void ClientConnector::connect_to_server()
{
    // Allocate socket file descriptor
    socket_fd = socket(socket_domain, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        throw InetException(InetException::ErrorId::SOCKET_ERROR);
    }

    // Bind the socket to a local address
    if (socket_domain == AF_INET6)
    {
        struct sockaddr_in6 local_address;
        const size_t local_address_length = sizeof (struct sockaddr_in6);

        int rc = inet_pton(AF_INET6, inet::IPV6_LOCAL_BIND_ADDRESS, &(local_address.sin6_addr));
        check_inet_pton_rc(rc);

        local_address.sin6_family = AF_INET6;
        local_address.sin6_port = 0;
        local_address.sin6_flowinfo = 0;
        local_address.sin6_scope_id = 0;

        if (bind(socket_fd, reinterpret_cast<struct sockaddr*> (&local_address), local_address_length) != 0)
        {
            throw InetException(InetException::ErrorId::BIND_FAILED);
        }
    }
    else
    if (socket_domain == AF_INET)
    {
        struct sockaddr_in local_address;
        const size_t local_address_length = sizeof (struct sockaddr_in);

        int rc = inet_pton(AF_INET, inet::IPV4_LOCAL_BIND_ADDRESS, &(local_address.sin_addr));
        check_inet_pton_rc(rc);

        local_address.sin_family = AF_INET;
        local_address.sin_port = 0;

        if (bind(socket_fd, reinterpret_cast<struct sockaddr*> (&local_address), local_address_length) != 0)
        {
            throw InetException(InetException::ErrorId::BIND_FAILED);
        }
    }
    else
    {
        throw InetException(InetException::ErrorId::UNKNOWN_AF);
    }

    // Connect to the selected peer
    if (connect(socket_fd, address, address_length) != 0)
    {
        if (errno == EADDRINUSE || errno == EBADF || errno == EISCONN)
        {
            throw InetException(InetException::ErrorId::SOCKET_ERROR);
        }
        else
        if (errno == ENETUNREACH)
        {
            throw InetException(InetException::ErrorId::NETWORK_UNREACHABLE);
        }
        else
        if (errno == ECONNREFUSED)
        {
            throw InetException(InetException::ErrorId::CONNECTION_REFUSED);
        }
        else
        {
            throw InetException(InetException::ErrorId::CONNECT_FAILED);
        }
    }
}

void ClientConnector::disconnect_from_server() noexcept
{
    sys::close_fd(socket_fd);
}

void ClientConnector::clear_io_buffer() noexcept
{
    zero_memory(io_buffer, IO_BUFFER_SIZE);
}

// @throws InetException, OsException
bool ClientConnector::check_connection()
{
    header.set_msg_type(MsgType::ECHO_REQUEST);
    header.data_length = MsgHeader::HEADER_SIZE;

    send_message();

    receive_message();

    return header.is_msg_type(MsgType::ECHO_REPLY);
}

// @throws std::bad_alloc, InetException, OsException, ProtocolException
bool ClientConnector::fence_poweroff(const std::string& nodename, const std::string& secret)
{
    return fence_action_impl(MsgType::POWER_OFF, nodename, secret);
}

// @throws std::bad_alloc, InetException, OsException, ProtocolException
bool ClientConnector::fence_poweron(const std::string& nodename, const std::string& secret)
{
    return fence_action_impl(MsgType::POWER_ON, nodename, secret);
}

// @throws std::bad_alloc, InetException, OsException, ProtocolException
bool ClientConnector::fence_reboot(const std::string& nodename, const std::string& secret)
{
    return fence_action_impl(MsgType::REBOOT, nodename, secret);
}

// @throws InetException, OsException, ProtocolException
bool ClientConnector::fence_action_impl(
    const MsgType& msg_type,
    const std::string& nodename,
    const std::string& secret
)
{
    std::string nodename_param(protocol::NODENAME);
    nodename_param += protocol::KEY_VALUE_SPLIT_SEQ.c_str() + nodename;

    std::string secret_param(protocol::SECRET);
    secret_param += protocol::KEY_VALUE_SPLIT_SEQ.c_str() + secret;


    header.set_msg_type(msg_type);
    size_t offset = MsgHeader::HEADER_SIZE;
    protocol::write_field(io_buffer, IO_BUFFER_SIZE, offset, nodename_param);
    protocol::write_field(io_buffer, IO_BUFFER_SIZE, offset, secret_param);
    header.data_length = static_cast<uint16_t> (offset);

    send_message();

    receive_message();

    bool rc = false;
    if (header.is_msg_type(MsgType::FENCE_SUCCESS))
    {
        rc = true;
    }
    else
    if (!header.is_msg_type(MsgType::FENCE_FAIL))
    {
        throw ProtocolException();
    }

    return rc;
}

// @throws InetException, OsException
void ClientConnector::send_message()
{
    size_t io_offset = 0;
    const size_t send_length = std::min(
        std::max(MsgHeader::HEADER_SIZE, static_cast<size_t> (header.data_length)),
        IO_BUFFER_SIZE
    );
    header.serialize(io_buffer);
    while (io_offset < send_length)
    {
        const size_t req_write_size = send_length - io_offset;
        const ssize_t write_size = send(
            socket_fd,
            &(io_buffer[io_offset]),
            req_write_size,
            0 // Flags
        );
        if (write_size > 0)
        {
            io_offset += static_cast<size_t> (write_size);
        }
        else
        if (write_size == -1)
        {
            disconnect_from_server();
        }
    }
    if (io_offset < header.data_length)
    {
        throw OsException(OsException::ErrorId::IO_ERROR);
    }
}

// @throws InetExeption, OsException
void ClientConnector::receive_message()
{
    header.clear();
    clear_io_buffer();

    bool have_header = false;
    size_t io_offset = 0;
    size_t receive_length = MsgHeader::HEADER_SIZE;
    while (io_offset < receive_length)
    {
        const size_t req_read_size = receive_length - io_offset;
        const ssize_t read_size = recv(
            socket_fd,
            &(io_buffer[io_offset]),
            req_read_size,
            0 // Flags
        );
        if (read_size > 0)
        {
            io_offset += read_size;

            if (!have_header && io_offset >= MsgHeader::HEADER_SIZE)
            {
                header.deserialize(io_buffer);
                have_header = true;

                receive_length = std::min(static_cast<size_t> (header.data_length), IO_BUFFER_SIZE);
            }
        }
        else
        if (read_size == -1)
        {
            disconnect_from_server();
        }
    }
    if (!have_header || io_offset < header.data_length)
    {
        throw OsException(OsException::ErrorId::IO_ERROR);
    }
}
