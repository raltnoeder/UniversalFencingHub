#ifndef SERVER_EXCEPTIONS_H
#define SERVER_EXCEPTIONS_H

#include <stdexcept>

class ServerInitException : public std::exception
{
  public:
    // @throws std::bad_alloc
    ServerInitException(const std::string& error_msg);
    virtual ~ServerInitException() noexcept;
    ServerInitException(const ServerInitException& other) = default;
    ServerInitException(ServerInitException&& orig) = default;
    virtual ServerInitException& operator=(const ServerInitException& other) = default;
    virtual ServerInitException& operator=(ServerInitException&& orig) = default;
    virtual const char* what() const noexcept override;

  private:
    std::string message;
};

class PluginException : public std::exception
{
  public:
    PluginException();
    virtual ~PluginException() noexcept;
    PluginException(const PluginException& other) = default;
    PluginException(PluginException&& orig) = default;
    virtual PluginException& operator=(const PluginException& other) = default;
    virtual PluginException& operator=(PluginException&& orig) = default;
};

#endif /* SERVER_EXCEPTIONS_H */
