#pragma once

#include <cstdint>

namespace ms
{

// --- Explorer Warrior ---
[[nodiscard]] bool is_hero_job(std::int32_t nJob);
[[nodiscard]] bool is_paladin_job(std::int32_t nJob);
[[nodiscard]] bool is_darkknight_job(std::int32_t nJob);

// --- Explorer Mage ---
[[nodiscard]] bool is_mage1_job(std::int32_t nJob);
[[nodiscard]] bool is_mage2_job(std::int32_t nJob);
[[nodiscard]] bool is_mage3_job(std::int32_t nJob);

// --- Explorer Bowman ---
[[nodiscard]] bool is_bowmaster_job(std::int32_t nJob);
[[nodiscard]] bool is_crossbow_job(std::int32_t nJob);

// --- Explorer Thief ---
[[nodiscard]] bool is_nightlord_job(std::int32_t nJob);
[[nodiscard]] bool is_shadower_job(std::int32_t nJob);
[[nodiscard]] bool is_dual_job_born(std::int32_t nJob, std::int16_t nSubJob);

// --- Explorer Pirate ---
[[nodiscard]] bool is_viper_job(std::int32_t nJob);
[[nodiscard]] bool is_captain_job(std::int32_t nJob);
[[nodiscard]] bool is_cannonshooter_job(std::int32_t nJob);

// --- Cygnus Knights ---
[[nodiscard]] bool is_cygnus_job(std::int32_t nJob);

// --- Mihile ---
[[nodiscard]] bool is_michael_job(std::int32_t nJob);

// --- Heroes of Maple ---
[[nodiscard]] bool is_aran_job(std::int32_t nJob);
[[nodiscard]] bool is_evan_job(std::int32_t nJob);
[[nodiscard]] bool is_phantom_job(std::int32_t nJob);
[[nodiscard]] bool is_luminous_job(std::int32_t nJob);

// --- Resistance ---
[[nodiscard]] bool is_dslayer_job_born(std::int32_t nJob);
[[nodiscard]] bool is_bmage_job(std::int32_t nJob);
[[nodiscard]] bool is_wildhunter_job(std::int32_t nJob);
[[nodiscard]] bool is_mechanic_job(std::int32_t nJob);
[[nodiscard]] bool is_res_hybrid_job(std::int32_t nJob);

// --- Nova ---
[[nodiscard]] bool is_kaiser_job(std::int32_t nJob);
[[nodiscard]] bool is_angelic_burster_job(std::int32_t nJob);

// --- Zero ---
[[nodiscard]] bool is_zero_job(std::int32_t nJob);

// --- Kinesis ---
[[nodiscard]] bool is_kinesis_job(std::int32_t nJob);

// --- Beginner ---
[[nodiscard]] bool is_beginner_job(std::int32_t nJob);

} // namespace ms
