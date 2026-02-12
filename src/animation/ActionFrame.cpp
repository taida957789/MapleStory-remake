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

} // namespace ms
