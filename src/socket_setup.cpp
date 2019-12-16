#include "socket_setup.h"

#include <iostream>

extern "C"
{
    #include <sys/socket.h>
}

namespace socket_setup
{
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
            std::cerr << "Warning: Clearing the SO_LINGER option for socket with socket_fd = " << socket_fd <<
                " failed" << std::endl;
        }
    }
}
