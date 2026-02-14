#pragma once

#include "enums/CharacterAction.h"

#include <cstdint>

namespace ms
{

/// Check if action is a dance action.
/// Dance0..Dance8 (874-882), DanceStarplanet0..5 (940-945),
/// DanceStarplanetEvent0..5 (946-951), SpinoffGuitarStandM..StrokeW (1156-1159).
[[nodiscard]] bool is_dance_action(CharacterAction nAction);

/// Check if action hides the weapon.
/// Includes Sanctuary, Showdown, AirStrike, Blade, Supercannon, HideBody,
/// all dance actions, spinoff guitar range, BattlePvP manji range.
[[nodiscard]] bool is_weapon_hide_action(CharacterAction nAction);

/// Remap ghost action codes to their standard equivalents.
/// GhostWalk→Walk1, GhostStand→Stand1, etc.
void action_mapping_for_ghost(std::int32_t& nAction);

/// Check if item is a gathering tool (prefix 150 or 151).
[[nodiscard]] bool is_gather_tool_item(std::int32_t nItemID);

/// Check if item is a weapon sticker (prefix 17xxxxx).
[[nodiscard]] bool is_weapon_sticker_item(std::int32_t nItemID);

/// Check if item is a Vari Cane weapon (specific item IDs).
[[nodiscard]] bool is_vari_cane_weapon(std::int32_t nItemID);

/// Check if the action is a pronestab variant (Pronestab, PronestabJaguar).
[[nodiscard]] bool is_pronestab_action(std::int32_t nAction);

/// Check if the vehicle ID represents a valid vehicle (taming mob).
[[nodiscard]] bool is_vehicle(std::int32_t nVehicleID);

/// Check if the action is a walk-type action (Walk1, Walk2, etc.).
[[nodiscard]] bool is_walk_action(std::int32_t nAction);

/// Check if the action is a shooting action (Shoot1, Shoot2, etc.).
[[nodiscard]] bool is_shoot_action(std::int32_t nAction);

/// Check if the action is a BattlePvP dead action.
[[nodiscard]] bool is_battle_pvp_dead_action(std::int32_t nAction);

/// Check if the action is a BattlePvP basic attack action.
[[nodiscard]] bool is_battle_pvp_basic_attack_action(std::int32_t nAction);

/// Check if the action is a BattlePvP rope action.
[[nodiscard]] bool is_battle_pvp_rope_action(std::int32_t nAction);

/// Check if the action is a BattlePvP walk action.
[[nodiscard]] bool is_battle_pvp_walk_action(std::int32_t nAction);

/// Check if the action is a BattlePvP stand action.
[[nodiscard]] bool is_battle_pvp_stand_action(std::int32_t nAction);

/// Check if the action is any stand-type action (Stand1, Stand2, ride stand, ghost stand, PvP stand).
[[nodiscard]] bool is_stand_action(std::int32_t nAction);

/// Check if a one-time action can be performed on a taming mob.
[[nodiscard]] bool IsAbleTamingMobOneTimeAction(
    CharacterAction nAction, std::int32_t nVehicleID);

/// Remap a standard action to a Pinkbean action (jobs 13000/13100).
[[nodiscard]] std::int32_t get_change_pinkbean_action(std::int32_t nAction);

/// Check if the avatar is holding an action (keydown held on specific frame).
[[nodiscard]] bool IsActionHold(std::int32_t nAction, std::int32_t nFrame);

/// Check if the action is a backward-movement action (walks, climbs, etc.)
/// that should freeze frame when position hasn't changed.
[[nodiscard]] bool is_back_action(std::int32_t nAction, std::int32_t nVehicleID);

/// Check if the job is a Pinkbean job (13000 or 13100).
[[nodiscard]] bool is_pinkbean_job(std::int32_t nJob);

/// Check if the action is a shoot morph action (18 or 19).
[[nodiscard]] bool is_shoot_morph_action(std::int32_t nAction);

/// Check if the action is a hat-dance action (Dance1, Dance3, DanceStarplanet0-2).
[[nodiscard]] bool is_hatdance_action(std::int32_t nAction);

/// Check if the action is a hide-body action (HideBody, SpinoffGuitar range).
[[nodiscard]] bool is_hide_body_action(std::int32_t nAction);

/// Check if the action is a non-pieced BattlePvP action.
[[nodiscard]] bool is_battle_pvp_not_pieced_action(std::int32_t nAction);

/// Check if the action should not use pieced mode (ghosts, making skills,
/// dances, hide body, BattlePvP non-pieced, etc.).
[[nodiscard]] bool is_not_pieced_action(std::int32_t nAction);

/// Check if item is a long coat (category 105).
[[nodiscard]] bool is_long_coat(std::int32_t nItemID);

} // namespace ms
