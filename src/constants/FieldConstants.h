#pragma once

#include <cstdint>

namespace ms
{

/// Check if current field is PvP type.
[[nodiscard]] bool is_pvp_field();

/// Check if current field is Urus boss type.
[[nodiscard]] bool is_field_type_urus();

/// Check if the given skill ID is a dance skill.
[[nodiscard]] bool is_dance_skill(std::int32_t nSkillID);

} // namespace ms
