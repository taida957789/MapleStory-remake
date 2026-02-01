#pragma once

#include <cstdint>
#include <vector>

namespace ms
{

/**
 * @brief WZ Cryptography utilities
 *
 * Based on MapleStory's WZ file encryption.
 * Uses AES-256 for key generation and XOR for string decryption.
 */
class WzCrypto
{
public:
    /**
     * @brief Initialize crypto with IV
     */
    static void Initialize(const std::uint8_t* iv);

    /**
     * @brief Generate key bytes for decryption
     */
    static void GenerateKey(std::vector<std::uint8_t>& outKey, std::size_t size);

    /**
     * @brief Get a single key byte
     */
    [[nodiscard]] static auto GetKeyByte(std::size_t index) -> std::uint8_t;

    /**
     * @brief XOR decrypt data with key
     */
    static void XorDecrypt(std::uint8_t* data, std::size_t size, std::size_t keyOffset = 0);

    /**
     * @brief Decompress zlib data
     *
     * @param compressedData Compressed input data
     * @param expectedSize Expected uncompressed size
     * @return Decompressed data, or empty vector on failure
     */
    [[nodiscard]] static auto Decompress(const std::vector<std::uint8_t>& compressedData,
                                         std::size_t expectedSize) -> std::vector<std::uint8_t>;

    /**
     * @brief Decompress zlib data from raw pointer
     */
    [[nodiscard]] static auto Decompress(const std::uint8_t* data, std::size_t size,
                                         std::size_t expectedSize) -> std::vector<std::uint8_t>;

private:
    // AES S-Box (substitution box)
    static const std::uint8_t s_sbox[256];

    // AES round constant
    static const std::uint8_t s_rcon[10];

    // AES key schedule
    static void KeyExpansion(const std::uint8_t* key, std::uint8_t* expandedKey);

    // AES encrypt single block
    static void AesEncryptBlock(const std::uint8_t* in, std::uint8_t* out, const std::uint8_t* expandedKey);

    // Generate key batch using AES
    static void GenerateKeyBatch(std::vector<std::uint8_t>& keys, std::size_t startIndex);

    // Current IV
    static std::uint8_t s_iv[4];

    // AES expanded key (240 bytes for AES-256)
    static std::uint8_t s_expandedKey[240];

    // Cached keys
    static std::vector<std::uint8_t> s_keys;

    // Batch size for key generation
    static constexpr std::size_t kBatchSize = 0x10000;

    // Initialization flag
    static bool s_initialized;
};

} // namespace ms
