#ifndef IP_PARSE_H
#define IP_PARSE_H

#include <CharBuffer.h>
#include "exceptions.h"

extern "C"
{
    #include <arpa/inet.h>
}

// @throws InetException
void parse_ipv4(const CharBuffer& ip_string, const CharBuffer& port_string, struct sockaddr_in& address);

// @throws InetException
void parse_ipv6(const CharBuffer& ip_string, const CharBuffer& port_string, struct sockaddr_in6& address);

#endif /* IP_PARSE_H */
