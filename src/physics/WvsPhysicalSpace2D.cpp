#include "WvsPhysicalSpace2D.h"

#include <algorithm>
#include <climits>
#include <cstdlib>

namespace ms
{

void WvsPhysicalSpace2D::GetCrossCandidate(
    std::int32_t x1, std::int32_t y1,
    std::int32_t x2, std::int32_t y2,
    std::vector<const IStaticFoothold*>* apResult)
{
    if (!apResult)
        return;

    using Tree = TRSTree<std::int32_t, std::shared_ptr<StaticFoothold>, 2, 4, 2>;
    const auto query = Tree::MakeBounds2D(x1, y1, x2, y2);

    std::vector<std::shared_ptr<StaticFoothold>> hits;
    m_rtFoothold.Search(query, hits);

    for (const auto& pFH : hits)
        apResult->push_back(pFH.get());
}

auto WvsPhysicalSpace2D::GetBaseZMass() -> std::int32_t
{
    return m_nBaseZMass;
}

auto WvsPhysicalSpace2D::GetFieldAttr() -> std::shared_ptr<CAttrField>
{
    return m_pAttrField;
}

auto WvsPhysicalSpace2D::GetBound() -> Rect
{
    return m_rcMBR;
}

auto WvsPhysicalSpace2D::GetFoothold(std::uint32_t dwSN) -> const IStaticFoothold*
{
    auto it = m_mFoothold.find(dwSN);
    if (it != m_mFoothold.end())
        return it->second.get();
    return nullptr;
}

auto WvsPhysicalSpace2D::GetLadderOrRope(
    [[maybe_unused]] Rect rc) -> const LadderOrRope*
{
    // TODO: search m_aLadderOrRope by rect intersection
    return nullptr;
}

auto WvsPhysicalSpace2D::GetLadderOrRopeBySN(std::uint32_t dwSN) -> const LadderOrRope*
{
    for (const auto& lr : m_aLadderOrRope)
    {
        if (lr.dwSN == dwSN)
            return &lr;
    }
    return nullptr;
}

// ========== Accessors ==========

auto WvsPhysicalSpace2D::GetZMassByIndex(std::int32_t nIndex) const -> std::int32_t
{
    return m_aIndexZMass[nIndex];
}

auto WvsPhysicalSpace2D::GetMassRange(std::int32_t nZMass) const -> const Range&
{
    return m_aMassRange[nZMass];
}

auto WvsPhysicalSpace2D::GetMassFootholdList(std::int32_t nZMass) const -> const std::vector<std::uint32_t>&
{
    return m_aaMassFootholdList[nZMass];
}

auto WvsPhysicalSpace2D::GetMassCount() const -> std::int32_t
{
    return static_cast<std::int32_t>(m_aIndexZMass.size());
}

// ========== Walk-Through ==========

auto WvsPhysicalSpace2D::CanWalkThrough(
    const StaticFoothold* pfhFrom,
    const StaticFoothold* pfhTo) const -> bool
{
    if (!pfhFrom || !pfhTo)
        return false;
    if (pfhFrom->IsOff())
        return false;
    if (pfhTo->IsOff() || pfhFrom->IsVertical() || pfhTo->IsVertical())
        return false;
    if (pfhFrom == pfhTo)
        return true;

    if (pfhFrom->GetX2() <= pfhTo->GetX1())
    {
        // pfhFrom is to the left of pfhTo → walk right via NextLink
        auto* cur = static_cast<const StaticFoothold*>(
            static_cast<const IStaticFoothold*>(pfhFrom));
        do
        {
            if (cur->IsVertical())
                break;
            if (cur == pfhTo)
                return true;
            cur = static_cast<const StaticFoothold*>(cur->GetNextLink());
        } while (cur);
    }
    else
    {
        // Check if pfhTo is entirely to the left of pfhFrom
        if (pfhTo->GetX2() <= pfhFrom->GetX1())
        {
            // Walk left via PrevLink
            auto* cur = pfhFrom;
            while (!cur->IsVertical())
            {
                if (cur == pfhTo)
                    return true;
                cur = static_cast<const StaticFoothold*>(cur->GetPrevLink());
                if (!cur)
                    return false;
            }
        }
    }
    return false;
}

auto WvsPhysicalSpace2D::FindRightEndX_CanWalkThrough(
    const StaticFoothold* pfhFrom) -> std::int32_t
{
    auto result = pfhFrom->GetX2();
    const IStaticFoothold* cur = pfhFrom;
    do
    {
        if (cur->GetUVX() <= 0.0L)
            break;
        if (cur->IsOff())
            break;
        result = cur->GetX2();
        cur = cur->GetNextLink();
    } while (cur);
    return result;
}

auto WvsPhysicalSpace2D::FindLeftEndX_CanWalkThrough(
    const StaticFoothold* pfhFrom) -> std::int32_t
{
    auto result = pfhFrom->GetX1();
    const IStaticFoothold* cur = pfhFrom;
    do
    {
        if (cur->GetUVX() <= 0.0L)
            break;
        if (cur->IsOff())
            break;
        result = cur->GetX1();
        cur = cur->GetPrevLink();
    } while (cur);
    return result;
}

// ========== Foothold Spatial Queries ==========

auto WvsPhysicalSpace2D::GetFootholdAbove(
    std::int32_t x, std::int32_t y,
    std::int32_t* pcy, std::int32_t yMax) -> StaticFoothold*
{
    if (yMax > y)
        return nullptr;

    using Tree = TRSTree<std::int32_t, std::shared_ptr<StaticFoothold>, 2, 4, 2>;
    const auto query = Tree::MakeBounds2D(x - 1, yMax, x + 1, y + 1);

    std::vector<std::shared_ptr<StaticFoothold>> hits;
    m_rtFoothold.Search(query, hits);

    StaticFoothold* pfhAbove = nullptr;
    for (const auto& pfh : hits)
    {
        if (!pfh || pfh->IsOff())
            continue;
        if (pfh->GetX1() >= pfh->GetX2())
            continue;
        if (pfh->GetX1() > x || pfh->GetX2() < x)
            continue;

        auto dy = pfh->GetY2() - pfh->GetY1();
        auto yAtX = dy * (x - pfh->GetX1())
                    / (pfh->GetX2() - pfh->GetX1())
                    + pfh->GetY1();

        if (yAtX <= y && yAtX > yMax)
        {
            yMax = yAtX;
            *pcy = yAtX;
            pfhAbove = pfh.get();
        }
    }
    return pfhAbove;
}

auto WvsPhysicalSpace2D::GetFootholdUnderneath(
    std::int32_t x, std::int32_t y,
    std::int32_t* pcy, std::int32_t yMin,
    std::int32_t nRangeX) -> StaticFoothold*
{
    if (yMin < y)
        return nullptr;

    using Tree = TRSTree<std::int32_t, std::shared_ptr<StaticFoothold>, 2, 4, 2>;
    const auto query = Tree::MakeBounds2D(x - nRangeX, y - 1, x + nRangeX, yMin);

    std::vector<std::shared_ptr<StaticFoothold>> hits;
    m_rtFoothold.Search(query, hits);

    StaticFoothold* pfhUnderneath = nullptr;
    for (const auto& pfh : hits)
    {
        if (!pfh || pfh->IsOff())
            continue;
        if (pfh->GetX1() >= pfh->GetX2())
            continue;
        if (pfh->GetX1() > x || pfh->GetX2() < x)
            continue;

        auto dy = pfh->GetY2() - pfh->GetY1();
        auto yAtX = dy * (x - pfh->GetX1())
                    / (pfh->GetX2() - pfh->GetX1())
                    + pfh->GetY1();

        if (yAtX >= y && yAtX < yMin)
        {
            yMin = yAtX;
            pfhUnderneath = pfh.get();
        }
    }

    if (pcy)
        *pcy = yMin;

    return pfhUnderneath;
}

auto WvsPhysicalSpace2D::GetFootholdClosest(
    std::int32_t x, std::int32_t y) -> StaticFoothold*
{
    StaticFoothold* pRet = nullptr;
    std::int32_t zmin = INT_MAX;

    for (const auto& pfh : m_lFoothold)
    {
        if (!pfh || pfh->IsOff())
            continue;
        // Skip narrow footholds (width < 8)
        if (pfh->GetX1() + 8 > pfh->GetX2())
            continue;

        auto cx = (pfh->GetX2() + pfh->GetX1()) / 2 - x;
        auto cy = (pfh->GetY2() + pfh->GetY1()) / 2 - y;
        auto dist = cx * cx + cy * cy;

        if (dist < zmin)
        {
            zmin = dist;
            pRet = pfh.get();
        }
    }
    return pRet;
}

auto WvsPhysicalSpace2D::GetFootholdVerticalClosest(
    std::int32_t x, std::int32_t y,
    std::int32_t* pcy,
    std::int32_t yMin, std::int32_t yMax) -> const StaticFoothold*
{
    std::int32_t cyUnder = 0;
    std::int32_t cyAbove = 0;

    auto* pfhUnder = GetFootholdUnderneath(x, y, &cyUnder, yMin, 1);
    auto* pfhAbove = GetFootholdAbove(x, y, &cyAbove, yMax);

    const StaticFoothold* result = nullptr;
    std::int32_t cy = 0;

    if (pfhUnder)
    {
        if (pfhAbove)
        {
            if (cyUnder - y > y - cyAbove)
            {
                result = pfhAbove;
                cy = cyAbove;
            }
            else
            {
                result = pfhUnder;
                cy = cyUnder;
            }
        }
        else
        {
            result = pfhUnder;
            cy = cyUnder;
        }
    }
    else
    {
        if (!pfhAbove)
            return nullptr;
        result = pfhAbove;
        cy = cyAbove;
    }

    if (pcy)
        *pcy = cy;
    return result;
}

auto WvsPhysicalSpace2D::GetGapY_FromFootholdAbove(
    Point2D pt, std::int32_t nSearchRangeY) -> std::int32_t
{
    std::int32_t cy{};
    if (GetFootholdAbove(pt.x, pt.y, &cy, pt.y - nSearchRangeY))
        return std::abs(cy - pt.y);
    return -1;
}

auto WvsPhysicalSpace2D::GetGapY_FromFootholdUnderneath(
    Point2D pt, std::int32_t nSearchRangeY) -> std::int32_t
{
    std::int32_t cy{};
    if (GetFootholdUnderneath(pt.x, pt.y, &cy, pt.y + nSearchRangeY, 1))
        return std::abs(cy - pt.y);
    return -1;
}

auto WvsPhysicalSpace2D::AdjustPoint_ToVerticallyClosestFoothold(
    Point2D& pt, std::int32_t nSearchRangeY) -> std::int32_t
{
    std::int32_t cyAbove{};
    std::int32_t cyUnder{};

    std::int32_t gapAbove = GetFootholdAbove(pt.x, pt.y, &cyAbove, pt.y - nSearchRangeY)
                            ? std::abs(cyAbove - pt.y)
                            : -1;

    std::int32_t gapUnder = GetFootholdUnderneath(pt.x, pt.y, &cyUnder, pt.y + nSearchRangeY, 1)
                            ? std::abs(cyUnder - pt.y)
                            : -1;

    if (gapAbove < 0)
    {
        if (gapUnder >= 0)
        {
            pt.y += gapUnder - 1;
            return 1;
        }
    }
    else if (gapUnder >= 0)
    {
        if (gapAbove < gapUnder)
        {
            pt.y += -1 - gapAbove;
            return 1;
        }
        pt.y += gapUnder - 1;
        return 1;
    }

    if (gapAbove < 0)
        return 0;

    pt.y += -1 - gapAbove;
    return 1;
}

auto WvsPhysicalSpace2D::AdjustPoint_ToVerticallyClosestFoothold_ByFirst(
    Point2D& pt,
    std::int32_t nSearchRangeAboveY,
    std::int32_t nSearchRangeUnderneathY,
    FindType nFindType) -> std::int32_t
{
    std::int32_t cyAbove{};
    std::int32_t cyUnder{};

    std::int32_t gapAbove = GetFootholdAbove(pt.x, pt.y, &cyAbove, pt.y - nSearchRangeAboveY)
                            ? std::abs(cyAbove - pt.y)
                            : -1;

    std::int32_t gapUnder = GetFootholdUnderneath(
                                pt.x, pt.y, &cyUnder,
                                pt.y + nSearchRangeUnderneathY, 1)
                            ? std::abs(cyUnder - pt.y)
                            : -1;

    switch (nFindType)
    {
    case FIRST_CLOSEST:
        if (gapAbove >= 0)
        {
            if (gapUnder >= 0)
            {
                if (gapAbove < gapUnder)
                {
                    pt.y += -1 - gapAbove;
                    return 1;
                }
                pt.y += gapUnder - 1;
                return 1;
            }
            pt.y += -1 - gapAbove;
            return 1;
        }
        if (gapUnder < 0)
        {
            if (gapAbove >= 0)
            {
                pt.y += -1 - gapAbove;
                return 1;
            }
            return 0;
        }
        pt.y += gapUnder - 1;
        return 1;

    case FIRST_UNDERNEATH:
        if (gapUnder < 0)
        {
            if (gapAbove >= 0)
            {
                pt.y += -1 - gapAbove;
                return 1;
            }
            return 0;
        }
        pt.y += gapUnder - 1;
        return 1;

    case FIRST_ABOVE:
        if (gapAbove >= 0)
        {
            pt.y += -1 - gapAbove;
            return 1;
        }
        if (gapUnder >= 0)
        {
            pt.y += gapUnder - 1;
            return 1;
        }
        break;
    }
    return 0;
}

void WvsPhysicalSpace2D::FootHoldStateChange(std::uint32_t dwSN, std::int32_t nState)
{
    auto it = m_mFoothold.find(dwSN);
    if (it != m_mFoothold.end() && it->second)
        it->second->SetState(nState);
}

} // namespace ms
