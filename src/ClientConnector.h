#ifndef CLIENTCONNECTOR_H
#define CLIENTCONNECTOR_H

extern "C"
{
    #include <sys/types.h>
    #include <sys/socket.h>
}

class ClientConnector
{
  public:
    // @throws std::bad_alloc, InetException
    ClientConnector(
        const CharBuffer& protocol_string,
        const CharBuffer& ip_string,
        const CharBuffer& port_string
    );
    virtual ~ClientConnector() noexcept;
    ClientConnector(const ClientConnector& other) = default;
    ClientConnector(ClientConnector&& orig) = default;
    virtual ClientConnector& operator=(const ClientConnector& other) = default;
    virtual ClientConnector& operator=(ClientConnector&& orig) = default;

  private:
    std::unique_ptr<char[]> address_mgr;
    std::unique_ptr<char[]> io_buffer_mgr;

    char*               io_buffer       = nullptr;
    MsgHeader           header;
    bool                have_header     = false;

    struct sockaddr*    address         = nullptr;
    socklen_t           address_length  = 0;
    int                 socket_domain   = AF_INET6;
    int                 socket_fd       = sys::FD_NONE;
};

#endif /* CLIENTCONNECTOR_H */
