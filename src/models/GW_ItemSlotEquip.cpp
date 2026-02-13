#include "GW_ItemSlotEquip.h"

namespace ms
{

auto GW_ItemSlotEquip::IsSetItem() -> std::int32_t
{
    return GetSetItemID() != 0;
}

auto GW_ItemSlotEquip::GetSetItemID() -> std::int32_t
{
    // TODO: CItemInfo::GetEquipItem(nItemID) -> get nSetItemID
    return 0;
}

} // namespace ms
