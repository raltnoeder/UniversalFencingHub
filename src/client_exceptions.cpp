#include "client_exceptions.h"

ClientException::ClientException(const std::string& error_msg):
    message(error_msg)
{
}

ClientException::~ClientException() noexcept
{
}

const char* ClientException::what() const noexcept
{
    return message.c_str();
}
