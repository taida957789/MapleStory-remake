#pragma once

#include <cstdint>
#include <vector>

namespace ms
{

/**
 * @brief WZ Node Types
 *
 * Based on reverse engineering of MapleStory client COM interfaces:
 * - IWzProperty (GUID: 986515d9-0a0b-4929-8b4f-718682177b92)
 * - IWzCanvas (GUID: 7600dc6c-9328-4bff-9624-5b0f5c01179e)
 * - IWzUOL (GUID: f945bf59-d1ec-45e8-8bd9-3dd11ac1a48a)
 * - IWzVector2D (GUID: f28bd1ed-3deb-4f92-9eec-10ef5a1c3fb4)
 * - IWzShape2D (GUID: 4cfb57c7-eae3-40b3_ac98-4b2750e3642a)
 */
enum class WzNodeType : std::uint8_t
{
    NotSet = 0x00,
    Directory = 0x10,
    Image = 0x20,
    Property = 0x30,

    // Property sub-types
    Null = 0x31,
    Int = 0x32,
    UnsignedShort = 0x33,
    Float = 0x34,
    Double = 0x35,
    String = 0x36,

    SubProperty = 0x37,
    Canvas = 0x38,
    Vector2D = 0x39,
    Convex2D = 0x3A,
    Sound = 0x3B,
    UOL = 0x3C,
    RawData = 0x3D,
    Video = 0x3E,
};

/**
 * @brief Check if type is a property type (not directory/image)
 */
[[nodiscard]] constexpr auto IsPropertyType(WzNodeType type) noexcept -> bool
{
    return (static_cast<std::uint8_t>(type) & static_cast<std::uint8_t>(WzNodeType::Property)) ==
           static_cast<std::uint8_t>(WzNodeType::Property);
}

/**
 * @brief WZ Canvas format types
 */
enum class WzCanvasFormat : std::int32_t
{
    ARGB4444 = 1,
    ARGB8888 = 2,
    RGB565 = 513,  // Format16bppRgb565
    DXT3 = 517,
    DXT5 = 1026,
};

/**
 * @brief WZ Sound data
 */
struct WzSoundData
{
    std::int32_t length{};
    std::int32_t frequency{};
    std::int32_t size{};
    std::size_t offset{};
};

/**
 * @brief WZ Canvas data (metadata only, actual pixels loaded separately)
 */
struct WzCanvasData
{
    std::int32_t width{};
    std::int32_t height{};
    std::int32_t format{};
    std::int32_t format2{};
    bool isEncrypted{false};
    std::int32_t size{};
    std::int32_t uncompressedSize{};
    std::size_t offset{};
};

/**
 * @brief WZ RawData metadata
 */
struct WzRawData
{
    std::int32_t type{};
    std::size_t offset{};
    std::size_t size{};
};

/**
 * @brief WZ Video metadata
 */
struct WzVideoData
{
    std::int32_t type{};
    std::size_t offset{};
    std::size_t size{};
};

/**
 * @brief WZ Vector2D data
 */
struct WzVector2D
{
    std::int32_t x{};
    std::int32_t y{};
};

/**
 * @brief WZ encryption keys for KMS (Korean MapleStory)
 *
 * IV: 0xB9, 0x7D, 0x63, 0xE9
 */
namespace WzKeys
{
    // Zero IV for older/unencrypted WZ files
    constexpr std::uint8_t ZERO_IV[4] = {0x00, 0x00, 0x00, 0x00};

    constexpr std::uint8_t KMS_IV[4] = {0xB9, 0x7D, 0x63, 0xE9};
    constexpr std::uint8_t GMS_IV[4] = {0x4D, 0x23, 0xC7, 0x2B};

    // AES key used for string encryption
    constexpr std::uint8_t AesKey[32] = {
        0x13, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x06, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x00, 0x00,
        0x1B, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00,
        0x33, 0x00, 0x00, 0x00, 0x52, 0x00, 0x00, 0x00,
    };

    // Offset key for decrypting file offsets
    constexpr std::uint32_t OffsetKey = 0x581C3F6D;

    // PKG1 magic header
    constexpr std::uint32_t HeaderMagic = 0x31474B50;
}

} // namespace ms
