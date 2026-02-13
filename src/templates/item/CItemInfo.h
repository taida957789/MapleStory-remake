#pragma once

#include "ItemInfoTypes.h"

#include "util/Singleton.h"

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ms
{

// Forward declarations
class GW_ItemSlotBase;

/**
 * @brief Central item information manager (singleton)
 *
 * Based on CItemInfo from the original MapleStory client (v1029).
 * Holds all item data loaded from WZ files and provides query methods
 * for every item type: equips, bundles, pets, and cash items.
 *
 * Original: TSingleton<CItemInfo>, constructor @ 0xafad70
 * Original class size: ~0x2E0+ bytes with 60+ member fields
 *
 * Implementation strategy: on-demand. Methods are added as other
 * systems require them. Use MCP IDA tools to decompile specific
 * functions when needed.
 */
class CItemInfo final : public Singleton<CItemInfo>
{
    friend class Singleton<CItemInfo>;

public:
    ~CItemInfo() override = default;

    // ============================================================
    // Methods are added on-demand as other systems need them.
    // Use MCP to decompile from the address table below.
    // ============================================================

    // --- Core Lookup ---
    // GetItemProp(long)const                    @ 0xaae510
    // GetItemInfo(long)                         @ 0xaaede0
    // GetEquipItem(long)                        @ 0xae54c0
    // GetBundleItem(long)                       @ 0xaf9310
    // GetItemSlot(long, int)                    @ 0xae6c00
    // GetItemName(long)                         @ 0xacfb80
    // GetItemDesc(long)                         @ 0xacfe90

    // --- Type Checking ---
    // IsEquipItem(long)                         @ 0x5c0050
    // IsCashItem(long)                          @ 0xaafbe0
    // IsCashItem(GW_ItemSlotBase*)              @ 0x788d20
    // IsQuestItem(long)                         @ 0xab1040
    // IsTradeBlockItem(long)                    @ 0xab09d0

    // --- Equipment ---
    // IsAbleToEquip(...)                        @ 0xaea9e0
    // GetRequiredLEV(long)                      @ 0xab23b0
    // GetSetItemID(long)                        @ 0xae6700
    // CalcEquipItemQuality(ZRef<GW_ItemSlotBase>) @ 0xaed3a0

    // --- Registration (WZ Loading) ---
    // IterateItemInfo(void)                     @ 0xafb5d0
    // RegisterEquipItemInfo(long, ushort const*) @ 0xad9ca0

    // --- Price / Misc ---
    // GetItemPrice(long, long&, double&)        @ 0xaf4db0
    // GetItemCoolTime(long, long&, long&)       @ 0xafa8c0

private:
    CItemInfo() = default;

    // Non-copyable, non-movable (inherited from Singleton)
    CItemInfo(const CItemInfo&) = delete;
    auto operator=(const CItemInfo&) -> CItemInfo& = delete;
    CItemInfo(CItemInfo&&) = delete;
    auto operator=(CItemInfo&&) -> CItemInfo& = delete;

    // ============================================================
    // Member variables — from constructor @ 0xafad70
    // Using std::map / std::shared_ptr as stand-ins for ZMap / ZRef
    // until those containers are implemented.
    // ============================================================

    // --- Item data caches ---
    std::map<std::int32_t, std::shared_ptr<EQUIPITEM>> m_mEquipItem;
    std::map<std::int32_t, std::shared_ptr<BUNDLEITEM>> m_mBundleItem;
    std::map<std::int32_t, std::shared_ptr<GROWTHOPTION>> m_mGrowthOptionItem;

    // --- Item string / map string tables ---
    std::map<std::int32_t, std::map<std::string, std::string>> m_mItemString;
    std::map<std::uint32_t, std::map<std::string, std::string>> m_mMapString;

    // --- Item ID set ---
    std::set<std::int32_t> m_sItemID;

    // --- Set item system ---
    std::map<std::int32_t, std::shared_ptr<SETITEMINFO>> m_mSetItemInfo;
    // std::list<SET_EFFECT> m_lSetItemEffect;
    // std::list<SET_ACTION> m_lSetItemAction;

    // --- Specialized item registries ---
    std::map<std::int32_t, std::shared_ptr<PIECEITEMINFO>> m_mPieceItemInfo;
    std::map<std::int32_t, std::shared_ptr<SETTOWERCHAIR>> m_mSetTowerChairInfo;
    std::map<std::int32_t, std::int32_t> m_mSetTowerChairItemInfo;
    std::map<std::int32_t, std::shared_ptr<PETFOODITEM>> m_mPetFoodItem;
    std::map<std::int32_t, std::shared_ptr<BRIDLEITEM>> m_mBridleItem;
    std::map<std::int32_t, std::shared_ptr<EXTENDEXPIREDATEITEM>> m_mExtendExpireDateItem;
    std::map<std::int32_t, std::shared_ptr<EXPIREDPROTECTINGITEM>> m_mExpiredProtectingItem;
    std::map<std::int32_t, std::shared_ptr<PROTECTONDIEITEM>> m_mProtectOnDieItem;
    std::map<std::int32_t, std::shared_ptr<KARMASCISSORSITEM>> m_mKarmaScissorsItem;
    std::map<std::int32_t, std::shared_ptr<BAGINFO>> m_mBagItem;
    std::map<std::int32_t, std::shared_ptr<GATHERINGTOOLITEM>> m_mGatheringToolItem;
    std::map<std::int32_t, std::shared_ptr<RECIPE_OPEN_ITEM>> m_mRecipeOpenItem;
    std::map<std::int32_t, std::shared_ptr<ITEMPOT_CREATE_ITEM>> m_mItemPotCreateItem;
    std::map<std::int32_t, std::shared_ptr<ITEMPOT_CURE_ITEM>> m_mItemPotCureItem;
    std::map<std::int32_t, std::shared_ptr<DECOMPOSER_INSTALL_ITEM>> m_mDecomposerInstallItem;
    std::map<std::int32_t, std::shared_ptr<EQUIPSLOTLEVELMINUSITEM>> m_mEquipSlotLevelMinusItem;
    std::map<std::int32_t, std::shared_ptr<DYEINGITEM>> m_mDyeingItem;
    std::map<std::int32_t, std::shared_ptr<DRESSUPCLOTHESITEM>> m_mDressUpClothesItem;
    std::map<std::int32_t, std::shared_ptr<DRESSUPCLOTHESITEM>> m_mDressUpClothesItemByClothesID;
    std::map<std::int32_t, std::shared_ptr<COREITEM>> m_mCoreItem;
    std::map<std::int32_t, std::shared_ptr<AREABUFFITEM>> m_mAreaBuffItem;
    std::map<std::int32_t, std::shared_ptr<BITSCASEITEM>> m_mBitsCaseItem;
    std::map<std::int32_t, std::shared_ptr<GACHAPONITEMINFO>> m_mGachaponItemInfo;
    std::map<std::int32_t, std::shared_ptr<COUPLECHAIRITEM>> m_mCoupleChairItem;
    std::map<std::int32_t, std::shared_ptr<GROUPEFFECTINFO>> m_mGroupEffectInfo;

    // --- Misc registries ---
    std::map<std::int32_t, std::uint32_t> m_mItemCRC;
    std::map<std::uint32_t, std::int32_t> m_mPremiumMapTransferBasicMap;
    std::map<std::int32_t, std::int32_t> m_mSkillID_CastItemID;
    std::map<std::int32_t, std::int32_t> m_mItemCosmetic;
    std::map<std::int32_t, std::vector<std::int32_t>> m_mMiracleCubeExAvailableItem;
    std::map<std::int32_t, std::int64_t> m_ConsumeLimitItem;  // FILETIME
    std::map<std::int32_t, std::int32_t> m_mNoScanItem;
    std::map<std::int32_t, std::int32_t> m_mExclusiveEquip;
    std::map<std::int32_t, std::string> m_mExclusiveEquipString;
    std::map<std::int32_t, std::string> m_mExclusiveEquipName;
    std::map<std::int32_t, std::string> m_mExclusiveEquipCategory;

    // --- Sell price by level ---
    std::map<std::int32_t, std::map<std::int32_t, std::int32_t>> m_mItemSellPriceByLv;

    // --- Cash item tags ---
    std::map<std::string, std::vector<std::int32_t>> m_CashItemTag;

    // --- Scanner ---
    // std::list<std::shared_ptr<ITEMNAME>> m_lItemNameForScanner;
    bool m_bItemScannerInfoLoaded{};

    // --- Map string state ---
    bool m_bReleaseMapString{};
};

} // namespace ms
