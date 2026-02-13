#pragma once

#include "field/foothold/CAttrFoothold.h"
#include "field/foothold/IStaticFoothold.h"

#include <cmath>
#include <cstdint>
#include <memory>

namespace ms
{

/**
 * @brief Concrete static foothold segment
 *
 * Based on CStaticFoothold (__cppobj : ZRefCounted, IStaticFoothold) from
 * the original MapleStory client (v1029). Stores geometry, unit vectors,
 * linking, and attribute data for a single foothold line segment.
 */
class StaticFoothold : public IStaticFoothold
{
public:
    StaticFoothold(
        std::uint32_t dwSN,
        std::int32_t x1, std::int32_t y1,
        std::int32_t x2, std::int32_t y2,
        std::int32_t page, std::int32_t zMass,
        std::uint32_t dwSNPrev, std::uint32_t dwSNNext,
        std::shared_ptr<CAttrFoothold> pAttrFoothold)
        : m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2)
        , m_xReal1(x1), m_yReal1(y1), m_xReal2(x2), m_yReal2(y2)
        , m_lPage(page), m_lZMass(zMass)
        , m_pAttrFoothold(std::move(pAttrFoothold))
        , m_dwSN(dwSN), m_nState(1)
        , m_dwSNPrev(dwSNPrev), m_dwSNNext(dwSNNext)
    {
        const auto dx = static_cast<double>(m_x2 - m_x1);
        const auto dy = static_cast<double>(m_y2 - m_y1);
        m_len = std::sqrt(dx * dx + dy * dy);
        m_uvx = dx / m_len;
        m_uvy = dy / m_len;
    }

    // ========== IStaticFoothold ==========

    [[nodiscard]] auto GetSN() const -> std::uint32_t override { return m_dwSN; }
    [[nodiscard]] auto GetX1() const -> std::int32_t override { return m_x1; }
    [[nodiscard]] auto GetX2() const -> std::int32_t override { return m_x2; }
    [[nodiscard]] auto GetY1() const -> std::int32_t override { return m_y1; }
    [[nodiscard]] auto GetY2() const -> std::int32_t override { return m_y2; }
    [[nodiscard]] auto GetPage() const -> std::int32_t override { return m_lPage; }
    [[nodiscard]] auto GetZMass() const -> std::int32_t override { return m_lZMass; }
    [[nodiscard]] auto GetUVX() const -> long double override { return m_uvx; }
    [[nodiscard]] auto GetUVY() const -> long double override { return m_uvy; }
    [[nodiscard]] auto GetLen() const -> long double override { return m_len; }
    [[nodiscard]] auto GetPrevLink() const -> const IStaticFoothold* override { return m_pPrev; }
    [[nodiscard]] auto GetNextLink() const -> const IStaticFoothold* override { return m_pNext; }
    [[nodiscard]] auto GetAttribute() -> CAttrFoothold* override { return m_pAttrFoothold.get(); }
    [[nodiscard]] auto IsOff() const -> bool override { return m_bOff; }

    // ========== Link Resolution ==========

    [[nodiscard]] auto GetSNPrev() const -> std::uint32_t { return m_dwSNPrev; }
    [[nodiscard]] auto GetSNNext() const -> std::uint32_t { return m_dwSNNext; }
    void SetPrevLink(const IStaticFoothold* p) { m_pPrev = p; }
    void SetNextLink(const IStaticFoothold* p) { m_pNext = p; }

    // ========== Static Helpers ==========

    [[nodiscard]] auto IsVertical() const -> bool { return GetUVX() <= 0.0L; }

    // ========== Mutators ==========

    void SetState(std::int32_t nState) { m_nState = nState; }

    // ========== Queries ==========

    /**
     * @brief Get Y coordinate at a given X position on this foothold
     * @return Y coordinate, or 0x80000000 if x is out of range or foothold is vertical
     */
    [[nodiscard]] auto GetY_ByX(std::int32_t x) const -> std::int32_t
    {
        if (x < m_x1 || x > m_x2 || m_uvx == 0.0L)
            return static_cast<std::int32_t>(0x80000000u);

        return static_cast<std::int32_t>(
            static_cast<double>(static_cast<std::int32_t>(
                static_cast<double>(x - m_x1) / m_uvx)) * m_uvy
            + static_cast<double>(m_y1));
    }

private:
    std::int32_t m_x1{};
    std::int32_t m_y1{};
    std::int32_t m_x2{};
    std::int32_t m_y2{};
    std::int32_t m_xReal1{};
    std::int32_t m_yReal1{};
    std::int32_t m_xReal2{};
    std::int32_t m_yReal2{};
    std::int32_t m_lPage{};
    std::int32_t m_lZMass{};
    std::shared_ptr<CAttrFoothold> m_pAttrFoothold;  // original: ZRef<CAttrFoothold>
    long double m_uvx{};
    long double m_uvy{};
    long double m_len{};
    std::uint32_t m_dwSN{};
    std::int32_t m_nState{};
    // original: anonymous unions ___u19/___u20
    // During construction, stores SN values (dwSNPrev/dwSNNext).
    // After link resolution, m_pPrev/m_pNext point to resolved footholds.
    std::uint32_t m_dwSNPrev{};
    std::uint32_t m_dwSNNext{};
    const IStaticFoothold* m_pPrev{};
    const IStaticFoothold* m_pNext{};
    bool m_bDynamicFootHold{};
    bool m_bOff{};
};

} // namespace ms
