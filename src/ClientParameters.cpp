#include "ClientParameters.h"

#include <iostream>
#include <RangeException.h>
#include "Shared.h"
#include "exceptions.h"

const char* const ClientParameters::KEY_ACTION("action");
const char* const ClientParameters::KEY_PROTOCOL("protocol");
const char* const ClientParameters::KEY_IP_ADDRESS("ip_address");
const char* const ClientParameters::KEY_TCP_PORT("tcp_port");
const char* const ClientParameters::KEY_SECRET("secret");
const char* const ClientParameters::KEY_NODENAME("nodename");

const char* const ClientParameters::ACTION_OFF("off");
const char* const ClientParameters::ACTION_ON("on");
const char* const ClientParameters::ACTION_REBOOT("reboot");
const char* const ClientParameters::ACTION_METADATA("metadata");
const char* const ClientParameters::ACTION_STATUS("status");
const char* const ClientParameters::ACTION_LIST("list");
const char* const ClientParameters::ACTION_MONITOR("monitor");
const char* const ClientParameters::ACTION_START("start");
const char* const ClientParameters::ACTION_STOP("stop");

const size_t ClientParameters::MAX_PARAMETER_SIZE = 512;

ClientParameters::ClientParameters()
{
}

ClientParameters::~ClientParameters() noexcept
{
}

// @throws std::bad_alloc
void ClientParameters::initialize()
{
    add_entry(KEY_ACTION, constraints::ACTION_PARAM_SIZE);
    add_entry(KEY_PROTOCOL, constraints::PROTOCOL_PARAM_SIZE);
    add_entry(KEY_IP_ADDRESS, constraints::IP_ADDR_PARAM_SIZE);
    add_entry(KEY_TCP_PORT, constraints::PORT_PARAM_SIZE);
    add_entry(KEY_NODENAME, constraints::NODENAME_PARAM_SIZE);
    add_entry(KEY_SECRET, constraints::SECRET_PARAM_SIZE);

    mark_required(KEY_ACTION);
}

// @throws std::bad_alloc, OsException, ArgumentsException
void ClientParameters::read_parameters()
{
    std::unique_ptr<char[]> line_buffer_mgr(new char[MAX_PARAMETER_SIZE]);
    char* const line_buffer = line_buffer_mgr.get();

    try
    {
        std::cin.exceptions(std::ios_base::badbit);
        while (std::cin.good())
        {
            std::cin.getline(line_buffer, MAX_PARAMETER_SIZE);
            const std::streamsize read_count = std::cin.gcount();
            if (read_count > 1)
            {
                const std::streamsize line_length = read_count - 1;
                CharBuffer crt_arg(static_cast<size_t> (line_length));
                crt_arg = line_buffer;

                process_argument(crt_arg);
            }
        }
        if (std::cin.fail() && !std::cin.eof())
        {
            throw OsException(OsException::ErrorId::IO_ERROR);
        }
    }
    catch (std::ios_base::failure&)
    {
        throw OsException(OsException::ErrorId::IO_ERROR);
    }
}
