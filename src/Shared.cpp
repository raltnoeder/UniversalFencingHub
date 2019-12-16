#include "Shared.h"
#include <RangeException.h>
#include "exceptions.h"

#include <iostream>

namespace sys
{
    const int FD_NONE = -1;

    const size_t PIPE_READ_END  = 0;
    const size_t PIPE_WRITE_END = 1;

    void close_fd(int& fd) noexcept
    {
        if (fd >= 0)
        {
            int rc = 0;
            do
            {
                rc = close(fd);
            }
            while (rc != 0 && errno == EINTR);
            fd = FD_NONE;
        }
    }
}

namespace keyword
{
    const char* const PROTO_IPV4 = "IPV4";
    const char* const PROTO_IPV6 = "IPV6";
}

namespace protocol
{
    const CharBuffer KEY_VALUE_SPLIT_SEQ("=");

    const char* const NODE_NAME = "NODENAME";

    // @throws ProtocolException
    bool read_field(
        const char* const   io_buffer,
        const size_t        io_buffer_data_length,
        size_t&             offset,
        CharBuffer&         field_buffer
    )
    {
        bool have_field = false;
        try
        {
            field_buffer.clear();
            if (offset < io_buffer_data_length && io_buffer_data_length - offset >= 2)
            {
                uint16_t field_length = static_cast<unsigned char> (io_buffer[offset]) << 8;
                field_length |= static_cast<unsigned char> (io_buffer[offset + 1]);

                const size_t remain_length = io_buffer_data_length - offset - 2;
                if (field_length <= remain_length)
                {
                    field_buffer.substring_raw_from(io_buffer, offset + 2, offset + 2 + field_length);
                    offset += field_length + 2;
                    have_field = true;
                }
                else
                {
                    throw ProtocolException();
                }
            }
            else
            {
                throw ProtocolException();
            }
        }
        catch (RangeException&)
        {
            throw ProtocolException();
        }
        return have_field;
    }

    // @throws ProtocolException
    void split_key_value_pair(
        CharBuffer& key,
        CharBuffer& value
    )
    {
        try
        {
            const size_t split_idx = key.index_of(KEY_VALUE_SPLIT_SEQ);
            if (split_idx != CharBuffer::NPOS)
            {
                value.substring_from(key, split_idx + KEY_VALUE_SPLIT_SEQ.length(), key.length());
                key.substring(0, split_idx);
            }
            else
            {
                throw ProtocolException();
            }
        }
        catch (RangeException&)
        {
            throw ProtocolException();
        }
    }
}
