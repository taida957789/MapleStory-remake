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

} // namespace ms
