#include "ClientConnector.h"

#include "ip_parse.h"
#include "exceptions.h"

// @throws std::bad_alloc, InetException
ClientConnector::ClientConnector(
    const CharBuffer& protocol_string,
    const CharBuffer& ip_string,
    const CharBuffer& port_string
)
{
    init_socket_address(protocol_string, ip_string, port_string, socket_domain, address_mgr, address, address_length);
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
