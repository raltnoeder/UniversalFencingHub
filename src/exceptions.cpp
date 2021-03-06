#include "exceptions.h"

const char* const InetException::DSC_UNKNOWN                = "No error description is available";
const char* const InetException::DSC_INVALID_ADDRESS        =
    "Network address not valid for the selected address family";
const char* const InetException::DSC_UNKNOWN_AF             = "Unknown address family specified";
const char* const InetException::DSC_UNSUPPORTED_AF         = "Selected address family not supported by the platform";
const char* const InetException::DSC_INVALID_PORT_NUMBER    = "IP port number not valid";
const char* const InetException::DSC_SOCKET_ERROR           = "Network socket error";
const char* const InetException::DSC_BIND_FAILED            = "Binding a socket failed";
const char* const InetException::DSC_LISTEN_ERROR           = "Server socket listen(...) failed";
const char* const InetException::DSC_NETWORK_UNREACHABLE    = "The destination network is unreachable";
const char* const InetException::DSC_CONNECTION_REFUSED     = "The connection was refused by the peer";
const char* const InetException::DSC_CONNECT_FAILED =
    "The connection attempt failed (connect() system call failed)";

InetException::InetException() noexcept
{
}

InetException::InetException(const InetException::ErrorId error) noexcept
{
    exc_error = error;
}

InetException::~InetException() noexcept
{
}

const InetException::ErrorId InetException::get_error_id() const noexcept
{
    return exc_error;
}

const char* InetException::what() const noexcept
{
    return get_error_description();
}

const char* InetException::get_error_description() const noexcept
{
    const char* description = DSC_UNKNOWN;
    switch (exc_error)
    {
        case ErrorId::INVALID_ADDRESS:
            description = DSC_INVALID_ADDRESS;
            break;
        case ErrorId::UNKNOWN_AF:
            description = DSC_UNKNOWN_AF;
            break;
        case ErrorId::UNSUPPORTED_AF:
            description = DSC_UNSUPPORTED_AF;
            break;
        case ErrorId::INVALID_PORT_NUMBER:
            description = DSC_INVALID_PORT_NUMBER;
            break;
        case ErrorId::SOCKET_ERROR:
            description = DSC_SOCKET_ERROR;
            break;
        case ErrorId::BIND_FAILED:
            description = DSC_BIND_FAILED;
            break;
        case ErrorId::LISTEN_ERROR:
            description = DSC_LISTEN_ERROR;
            break;
        case ErrorId::NETWORK_UNREACHABLE:
            description = DSC_NETWORK_UNREACHABLE;
            break;
        case ErrorId::CONNECTION_REFUSED:
            description = DSC_CONNECTION_REFUSED;
            break;
        case ErrorId::CONNECT_FAILED:
            description = DSC_CONNECT_FAILED;
            break;
        case ErrorId::UNKNOWN:
            // fall-through
        default:
            break;
    }
    return description;
}

const char* const OsException::DSC_UNKNOWN              = "No error description is available";
const char* const OsException::DSC_INVALID_SELECT_FD    = "Invalid filedescriptor for select()";
const char* const OsException::DSC_IO_ERROR             = "I/O error";
const char* const OsException::DSC_NBLK_IO_ERROR        = "Non-blocking I/O initialization failed";
const char* const OsException::DSC_IPC_ERROR            = "IPC setup failed";
const char* const OsException::DSC_DYN_LOAD_ERROR       = "Dynamic loader/linker failed";
const char* const OsException::DSC_SIGNAL_HND_ERROR     = "Signal handler setup failed";

OsException::OsException() noexcept
{
}

OsException::OsException(const OsException::ErrorId error) noexcept
{
    exc_error = error;
}

OsException::~OsException() noexcept
{
}

const OsException::ErrorId OsException::get_error_id() const noexcept
{
    return exc_error;
}

const char* OsException::get_error_description() const noexcept
{
    const char* description = DSC_UNKNOWN;
    switch (exc_error)
    {
        case ErrorId::INVALID_SELECT_FD:
            description = DSC_INVALID_SELECT_FD;
            break;
        case ErrorId::IO_ERROR:
            description = DSC_IO_ERROR;
            break;
        case ErrorId::NBLK_IO_ERROR:
            description = DSC_NBLK_IO_ERROR;
            break;
        case ErrorId::IPC_ERROR:
            description = DSC_IPC_ERROR;
            break;
        case ErrorId::DYN_LOAD_ERROR:
            description = DSC_DYN_LOAD_ERROR;
            break;
        case ErrorId::SIGNAL_HND_ERROR:
            description = DSC_SIGNAL_HND_ERROR;
            break;
        case ErrorId::UNKNOWN:
            // fall-through
        default:
            break;
    }
    return description;
}

const char* OsException::what() const noexcept
{
    return get_error_description();
}

ProtocolException::ProtocolException()
{
}

ProtocolException::~ProtocolException() noexcept
{
}
