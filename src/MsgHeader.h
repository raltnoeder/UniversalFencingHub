#ifndef MSGHEADER_H
#define MSGHEADER_H

#include <cstddef>
#include <cstdint>

#include "Shared.h"

class MsgHeader
{
  public:
    static const size_t HEADER_SIZE;
    static const size_t MSG_TYPE_OFFSET;
    static const size_t DATA_LENGTH_OFFSET;

    uint16_t        msg_type    = 0xFFFF;
    uint16_t        data_length = 0;

    MsgHeader();
    virtual ~MsgHeader() noexcept;
    MsgHeader(const MsgHeader& orig) = default;
    MsgHeader(MsgHeader&& orig) = default;
    virtual MsgHeader& operator=(const MsgHeader& orig) = default;
    virtual MsgHeader& operator=(MsgHeader&& orig) = default;
    virtual void clear() noexcept;

    virtual bool is_msg_type(const MsgType value);
    virtual void set_msg_type(const MsgType value);
    virtual void serialize(char* io_buffer) const;
    virtual void deserialize(const char* io_buffer);

    static uint16_t bytes_to_field_value(const char* buffer, size_t offset) noexcept;
    static void field_value_to_bytes(uint16_t value, char* buffer, size_t offset) noexcept;
};

#endif /* MSGHEADER_H */
