#include "GW_ItemSlotEquip.h"

#include "templates/item/ItemInfo.h"

namespace ms
{

auto GW_ItemSlotEquip::IsSetItem() -> std::int32_t
{
    return GetSetItemID() != 0;
}

auto GW_ItemSlotEquip::GetSetItemID() -> std::int32_t
{
    auto& info = ItemInfo::GetInstance();
    auto* pEquip = info.GetEquipItem(nItemID);
    return pEquip ? pEquip->nSetItemID : 0;
}

} // namespace ms
