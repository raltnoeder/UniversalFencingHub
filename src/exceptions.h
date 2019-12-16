#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>

class InetException : std::exception
{
  public:
    enum class ErrorId : uint32_t
    {
        // Undetermined error (e.g. unknown error code returned by the OS)
        UNKNOWN             = 0,
        // Network address not valid for the selected address family
        INVALID_ADDRESS     = 1,
        // Unknown address family specified
        UNKNOWN_AF          = 3,
        // Selected address family not supported by the platform
        UNSUPPORTED_AF      = 4,
        // IP port number not valid
        INVALID_PORT_NUMBER = 5,
        // Network socket error
        SOCKET_ERROR        = 6,
        // Binding a socket failed
        BIND_FAILED         = 7,
        // Error initializing non-blocking I/O
        NIO_ERROR           = 8,
        // Setting up server socket with listen(...) failed
        LISTEN_ERROR        = 9
    };

  private:
    ErrorId exc_error = ErrorId::UNKNOWN;

  public:
    InetException() noexcept;
    InetException(const ErrorId error) noexcept;
    virtual ~InetException() noexcept;
    InetException(const InetException& other) noexcept = default;
    InetException(InetException&& orig) noexcept = default;
    virtual InetException& operator=(const InetException& other) noexcept = default;
    virtual InetException& operator=(InetException&& orig) noexcept = default;
    virtual const ErrorId get_error_id() const noexcept;
};

class OsException : std::exception
{
  public:
    enum class ErrorId : uint32_t
    {
        UNKNOWN             = 0,
        // Invalid file descriptor for select(...)
        INVALID_SELECT_FD   = 1,
        // I/O error
        IO_ERROR            = 2
    };

  private:
    ErrorId exc_error = ErrorId::UNKNOWN;

  public:
    OsException() noexcept;
    OsException(const ErrorId error) noexcept;
    virtual ~OsException() noexcept;
    OsException(const OsException& other) noexcept = default;
    OsException(OsException&& orig) noexcept = default;
    virtual OsException& operator=(const OsException& other) = default;
    virtual OsException& operator=(OsException&& other) = default;
    virtual const ErrorId get_error_id() const noexcept;
};

#endif /* EXCEPTIONS_H */
