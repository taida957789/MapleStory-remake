#include "SpineSkeletonBinary.h"

#include <spine/extension.h>

#include <cstring>

namespace Spine
{

// ---------------------------------------------------------------------------
// Primitive readers
// ---------------------------------------------------------------------------

auto SkeletonBinary::ReadSByte(BufferedStream& input) -> char
{
    return static_cast<char>(input.ReadByte());
}

auto SkeletonBinary::ReadBoolean(BufferedStream& input) -> bool
{
    return input.ReadByte() != 0;
}

auto SkeletonBinary::ReadFloat(BufferedStream& input) -> float
{
    // Big-endian 4-byte float
    union
    {
        std::uint32_t i;
        float f;
    } t;

    auto b0 = static_cast<std::uint32_t>(input.ReadByte());
    auto b1 = static_cast<std::uint32_t>(input.ReadByte());
    auto b2 = static_cast<std::uint32_t>(input.ReadByte());
    auto b3 = static_cast<std::uint32_t>(input.ReadByte());
    t.i = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    return t.f;
}

auto SkeletonBinary::ReadInt(BufferedStream& input) -> int
{
    // Big-endian 4-byte integer
    auto b0 = static_cast<std::uint32_t>(input.ReadByte());
    auto b1 = static_cast<std::uint32_t>(input.ReadByte());
    auto b2 = static_cast<std::uint32_t>(input.ReadByte());
    auto b3 = static_cast<std::uint32_t>(input.ReadByte());
    return static_cast<int>((b0 << 24) | (b1 << 16) | (b2 << 8) | b3);
}

auto SkeletonBinary::ReadInt(BufferedStream& input, bool optimizePositive) -> int
{
    // Variable-length integer encoding (1-5 bytes)
    auto b = static_cast<int>(input.ReadByte());
    int result = b & 0x7F;

    if (b & 0x80)
    {
        b = static_cast<int>(input.ReadByte());
        result |= (b & 0x7F) << 7;
        if (b & 0x80)
        {
            b = static_cast<int>(input.ReadByte());
            result |= (b & 0x7F) << 14;
            if (b & 0x80)
            {
                b = static_cast<int>(input.ReadByte());
                result |= (b & 0x7F) << 21;
                if (b & 0x80)
                {
                    b = static_cast<int>(input.ReadByte());
                    result |= b << 28;
                }
            }
        }
    }

    if (!optimizePositive)
        result = static_cast<int>(static_cast<unsigned int>(result) >> 1) ^ -(result & 1);

    return result;
}

void SkeletonBinary::ReadUtf8Slow(BufferedStream& input, std::string& chars,
                                   unsigned int charCount, int b)
{
    while (true)
    {
        switch (b >> 4)
        {
        case 0: case 1: case 2: case 3:
        case 4: case 5: case 6: case 7:
            // Single-byte character
            chars += static_cast<char>(b);
            break;

        case 12: case 13:
        {
            // Two-byte character
            int b2 = input.ReadByte();
            chars += static_cast<char>(((b & 0x1F) << 6) | (b2 & 0x3F));
            break;
        }
        case 14:
        {
            // Three-byte character
            int b2 = input.ReadByte();
            int b3 = input.ReadByte();
            chars += static_cast<char>(((b & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F));
            break;
        }
        default:
            break;
        }

        if (chars.size() >= charCount)
            return;

        b = input.ReadByte();
    }
}

auto SkeletonBinary::ReadString(BufferedStream& input) -> std::string
{
    auto charCount = static_cast<unsigned int>(ReadInt(input, true));
    if (charCount <= 1)
        return {};

    charCount -= 1; // Length includes null terminator marker

    std::string chars;
    chars.reserve(charCount);

    while (chars.size() < charCount)
    {
        auto b = input.ReadByte();
        if (b > 0x7F)
        {
            ReadUtf8Slow(input, chars, charCount, b);
            break;
        }
        chars += static_cast<char>(b);
    }

    return chars;
}

void SkeletonBinary::ReadCurve(BufferedStream& input, int frameIndex,
                                spCurveTimeline* timeline)
{
    auto type = input.ReadByte();
    if (type == 1)
    {
        spCurveTimeline_setStepped(timeline, frameIndex);
    }
    else if (type == 2)
    {
        float cx1 = ReadFloat(input);
        float cy1 = ReadFloat(input);
        float cx2 = ReadFloat(input);
        float cy2 = ReadFloat(input);
        spCurveTimeline_setCurve(timeline, frameIndex, cx1, cy1, cx2, cy2);
    }
    // type == 0 is linear (default)
}

// ---------------------------------------------------------------------------
// Array readers
// ---------------------------------------------------------------------------

auto SkeletonBinary::ReadFloatArray(BufferedStream& input, float scale) -> std::vector<float>
{
    int n = ReadInt(input, true);
    std::vector<float> result;
    result.reserve(static_cast<std::size_t>(n));

    if (scale == 1.0f)
    {
        for (int i = 0; i < n; ++i)
            result.push_back(ReadFloat(input));
    }
    else
    {
        for (int i = 0; i < n; ++i)
            result.push_back(ReadFloat(input) * scale);
    }

    return result;
}

auto SkeletonBinary::ReadShortArray(BufferedStream& input) -> std::vector<int>
{
    int n = ReadInt(input, true);
    std::vector<int> result;
    result.reserve(static_cast<std::size_t>(n));

    for (int i = 0; i < n; ++i)
    {
        // Big-endian 2-byte short, stored as int
        auto hi = static_cast<int>(input.ReadByte());
        auto lo = static_cast<int>(input.ReadByte());
        result.push_back((hi << 8) | lo);
    }

    return result;
}

auto SkeletonBinary::ReadIntArray(BufferedStream& input) -> std::vector<int>
{
    int n = ReadInt(input, true);
    std::vector<int> result;
    result.reserve(static_cast<std::size_t>(n));

    for (int i = 0; i < n; ++i)
        result.push_back(ReadInt(input, true));

    return result;
}

// ---------------------------------------------------------------------------
// Main entry point — delegates to C implementation
// ---------------------------------------------------------------------------

auto SkeletonBinary::Read(spSkeletonJson* self, BufferedStream& input) -> spSkeletonData*
{
    // Forward to the existing C implementation which contains the full
    // skeleton parsing logic (ReadAttachment, ReadSkin, ReadAnimation, etc.)
    unsigned int remaining = input.GetLength() - input.GetPosition();
    const void* pData = input.GetBuffer() + input.GetPosition();
    return spSkeletonBinary_readSkeletonData(self, pData, remaining);
}

// ---------------------------------------------------------------------------
// Structure readers — stubs (full logic is in SkeletonBinary.c)
// ---------------------------------------------------------------------------

auto SkeletonBinary::ReadAttachment(BufferedStream& /*input*/, spSkin* /*skin*/,
                                     const std::string& /*name*/, bool /*nonessential*/)
    -> spAttachment*
{
    // Full implementation is in SkeletonBinary.c (readAttachment)
    return nullptr;
}

auto SkeletonBinary::ReadSkin(BufferedStream& /*input*/, const std::string& /*name*/,
                               bool /*nonessential*/) -> spSkin*
{
    // Full implementation is in SkeletonBinary.c (readSkin)
    return nullptr;
}

void SkeletonBinary::ReadAnimation(const std::string& /*name*/, BufferedStream& /*input*/,
                                    spSkeletonData* /*skeletonData*/, int /*nonessential*/)
{
    // Full implementation is in SkeletonBinary.c (readAnimation)
}

} // namespace Spine
