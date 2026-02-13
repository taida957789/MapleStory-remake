#include "GW_ItemSlotBase.h"

#include "GW_ItemSlotBundle.h"
#include "GW_ItemSlotEquip.h"
#include "GW_ItemSlotPet.h"

namespace ms
{

auto GW_ItemSlotBase::CreateItem(std::int32_t nType) -> std::shared_ptr<GW_ItemSlotBase>
{
    switch (nType)
    {
    case GW_ItemSlotEquip_TYPE:
        return std::make_shared<GW_ItemSlotEquip>();
    case GW_ItemSlotBundle_TYPE:
        return std::make_shared<GW_ItemSlotBundle>();
    case GW_ItemSlotPet_TYPE:
        return std::make_shared<GW_ItemSlotPet>();
    default:
        return nullptr;
    }
}

} // namespace ms
