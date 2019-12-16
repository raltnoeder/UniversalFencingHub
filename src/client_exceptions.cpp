#include "client_exceptions.h"

// @throws std::bad_alloc
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

// @throws std::bad_alloc
void ClientException::param_length_error_msg(
    std::string& error_msg,
    const std::string& param_key,
    const size_t max_length
)
{
    error_msg += "The value of the " + param_key + " parameter exceeds the maximum length of " +
        std::to_string(max_length) + " bytes";
}
