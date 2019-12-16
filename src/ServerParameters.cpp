#include "ServerParameters.h"

#include <cstddef>
#include <RangeException.h>
#include "server_exceptions.h"
#include "Shared.h"

// Should allow enough space for the parameter key, the split character and the maximum length
// of any of the parameter values
const size_t ServerParameters::MAX_PARAMETER_SIZE = 1100;

const char* const ServerParameters::KEY_PROTOCOL       = "protocol";
const char* const ServerParameters::KEY_BIND_ADDRESS   = "bind_address";
const char* const ServerParameters::KEY_TCP_PORT       = "tcp_port";
const char* const ServerParameters::KEY_FENCE_MODULE   = "fence_module";

const CharBuffer ServerParameters::OPT_PREFIX("--");

// @throws std::bad_alloc
ServerParameters::ServerParameters()
{
}

ServerParameters::~ServerParameters() noexcept
{
}

// @throws std::bad_alloc
void ServerParameters::initialize()
{
    add_entry(KEY_PROTOCOL, constraints::PROTOCOL_PARAM_SIZE);
    add_entry(KEY_BIND_ADDRESS, constraints::IP_ADDR_PARAM_SIZE);
    add_entry(KEY_TCP_PORT, constraints::PORT_PARAM_SIZE);
    add_entry(KEY_FENCE_MODULE, constraints::MODULE_PARAM_SIZE);

    mark_required(KEY_PROTOCOL);
    mark_required(KEY_BIND_ADDRESS);
    mark_required(KEY_TCP_PORT);
    mark_required(KEY_FENCE_MODULE);
}

// @throws std::bad_alloc, ServerException
void ServerParameters::read_parameters(const int argc, const char* const argv[])
{
    CharBuffer crt_arg(MAX_PARAMETER_SIZE);
    try
    {
        for (int idx = 1; idx < argc; ++idx)
        {
            crt_arg = argv[idx];

            if (crt_arg.starts_with(OPT_PREFIX))
            {
                crt_arg.substring(OPT_PREFIX.length(), crt_arg.length());
            }

            process_argument(crt_arg);
        }
    }
    catch (RangeException&)
    {
        std::string error_msg("The command line contains arguments that exceed the allowed length");
        throw Arguments::ArgumentsException(error_msg);
    }
}
