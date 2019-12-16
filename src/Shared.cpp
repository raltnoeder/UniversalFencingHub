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

namespace ufh
{
    const char WAKEUP_TRIGGER_BYTE = static_cast<char> (0xFF);

    const char* const LOGPFX_START      = "[ START ] ";
    const char* const LOGPFX_STOP       = "[ STOP  ] ";
    const char* const LOGPFX_WARNING    = "[ WARN  ] ";
    const char* const LOGPFX_ERROR      = "[ ERROR ] ";
    const char* const LOGPFX_FENCE      = "[ FENCE ] ";
    const char* const LOGPFX_MONITOR    = "[ MONTR ] ";
    const char* const LOGPFX_CONT       = "[  ...  ] ";
}

namespace inet
{
    extern const char* const IPV6_LOCAL_BIND_ADDRESS = "::";
    extern const char* const IPV4_LOCAL_BIND_ADDRESS = "0.0.0.0";
}

namespace keyword
{
    const char* const PROTO_IPV4 = "IPV4";
    const char* const PROTO_IPV6 = "IPV6";
}

namespace constraints
{
    const size_t PROTOCOL_PARAM_SIZE    = 10;
    const size_t IP_ADDR_PARAM_SIZE     = 60;
    const size_t PORT_PARAM_SIZE        = 6;
}

namespace protocol
{
    const CharBuffer KEY_VALUE_SPLIT_SEQ("=");

    const char* const NODENAME  = "NODENAME";
    const char* const SECRET    = "SECRET";

    const size_t MAX_SECRET_LENGTH = 64;

    // @throws ProtocolException
    static void write_field_impl(
        char* const io_buffer,
        const size_t io_buffer_capacity,
        size_t& offset,
        const char* const field_data,
        const size_t field_length
    );

    // @throws ProtocolException
    bool read_field(
        const char* const   io_buffer,
        const size_t        io_buffer_capacity,
        size_t&             offset,
        CharBuffer&         field_buffer
    )
    {
        bool have_field = false;
        try
        {
            field_buffer.clear();
            if (offset < io_buffer_capacity && io_buffer_capacity - offset >= 2)
            {
                uint16_t field_length = static_cast<unsigned char> (io_buffer[offset]) << 8;
                field_length |= static_cast<unsigned char> (io_buffer[offset + 1]);

                const size_t remain_length = io_buffer_capacity - offset - 2;
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

    // @throws std::bad_alloc, ProtocolException
    bool read_field(
        const char* const io_buffer,
        const size_t io_buffer_capacity,
        size_t& offset,
        std::string& field_contents
    )
    {
        bool have_field = false;
        if (offset < io_buffer_capacity && io_buffer_capacity - offset >= 2)
        {
            uint16_t field_length = static_cast<unsigned char> (io_buffer[offset]) << 8;
            field_length |= static_cast<unsigned char> (io_buffer[offset + 1]);

            const size_t remain_length = io_buffer_capacity - offset - 2;
            if (field_length <= remain_length)
            {
                field_contents.append(&(io_buffer[offset + 2]), field_length);
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
        return have_field;
    }

    // @throws ProtocolException
    void write_field(
        char* const io_buffer,
        const size_t io_buffer_capacity,
        size_t& offset,
        const CharBuffer& field_buffer
    )
    {
        write_field_impl(io_buffer, io_buffer_capacity, offset, field_buffer.c_str(), field_buffer.length());
    }

    // @throws ProtocolException
    void write_field(
        char* const io_buffer,
        const size_t io_buffer_capacity,
        size_t& offset,
        const std::string& field_contents
    )
    {
        write_field_impl(io_buffer, io_buffer_capacity, offset, field_contents.c_str(), field_contents.length());
    }

    // @throws ProtocolException
    static void write_field_impl(
        char* const io_buffer,
        const size_t io_buffer_capacity,
        size_t& offset,
        const char* const field_data,
        const size_t field_length
    )
    {
        if (field_length > 0xFFFF)
        {
            throw ProtocolException();
        }

        if (offset < io_buffer_capacity)
        {
            const size_t remain_capacity = io_buffer_capacity - offset;
            if (field_length < remain_capacity && remain_capacity - field_length >= 2)
            {
                io_buffer[offset] = static_cast<char> (field_length >> 8);
                io_buffer[offset + 1] = static_cast<char> (field_length & 0xFF);
                offset += 2;

                for (size_t idx = 0; idx < field_length; ++idx)
                {
                    io_buffer[offset] = field_data[idx];
                    ++offset;
                }
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

    // @throws ProtocolException
    void split_key_value_pair(
        CharBuffer& src_data,
        CharBuffer& value
    )
    {
        try
        {
            const size_t split_idx = src_data.index_of(KEY_VALUE_SPLIT_SEQ);
            if (split_idx != CharBuffer::NPOS)
            {
                value.substring_from(src_data, split_idx + KEY_VALUE_SPLIT_SEQ.length(), src_data.length());
                src_data.substring(0, split_idx);
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

    // @throws std::bad_alloc, ProtocolException
    void split_key_value_pair(
        std::string& src_data,
        std::string& value
    )
    {
        const size_t split_idx = src_data.find(KEY_VALUE_SPLIT_SEQ.c_str());
        if (split_idx != std::string::npos)
        {
            value.append(src_data, split_idx + KEY_VALUE_SPLIT_SEQ.length(), std::string::npos);
            src_data.resize(split_idx);
        }
        else
        {
            throw ProtocolException();
        }
    }
}
