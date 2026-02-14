#pragma once

#include <cstdint>

namespace ms
{

/// Weapon type code table (from g_anWeaponType).
/// Index 0 is unused sentinel; indices 1..30 are valid weapon type codes.
inline constexpr std::int32_t g_anWeaponType[31] = {
     0,  // [0]  sentinel
    30,  // [1]  one-handed sword
    31,  // [2]  one-handed axe
    32,  // [3]  one-handed blunt
    33,  // [4]  dagger
    37,  // [5]  wand
    38,  // [6]  staff
    40,  // [7]  two-handed sword
    41,  // [8]  two-handed axe
    42,  // [9]  two-handed blunt
    43,  // [10] spear
    44,  // [11] polearm
    45,  // [12] bow
    46,  // [13] crossbow
    47,  // [14] claw
    48,  // [15] knuckle
    49,  // [16] gun
    39,  // [17] shield (sub)
    34,  // [18] katara
    52,  // [19] dual bowgun
    53,  // [20] cannon
    35,  // [21] cane
    36,  // [22] (secondary)
    21,  // [23] shining rod
    22,  // [24] soul shooter
    23,  // [25] desperado
    24,  // [26] whip blade
    56,  // [27] sword (zero alpha)
    57,  // [28] sword (zero beta)
    26,  // [29] (sub)
    58,  // [30] gauntlet revolver
};

/// Extract weapon type code from an item ID.
/// Returns 0 if not a weapon.
[[nodiscard]] std::int32_t get_weapon_type(std::int32_t nItemID);

/// Check if item is a two-handed weapon (weapon type 40-53, 56, 57, 58).
[[nodiscard]] bool is_two_hand_weapon(std::int32_t nItemID);

// --- Sub-weapon item type helpers ---

[[nodiscard]] bool is_orb(std::int32_t nItemID);
[[nodiscard]] bool is_dragon_soul(std::int32_t nItemID);
[[nodiscard]] bool is_soulring(std::int32_t nItemID);
[[nodiscard]] bool is_magnum(std::int32_t nItemID);

[[nodiscard]] bool is_hero_medal(std::int32_t nItemID);
[[nodiscard]] bool is_paladin_rosario(std::int32_t nItemID);
[[nodiscard]] bool is_darknight_chain(std::int32_t nItemID);
[[nodiscard]] bool is_mage1_book(std::int32_t nItemID);
[[nodiscard]] bool is_mage2_book(std::int32_t nItemID);
[[nodiscard]] bool is_mage3_book(std::int32_t nItemID);
[[nodiscard]] bool is_bowmaster_feather(std::int32_t nItemID);
[[nodiscard]] bool is_crossbow_thimble(std::int32_t nItemID);
[[nodiscard]] bool is_shadower_sheath(std::int32_t nItemID);
[[nodiscard]] bool is_nightlord_pouch(std::int32_t nItemID);
[[nodiscard]] bool is_viper_wristband(std::int32_t nItemID);
[[nodiscard]] bool is_captain_sight(std::int32_t nItemID);
[[nodiscard]] bool is_cannon_gunpowder(std::int32_t nItemID);

[[nodiscard]] bool is_aran_pendulum(std::int32_t nItemID);
[[nodiscard]] bool is_evan_paper(std::int32_t nItemID);
[[nodiscard]] bool is_battlemage_orb(std::int32_t nItemID);
[[nodiscard]] bool is_wildhunter_arrowhead(std::int32_t nItemID);
[[nodiscard]] bool is_cygnus_gem(std::int32_t nItemID);

[[nodiscard]] bool is_zero_sub_weapon_item(std::int32_t nItemID);
[[nodiscard]] bool is_kiness_sub_weapon_item(std::int32_t nItemID);

} // namespace ms
