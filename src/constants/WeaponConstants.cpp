#include "WeaponConstants.h"

#include "templates/item/ItemHelper.h"

namespace ms
{

std::int32_t get_weapon_type(std::int32_t nItemID)
{
    // Only equipment items (1xxxxxxx)
    if (helper::GetItemType(nItemID) != helper::kEquip)
        return 0;

    auto weaponCode = nItemID / 10000 % 100;

    for (std::int32_t i = 1; i <= 30; ++i)
    {
        if (g_anWeaponType[i] == weaponCode)
            return weaponCode;
    }

    return 0;
}

} // namespace ms
