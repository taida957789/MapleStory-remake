#pragma once

#include "field/CAttrField.h"
#include "field/LadderOrRope.h"
#include "field/foothold/FootholdSplit.h"
#include "field/foothold/StaticFoothold.h"
#include "physics/IWvsPhysicalSpace2D.h"
#include "util/Singleton.h"
#include "util/TRSTree.h"

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <vector>

namespace ms
{

/**
 * @brief Concrete 2D physical space (singleton)
 *
 * Based on CWvsPhysicalSpace2D (__cppobj : IWvsPhysicalSpace2D,
 * TSingleton<CWvsPhysicalSpace2D>) from the original MapleStory client (v1029).
 * Manages footholds, ladders/ropes, and spatial queries for physics.
 */
class WvsPhysicalSpace2D
    : public IWvsPhysicalSpace2D
    , public Singleton<WvsPhysicalSpace2D>
{
    friend class Singleton<WvsPhysicalSpace2D>;

public:
    enum FindType : std::int32_t
    {
        FIRST_CLOSEST = 0,
        FIRST_UNDERNEATH = 1,
        FIRST_ABOVE = 2,
    };

    // ========== IWvsPhysicalSpace2D ==========

    void GetCrossCandidate(
        std::int32_t x1, std::int32_t y1,
        std::int32_t x2, std::int32_t y2,
        std::vector<const IStaticFoothold*>* apResult) override;

    [[nodiscard]] auto GetBaseZMass() -> std::int32_t override;
    [[nodiscard]] auto GetFieldAttr() -> std::shared_ptr<CAttrField> override;
    [[nodiscard]] auto GetBound() -> Rect override;
    [[nodiscard]] auto GetFoothold(std::uint32_t dwSN) -> const IStaticFoothold* override;
    [[nodiscard]] auto GetLadderOrRope(Rect rc) -> const LadderOrRope* override;
    [[nodiscard]] auto GetLadderOrRopeBySN(std::uint32_t dwSN) -> const LadderOrRope* override;

    // ========== Accessors ==========

    [[nodiscard]] auto GetMBR() const -> const Rect& { return m_rcMBR; }
    [[nodiscard]] auto GetCRC() -> std::uint32_t { return m_dwCRC; }
    [[nodiscard]] auto GetZMassByIndex(std::int32_t nIndex) const -> std::int32_t;
    [[nodiscard]] auto GetMassRange(std::int32_t nZMass) const -> const Range&;
    [[nodiscard]] auto GetMassFootholdList(std::int32_t nZMass) const -> const std::vector<std::uint32_t>&;
    [[nodiscard]] auto GetMassCount() const -> std::int32_t;

    // ========== Foothold Queries ==========

    [[nodiscard]] auto CanWalkThrough(
        const StaticFoothold* pfhFrom,
        const StaticFoothold* pfhTo) const -> bool;

    auto FindRightEndX_CanWalkThrough(const StaticFoothold* pfhFrom) -> std::int32_t;
    auto FindLeftEndX_CanWalkThrough(const StaticFoothold* pfhFrom) -> std::int32_t;

    [[nodiscard]] auto GetFootholdAbove(
        std::int32_t x, std::int32_t y,
        std::int32_t* pcy, std::int32_t yMax) -> StaticFoothold*;

    [[nodiscard]] auto GetFootholdUnderneath(
        std::int32_t x, std::int32_t y,
        std::int32_t* pcy, std::int32_t yMin,
        std::int32_t nRangeX) -> StaticFoothold*;

    [[nodiscard]] auto GetFootholdClosest(
        std::int32_t x, std::int32_t y) -> StaticFoothold*;

    [[nodiscard]] auto GetFootholdVerticalClosest(
        std::int32_t x, std::int32_t y,
        std::int32_t* pcy,
        std::int32_t yMin, std::int32_t yMax) -> const StaticFoothold*;

    [[nodiscard]] auto GetGapY_FromFootholdAbove(
        Point2D pt, std::int32_t nSearchRangeY) -> std::int32_t;

    [[nodiscard]] auto GetGapY_FromFootholdUnderneath(
        Point2D pt, std::int32_t nSearchRangeY) -> std::int32_t;

    auto AdjustPoint_ToVerticallyClosestFoothold(
        Point2D& pt, std::int32_t nSearchRangeY) -> std::int32_t;

    auto AdjustPoint_ToVerticallyClosestFoothold_ByFirst(
        Point2D& pt,
        std::int32_t nSearchRangeAboveY,
        std::int32_t nSearchRangeUnderneathY,
        FindType nFindType) -> std::int32_t;

    void FootHoldStateChange(std::uint32_t dwSN, std::int32_t nState);

private:
    WvsPhysicalSpace2D() = default;

    std::shared_ptr<Geometry::InclusionChecker> m_pIC_SwimArea;
    Rect m_rcMBR;
    std::vector<Range> m_aMassRange;
    std::vector<std::int32_t> m_aIndexZMass;
    std::vector<std::vector<std::uint32_t>> m_aaMassFootholdList;
    std::int32_t m_nBaseZMass{};
    TRSTree<std::int32_t, std::shared_ptr<StaticFoothold>, 2, 4, 2> m_rtFoothold;
    std::list<std::shared_ptr<StaticFoothold>> m_lFoothold;
    std::map<std::uint32_t, std::shared_ptr<StaticFoothold>> m_mFoothold;
    std::vector<LadderOrRope> m_aLadderOrRope;
    std::shared_ptr<CAttrField> m_pAttrField;
    std::vector<std::vector<std::shared_ptr<FootholdSplit>>> m_aaFootHoldSplit;
    std::uint32_t m_dwCRC{};
};

} // namespace ms
