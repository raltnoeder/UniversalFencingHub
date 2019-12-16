#ifndef CLIENT_EXCEPTIONS_H
#define CLIENT_EXCEPTIONS_H

#include <stdexcept>

class ClientException : public std::exception
{
  public:
    ClientException(const std::string& error_msg);
    virtual ~ClientException() noexcept;
    ClientException(const ClientException& other) = default;
    ClientException(ClientException&& orig) = default;
    virtual ClientException& operator=(const ClientException& other) = default;
    virtual ClientException& operator=(ClientException&& orig) = default;
    virtual const char* what() const noexcept override;
  private:
    std::string message;
};

#endif /* CLIENT_EXCEPTIONS_H */
