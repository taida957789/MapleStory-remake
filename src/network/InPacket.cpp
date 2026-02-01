#include "InPacket.h"

#include <cstring>

namespace ms
{

InPacket::InPacket() = default;

InPacket::InPacket(const std::uint8_t* data, std::size_t length)
    : m_data(data, data + length)
{
    if (length >= 2)
    {
        m_nHeader = Decode2();
    }
}

InPacket::InPacket(const std::vector<std::uint8_t>& data)
    : m_data(data)
{
    if (data.size() >= 2)
    {
        m_nHeader = Decode2();
    }
}

InPacket::~InPacket() = default;

auto InPacket::Decode1() -> std::int8_t
{
    if (m_nPosition + 1 > m_data.size())
        return 0;

    return static_cast<std::int8_t>(m_data[m_nPosition++]);
}

auto InPacket::Decode2() -> std::int16_t
{
    if (m_nPosition + 2 > m_data.size())
        return 0;

    std::int16_t value = static_cast<std::int16_t>(m_data[m_nPosition]) |
                         (static_cast<std::int16_t>(m_data[m_nPosition + 1]) << 8);
    m_nPosition += 2;
    return value;
}

auto InPacket::Decode4() -> std::int32_t
{
    if (m_nPosition + 4 > m_data.size())
        return 0;

    std::int32_t value = static_cast<std::int32_t>(m_data[m_nPosition]) |
                         (static_cast<std::int32_t>(m_data[m_nPosition + 1]) << 8) |
                         (static_cast<std::int32_t>(m_data[m_nPosition + 2]) << 16) |
                         (static_cast<std::int32_t>(m_data[m_nPosition + 3]) << 24);
    m_nPosition += 4;
    return value;
}

auto InPacket::Decode8() -> std::int64_t
{
    if (m_nPosition + 8 > m_data.size())
        return 0;

    std::int64_t low = static_cast<std::uint32_t>(Decode4());
    std::int64_t high = static_cast<std::uint32_t>(Decode4());
    return low | (high << 32);
}

auto InPacket::DecodeStr() -> std::string
{
    std::int16_t length = Decode2();
    if (length <= 0 || m_nPosition + static_cast<std::size_t>(length) > m_data.size())
        return "";

    std::string result(reinterpret_cast<const char*>(&m_data[m_nPosition]),
                       static_cast<std::size_t>(length));
    m_nPosition += static_cast<std::size_t>(length);
    return result;
}

void InPacket::DecodeBuffer(std::uint8_t* buffer, std::size_t length)
{
    if (m_nPosition + length > m_data.size())
        return;

    std::memcpy(buffer, &m_data[m_nPosition], length);
    m_nPosition += length;
}

auto InPacket::GetRemaining() const noexcept -> std::size_t
{
    return m_data.size() - m_nPosition;
}

auto InPacket::IsEnd() const noexcept -> bool
{
    return m_nPosition >= m_data.size();
}

} // namespace ms
