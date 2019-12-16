#include "MsgHeader.h"

constexpr const size_t MsgHeader::HEADER_SIZE           = 4;
constexpr const size_t MsgHeader::MSG_TYPE_OFFSET       = 0;
constexpr const size_t MsgHeader::DATA_LENGTH_OFFSET    = 2;

MsgHeader::MsgHeader()
{
}

MsgHeader::~MsgHeader() noexcept
{
}

void MsgHeader::clear() noexcept
{
    msg_type    = 0xFFFF;
    data_length = 0;
}

bool MsgHeader::is_msg_type(const protocol::MsgType value)
{
    return msg_type == static_cast<uint16_t> (value);
}

void MsgHeader::set_msg_type(const protocol::MsgType value)
{
    msg_type = static_cast<uint16_t> (value);
}

void MsgHeader::serialize(char* const io_buffer) const
{
    field_value_to_bytes(msg_type, io_buffer, MSG_TYPE_OFFSET);
    field_value_to_bytes(data_length, io_buffer, DATA_LENGTH_OFFSET);
}

void MsgHeader::deserialize(const char* const io_buffer)
{
    msg_type = bytes_to_field_value(io_buffer, MSG_TYPE_OFFSET);
    data_length = bytes_to_field_value(io_buffer, DATA_LENGTH_OFFSET);
}

uint16_t MsgHeader::bytes_to_field_value(const char* const buffer, const size_t offset) noexcept
{
    uint16_t value = static_cast<unsigned char> (buffer[offset]);
    value <<= 8;
    value |= static_cast<unsigned char> (buffer[offset + 1]);
    return value;
}

void MsgHeader::field_value_to_bytes(const uint16_t value, char* const buffer, const size_t offset) noexcept
{
    buffer[offset] = static_cast<char> (value >> 8);
    buffer[offset + 1] = static_cast<char> (value & 0xFF);
}
