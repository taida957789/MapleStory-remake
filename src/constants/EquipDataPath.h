#pragma once

#include <cstdint>
#include <string>

namespace ms
{

/// Map item ID to WZ path string (e.g. "Character/Weapon/01302000.img").
/// Returns empty string if the item ID doesn't map to a known equip category.
/// Based on CharacterResourceHelper.GetEquipDataPath from C# reference.
[[nodiscard]] std::string get_equip_data_path(std::int32_t nItemID);

/// Check if item is an accessory (face accessory, eye deco, earrings, etc.)
/// nID/10000 in {101,102,103,112,113,114,115,116,118,119}
[[nodiscard]] bool is_accessory(std::int32_t nItemID);

/// Check if job belongs to the Kaiser class tree (nJob/100 == 61 || nJob == 6000)
[[nodiscard]] bool is_kaiser_job(std::int32_t nJob);

/// Check if job belongs to the Luminous class tree (nJob/100 == 27 || nJob == 2004)
[[nodiscard]] bool is_luminous_job(std::int32_t nJob);

/// Check if job belongs to the Demon Avenger class tree (nJob/100 == 31)
[[nodiscard]] bool is_davenger_job(std::int32_t nJob);

/// Remap BattlePvP action codes to standard action codes.
/// Modifies nAction in-place.
void action_mapping_for_battle_pvp(std::int32_t& nAction);

} // namespace ms
