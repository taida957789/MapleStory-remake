#include "SpriteSource.h"

#include "ActionFrame.h"
#include "graphics/WzGr2DCanvas.h"
#include "wz/WzProperty.h"

#include <limits>

namespace ms
{

// === CSpriteSource::Init ===
// 初始化 sprite source：
// 1. 複製 slot 字串（sISlot → m_sISlot, sVSlot → m_sBaseVSlot）
// 2. 設定 canvas（pRawSprite → m_pSprite）
// 3. 從 zmap 查詢 sISlot 的 2 字元 code，取最大值作為 m_nISlot
// 4. 讀取 canvas 尺寸（m_cx, m_cy）和中心點（m_ptCenter）
// 5. 透過 QueryZ 取得 z-order 並設定 m_sVSlot
void SpriteSource::Init(
    const std::string& sISlot,
    const std::string& sVSlot,
    const std::shared_ptr<WzGr2DCanvas>& pRawSprite,
    std::int32_t nJob,
    const std::shared_ptr<WzProperty>& pSpriteProp
)
{
    // 1. Copy slot strings
    //    Note: Init's 2nd param is sBaseVSlot in original; Merge passes sVSlot here
    m_sISlot = sISlot;
    m_sBaseVSlot = sVSlot;

    // 2. Set canvas
    m_pSprite = pRawSprite;

    // 3. Compute m_nISlot from zmap lookup
    //    Iterate sISlot's 2-char pairs, look up each in zmap, keep max value.
    //    Original initializes to -2147483647 (INT_MIN + 1).
    auto zMapper = ActionFrame::GetZMapper();
    m_nISlot = std::numeric_limits<std::int32_t>::min() + 1;

    if (zMapper)
    {
        for (std::size_t i = 0; i + 1 < sISlot.size(); i += 2)
        {
            std::string key = sISlot.substr(i, 2);
            if (auto child = zMapper->GetChild(key))
            {
                auto val = child->GetInt(0);
                if (val > m_nISlot)
                    m_nISlot = val;
            }
        }
    }

    // 4. Clear m_sVSlot (will be set by QueryZ)
    m_sVSlot.clear();

    // 5. Read dimensions from canvas
    if (pRawSprite)
    {
        m_cx = pRawSprite->GetWidth();
        m_cy = pRawSprite->GetHeight();
        m_ptCenter = pRawSprite->GetOrigin();
    }

    // 6. Get sprite property (if null, would use canvas back-reference;
    //    we require caller to provide it)
    const auto& prop = pSpriteProp;

    // 7. Resolve z-order via QueryZ
    //    Special case: "Ae" (accessoryEar) slot for jobs 23xx or 2002
    //    checks "z" property against "backAccessoryEar" and uses different VSlot.
    bool isAccessoryEarJob = (m_sBaseVSlot == "Ae") && (nJob / 100 == 23 || nJob == 2002);

    if (isAccessoryEarJob && prop)
    {
        // Read "z" property as string to check against "backAccessoryEar"
        auto zStr = prop->GetChild("z") ? prop->GetChild("z")->GetString("") : std::string{};
        if (zStr == "backAccessoryEar")
        {
            // Use base VSlot as-is
            m_z = QueryZ(prop, m_sBaseVSlot, m_sVSlot, "");
        }
        else
        {
            // Override z with "accessoryEarOverHair"
            m_z = QueryZ(prop, m_sBaseVSlot, m_sVSlot, "accessoryEarOverHair");
        }
    }
    else
    {
        // General case
        m_z = QueryZ(prop, m_sBaseVSlot, m_sVSlot, "");
    }
}

// === CommonSlot ===
// 取 sBase 和 sSmap 兩個 VSlot 字串的交集（以 2 字元 code 為單位）。
// 例如 CommonSlot("BdAf", "AfFcMa") = "Af"
static auto CommonSlot(
    const std::string& sBase,
    const std::string& sSmap
) -> std::string
{
    std::string result;
    for (std::size_t i = 0; i + 1 < sBase.size(); i += 2)
    {
        for (std::size_t j = 0; j + 1 < sSmap.size(); j += 2)
        {
            if (sBase[i] == sSmap[j] && sBase[i + 1] == sSmap[j + 1])
            {
                result += sBase[i];
                result += sBase[i + 1];
                break;
            }
        }
    }
    return result;
}

// === QueryZ (from decompiled CSpriteSource::QueryZ @ 0xfbd4d0) ===
// 從 sprite property 的 "z" 屬性解析 z-order：
//   1. 若提供 sModifiedZ，直接用它作為 z 值字串
//   2. 否則讀取 pProp["z"]
//   3. 嘗試轉為整數 → 直接 z-order
//   4. 嘗試轉為字串 → 查 zmap.img 得到 z-order
//   5. 查 smap.img 得到 slot mapping，再和 sBaseVSlot 做 CommonSlot → outVSlot
auto SpriteSource::QueryZ(
    const std::shared_ptr<WzProperty>& pProp,
    const std::string& sBaseVSlot,
    std::string& outVSlot,
    const std::string& sModifiedZ
) -> std::int32_t
{
    if (!pProp)
        return 0;

    auto zMapper = ActionFrame::GetZMapper();
    std::int32_t z = 0;

    // 1. Determine the z value source
    std::string zStr;
    bool hasZ = false;

    if (!sModifiedZ.empty())
    {
        // Caller provided a modified z string (e.g., "accessoryEarOverHair")
        zStr = sModifiedZ;
        hasZ = true;
    }
    else
    {
        // Read "z" property from sprite property
        if (auto zChild = pProp->GetChild("z"))
        {
            zStr = zChild->GetString("");
            z = zChild->GetInt(static_cast<std::int32_t>(0x80000000));
            hasZ = true;
        }
    }

    if (!hasZ)
        return z;

    // 2. If z string is available, look up in zmap for numeric z-order
    if (!zStr.empty() && zMapper)
    {
        if (auto zmapChild = zMapper->GetChild(zStr))
            z = zmapChild->GetInt(z);

        // 3. Look up in smap to resolve VSlot
        if (!sBaseVSlot.empty())
        {
            auto sMapper = ActionFrame::GetSMapper();
            if (sMapper)
            {
                if (auto smapChild = sMapper->GetChild(zStr))
                {
                    auto smapSlot = smapChild->GetString("");
                    if (!smapSlot.empty())
                        outVSlot = CommonSlot(sBaseVSlot, smapSlot);
                }
            }
        }
    }

    return z;
}

// === Simplified QueryZ for face-look loading ===
// Reads "z" from a canvas property node and resolves to numeric z-index.
// No slot resolution — used by LoadFaceLook where only z-order matters.
auto SpriteSource::QueryZ(const std::shared_ptr<WzProperty>& pCanvasProp)
    -> std::int32_t
{
    if (!pCanvasProp)
        return 0;

    auto pZ = pCanvasProp->GetChild("z");
    if (!pZ)
        return 0;

    // Direct integer value
    auto nodeType = pZ->GetNodeType();
    if (nodeType == WzNodeType::Int || nodeType == WzNodeType::UnsignedShort)
        return pZ->GetInt(0);

    // String z-value → lookup in ZMapper (Base/zmap.img)
    auto sZ = pZ->GetString("");
    if (sZ.empty())
        return 0;

    auto& pZMapper = ActionFrame::GetZMapper();
    if (!pZMapper)
        return 0;

    // ZMapper children are ordered by z-index; find matching entry index
    auto& children = pZMapper->GetChildren();
    std::int32_t idx = 0;
    for (auto& [name, child] : children)
    {
        if (name == sZ)
            return idx;
        ++idx;
    }

    return 0;
}

} // namespace ms
