#ifndef CLIENT_H
#define CLIENT_H

class Client
{
  public:
    Client();
    virtual ~Client() noexcept;
    Client(const Client& other) = delete;
    Client(Client&& orig) = default;
    virtual Client& operator=(const Client& other) = delete;
    virtual Client& operator=(Client&& orig) = default;

    virtual const uint32_t get_version_code() noexcept;
    virtual const char* get_version() noexcept;
};

#endif /* CLIENT_H */
