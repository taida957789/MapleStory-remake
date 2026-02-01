#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ms
{

/**
 * @brief Incoming packet reader
 *
 * Based on CInPacket from the original MapleStory client.
 */
class InPacket
{
public:
    InPacket();
    InPacket(const std::uint8_t* data, std::size_t length);
    explicit InPacket(const std::vector<std::uint8_t>& data);
    ~InPacket();

    // Non-copyable, movable
    InPacket(const InPacket&) = delete;
    auto operator=(const InPacket&) -> InPacket& = delete;
    InPacket(InPacket&&) noexcept = default;
    auto operator=(InPacket&&) noexcept -> InPacket& = default;

    /**
     * @brief Decode operations (matching original names)
     */
    auto Decode1() -> std::int8_t;
    auto Decode2() -> std::int16_t;
    auto Decode4() -> std::int32_t;
    auto Decode8() -> std::int64_t;
    auto DecodeStr() -> std::string;
    void DecodeBuffer(std::uint8_t* buffer, std::size_t length);

    /**
     * @brief Get remaining bytes
     */
    [[nodiscard]] auto GetRemaining() const noexcept -> std::size_t;

    /**
     * @brief Check if at end of packet
     */
    [[nodiscard]] auto IsEnd() const noexcept -> bool;

    /**
     * @brief Get current position
     */
    [[nodiscard]] auto GetPosition() const noexcept -> std::size_t { return m_nPosition; }

    /**
     * @brief Set position
     */
    void SetPosition(std::size_t pos) noexcept { m_nPosition = pos; }

    /**
     * @brief Get packet header (opcode)
     */
    [[nodiscard]] auto GetHeader() const noexcept -> std::int16_t { return m_nHeader; }

    /**
     * @brief Get raw data
     */
    [[nodiscard]] auto GetData() const noexcept -> const std::vector<std::uint8_t>& { return m_data; }

private:
    std::vector<std::uint8_t> m_data;
    std::size_t m_nPosition{};
    std::int16_t m_nHeader{};
};

} // namespace ms
