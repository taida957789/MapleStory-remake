#pragma once

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

// ============================================================
// Forward declarations for CItemInfo inner structs.
// These are expanded on-demand as methods that access their
// fields are implemented.
// ============================================================

/// Equipment item data (original size: 0x2F8 = 760 bytes, 166 fields)
/// Address: CItemInfo::EQUIPITEM::EQUIPITEM @ 0xaa4b80
struct EQUIPITEM;

/// Bundle (consumable / setup / etc) item data (original size: 0x180 = 384 bytes, 70 fields)
/// Address: CItemInfo::GetBundleItemInfoData @ 0xaf57a0
struct BUNDLEITEM;

/// Set item info (original size: 0x130 = 304 bytes, 13 fields)
/// Address: CItemInfo::RegisterSetItemInfo @ 0xaf1540
struct SETITEMINFO;

/// Equipment stat requirement for IsAbleToEquip checks (original size: 0x2C = 44 bytes)
/// Address: CItemInfo::IsAbleToEquipStat @ 0xaea8d0
struct paramEquipStat;

/// Growth option for level-up equipment (original size: 0x14 = 20 bytes)
/// Address: CItemInfo::GROWTHOPTION
struct GROWTHOPTION;

/// Set item effect entry
struct SET_EFFECT;

/// Set item action entry
struct SET_ACTION;

/// Piece item info
struct PIECEITEMINFO;

/// Tower chair set info
struct SETTOWERCHAIR;

/// Pet food item info
struct PETFOODITEM;

/// Bridle item info
struct BRIDLEITEM;

/// Extend expire date item info
struct EXTENDEXPIREDATEITEM;

/// Expired protecting item info
struct EXPIREDPROTECTINGITEM;

/// Protect on die item info
struct PROTECTONDIEITEM;

/// Karma scissors item info
struct KARMASCISSORSITEM;

/// Bag item info
struct BAGINFO;

/// Gathering tool item info
struct GATHERINGTOOLITEM;

/// Recipe open item info
struct RECIPE_OPEN_ITEM;

/// Item pot create item info
struct ITEMPOT_CREATE_ITEM;

/// Item pot cure item info
struct ITEMPOT_CURE_ITEM;

/// Decomposer install item info
struct DECOMPOSER_INSTALL_ITEM;

/// Equip slot level minus item info
struct EQUIPSLOTLEVELMINUSITEM;

/// Dyeing item info
struct DYEINGITEM;

/// Dress-up clothes item info
struct DRESSUPCLOTHESITEM;

/// Core item info
struct COREITEM;

/// Area buff item info
struct AREABUFFITEM;

/// Bits case item info
struct BITSCASEITEM;

/// Gachapon item info
struct GACHAPONITEMINFO;

/// Gachapon aggregation scope
struct GACHAPONAGGSCOPE;

/// Couple chair item info
struct COUPLECHAIRITEM;

/// Group effect info
struct GROUPEFFECTINFO;

/// Item name entry (for scanner)
struct ITEMNAME;

} // namespace ms
