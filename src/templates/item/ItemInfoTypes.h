#pragma once

// Item type classification
#include <cstdint>

namespace ms
{

/// Item type classification based on nItemID / 1000000
/// Original: used throughout CItemInfo for item category dispatch
namespace ItemType
{
    inline constexpr std::int32_t kEquip = 1;
    inline constexpr std::int32_t kConsume = 2;   // Use items
    inline constexpr std::int32_t kInstall = 3;   // Setup items (chairs, etc.)
    inline constexpr std::int32_t kEtc = 4;
    inline constexpr std::int32_t kCash = 5;

    /// Get item type from item ID (nItemID / 1000000)
    [[nodiscard]] inline constexpr auto GetItemType(std::int32_t nItemID) noexcept -> std::int32_t
    {
        return nItemID / 1000000;
    }

    /// Check if item ID belongs to equip category
    [[nodiscard]] inline constexpr auto IsEquipItemID(std::int32_t nItemID) noexcept -> bool
    {
        return GetItemType(nItemID) == kEquip;
    }
} // namespace ItemType

} // namespace ms
