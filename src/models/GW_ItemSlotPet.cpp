#include "GW_ItemSlotPet.h"

#include "templates/item/ItemInfo.h"
#include "wz/WzProperty.h"

namespace ms
{

auto GW_ItemSlotPet::IsSetItem() -> std::int32_t
{
    return GetSetItemID() != 0;
}

auto GW_ItemSlotPet::GetSetItemID() -> std::int32_t
{
    auto& info = ItemInfo::GetInstance();
    auto pInfo = info.GetItemInfo(static_cast<std::int32_t>(nItemID));
    if (!pInfo)
        return 0;
    auto pChild = pInfo->GetChild("setItemID");
    return pChild ? pChild->GetInt(0) : 0;
}

auto GW_ItemSlotPet::IsAllowedOverlappedSet() -> bool
{
    auto& info = ItemInfo::GetInstance();
    auto pInfo = info.GetItemInfo(static_cast<std::int32_t>(nItemID));
    if (!pInfo)
        return false;
    auto pChild = pInfo->GetChild("allowOverlappedSet");
    return pChild ? pChild->GetInt(0) != 0 : false;
}

auto GW_ItemSlotPet::IsDead() const -> bool
{
    auto& info = ItemInfo::GetInstance();
    auto pInfo = info.GetItemInfo(static_cast<std::int32_t>(nItemID));
    if (pInfo)
    {
        auto pLimited = pInfo->GetChild("limitedLife");
        if (pLimited && pLimited->GetInt(0))
            return nRemainLife.Get() <= 0;
        auto pImmortal = pInfo->GetChild("permanent");
        if (pImmortal && pImmortal->GetInt(0))
            return false;
    }
    return dateDead >= kDbDate20790101;
}

auto GW_ItemSlotPet::IsDeadByDate() const -> bool
{
    auto& info = ItemInfo::GetInstance();
    auto pInfo = info.GetItemInfo(static_cast<std::int32_t>(nItemID));
    if (pInfo)
    {
        auto pImmortal = pInfo->GetChild("permanent");
        if (pImmortal && pImmortal->GetInt(0))
            return false;
    }
    return dateDead >= kDbDate20790101;
}

} // namespace ms
