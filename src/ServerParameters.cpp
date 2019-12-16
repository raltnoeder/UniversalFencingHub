#include "ServerParameters.h"

#include <cstddef>
#include <RangeException.h>
#include "server_exceptions.h"
#include "Shared.h"

// Should be large enough to hold any of the parameter keys
const size_t ServerParameters::MAX_KEY_SIZE = 32;

// Should allow enough space for the parameter key, the split character and the maximum length
// of any of the parameter values
const size_t ServerParameters::MAX_PARAMETER_SIZE = MAX_KEY_SIZE + 1024 + 1;

const char* const ServerParameters::KEY_PROTOCOL       = "protocol";
const char* const ServerParameters::KEY_BIND_ADDRESS   = "bind_address";
const char* const ServerParameters::KEY_PORT           = "port";
const char* const ServerParameters::KEY_FENCE_MODULE   = "fence_module";

const CharBuffer ServerParameters::OPT_PREFIX("--");
const char ServerParameters::KEY_VALUE_SPLIT_CHAR = '=';

// @throws std::bad_alloc, ServerException
static void update_parameter(
    const CharBuffer& key,
    CharBuffer& parameter,
    bool& parameter_flag,
    const CharBuffer& argument,
    size_t split_idx
);

// @throws std::bad_alloc
static std::string missing_parameter_error_msg(const char* const param_key);

// @throws std::bad_alloc
ServerParameters::ServerParameters():
    protocol(constraints::PROTOCOL_PARAM_SIZE),
    bind_address(constraints::IP_ADDR_PARAM_SIZE),
    port(constraints::PORT_PARAM_SIZE),
    fence_module(constraints::MODULE_PARAM_SIZE)
{
}

ServerParameters::~ServerParameters() noexcept
{
}

// @throws std::bad_alloc, ServerException
void ServerParameters::read_parameters(const int argc, const char* const argv[])
{
    CharBuffer argument(MAX_PARAMETER_SIZE);
    CharBuffer key(MAX_KEY_SIZE);

    try
    {
        const size_t arg_count = static_cast<size_t> (argc);
        for (size_t idx = 1; idx < arg_count; ++idx)
        {
            argument = argv[idx];

            const size_t split_idx = argument.index_of(KEY_VALUE_SPLIT_CHAR);
            if (split_idx != CharBuffer::NPOS)
            {
                key.substring_from(argument, 0, split_idx);
                if (key.starts_with(OPT_PREFIX))
                {
                    key.substring(OPT_PREFIX.length(), key.length());
                }

                if (key == KEY_PROTOCOL)
                {
                    update_parameter(key, protocol, have_protocol, argument, split_idx);
                }
                else
                if (key == KEY_BIND_ADDRESS)
                {
                    update_parameter(key, bind_address, have_bind_address, argument, split_idx);
                }
                else
                if (key == KEY_PORT)
                {
                    update_parameter(key, port, have_port, argument, split_idx);
                }
                else
                if (key == KEY_FENCE_MODULE)
                {
                    update_parameter(key, fence_module, have_fence_module, argument, split_idx);
                }
                else
                {
                    std::string error_msg("Invalid command line parameter \"");
                    error_msg += key.c_str();
                    error_msg += "\": Unknown parameter key";
                    throw ServerInitException(error_msg);
                }
            }
            else
            {
                std::string error_msg("Invalid command line parameter \"");
                error_msg += key.c_str();
                error_msg += "\": No value was assigned";
                throw ServerInitException(error_msg);
            }
        }
    }
    catch (RangeException&)
    {
        std::string error_msg("The command line contains parameters with an out-of-range length");
        throw ServerInitException(error_msg);
    }
}

// @throws std::bad_alloc, ServerException
CharBuffer& ServerParameters::get_protocol()
{
    if (!have_protocol)
    {
        throw ServerInitException(missing_parameter_error_msg(KEY_PROTOCOL));
    }
    return protocol;
}

// @throws std::bad_alloc, ServerException
CharBuffer& ServerParameters::get_bind_address()
{
    if (!have_bind_address)
    {
        throw ServerInitException(missing_parameter_error_msg(KEY_BIND_ADDRESS));
    }
    return bind_address;
}

// @throws std::bad_alloc, ServerException
CharBuffer& ServerParameters::get_port()
{
    if (!have_port)
    {
        throw ServerInitException(missing_parameter_error_msg(KEY_PORT));
    }
    return port;
}

// @throws std::bad_alloc, ServerException
CharBuffer& ServerParameters::get_fence_module()
{
    if (!have_fence_module)
    {
        throw ServerInitException(missing_parameter_error_msg(KEY_FENCE_MODULE));
    }
    return fence_module;
}

// @throws std::bad_alloc, ServerException
static void update_parameter(
    const CharBuffer& key,
    CharBuffer& parameter,
    bool& parameter_flag,
    const CharBuffer& argument,
    size_t split_idx
)
{
    if (!parameter_flag)
    {
        parameter.substring_from(argument, split_idx + 1, argument.length());
        parameter_flag = true;
    }
    else
    {
        std::string error_msg("Duplicate parameter key \"");
        error_msg += key.c_str();
        error_msg += "\"";
        throw ServerInitException(error_msg);
    }
}

// @throws std::bad_alloc
static std::string missing_parameter_error_msg(const char* const param_key)
{
    std::string error_msg("Missing required command line parameter \"");
    error_msg += param_key;
    error_msg += "\"";
    return error_msg;
}
