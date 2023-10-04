#ifndef SOCKET_SETUP_H
#define SOCKET_SETUP_H

#include <cstdint>

namespace socket_setup
{
    extern const uint8_t DSCP_CLASS_5;

    void set_no_linger(const int socket_fd);
    void set_dscp(const int socket_fd, const int socket_domain, const uint8_t dscp);
}

#endif /* SOCKET_SETUP_H */
