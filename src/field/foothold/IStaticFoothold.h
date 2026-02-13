#pragma once

#include <cstdint>

namespace ms
{

class CAttrFoothold;

/**
 * @brief Interface for static foothold data
 *
 * Based on IStaticFoothold from the original MapleStory client (v1029).
 * Provides read-only access to foothold geometry, linking, and attributes.
 *
 * VFT layout (sizeof=0x38):
 *   0x00  GetSN
 *   0x04  GetX1
 *   0x08  GetX2
 *   0x0C  GetY1
 *   0x10  GetY2
 *   0x14  GetPage
 *   0x18  GetZMass
 *   0x1C  GetUVX
 *   0x20  GetUVY
 *   0x24  GetLen
 *   0x28  GetPrevLink
 *   0x2C  GetNextLink
 *   0x30  GetAttribute
 *   0x34  IsOff
 */
class IStaticFoothold
{
public:
    virtual ~IStaticFoothold() = default;

    // ========== Geometry ==========

    [[nodiscard]] virtual auto GetSN() const -> std::uint32_t = 0;
    [[nodiscard]] virtual auto GetX1() const -> std::int32_t = 0;
    [[nodiscard]] virtual auto GetX2() const -> std::int32_t = 0;
    [[nodiscard]] virtual auto GetY1() const -> std::int32_t = 0;
    [[nodiscard]] virtual auto GetY2() const -> std::int32_t = 0;
    [[nodiscard]] virtual auto GetPage() const -> std::int32_t = 0;
    [[nodiscard]] virtual auto GetZMass() const -> std::int32_t = 0;

    // ========== Unit Vector / Length ==========

    [[nodiscard]] virtual auto GetUVX() const -> long double = 0;
    [[nodiscard]] virtual auto GetUVY() const -> long double = 0;
    [[nodiscard]] virtual auto GetLen() const -> long double = 0;

    // ========== Linking ==========

    [[nodiscard]] virtual auto GetPrevLink() const -> const IStaticFoothold* = 0;
    [[nodiscard]] virtual auto GetNextLink() const -> const IStaticFoothold* = 0;

    // ========== Attribute ==========

    [[nodiscard]] virtual auto GetAttribute() -> CAttrFoothold* = 0;
    [[nodiscard]] virtual auto IsOff() const -> bool = 0;
};

} // namespace ms
