#pragma once

#include "WzTypes.h"

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

/**
 * @brief WZ File Reader
 *
 * Low-level reader for WZ file format.
 * Handles memory-mapped file reading and WZ-specific data decoding.
 */
class WzReader
{
public:
    WzReader();
    ~WzReader();

    // Non-copyable, movable
    WzReader(const WzReader&) = delete;
    auto operator=(const WzReader&) -> WzReader& = delete;
    WzReader(WzReader&&) noexcept;
    auto operator=(WzReader&&) noexcept -> WzReader&;

    /**
     * @brief Open a WZ file
     */
    [[nodiscard]] auto Open(const std::string& path) -> bool;

    /**
     * @brief Close the file
     */
    void Close();

    /**
     * @brief Check if file is open
     */
    [[nodiscard]] auto IsOpen() const noexcept -> bool;

    /**
     * @brief Get file size
     */
    [[nodiscard]] auto GetSize() const noexcept -> std::size_t { return m_nSize; }

    /**
     * @brief Get current position
     */
    [[nodiscard]] auto GetPosition() const noexcept -> std::size_t { return m_nCursor; }

    /**
     * @brief Set current position
     */
    void SetPosition(std::size_t pos) noexcept { m_nCursor = pos; }

    /**
     * @brief Skip bytes
     */
    void Skip(std::size_t count) noexcept { m_nCursor += count; }

    /**
     * @brief Read raw bytes
     */
    [[nodiscard]] auto ReadBytes(std::size_t count) -> std::vector<std::uint8_t>;

    /**
     * @brief Read single byte
     */
    [[nodiscard]] auto ReadByte() -> std::uint8_t;

    /**
     * @brief Read primitive types (little-endian)
     */
    template <typename T>
    [[nodiscard]] auto Read() -> T
    {
        T result{};
        if (m_nCursor + sizeof(T) <= m_nSize)
        {
            result = *reinterpret_cast<const T*>(&m_pData[m_nCursor]);
            m_nCursor += sizeof(T);
        }
        return result;
    }

    /**
     * @brief Read compressed int (WZ format)
     */
    [[nodiscard]] auto ReadCompressedInt() -> std::int32_t;

    /**
     * @brief Read null-terminated string
     */
    [[nodiscard]] auto ReadString() -> std::u16string;

    /**
     * @brief Read string with length prefix
     */
    [[nodiscard]] auto ReadString(std::size_t length) -> std::u16string;

    /**
     * @brief Read WZ encrypted string
     */
    [[nodiscard]] auto ReadWzString() -> std::u16string;

    /**
     * @brief Read WZ string from offset
     */
    [[nodiscard]] auto ReadWzStringFromOffset(std::size_t offset) -> std::u16string;

    /**
     * @brief Read string block (type-prefixed)
     */
    [[nodiscard]] auto ReadStringBlock(std::size_t baseOffset) -> std::u16string;

    /**
     * @brief Check if current position is a valid WZ image
     */
    [[nodiscard]] auto IsWzImage() -> bool;

    /**
     * @brief Set encryption key
     */
    void SetKey(const std::uint8_t* iv);

    /**
     * @brief Get key byte for decryption
     */
    [[nodiscard]] auto GetKeyByte(std::size_t index) -> std::uint8_t;

private:
    void EnsureKeySize(std::size_t size);
    void InitializeKey();

    const std::uint8_t* m_pData{nullptr};
    std::size_t m_nSize{};
    std::size_t m_nCursor{};

    // Memory-mapped file data
    std::vector<std::uint8_t> m_fileData;

    // Encryption
    std::uint8_t m_iv[4]{};
    std::vector<std::uint8_t> m_aesKey;
    std::vector<std::uint8_t> m_keys;
    static constexpr std::size_t kBatchSize = 0x10000;
};

} // namespace ms
