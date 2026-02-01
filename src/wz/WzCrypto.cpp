#include "WzCrypto.h"
#include "WzTypes.h"

#include <zlib.h>

#include <algorithm>
#include <cstring>

namespace ms
{

// Static member definitions
std::uint8_t WzCrypto::s_iv[4] = {0};
std::uint8_t WzCrypto::s_expandedKey[240] = {0};
std::vector<std::uint8_t> WzCrypto::s_keys;
bool WzCrypto::s_initialized = false;

// AES S-Box
const std::uint8_t WzCrypto::s_sbox[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
};

// AES round constant
const std::uint8_t WzCrypto::s_rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36};

void WzCrypto::Initialize(const std::uint8_t* iv)
{
    std::memcpy(s_iv, iv, 4);
    s_keys.clear();
    s_initialized = true;

    // Expand AES key
    KeyExpansion(WzKeys::AesKey, s_expandedKey);
}

void WzCrypto::KeyExpansion(const std::uint8_t* key, std::uint8_t* expandedKey)
{
    // AES-256 key expansion
    constexpr int Nk = 8;  // Key length in 32-bit words
    constexpr int Nr = 14; // Number of rounds
    constexpr int Nb = 4;  // Block size in 32-bit words

    std::uint8_t temp[4];

    // Copy original key
    for (int i = 0; i < Nk * 4; ++i)
    {
        expandedKey[i] = key[i];
    }

    // Expand key
    for (int i = Nk; i < Nb * (Nr + 1); ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            temp[j] = expandedKey[(i - 1) * 4 + j];
        }

        if (i % Nk == 0)
        {
            // RotWord
            std::uint8_t t = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = t;

            // SubWord
            for (int j = 0; j < 4; ++j)
            {
                temp[j] = s_sbox[temp[j]];
            }

            // XOR with Rcon
            temp[0] ^= s_rcon[(i / Nk) - 1];
        }
        else if (Nk > 6 && i % Nk == 4)
        {
            // SubWord for AES-256
            for (int j = 0; j < 4; ++j)
            {
                temp[j] = s_sbox[temp[j]];
            }
        }

        for (int j = 0; j < 4; ++j)
        {
            expandedKey[i * 4 + j] = expandedKey[(i - Nk) * 4 + j] ^ temp[j];
        }
    }
}

void WzCrypto::AesEncryptBlock(const std::uint8_t* in, std::uint8_t* out, const std::uint8_t* expandedKey)
{
    // Simplified AES encryption (single block)
    std::uint8_t state[16];
    std::memcpy(state, in, 16);

    constexpr int Nr = 14;

    // Initial round key addition
    for (int i = 0; i < 16; ++i)
    {
        state[i] ^= expandedKey[i];
    }

    // Main rounds
    for (int round = 1; round < Nr; ++round)
    {
        // SubBytes
        for (int i = 0; i < 16; ++i)
        {
            state[i] = s_sbox[state[i]];
        }

        // ShiftRows
        std::uint8_t temp = state[1];
        state[1] = state[5];
        state[5] = state[9];
        state[9] = state[13];
        state[13] = temp;

        temp = state[2];
        state[2] = state[10];
        state[10] = temp;
        temp = state[6];
        state[6] = state[14];
        state[14] = temp;

        temp = state[15];
        state[15] = state[11];
        state[11] = state[7];
        state[7] = state[3];
        state[3] = temp;

        // MixColumns (skipped in final round)
        for (int col = 0; col < 4; ++col)
        {
            int idx = col * 4;
            std::uint8_t a[4];
            std::uint8_t b[4];

            for (int i = 0; i < 4; ++i)
            {
                a[i] = state[idx + i];
                b[i] = static_cast<std::uint8_t>((state[idx + i] << 1) ^
                                                  (((state[idx + i] >> 7) & 1) * 0x1B));
            }

            state[idx] = static_cast<std::uint8_t>(b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3]);
            state[idx + 1] = static_cast<std::uint8_t>(a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3]);
            state[idx + 2] = static_cast<std::uint8_t>(a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3]);
            state[idx + 3] = static_cast<std::uint8_t>(a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3]);
        }

        // AddRoundKey
        for (int i = 0; i < 16; ++i)
        {
            state[i] ^= expandedKey[round * 16 + i];
        }
    }

    // Final round (no MixColumns)
    for (int i = 0; i < 16; ++i)
    {
        state[i] = s_sbox[state[i]];
    }

    // ShiftRows
    std::uint8_t temp = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = temp;

    temp = state[2];
    state[2] = state[10];
    state[10] = temp;
    temp = state[6];
    state[6] = state[14];
    state[14] = temp;

    temp = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7] = state[3];
    state[3] = temp;

    // AddRoundKey
    for (int i = 0; i < 16; ++i)
    {
        state[i] ^= expandedKey[Nr * 16 + i];
    }

    std::memcpy(out, state, 16);
}

void WzCrypto::GenerateKeyBatch(std::vector<std::uint8_t>& keys, std::size_t startIndex)
{
    // Generate one batch of keys using AES OFB mode
    std::uint8_t block[16];

    // Initialize with IV (expanded to 16 bytes)
    for (int i = 0; i < 16; i += 4)
    {
        block[i] = s_iv[0];
        block[i + 1] = s_iv[1];
        block[i + 2] = s_iv[2];
        block[i + 3] = s_iv[3];
    }

    // Fast-forward to the correct position
    std::size_t skipBlocks = startIndex / 16;
    for (std::size_t i = 0; i < skipBlocks; ++i)
    {
        std::uint8_t output[16];
        AesEncryptBlock(block, output, s_expandedKey);
        std::memcpy(block, output, 16);
    }

    // Generate keys
    std::size_t keysNeeded = kBatchSize;
    std::size_t generated = 0;
    std::size_t offset = startIndex % 16;

    while (generated < keysNeeded)
    {
        std::uint8_t output[16];
        AesEncryptBlock(block, output, s_expandedKey);
        std::memcpy(block, output, 16);

        std::size_t toCopy = std::min<std::size_t>(16 - offset, keysNeeded - generated);
        for (std::size_t i = 0; i < toCopy; ++i)
        {
            keys.push_back(output[offset + i]);
        }
        generated += toCopy;
        offset = 0;
    }
}

void WzCrypto::GenerateKey(std::vector<std::uint8_t>& outKey, std::size_t size)
{
    if (!s_initialized)
    {
        Initialize(WzKeys::KMS_IV);
    }

    while (s_keys.size() < size)
    {
        GenerateKeyBatch(s_keys, s_keys.size());
    }

    outKey.assign(s_keys.begin(), s_keys.begin() + static_cast<std::ptrdiff_t>(size));
}

auto WzCrypto::GetKeyByte(std::size_t index) -> std::uint8_t
{
    if (!s_initialized)
    {
        Initialize(WzKeys::KMS_IV);
    }

    while (s_keys.size() <= index)
    {
        GenerateKeyBatch(s_keys, s_keys.size());
    }

    return s_keys[index];
}

void WzCrypto::XorDecrypt(std::uint8_t* data, std::size_t size, std::size_t keyOffset)
{
    if (!s_initialized)
    {
        Initialize(WzKeys::KMS_IV);
    }

    while (s_keys.size() < keyOffset + size)
    {
        GenerateKeyBatch(s_keys, s_keys.size());
    }

    for (std::size_t i = 0; i < size; ++i)
    {
        data[i] ^= s_keys[keyOffset + i];
    }
}

auto WzCrypto::Decompress(const std::vector<std::uint8_t>& compressedData,
                          std::size_t expectedSize) -> std::vector<std::uint8_t>
{
    return Decompress(compressedData.data(), compressedData.size(), expectedSize);
}

auto WzCrypto::Decompress(const std::uint8_t* data, std::size_t size,
                          std::size_t expectedSize) -> std::vector<std::uint8_t>
{
    if (size == 0 || data == nullptr)
    {
        return {};
    }

    // Use zlib inflate with Z_NO_FLUSH
    // Note: WZ file zlib streams don't have proper end markers, so uncompress() fails.
    // Using inflate with Z_NO_FLUSH handles this correctly.
    z_stream strm{};
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = static_cast<uInt>(size);
    strm.next_in = const_cast<Bytef*>(data);

    if (inflateInit(&strm) != Z_OK)
    {
        return {};
    }

    std::vector<std::uint8_t> result(expectedSize);
    strm.avail_out = static_cast<uInt>(expectedSize);
    strm.next_out = result.data();

    int ret = inflate(&strm, Z_NO_FLUSH);

    // Keep inflating until done
    while (ret == Z_OK && strm.avail_in > 0 && strm.avail_out > 0)
    {
        ret = inflate(&strm, Z_NO_FLUSH);
    }

    auto totalOut = strm.total_out;
    inflateEnd(&strm);

    if ((ret == Z_STREAM_END || ret == Z_OK) && totalOut > 0)
    {
        result.resize(totalOut);
        return result;
    }

    // If data size equals expected size, it might be uncompressed
    if (size == expectedSize)
    {
        return std::vector<std::uint8_t>(data, data + size);
    }

    return {};
}

} // namespace ms
