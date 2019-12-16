#ifndef MSGHEADER_H
#define MSGHEADER_H

#include <cstdint>

class MsgHeader
{
  public:
    uint16_t    msg_type        = 0;
    uint16_t    data_length     = 0;

    MsgHeader();
    virtual ~MsgHeader() noexcept;
    MsgHeader(const MsgHeader& orig) = default;
    MsgHeader(MsgHeader&& orig) = default;
    virtual MsgHeader& operator=(const MsgHeader& orig) = default;
    virtual MsgHeader& operator=(MsgHeader&& orig) = default;
    virtual void clear() noexcept;

    static uint16_t bytes_to_field_value(const char* buffer) noexcept;
    static void field_value_to_bytes(uint16_t value, char* buffer) noexcept;
};

#endif /* MSGHEADER_H */
