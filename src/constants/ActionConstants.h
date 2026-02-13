#pragma once

#include "enums/CharacterAction.h"

#include <cstdint>

namespace ms
{

/// Replaced stand action type constants
enum class ReplacedStandAction : std::int32_t
{
    None = 0,
    Floating = 1,
    Floating2 = 2,
};

/// Get the replaced stand action based on type and stand variant.
[[nodiscard]] CharacterAction get_replaced_stand_action(
    std::int32_t nReplacedStandActionType, std::int32_t nStandType);

/// Get replaced action by pose and weapon type.
[[nodiscard]] CharacterAction get_replaced_action_by_pose(
    std::int32_t nPose, std::int32_t nWeaponType);

} // namespace ms
