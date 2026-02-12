#include "ActionFrame.h"

#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

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

} // namespace ms
