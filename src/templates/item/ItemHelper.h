#pragma once

#include <cstdint>

namespace ms
{
namespace helper
{

/// Item type constants based on nItemID / 1000000
inline constexpr std::int32_t kEquip = 1;
inline constexpr std::int32_t kConsume = 2;   // Use items
inline constexpr std::int32_t kInstall = 3;   // Setup items (chairs, etc.)
inline constexpr std::int32_t kEtc = 4;
inline constexpr std::int32_t kCash = 5;

// === Item type helpers ===

[[nodiscard]] auto GetItemType(std::int32_t nItemID) -> std::int32_t;
[[nodiscard]] auto IsEquipItemID(std::int32_t nItemID) -> bool;

// === Cash item ID predicates ===

[[nodiscard]] auto is_script_run_pet_life_item(std::int32_t nItemID) -> bool;
[[nodiscard]] auto is_extend_riding_skill_period_item(std::int32_t nItemID) -> bool;
[[nodiscard]] auto is_equip_req_lev_liberation_item(std::int32_t nItemID) -> bool;

// === Cash slot item type classification ===

[[nodiscard]] auto get_cashslot_item_type(std::int32_t nItemID) -> std::int32_t;
[[nodiscard]] auto get_etc_cash_item_type(std::int32_t nItemID) -> std::int32_t;
[[nodiscard]] auto get_consume_cash_item_type(std::int32_t nItemID) -> std::int32_t;
[[nodiscard]] auto get_bundle_cash_item_type(std::int32_t nItemID) -> std::int32_t;

} // namespace helper
} // namespace ms
