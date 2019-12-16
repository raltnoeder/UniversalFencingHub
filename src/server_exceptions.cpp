#include "server_exceptions.h"

// @throws std::bad_alloc
ServerInitException::ServerInitException(const std::string& error_msg):
    message(error_msg)
{
}

ServerInitException::~ServerInitException() noexcept
{
}

const char* ServerInitException::what() const noexcept
{
    return message.c_str();
}

PluginException::PluginException()
{
}

PluginException::~PluginException() noexcept
{
}
