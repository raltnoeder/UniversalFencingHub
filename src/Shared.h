#ifndef SHARED_H
#define SHARED_H

#include <CharBuffer.h>

namespace keyword
{
    extern const char* const PROTO_IPV4;
    extern const char* const PROTO_IPV6;
}

enum class msgtype : uint16_t
{
    ECHO_REQUEST    = 0x0,
    ECHO_REPLY      = 0x1,
    VERSION_REQUEST = 0x2,
    POWER_OFF       = 0x81,
    POWER_ON        = 0x82,
    REBOOT          = 0x83,
    FENCE_SUCCESS   = 0xA0,
    FENCE_FAIL      = 0xA1
};

namespace protocol
{
    extern const CharBuffer KEY_VALUE_SPLIT_SEQ;

    extern const char* const NODE_NAME;

    // @throws ProtocolException
    bool read_field(
        const char* const io_buffer,
        const size_t io_buffer_length,
        size_t& offset,
        CharBuffer& field_buffer
    );

    // @throws ProtocolException
    void split_key_value_pair(
        CharBuffer& src_data,
        CharBuffer& value
    );
}

#endif /* SHARED_H */
