#ifndef SHARED_H
#define SHARED_H

#include <CharBuffer.h>
#include <string>

extern "C"
{
    #include <unistd.h>
}

namespace sys
{
    extern const int FD_NONE;

    extern const size_t PIPE_READ_END;
    extern const size_t PIPE_WRITE_END;

    void close_fd(int& fd) noexcept;
}

namespace inet
{
    extern const char* const IPV6_LOCAL_BIND_ADDRESS;
    extern const char* const IPV4_LOCAL_BIND_ADDRESS;
}

namespace keyword
{
    extern const char* const PROTO_IPV4;
    extern const char* const PROTO_IPV6;
}

namespace constraints
{
    extern const size_t PROTOCOL_PARAM_SIZE;
    extern const size_t IP_ADDR_PARAM_SIZE;
    extern const size_t PORT_PARAM_SIZE;
}

enum class MsgType : uint16_t
{
    ECHO_REQUEST    = 0x0,
    ECHO_REPLY      = 0x1,
    VERSION_REQUEST = 0x2,
    FENCE_OFF       = 0x81,
    FENCE_ON        = 0x82,
    FENCE_REBOOT    = 0x83,
    FENCE_SUCCESS   = 0xA0,
    FENCE_FAIL      = 0xA1
};

namespace protocol
{
    extern const CharBuffer KEY_VALUE_SPLIT_SEQ;

    extern const char* const NODENAME;
    extern const char* const SECRET;

    extern const size_t MAX_SECRET_LENGTH;

    // @throws ProtocolException
    bool read_field(
        const char* io_buffer,
        size_t io_buffer_capacity,
        size_t& offset,
        CharBuffer& field_buffer
    );

    // @throws ProtocolException
    bool read_field(
        const char* io_buffer,
        size_t io_buffer_capacity,
        size_t& offset,
        std::string& field_contents
    );

    // @throws ProtocolException
    void write_field(
        char* io_buffer,
        size_t io_buffer_capacity,
        size_t& offset,
        const CharBuffer& field_buffer
    );

    // @throws ProtocolException
    void write_field(
        char* io_buffer,
        size_t io_buffer_capacity,
        size_t& offset,
        const std::string& field_contents
    );

    // @throws ProtocolException
    void split_key_value_pair(
        CharBuffer& src_data,
        CharBuffer& value
    );

    // @throws std::bad_alloc, ProtocolException
    void split_key_value_pair(
        std::string& src_data,
        std::string& value
    );
}

#endif /* SHARED_H */
