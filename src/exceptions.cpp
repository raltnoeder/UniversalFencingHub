#include "exceptions.h"

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
