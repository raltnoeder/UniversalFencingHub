#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>

class InetException : public std::exception
{
  public:
    enum class ErrorId : uint32_t
    {
        // Undetermined error (e.g. unknown error code returned by the OS)
        UNKNOWN             = 0,
        // Network address not valid for the selected address family
        INVALID_ADDRESS     = 1,
        // Unknown address family specified
        UNKNOWN_AF          = 2,
        // Selected address family not supported by the platform
        UNSUPPORTED_AF      = 3,
        // IP port number not valid
        INVALID_PORT_NUMBER = 4,
        // Network socket error
        SOCKET_ERROR        = 5,
        // Binding a socket failed
        BIND_FAILED         = 6,
        // Setting up server socket with listen(...) failed
        LISTEN_ERROR        = 7
    };

    static const char* const DSC_UNKNOWN;
    static const char* const DSC_INVALID_ADDRESS;
    static const char* const DSC_UNKNOWN_AF;
    static const char* const DSC_UNSUPPORTED_AF;
    static const char* const DSC_INVALID_PORT_NUMBER;
    static const char* const DSC_SOCKET_ERROR;
    static const char* const DSC_BIND_FAILED;
    static const char* const DSC_LISTEN_ERROR;

  private:
    ErrorId exc_error = ErrorId::UNKNOWN;

  public:
    InetException() noexcept;
    InetException(const ErrorId error) noexcept;
    virtual ~InetException() noexcept override;
    InetException(const InetException& other) noexcept = default;
    InetException(InetException&& orig) noexcept = default;
    virtual InetException& operator=(const InetException& other) noexcept = default;
    virtual InetException& operator=(InetException&& orig) noexcept = default;
    virtual const ErrorId get_error_id() const noexcept;
    virtual const char* get_error_description() const noexcept;
    virtual const char* what() const noexcept override;
};

class OsException : public std::exception
{
  public:
    enum class ErrorId : uint32_t
    {
        UNKNOWN             = 0,
        // Invalid file descriptor for select(...)
        INVALID_SELECT_FD   = 1,
        // I/O error
        IO_ERROR            = 2,
        // Error setting up nonblocking I/O
        NBLK_IO_ERROR       = 3,
        // Setting up the selector failed
        IPC_ERROR           = 4,
        // Dynamic load of a library failed
        DYN_LOAD_ERROR      = 5
    };

    static const char* const DSC_UNKNOWN;
    static const char* const DSC_INVALID_SELECT_FD;
    static const char* const DSC_IO_ERROR;
    static const char* const DSC_NBLK_IO_ERROR;
    static const char* const DSC_IPC_ERROR;
    static const char* const DSC_DYN_LOAD_ERROR;

  private:
    ErrorId exc_error = ErrorId::UNKNOWN;

  public:
    OsException() noexcept;
    OsException(const ErrorId error) noexcept;
    virtual ~OsException() noexcept override;
    OsException(const OsException& other) noexcept = default;
    OsException(OsException&& orig) noexcept = default;
    virtual OsException& operator=(const OsException& other) = default;
    virtual OsException& operator=(OsException&& other) = default;
    virtual const ErrorId get_error_id() const noexcept;
    virtual const char* get_error_description() const noexcept;
    virtual const char* what() const noexcept override;
};

class ProtocolException : public std::exception
{
  public:
    ProtocolException();
    virtual ~ProtocolException() noexcept;
    ProtocolException(const ProtocolException& other) = default;
    ProtocolException(ProtocolException&& orig) = default;
    virtual ProtocolException& operator=(const ProtocolException& other) = default;
    virtual ProtocolException& operator=(ProtocolException&& orig) = default;
};

#endif /* EXCEPTIONS_H */
