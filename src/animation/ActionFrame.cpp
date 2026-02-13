#include "ActionFrame.h"

#include "SpriteInstance.h"
#include "enums/CharacterAction.h"
#include "user/UserLocal.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

#include <algorithm>
#include <unordered_map>

namespace ms
{

// === Static member definitions ===

const std::string ActionFrame::s_sZMapImg = "Base/zmap.img";
const std::string ActionFrame::s_sSMapImg = "Base/smap.img";

std::shared_ptr<WzProperty> ActionFrame::s_pZMapper;
std::shared_ptr<WzProperty> ActionFrame::s_pSMapper;

std::int32_t ActionFrame::s_nFaceZ = 0;
std::int32_t ActionFrame::s_nCharacterStartZ = 0;
std::int32_t ActionFrame::s_nCharacterEndZ = 0;

// === Default constructor (from decompiled CActionFrame::CActionFrame) ===
// Original clears static mappers on construction; we only reset instance state.
ActionFrame::ActionFrame()
{
    // Note: original decompiled constructor releases s_pSMapper and s_pZMapper.
    // This appears to be a one-time global init pattern. We preserve the static
    // mappers across instances — call LoadMappers() once at startup instead.
}

// === LoadMappers (from decompiled CActionFrame::LoadMappers) ===
void ActionFrame::LoadMappers()
{
    auto& resMan = WzResMan::GetInstance();

    // 1. Load zMap (z-order mapper) from Base/zmap.img
    auto pZMapper = resMan.GetProperty(s_sZMapImg);
    if (!pZMapper)
        return;

    // 2. Enumerate zMap entries and assign z-order values.
    //    Entries with explicit integer values set the counter;
    //    entries without values get auto-decremented z-orders.
    //
    //    NOTE: Original iterates in WZ file insertion order via IEnumVARIANT.
    //    Our std::map iterates alphabetically. This is correct when all entries
    //    have explicit values (common case). If entries rely on file order for
    //    auto-assignment, the property container would need insertion-order iteration.
    std::int32_t zCounter = 0;
    for (auto& [name, child] : pZMapper->GetChildren())
    {
        // Try to read as integer; use sentinel to detect missing values
        constexpr std::int32_t kNoValue = std::numeric_limits<std::int32_t>::min();
        auto val = child->GetInt(kNoValue);

        if (val != kNoValue)
        {
            // Entry has an explicit value — update counter
            zCounter = val;
        }
        else
        {
            // No value — auto-assign decremented z-order
            --zCounter;
            child->SetInt(zCounter);
        }
    }

    s_pZMapper = pZMapper;

    // 3. Read specific z-values (from StringPool IDs: face=11046,
    //    characterStart=5573, characterEnd=5574)
    if (auto prop = pZMapper->GetChild("face"))
        s_nFaceZ = prop->GetInt(0);
    if (auto prop = pZMapper->GetChild("characterStart"))
        s_nCharacterStartZ = prop->GetInt(0);
    if (auto prop = pZMapper->GetChild("characterEnd"))
        s_nCharacterEndZ = prop->GetInt(0);

    // 4. Load sMap (slot mapper) from Base/smap.img
    s_pSMapper = resMan.GetProperty(s_sSMapImg);
}

// === UpdateMBR (from decompiled CActionFrame::UpdateMBR) ===
// 遍歷 m_lpSprites，用每個 sprite 的位置 + source 尺寸算出矩形，
// 再用 UnionRect 合併到 m_rcMBR。
void ActionFrame::UpdateMBR()
{
    if (m_bMBRValid)
        return;

    m_bMBRValid = true;
    m_rcMBR = {0, 0, 0, 0};

    for (const auto& sprite : m_lpSprites)
    {
        if (!sprite || !sprite->m_pSource)
            continue;

        const auto& src = sprite->m_pSource;
        Rect rc{
            sprite->m_pt.x,
            sprite->m_pt.y,
            sprite->m_pt.x + src->m_cx,
            sprite->m_pt.y + src->m_cy
        };

        if (rc.IsEmpty())
            continue;

        // UnionRect: 擴展 m_rcMBR 以包含 rc
        if (m_rcMBR.IsEmpty())
        {
            m_rcMBR = rc;
        }
        else
        {
            m_rcMBR.left = std::min(m_rcMBR.left, rc.left);
            m_rcMBR.top = std::min(m_rcMBR.top, rc.top);
            m_rcMBR.right = std::max(m_rcMBR.right, rc.right);
            m_rcMBR.bottom = std::max(m_rcMBR.bottom, rc.bottom);
        }
    }
}

// === ExtractMap (from decompiled CActionFrame::ExtractMap) ===
// Reads the "map" sub-property of a sprite frame property.
// Each child is a named attachment point (e.g., "navel", "neck", "hand")
// with a vector value. Original uses IWzShape2D bounds center;
// for vector entries (common case) center == the vector itself.
auto ActionFrame::ExtractMap(
    const std::shared_ptr<WzGr2DCanvas>& pRawSprite,
    const std::shared_ptr<WzProperty>& pProperty
) -> std::shared_ptr<std::vector<MapInfo>>
{
    auto pList = std::make_shared<std::vector<MapInfo>>();

    if (!pRawSprite)
        return pList;

    // Get the property to examine.
    // Original: if pProperty is null, uses IWzCanvas::Getproperty(pRawSprite).
    // In our code, the canvas doesn't store a property back-reference,
    // so the caller must provide pProperty.
    if (!pProperty)
        return pList;

    // Look up "map" child (original: s_sMap static string)
    auto mapProp = pProperty->GetChild("map");
    if (!mapProp)
        return pList;

    // Enumerate map entries
    for (const auto& [name, child] : mapProp->GetChildren())
    {
        // Original QIs for IWzShape2D and reads bounds:
        //   pt.x = (left + right) / 2
        //   pt.y = (top + bottom) / 2
        // For vector entries (the common case), bounds == point itself.
        auto vec = child->GetVector();

        MapInfo info;
        info.sName = name;
        info.pt = {vec.x, vec.y};
        pList->push_back(std::move(info));
    }

    return pList;
}

// === FindGroup (from decompiled CActionFrame::FindGroup) ===
// Searches m_lGroups for a group (not pMIL itself) that shares any
// MapInfo::sName with pMIL. Returns the first matching group, or nullptr.
auto ActionFrame::FindGroup(const std::shared_ptr<std::vector<MapInfo>>& pMIL)
    -> std::shared_ptr<std::vector<MapInfo>>
{
    for (auto& group : m_lGroups)
    {
        if (group.get() == pMIL.get())
            continue; // skip self
        if (!group || group->empty())
            continue;

        for (const auto& groupEntry : *group)
        {
            for (const auto& targetEntry : *pMIL)
            {
                if (groupEntry.sName == targetEntry.sName)
                    return group;
            }
        }
    }
    return nullptr;
}

// === MergeGroup (from decompiled CActionFrame::MergeGroup) ===
// Merges pSrc group into pDst group:
// 1. Find common attachment points (by name), accumulate position sums.
// 2. Non-common entries go into a temp list.
// 3. Compute average offset = avgDst - avgSrc.
// 4. Copy non-common entries to pDst, shifted by offset.
// 5. Shift all sprites that belong to pSrc group, repoint them to pDst.
// 6. Remove pSrc from m_lGroups.
void ActionFrame::MergeGroup(
    const std::shared_ptr<std::vector<MapInfo>>& pDst,
    const std::shared_ptr<std::vector<MapInfo>>& pSrc
)
{
    std::int32_t nCommon = 0;
    Point2D ptDstSum{0, 0};
    Point2D ptSrcSum{0, 0};
    std::vector<MapInfo> milTemp;

    // Phase 1: classify src entries as common or non-common
    for (const auto& srcEntry : *pSrc)
    {
        bool found = false;
        for (const auto& dstEntry : *pDst)
        {
            if (srcEntry.sName == dstEntry.sName)
            {
                ptDstSum.x += dstEntry.pt.x;
                ptDstSum.y += dstEntry.pt.y;
                ptSrcSum.x += srcEntry.pt.x;
                ptSrcSum.y += srcEntry.pt.y;
                ++nCommon;
                found = true;
                break;
            }
        }

        if (!found)
        {
            milTemp.push_back(srcEntry);
        }
    }

    // Phase 2: compute average offset between common points
    // Original divides unconditionally — nCommon should always be > 0
    // (MergeGroup is only called when FindGroup found a match).
    auto szDiffX = ptDstSum.x / nCommon - ptSrcSum.x / nCommon;
    auto szDiffY = ptDstSum.y / nCommon - ptSrcSum.y / nCommon;

    // Phase 3: append non-common entries to dst, shifted by offset
    for (auto& entry : milTemp)
    {
        entry.pt.x += szDiffX;
        entry.pt.y += szDiffY;
        pDst->push_back(std::move(entry));
    }

    // Phase 4: update sprite instances that reference pSrc
    for (auto& sprite : m_lpSprites)
    {
        if (sprite && sprite->m_pGroup == pSrc.get())
        {
            sprite->m_pt.x += szDiffX;
            sprite->m_pt.y += szDiffY;
            sprite->m_pGroup = pDst.get();
        }
    }

    // Phase 5: remove pSrc from m_lGroups
    auto it = std::find(m_lGroups.begin(), m_lGroups.end(), pSrc);
    if (it != m_lGroups.end())
        m_lGroups.erase(it);
}

// === Merge (from decompiled CActionFrame::Merge) ===
// 將一個 sprite layer（canvas + 裝備屬性）加入此 frame：
// 1. 建立 SpriteInstance，透過 SpriteSource::Init 初始化
// 2. 設定位置為原點的反向（-center）
// 3. 跳過 face z-order 的 sprite（face 另外處理）
// 4. 第一個非 face sprite 設為 body
// 5. 從 canvas property 提取 attachment map
// 6. 按 z-order 排序插入 m_lpSprites
// 7. 將 attachment map 加入 m_lGroups，反覆合併重疊的 group
void ActionFrame::Merge(
    const std::string& sISlot,
    const std::string& sVSlot,
    const std::shared_ptr<WzGr2DCanvas>& pRawSprite,
    std::int32_t nJob,
    const std::shared_ptr<WzProperty>& pSpriteProp
)
{
    // 1. Create sprite instance and initialize source
    auto pInstance = std::make_shared<SpriteInstance>();
    pInstance->m_pSource = std::make_shared<SpriteSource>();
    pInstance->m_pSource->Init(sISlot, sVSlot, pRawSprite, nJob, pSpriteProp);

    // 2. Position = negated center (origin offset)
    pInstance->m_pt.x = -pInstance->m_pSource->m_ptCenter.x;
    pInstance->m_pt.y = -pInstance->m_pSource->m_ptCenter.y;

    // 3. Get z-order; skip face-layer sprites (handled separately)
    auto z = pInstance->m_pSource->m_z;
    if (z == s_nFaceZ)
        return;

    // 4. First non-face sprite becomes the body sprite
    if (m_bBody)
    {
        m_pSpriteBody = pInstance;
        m_bBody = false;
    }

    // 5. Extract attachment point map from canvas property
    auto pMIL = ExtractMap(pRawSprite, pSpriteProp);
    pInstance->m_pGroup = pMIL.get();

    // 6. Insert into sprite list sorted by z-order (ascending)
    m_bMBRValid = false;
    auto insertPos = m_lpSprites.end();
    for (auto it = m_lpSprites.begin(); it != m_lpSprites.end(); ++it)
    {
        if ((*it)->m_pSource->m_z > z)
        {
            insertPos = it;
            break;
        }
    }
    m_lpSprites.insert(insertPos, pInstance);

    // 7. Add group to list, then repeatedly merge overlapping groups
    m_lGroups.push_back(pMIL);

    auto pCurrent = pMIL;
    for (auto pFound = FindGroup(pCurrent); pFound; pFound = FindGroup(pCurrent))
    {
        // Original swaps merge direction when pCurrent is at head of m_lGroups.
        // This ensures the first-added group survives as the merge destination.
        if (!m_lGroups.empty() && m_lGroups.front() == pCurrent)
            std::swap(pFound, pCurrent);

        MergeGroup(pFound, pCurrent);
        pCurrent = pFound;
    }
}

// === UpdateVisibility (from decompiled CActionFrame::UpdateVisiblity) ===
// 根據 VSlot (Visual Slot) 解析裝備可見性衝突。
// 先將 m_sExclVSlot 的 2 字元 slot code 放入 map（value=nullptr → 強制隱藏），
// 再遍歷所有 sprite，對每個 sprite 的 VSlot 字元做 ISlot 優先級比較：
//   - 已有 sprite 且 ISlot 較高 → 隱藏當前 sprite
//   - 當前 sprite 的 ISlot 較高 → 隱藏已有 sprite，當前取代之
//   - 優先級相同 → 兩者都保持可見
//   - VSlot 在 exclusive 列表中（value=nullptr）→ 強制隱藏當前 sprite
void ActionFrame::UpdateVisibility()
{
    // 1. Build VSlot→SpriteInstance map, seeded with exclusive VSlot entries.
    //    Original: ZMap<unsigned long, CSpriteInstance*, unsigned long> with
    //    2-char wide-string pairs encoded as uint32 keys.
    //    In our narrow-char code: key = char[1] | (char[0] << 16).
    std::unordered_map<std::uint32_t, SpriteInstance*> vslotMap;

    // Parse m_sExclVSlot: each 2-char pair → uint32 key, value = nullptr (hide sentinel)
    for (std::size_t i = 0; i + 1 < m_sExclVSlot.size(); i += 2)
    {
        auto c0 = static_cast<std::uint32_t>(static_cast<unsigned char>(m_sExclVSlot[i]));
        auto c1 = static_cast<std::uint32_t>(static_cast<unsigned char>(m_sExclVSlot[i + 1]));
        vslotMap[c1 | (c0 << 16)] = nullptr;
    }

    // 2. Iterate sprites and resolve VSlot visibility conflicts
    for (auto& sprite : m_lpSprites)
    {
        if (!sprite || !sprite->m_pSource)
            continue;

        sprite->m_bVisible = true;

        // Original: queries CUserLocal singleton's avatar for current action.
        auto& userLocal = UserLocal::GetInstance();
        auto& avatar = userLocal.GetAvatar();
        (void)avatar.MoveAction2RawAction(userLocal.GetMoveAction(), nullptr, false);
        if (avatar.GetOneTimeAction() > static_cast<CharacterAction>(-1))
            (void)avatar.GetOneTimeAction();

        const auto& vslot = sprite->m_pSource->m_sVSlot;
        bool hidden = false;

        // Parse VSlot string: 2-char pairs, same encoding as above
        for (std::size_t i = 0; !hidden && i + 1 < vslot.size(); i += 2)
        {
            auto c0 = static_cast<std::uint32_t>(static_cast<unsigned char>(vslot[i]));
            auto c1 = static_cast<std::uint32_t>(static_cast<unsigned char>(vslot[i + 1]));
            std::uint32_t key = c1 | (c0 << 16);

            auto it = vslotMap.find(key);
            if (it == vslotMap.end())
            {
                // No entry for this VSlot — register current sprite
                vslotMap[key] = sprite.get();
                continue;
            }

            auto* existing = it->second;
            if (!existing)
            {
                // Exclusive VSlot (seeded from m_sExclVSlot) — hide current sprite
                sprite->m_bVisible = false;
                hidden = true;
                continue;
            }

            // Compare ISlot priorities (higher ISlot value = higher priority)
            auto diff = existing->m_pSource->m_nISlot - sprite->m_pSource->m_nISlot;

            if (diff > 0)
            {
                // Existing has higher priority — hide current sprite
                sprite->m_bVisible = false;
                hidden = true;
            }
            else if (diff < 0)
            {
                // Current has higher priority — hide existing, replace in map
                existing->m_bVisible = false;
                it->second = sprite.get();
            }
            // diff == 0: same priority — both stay visible, continue to next VSlot pair
        }
    }
}

} // namespace ms
