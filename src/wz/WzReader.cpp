#include "WzReader.h"
#include "WzCrypto.h"

#include <cstring>
#include <iostream>
#include <limits>
#include <system_error>

namespace ms
{

WzReader::WzReader() = default;

WzReader::~WzReader()
{
    Close();
}

WzReader::WzReader(WzReader&&) noexcept = default;
auto WzReader::operator=(WzReader&&) noexcept -> WzReader& = default;

auto WzReader::Open(const std::string& path) -> bool
{
    Close();

    std::error_code error;
    m_mmap.map(path, error);

    if (error)
    {
        return false;
    }

    m_pData = reinterpret_cast<const std::uint8_t*>(m_mmap.data());
    m_nSize = m_mmap.size();
    m_nCursor = 0;

    return true;
}

void WzReader::Close()
{
    if (m_mmap.is_mapped())
    {
        m_mmap.unmap();
    }
    m_pData = nullptr;
    m_nSize = 0;
    m_nCursor = 0;
}

auto WzReader::IsOpen() const noexcept -> bool
{
    return m_mmap.is_mapped();
}

auto WzReader::ReadBytes(std::size_t count) -> std::vector<std::uint8_t>
{
    std::vector<std::uint8_t> result;
    if (m_nCursor + count <= m_nSize)
    {
        result.assign(&m_pData[m_nCursor], &m_pData[m_nCursor + count]);
        m_nCursor += count;
    }
    return result;
}

auto WzReader::ReadByte() -> std::uint8_t
{
    if (m_nCursor < m_nSize)
        return m_pData[m_nCursor++];
    return 0;
}

auto WzReader::ReadCompressedInt() -> std::int32_t
{
    auto result = static_cast<std::int32_t>(Read<std::int8_t>());
    if (result == std::numeric_limits<std::int8_t>::min())
        return Read<std::int32_t>();
    return result;
}

auto WzReader::ReadString() -> std::u16string
{
    std::u16string result;
    while (m_nCursor < m_nSize)
    {
        auto c = ReadByte();
        if (c == 0)
            break;
        result.push_back(static_cast<char16_t>(c));
    }
    return result;
}

auto WzReader::ReadString(std::size_t length) -> std::u16string
{
    std::u16string result;
    for (std::size_t i = 0; i < length && m_nCursor < m_nSize; ++i)
    {
        result.push_back(static_cast<char16_t>(ReadByte()));
    }
    return result;
}

auto WzReader::ReadWzString() -> std::u16string
{
    auto len8 = Read<std::int8_t>();

    if (len8 == 0)
        return {};

    std::int32_t len;

    // Check if IV is zero (no key encryption)
    bool useKeyXor = (m_iv[0] != 0 || m_iv[1] != 0 || m_iv[2] != 0 || m_iv[3] != 0);

    if (len8 > 0)
    {
        // Unicode string
        std::uint16_t mask = 0xAAAA;
        len = (len8 == 127) ? Read<std::int32_t>() : len8;

        if (len <= 0)
            return {};

        std::u16string result;
        for (std::int32_t i = 0; i < len; ++i)
        {
            auto encryptedChar = Read<std::uint16_t>();
            encryptedChar ^= mask;
            if (useKeyXor)
            {
                EnsureKeySize(static_cast<std::size_t>((i + 1) * 2));
                encryptedChar ^= *reinterpret_cast<std::uint16_t*>(&m_keys[2 * i]);
            }
            result.push_back(static_cast<char16_t>(encryptedChar));
            mask++;
        }
        return result;
    }

    // ASCII string
    std::uint8_t mask = 0xAA;

    if (len8 == -128)
        len = Read<std::int32_t>();
    else
        len = -len8;

    if (len <= 0)
        return {};

    std::u16string result;
    for (std::int32_t i = 0; i < len; ++i)
    {
        auto encryptedChar = ReadByte();
        encryptedChar ^= mask;
        if (useKeyXor)
        {
            encryptedChar ^= GetKeyByte(static_cast<std::size_t>(i));
        }
        result.push_back(static_cast<char16_t>(encryptedChar));
        mask++;
    }
    return result;
}

auto WzReader::ReadWzStringFromOffset(std::size_t offset) -> std::u16string
{
    auto prev = m_nCursor;
    SetPosition(offset);
    auto result = ReadWzString();
    SetPosition(prev);
    return result;
}

auto WzReader::ReadStringBlock(std::size_t baseOffset) -> std::u16string
{
    auto type = Read<std::uint8_t>();
    switch (type)
    {
    case 0:
    case 0x73:
        return ReadWzString();
    case 1:
    case 0x1B:
        return ReadWzStringFromOffset(baseOffset + Read<std::uint32_t>());
    default:
        return {};
    }
}

auto WzReader::IsWzImage() -> bool
{
    if (Read<std::uint8_t>() != 0x73)
        return false;
    if (ReadWzString() != u"Property")
        return false;
    if (Read<std::uint16_t>() != 0)
        return false;
    return true;
}

void WzReader::SetKey(const std::uint8_t* iv)
{
    std::memcpy(m_iv, iv, 4);
    InitializeKey();
}

auto WzReader::GetKeyByte(std::size_t index) -> std::uint8_t
{
    EnsureKeySize(index + 1);
    return m_keys[index];
}

void WzReader::EnsureKeySize(std::size_t size)
{
    if (m_keys.size() >= size)
        return;

    // Use WzCrypto for key generation
    WzCrypto::GenerateKey(m_keys, size);
}

void WzReader::InitializeKey()
{
    m_aesKey.assign(WzKeys::AesKey, WzKeys::AesKey + 32);
    m_keys.clear();

    // Initialize WzCrypto with IV
    WzCrypto::Initialize(m_iv);
}

} // namespace ms
