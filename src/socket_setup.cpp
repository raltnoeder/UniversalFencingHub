#include "socket_setup.h"
#include "Shared.h"

#include <iostream>

extern "C"
{
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/ip.h>
}

namespace socket_setup
{
    const uint8_t DSCP_CLASS_5      = 0x28;

    void set_no_linger(const int socket_fd)
    {
        struct linger linger_setup;
        linger_setup.l_onoff = 0;
        linger_setup.l_linger = 0;

        int rc = setsockopt(
            socket_fd, SOL_SOCKET, SO_LINGER, &linger_setup,
            static_cast<socklen_t> (sizeof (linger_setup))
        );
        if (rc != 0)
        {
            std::cerr << ufh::LOGPFX_WARNING <<
                "Warning: Clearing the SO_LINGER option for socket with socket_fd = " << socket_fd <<
                " failed" << std::endl;
        }
    }

    void set_dscp(const int socket_fd, const int socket_domain, const uint8_t dscp)
    {
        if (dscp <= 0x3F)
        {
            const unsigned int field_value = dscp << 2;
            if (socket_domain == AF_INET6)
            {
                setsockopt(
                    socket_fd, IPPROTO_IPV6, IPV6_TCLASS,
                    static_cast<const void*> (&field_value), sizeof (field_value)
                );
            }
            setsockopt(
                socket_fd, IPPROTO_IP, IP_TOS,
                static_cast<const void*> (&field_value), sizeof (field_value)
            );
        }
    }
}
