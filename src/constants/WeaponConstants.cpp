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

bool is_two_hand_weapon(std::int32_t nItemID)
{
    auto nType = get_weapon_type(nItemID);
    return (nType >= 40 && nType <= 53)
        || nType == 56
        || nType == 57
        || nType == 58;
}

// --- Sub-weapon item type helpers ---

bool is_orb(std::int32_t nItemID)
{
    return nItemID / 10000 == 135
        && nItemID - 1350000 >= 2400
        && nItemID - 1350000 < 2500;
}

bool is_dragon_soul(std::int32_t nItemID)
{
    return nItemID / 10000 == 135
        && nItemID - 1350000 >= 2500
        && nItemID - 1350000 < 2600;
}

bool is_soulring(std::int32_t nItemID)
{
    return nItemID / 10000 == 135
        && nItemID - 1350000 >= 2600
        && nItemID - 1350000 < 2700;
}

bool is_magnum(std::int32_t nItemID)
{
    return nItemID / 10000 == 135
        && nItemID - 1350000 >= 2700
        && nItemID - 1350000 < 2800;
}

bool is_hero_medal(std::int32_t nItemID)
{
    return nItemID / 10 == 135220;
}

bool is_paladin_rosario(std::int32_t nItemID)
{
    return nItemID / 10 == 135221;
}

bool is_darknight_chain(std::int32_t nItemID)
{
    return nItemID / 10 == 135222;
}

bool is_mage1_book(std::int32_t nItemID)
{
    return nItemID / 10 == 135223;
}

bool is_mage2_book(std::int32_t nItemID)
{
    return nItemID / 10 == 135224;
}

bool is_mage3_book(std::int32_t nItemID)
{
    return nItemID / 10 == 135225;
}

bool is_bowmaster_feather(std::int32_t nItemID)
{
    return nItemID / 10 == 135226;
}

bool is_crossbow_thimble(std::int32_t nItemID)
{
    return nItemID / 10 == 135227;
}

bool is_shadower_sheath(std::int32_t nItemID)
{
    return nItemID / 10 == 135228;
}

bool is_nightlord_pouch(std::int32_t nItemID)
{
    return nItemID / 10 == 135229;
}

bool is_viper_wristband(std::int32_t nItemID)
{
    return nItemID / 10 == 135290;
}

bool is_captain_sight(std::int32_t nItemID)
{
    return nItemID / 10 == 135291;
}

bool is_cannon_gunpowder(std::int32_t nItemID)
{
    return nItemID / 10 == 135292 || nItemID / 10 == 135298;
}

bool is_aran_pendulum(std::int32_t nItemID)
{
    return nItemID / 10 == 135293;
}

bool is_evan_paper(std::int32_t nItemID)
{
    return nItemID / 10 == 135294;
}

bool is_battlemage_orb(std::int32_t nItemID)
{
    return nItemID / 10 == 135295;
}

bool is_wildhunter_arrowhead(std::int32_t nItemID)
{
    return nItemID / 10 == 135296;
}

bool is_cygnus_gem(std::int32_t nItemID)
{
    return nItemID / 10 == 135297;
}

bool is_zero_sub_weapon_item(std::int32_t nItemID)
{
    return nItemID / 10000 == 156;
}

bool is_kiness_sub_weapon_item(std::int32_t nItemID)
{
    return nItemID / 100 == 13532;
}

} // namespace ms
