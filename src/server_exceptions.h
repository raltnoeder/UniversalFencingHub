#ifndef SERVER_EXCEPTIONS_H
#define SERVER_EXCEPTIONS_H

#include <stdexcept>

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
