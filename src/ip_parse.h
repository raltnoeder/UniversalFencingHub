#ifndef IP_PARSE_H
#define IP_PARSE_H

#include <CharBuffer.h>
#include "exceptions.h"

extern "C"
{
    #include <arpa/inet.h>
    #include <sys/types.h>
    #include <sys/socket.h>
}

// @throws InetException
void parse_ipv4(const CharBuffer& ip_string, const CharBuffer& port_string, struct sockaddr_in& address);

// @throws InetException
void parse_ipv6(const CharBuffer& ip_string, const CharBuffer& port_string, struct sockaddr_in6& address);

// @throws std::bad_alloc, InetException
void init_socket_address(
    const CharBuffer& protocol_string,
    const CharBuffer& ip_string,
    const CharBuffer& port_string,
    int& socket_domain,
    std::unique_ptr<char[]>& address_mgr,
    struct sockaddr*& address,
    socklen_t& address_length
);

// @throws InetException
void check_inet_pton_rc(int rc);

#endif /* IP_PARSE_H */
