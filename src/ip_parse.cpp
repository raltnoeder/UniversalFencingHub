#include "ip_parse.h"

#include <cstdint>
#include <dsaext.h>
#include <integerparse.h>
#include <cstring>
#include "Shared.h"

// @throws InetException
static uint16_t parse_port_number(const CharBuffer& port_string)
{
    uint16_t port_number = 0;
    try
    {
        port_number = dsaext::parse_unsigned_int16_c_str(port_string.c_str(), port_string.length());
    }
    catch (dsaext::NumberFormatException&)
    {
        // No-op; handled the same way as port_number == 0
    }
    if (port_number == 0)
    {
        throw InetException(InetException::ErrorId::INVALID_PORT_NUMBER);
    }
    return port_number;
}

// @throws InetException
void parse_ipv4(const CharBuffer& ip_string, const CharBuffer& port_string, struct sockaddr_in& address)
{
    uint16_t port_number = parse_port_number(port_string);
    int rc = inet_pton(AF_INET, ip_string.c_str(), &(address.sin_addr));
    switch (rc)
    {
        case 0:
            // Invalid address string
            throw InetException(InetException::ErrorId::INVALID_ADDRESS);
            break;
        case 1:
            // Successful sockaddr initialization
            break;
        case -1:
            // Unsupported address family
            throw InetException(InetException::ErrorId::UNSUPPORTED_AF);
            break;
        default:
            // Undocumented return code
            throw InetException();
            break;
    }
    address.sin_family = AF_INET;
    address.sin_port = htons(port_number);
}

// @throws InetException
void parse_ipv6(const CharBuffer& ip_string, const CharBuffer& port_string, struct sockaddr_in6& address)
{
    uint16_t port_number = parse_port_number(port_string);
    int rc = inet_pton(AF_INET6, ip_string.c_str(), &(address.sin6_addr));
    switch (rc)
    {
        case 0:
            // Invalid address string
            throw InetException(InetException::ErrorId::INVALID_ADDRESS);
            break;
        case 1:
            // Successful sockaddr initialization
            break;
        case -1:
            // Unsupported address family
            throw InetException(InetException::ErrorId::UNSUPPORTED_AF);
            break;
        default:
            // Undocumented return code
            throw InetException();
            break;
    }
    address.sin6_family = AF_INET6;
    address.sin6_port = htons(port_number);
    address.sin6_flowinfo = 0;
    address.sin6_scope_id = 0;
}

// @throws std::bad_alloc, InetException
void init_socket_address(
    const CharBuffer& protocol_string,
    const CharBuffer& ip_string,
    const CharBuffer& port_string,
    int& socket_domain,
    std::unique_ptr<char[]>& address_mgr,
    struct sockaddr*& address,
    socklen_t& address_length
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
}
