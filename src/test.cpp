#include <cstddef>
#include <new>
#include <memory>
#include <stdexcept>
#include <iostream>

#include "ServerConnector.h"

int main()
{
    CharBuffer ipv4_proto(10);
    CharBuffer ipv4_address(40);
    CharBuffer ipv6_proto(10);
    CharBuffer ipv6_address(40);
    CharBuffer port_number(10);

    ipv4_proto = "IPV4";
    ipv4_address = "10.43.72.1";

    ipv6_proto = "IPV6";
    ipv6_address = "2001:858:107:1:bd56:84da:b564:4898";

    port_number = "2001";

    std::unique_ptr<ServerConnector> connector_ipv4(
        new ServerConnector(ipv4_proto, ipv4_address, port_number)
    );
    std::unique_ptr<ServerConnector> connector_ipv6(
        new ServerConnector(ipv6_proto, ipv6_address, port_number)
    );

    connector_ipv4->test();
    connector_ipv6->test();

    return 0;
}
