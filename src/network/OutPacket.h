#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ms
{

/**
 * @brief Outgoing packet builder
 *
 * Based on COutPacket from the original MapleStory client.
 */
class OutPacket
{
public:
    OutPacket();
    explicit OutPacket(std::int16_t opcode);
    ~OutPacket();

    // Non-copyable, movable
    OutPacket(const OutPacket&) = delete;
    auto operator=(const OutPacket&) -> OutPacket& = delete;
    OutPacket(OutPacket&&) noexcept = default;
    auto operator=(OutPacket&&) noexcept -> OutPacket& = default;

    /**
     * @brief Encode operations (matching original names)
     */
    void Encode1(std::int8_t value);
    void Encode2(std::int16_t value);
    void Encode4(std::int32_t value);
    void Encode8(std::int64_t value);
    void EncodeStr(const std::string& str);
    void EncodeBuffer(const std::uint8_t* buffer, std::size_t length);

    /**
     * @brief Get packet data
     */
    [[nodiscard]] auto GetData() const noexcept -> const std::vector<std::uint8_t>& { return m_data; }

    /**
     * @brief Get packet size
     */
    [[nodiscard]] auto GetSize() const noexcept -> std::size_t { return m_data.size(); }

    /**
     * @brief Get opcode
     */
    [[nodiscard]] auto GetOpcode() const noexcept -> std::int16_t { return m_nOpcode; }

    /**
     * @brief Reset packet
     */
    void Reset(std::int16_t opcode);

private:
    std::vector<std::uint8_t> m_data;
    std::int16_t m_nOpcode{};
};

} // namespace ms
