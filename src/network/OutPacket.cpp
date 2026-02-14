#include "OutPacket.h"

namespace ms
{

OutPacket::OutPacket() = default;

OutPacket::OutPacket(std::int16_t opcode)
    : m_nOpcode(opcode)
{
    Encode2(opcode);
}

OutPacket::~OutPacket() = default;

void OutPacket::Encode1(std::int8_t value)
{
    m_data.push_back(static_cast<std::uint8_t>(value));
}

void OutPacket::Encode2(std::int16_t value)
{
    m_data.push_back(static_cast<std::uint8_t>(value & 0xFF));
    m_data.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
}

void OutPacket::Encode4(std::int32_t value)
{
    m_data.push_back(static_cast<std::uint8_t>(value & 0xFF));
    m_data.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    m_data.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    m_data.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
}

void OutPacket::Encode8(std::int64_t value)
{
    Encode4(static_cast<std::int32_t>(value & 0xFFFFFFFF));
    Encode4(static_cast<std::int32_t>((value >> 32) & 0xFFFFFFFF));
}

void OutPacket::EncodeStr(const std::string& str)
{
    Encode2(static_cast<std::int16_t>(str.length()));
    for (char c : str)
    {
        m_data.push_back(static_cast<std::uint8_t>(c));
    }
}

void OutPacket::EncodeBuffer(const std::uint8_t* buffer, std::size_t length)
{
    m_data.insert(m_data.end(), buffer, buffer + length);
}

void OutPacket::Set4At(std::size_t offset, std::int32_t value)
{
    m_data[offset]     = static_cast<std::uint8_t>(value & 0xFF);
    m_data[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
    m_data[offset + 2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
    m_data[offset + 3] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
}

void OutPacket::Reset(std::int16_t opcode)
{
    m_data.clear();
    m_nOpcode = opcode;
    Encode2(opcode);
}

} // namespace ms
