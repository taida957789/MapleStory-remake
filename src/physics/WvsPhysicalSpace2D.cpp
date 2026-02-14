#include "WvsPhysicalSpace2D.h"

#include "util/CCrc32.h"
#include "util/Logger.h"
#include "wz/WzProperty.h"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <string>

namespace ms
{

// ========== Loading ==========

auto WvsPhysicalSpace2D::GetConstantCRC() -> std::uint32_t
{
    // Based on CWvsPhysicalSpace2D::GetConstantCRC
    // CRC32 chain over physics constants (version + 18 physics parameters)
    std::uint32_t crc = 0;

    // Version (v1029 = 0x405)
    std::int32_t v = 1029;
    crc = CCrc32::GetCrc32(v, crc);

    // Physics constants (from C# reference and IDA decompilation)
    constexpr std::int32_t kConstants[] = {
        140000,  // dWalkForce
        125,     // dWalkSpeed
        80000,   // dWalkDrag
        60000,   // dSlipForce
        120,     // dSlipSpeed
        100000,  // dFloatDrag1
        0,       // dFloatCoefficient
        120000,  // dSwimForce
        140,     // dSwimSpeed
        120000,  // dFlyForce
        200,     // dFlySpeed
        2000,    // dGravityAcc
        670,     // dFallSpeed
        555,     // dJumpSpeed
        2,       // dMaxFriction
        0,       // dMinFriction
        0,       // dSwimSpeedDec
        0,       // dFlyJumpDec
    };

    for (auto c : kConstants)
        crc = CCrc32::GetCrc32(c, crc);

    return crc;
}

void WvsPhysicalSpace2D::Load(
    std::shared_ptr<WzProperty> pPropFoothold,
    std::shared_ptr<WzProperty> pLadderRope,
    std::shared_ptr<WzProperty> pInfo)
{
    // Based on CWvsPhysicalSpace2D::Load from the original MapleStory client (v1029)

    // Initialize CRC with physics constants (computed once)
    static const std::uint32_t dwConstantCRC = GetConstantCRC();
    m_dwCRC = dwConstantCRC;

    // Clear previous state
    m_lFoothold.clear();
    m_mFoothold.clear();
    m_rtFoothold.Clear();
    m_aMassRange.clear();
    m_aIndexZMass.clear();
    m_aaMassFootholdList.clear();
    m_aLadderOrRope.clear();
    m_nBaseZMass = 0;

    // Initialize MBR to extremes for min/max tracking
    m_rcMBR.left = INT_MAX;
    m_rcMBR.top = INT_MAX;
    m_rcMBR.right = INT_MIN;
    m_rcMBR.bottom = INT_MIN;

    // ========== 1. Load Footholds ==========
    // Triple-nested: foothold/{page}/{mass}/{footholdId}

    if (pPropFoothold)
    {
        for (const auto& [pageName, pPageProp] : pPropFoothold->GetChildren())
        {
            auto nPage = std::stoi(pageName);

            for (const auto& [massName, pMassProp] : pPageProp->GetChildren())
            {
                auto nZMass = std::stoi(massName);

                for (const auto& [fhName, pFH] : pMassProp->GetChildren())
                {
                    auto dwSN = static_cast<std::uint32_t>(std::stoi(fhName));
                    if (dwSN == 0)
                        continue;

                    // Read foothold properties (StringPool IDs in comments)
                    auto x1Prop = pFH->GetChild("x1");            // 0xEAA = 3754
                    auto y1Prop = pFH->GetChild("y1");            // 0xEAC = 3756
                    auto x2Prop = pFH->GetChild("x2");            // 0xEAB = 3755
                    auto y2Prop = pFH->GetChild("y2");            // 0xEAD = 3757
                    auto dragProp = pFH->GetChild("drag");        // 0x1253 = 4691
                    auto forceProp = pFH->GetChild("force");      // 0x1254 = 4692
                    auto forbidProp = pFH->GetChild("forbidFallDown"); // 0x1888 = 6280
                    auto cantProp = pFH->GetChild("cantThrough"); // literal string
                    auto prevProp = pFH->GetChild("prev");        // 0xC6B = 3179
                    auto nextProp = pFH->GetChild("next");        // 0xC6C = 3180

                    auto x1 = x1Prop ? x1Prop->GetInt(0) : 0;
                    auto y1 = y1Prop ? y1Prop->GetInt(0) : 0;
                    auto x2 = x2Prop ? x2Prop->GetInt(0) : 0;
                    auto y2 = y2Prop ? y2Prop->GetInt(0) : 0;
                    auto nDrag = dragProp ? dragProp->GetInt(0) : 0;
                    auto nForce = forceProp ? forceProp->GetInt(0) : 0;
                    auto nForbidFallDown = forbidProp ? (forbidProp->GetInt(0) != 0 ? 1 : 0) : 0;
                    auto nCantThrough = cantProp ? (cantProp->GetInt(0) != 0 ? 1 : 0) : 0;
                    auto dwSNPrev = static_cast<std::uint32_t>(prevProp ? prevProp->GetInt(0) : 0);
                    auto dwSNNext = static_cast<std::uint32_t>(nextProp ? nextProp->GetInt(0) : 0);

                    // CRC32 chain over foothold data
                    m_dwCRC = CCrc32::GetCrc32(x1, m_dwCRC);
                    m_dwCRC = CCrc32::GetCrc32(y1, m_dwCRC);
                    m_dwCRC = CCrc32::GetCrc32(x2, m_dwCRC);
                    m_dwCRC = CCrc32::GetCrc32(y2, m_dwCRC);
                    m_dwCRC = CCrc32::GetCrc32(nDrag, m_dwCRC);
                    m_dwCRC = CCrc32::GetCrc32(nForce, m_dwCRC);
                    m_dwCRC = CCrc32::GetCrc32(nForbidFallDown, m_dwCRC);
                    m_dwCRC = CCrc32::GetCrc32(nCantThrough, m_dwCRC);
                    m_dwCRC = CCrc32::GetCrc32(dwSNPrev, m_dwCRC);
                    m_dwCRC = CCrc32::GetCrc32(dwSNNext, m_dwCRC);
                    m_dwCRC = CCrc32::GetCrc32(dwSN, m_dwCRC);

                    // Create foothold attribute
                    auto pAttr = std::make_shared<CAttrFoothold>();
                    if (nDrag)
                        pAttr->drag.SetData(static_cast<double>(nDrag) / 100.0);
                    if (nForce)
                        pAttr->force.SetData(static_cast<double>(nForce) / 100.0);
                    if (nForbidFallDown)
                        pAttr->forbidfalldown.SetData(nForbidFallDown);
                    if (nCantThrough)
                        pAttr->cantThrough.SetData(nCantThrough);

                    // Create static foothold
                    auto pfh = std::make_shared<StaticFoothold>(
                        dwSN, x1, y1, x2, y2,
                        nPage, nZMass,
                        dwSNPrev, dwSNNext,
                        std::move(pAttr));

                    // Add to collections
                    m_lFoothold.push_back(pfh);
                    m_mFoothold[dwSN] = pfh;

                    // Insert into R*-tree spatial index
                    using Tree = TRSTree<std::int32_t, std::shared_ptr<StaticFoothold>, 2, 4, 2>;
                    auto bounds = Tree::MakeBounds2D(x1, y1, x2, y2);
                    m_rtFoothold.Insert(bounds, pfh);

                    // Ensure mass arrays are large enough
                    auto nZMassU = static_cast<std::size_t>(nZMass);
                    if (m_aMassRange.size() <= nZMassU)
                    {
                        auto oldSize = m_aMassRange.size();
                        m_aMassRange.resize(nZMassU + 1);
                        m_aaMassFootholdList.resize(nZMassU + 1);

                        // Initialize new range entries to invalid (high < low)
                        for (auto i = oldSize; i <= nZMassU; ++i)
                        {
                            m_aMassRange[i].low = INT_MAX;    // 0x7FFFFFFF
                            m_aMassRange[i].high = INT_MIN;   // 0x80000000
                        }
                    }

                    // Compute bounding box of this foothold
                    auto xMin = std::min(x1, x2);
                    auto xMax = std::max(x1, x2);
                    auto yMin = std::min(y1, y2);
                    auto yMax = std::max(y1, y2);

                    // Update MBR (minimum bounding rectangle)
                    if (m_rcMBR.left > xMin + 30)
                        m_rcMBR.left = xMin + 30;
                    if (m_rcMBR.right < xMax - 30)
                        m_rcMBR.right = xMax - 30;
                    if (m_rcMBR.top > yMin - 300)
                        m_rcMBR.top = yMin - 300;
                    // Bottom only updated for non-vertical footholds (x1 != x2)
                    if (x1 != x2)
                    {
                        if (m_rcMBR.bottom < yMax + 10)
                            m_rcMBR.bottom = yMax + 10;
                    }

                    // Update mass range
                    if (m_aMassRange[nZMassU].low > xMin)
                        m_aMassRange[nZMassU].low = xMin;
                    if (m_aMassRange[nZMassU].high < xMax)
                        m_aMassRange[nZMassU].high = xMax;

                    // Track mass index (add to index list on first foothold in this mass)
                    if (m_aaMassFootholdList[nZMassU].empty())
                        m_aIndexZMass.push_back(nZMass);

                    // Add foothold SN to mass list
                    m_aaMassFootholdList[nZMassU].push_back(dwSN);
                }
            }
        }
    }

    // ========== 2. Adjust MBR from map info VR bounds ==========

    if (pInfo)
    {
        // StringPool 2578 = "VRLimit" — flag to enable VR override
        auto vrLimitProp = pInfo->GetChild("VRLimit");
        bool bVRLimit = vrLimitProp && (vrLimitProp->GetInt(0) != 0);

        if (bVRLimit)
        {
            auto vrLeftProp = pInfo->GetChild("VRLeft");      // StringPool 2574
            auto vrRightProp = pInfo->GetChild("VRRight");    // StringPool 2575
            auto vrTopProp = pInfo->GetChild("VRTop");        // StringPool 2576
            auto vrBottomProp = pInfo->GetChild("VRBottom");  // StringPool 2577

            auto nVRLeft = vrLeftProp ? vrLeftProp->GetInt(0) : 0;
            auto nVRRight = vrRightProp ? vrRightProp->GetInt(0) : 0;
            auto nVRTop = vrTopProp ? vrTopProp->GetInt(0) : 0;
            auto nVRBottom = vrBottomProp ? vrBottomProp->GetInt(0) : 0;

            if (nVRLeft && m_rcMBR.left < nVRLeft + 20)
                m_rcMBR.left = nVRLeft + 20;
            if (nVRRight && m_rcMBR.right > nVRRight - 20)
                m_rcMBR.right = nVRRight - 20;
            if (nVRTop && m_rcMBR.top < nVRTop + 65)
                m_rcMBR.top = nVRTop + 65;
            if (nVRBottom && m_rcMBR.bottom > nVRBottom)
                m_rcMBR.bottom = nVRBottom;
        }
    }

    // ========== 3. Find base ZMass ==========
    // First mass index where the range is valid (high >= low)

    m_nBaseZMass = 0;
    for (std::size_t i = 0; i < m_aMassRange.size(); ++i)
    {
        if (m_aMassRange[i].high >= m_aMassRange[i].low)
        {
            m_nBaseZMass = static_cast<std::int32_t>(i);
            break;
        }
    }

    // ========== 4. Link prev/next footholds ==========

    for (auto& pfh : m_lFoothold)
    {
        if (pfh->GetSNPrev())
        {
            auto it = m_mFoothold.find(pfh->GetSNPrev());
            if (it != m_mFoothold.end())
                pfh->SetPrevLink(it->second.get());
        }
        if (pfh->GetSNNext())
        {
            auto it = m_mFoothold.find(pfh->GetSNNext());
            if (it != m_mFoothold.end())
                pfh->SetNextLink(it->second.get());
        }
    }

    // ========== 5. Load Ladders/Ropes ==========
    // 1-based index enumeration

    if (pLadderRope)
    {
        auto nCount = static_cast<std::int32_t>(pLadderRope->GetChildCount());
        m_aLadderOrRope.resize(static_cast<std::size_t>(nCount));

        for (std::int32_t i = 1; i <= nCount; ++i)
        {
            auto pLR = pLadderRope->GetChild(std::to_string(i));
            if (!pLR)
                continue;

            auto idx = static_cast<std::size_t>(i - 1);
            auto& lr = m_aLadderOrRope[idx];

            lr.dwSN = static_cast<std::uint32_t>(i);

            // Read ladder/rope properties (StringPool IDs in comments)
            auto lProp = pLR->GetChild("l");          // 3758 — ladder flag
            auto ufProp = pLR->GetChild("uf");         // 3759 — upper foothold flag
            auto xProp = pLR->GetChild("x");           // 1682
            auto y1Prop = pLR->GetChild("y1");         // 3756
            auto y2Prop = pLR->GetChild("y2");         // 3757
            auto pageProp = pLR->GetChild("page");     // 2859

            lr.bLadder = lProp ? (lProp->GetInt(0) != 0 ? 1 : 0) : 0;
            lr.bUpperFoothold = ufProp ? (ufProp->GetInt(0) != 0 ? 1 : 0) : 0;
            lr.x = xProp ? xProp->GetInt(0) : 0;
            lr.y1 = y1Prop ? y1Prop->GetInt(0) : 0;
            lr.y2 = y2Prop ? y2Prop->GetInt(0) : 0;
            lr.nPage = pageProp ? pageProp->GetInt(0) : 0;
            lr.bOff = 0;

            // CRC32 chain over ladder/rope data
            m_dwCRC = CCrc32::GetCrc32(lr.dwSN, m_dwCRC);
            m_dwCRC = CCrc32::GetCrc32(lr.bLadder, m_dwCRC);
            m_dwCRC = CCrc32::GetCrc32(lr.bUpperFoothold, m_dwCRC);
            m_dwCRC = CCrc32::GetCrc32(lr.x, m_dwCRC);
            m_dwCRC = CCrc32::GetCrc32(lr.y1, m_dwCRC);
            m_dwCRC = CCrc32::GetCrc32(lr.y2, m_dwCRC);
            m_dwCRC = CCrc32::GetCrc32(lr.nPage, m_dwCRC);
        }
    }

    LOG_INFO("WvsPhysicalSpace2D::Load: {} footholds, {} ladders/ropes, {} masses, MBR=({},{})–({},{}) CRC={:#010x}",
             m_lFoothold.size(), m_aLadderOrRope.size(), m_aIndexZMass.size(),
             m_rcMBR.left, m_rcMBR.top, m_rcMBR.right, m_rcMBR.bottom,
             m_dwCRC);
}

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
