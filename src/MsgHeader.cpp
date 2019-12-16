#include "MsgHeader.h"

MsgHeader::MsgHeader()
{
}

MsgHeader::~MsgHeader() noexcept
{
}

void MsgHeader::clear() noexcept
{
    msg_type    = 0;
    data_length = 0;
}

uint16_t MsgHeader::bytes_to_field_value(const char* const buffer) noexcept
{
    uint16_t value = static_cast<uint16_t> (buffer[0]);
    value <<= 8;
    value |= static_cast<uint16_t> (buffer[1]);
    return value;
}

void MsgHeader::field_value_to_bytes(const uint16_t value, char* const buffer) noexcept
{
    buffer[0] = static_cast<char> (value >> 8);
    buffer[1] = static_cast<char> (value & 0xFF);
}
