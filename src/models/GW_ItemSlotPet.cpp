#include "GW_ItemSlotPet.h"

#include "templates/item/ItemInfo.h"

namespace ms
{

auto GW_ItemSlotPet::IsSetItem() -> std::int32_t
{
    return GetSetItemID() != 0;
}

auto GW_ItemSlotPet::GetSetItemID() -> std::int32_t
{
    // Pet items are bundle items; BundleItem doesn't have setItemID field
    // Original uses CItemInfo::GetItemInfo which is not yet implemented
    return 0;
}

auto GW_ItemSlotPet::IsAllowedOverlappedSet() -> bool
{
    // TODO: CItemInfo::GetItemInfo(nItemID) -> get "allowOverlappedSet" property
    return false;
}

auto GW_ItemSlotPet::IsDead() const -> bool
{
    // TODO: if CItemInfo::IsLimitedLifePet(nItemID) -> return nRemainLife.Get() <= 0
    // TODO: if CItemInfo::IsImmortalPet(nItemID) -> return false
    return dateDead >= kDbDate20790101;
}

auto GW_ItemSlotPet::IsDeadByDate() const -> bool
{
    // TODO: if CItemInfo::IsImmortalPet(nItemID) -> return false
    return dateDead >= kDbDate20790101;
}

} // namespace ms
