#ifndef CLIENTCONNECTOR_H
#define CLIENTCONNECTOR_H

class ClientConnector
{
    ClientConnector();
    virtual ~ClientConnector() noexcept;
    ClientConnector(const ClientConnector& other) = default;
    ClientConnector(ClientConnector&& orig) = default;
    virtual ClientConnector& operator=(const ClientConnector& other) = default;
    virtual ClientConnector& operator=(ClientConnector&& orig) = default;
};

#endif /* CLIENTCONNECTOR_H */
