#include "ClientConnector.h"

#include <CharBuffer.h>
#include "ip_parse.h"
#include "Shared.h"
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
