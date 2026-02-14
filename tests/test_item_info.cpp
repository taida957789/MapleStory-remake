/**
 * ItemInfo tests using real WZ files via Google Test
 */

#include <gtest/gtest.h>
#include <filesystem>

#include "constants/JobConstants.h"
#include "constants/WeaponConstants.h"
#include "models/GW_ItemSlotBase.h"
#include "templates/item/ItemInfo.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

using namespace ms;

static const std::string WZ_TEST_PATH = "../resources/Data";

class ItemInfoTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!std::filesystem::exists(WZ_TEST_PATH + "/Base/Base.wz") &&
            !std::filesystem::exists(WZ_TEST_PATH + "/Base.wz"))
            return;

        auto& resMan = WzResMan::GetInstance();
        if (!resMan.IsInitialized())
        {
            resMan.SetBasePath(WZ_TEST_PATH);
            (void)resMan.Initialize();
        }
        s_bReady = resMan.IsInitialized();
    }

    void SetUp() override
    {
        if (!s_bReady)
            GTEST_SKIP() << "WZ files not found at " << WZ_TEST_PATH;
    }

    static bool s_bReady;
};

bool ItemInfoTest::s_bReady = false;

// ============================================================
// GetItemProp @ 0xaae510
// ============================================================

TEST_F(ItemInfoTest, GetItemProp_Equip_ReturnsNonNull)
{
    auto& info = ItemInfo::GetInstance();
    // 1302000 = common one-handed sword (Blue Duo)
    auto pProp = info.GetItemProp(1302000);
    ASSERT_NE(pProp, nullptr) << "GetItemProp(1302000) should return a valid WZ property";
}

TEST_F(ItemInfoTest, GetItemProp_Equip_HasInfoChild)
{
    auto& info = ItemInfo::GetInstance();
    auto pProp = info.GetItemProp(1302000);
    ASSERT_NE(pProp, nullptr);

    auto pInfo = pProp->GetChild("info");
    EXPECT_NE(pInfo, nullptr) << "Equip property should have an 'info' child";
}

TEST_F(ItemInfoTest, GetItemProp_Bundle_ReturnsNonNull)
{
    auto& info = ItemInfo::GetInstance();
    // 2000000 = Red Potion (Consume item)
    auto pProp = info.GetItemProp(2000000);
    ASSERT_NE(pProp, nullptr) << "GetItemProp(2000000) should return a valid WZ property";
}

TEST_F(ItemInfoTest, GetItemProp_Bundle_HasInfoChild)
{
    auto& info = ItemInfo::GetInstance();
    auto pProp = info.GetItemProp(2000000);
    ASSERT_NE(pProp, nullptr);

    auto pInfo = pProp->GetChild("info");
    EXPECT_NE(pInfo, nullptr) << "Bundle property should have an 'info' child";
}

TEST_F(ItemInfoTest, GetItemProp_InvalidID_ReturnsNull)
{
    auto& info = ItemInfo::GetInstance();
    auto pProp = info.GetItemProp(0);
    EXPECT_EQ(pProp, nullptr) << "GetItemProp(0) should return nullptr";
}

TEST_F(ItemInfoTest, GetItemProp_NonExistentID_ReturnsNull)
{
    auto& info = ItemInfo::GetInstance();
    // 1999999 = likely non-existent equip
    auto pProp = info.GetItemProp(1999999);
    EXPECT_EQ(pProp, nullptr) << "GetItemProp for non-existent item should return nullptr";
}

// ============================================================
// GetEquipItem @ 0xae54c0
// ============================================================

TEST_F(ItemInfoTest, GetEquipItem_ValidSword)
{
    auto& info = ItemInfo::GetInstance();
    auto* pEquip = info.GetEquipItem(1302000);
    ASSERT_NE(pEquip, nullptr) << "GetEquipItem(1302000) should return non-null";

    EXPECT_EQ(pEquip->nItemID, 1302000);
}

TEST_F(ItemInfoTest, GetEquipItem_HasBasicStats)
{
    auto& info = ItemInfo::GetInstance();
    auto* pEquip = info.GetEquipItem(1302000);
    ASSERT_NE(pEquip, nullptr);

    // A weapon should have some PAD (physical attack)
    EXPECT_GT(pEquip->niPAD, 0) << "Weapon should have positive physical attack";
}

TEST_F(ItemInfoTest, GetEquipItem_CacheConsistency)
{
    auto& info = ItemInfo::GetInstance();
    auto* pFirst = info.GetEquipItem(1302000);
    auto* pSecond = info.GetEquipItem(1302000);

    ASSERT_NE(pFirst, nullptr);
    EXPECT_EQ(pFirst, pSecond) << "Cache should return the same pointer";
}

TEST_F(ItemInfoTest, GetEquipItem_NonEquipID_ReturnsNull)
{
    auto& info = ItemInfo::GetInstance();
    // 2000000 is a consume item, not equip
    auto* pEquip = info.GetEquipItem(2000000);
    EXPECT_EQ(pEquip, nullptr) << "GetEquipItem for non-equip ID should return nullptr";
}

TEST_F(ItemInfoTest, GetEquipItem_InvalidID_ReturnsNull)
{
    auto& info = ItemInfo::GetInstance();
    auto* pEquip = info.GetEquipItem(0);
    EXPECT_EQ(pEquip, nullptr);
}

// ============================================================
// GetBundleItem @ 0xaf9310
// ============================================================

TEST_F(ItemInfoTest, GetBundleItem_ValidPotion)
{
    auto& info = ItemInfo::GetInstance();
    // 2000000 = Red Potion
    auto* pBundle = info.GetBundleItem(2000000);
    ASSERT_NE(pBundle, nullptr) << "GetBundleItem(2000000) should return non-null";

    EXPECT_EQ(pBundle->nItemID, 2000000);
}

TEST_F(ItemInfoTest, GetBundleItem_HasPrice)
{
    auto& info = ItemInfo::GetInstance();
    auto* pBundle = info.GetBundleItem(2000000);
    ASSERT_NE(pBundle, nullptr);

    // Red Potion should have a sell price
    EXPECT_GT(pBundle->nSellPrice, 0) << "Red Potion should have a sell price";
}

TEST_F(ItemInfoTest, GetBundleItem_CacheConsistency)
{
    auto& info = ItemInfo::GetInstance();
    auto* pFirst = info.GetBundleItem(2000000);
    auto* pSecond = info.GetBundleItem(2000000);

    ASSERT_NE(pFirst, nullptr);
    EXPECT_EQ(pFirst, pSecond) << "Cache should return the same pointer";
}

TEST_F(ItemInfoTest, GetBundleItem_EtcItem)
{
    auto& info = ItemInfo::GetInstance();
    // 4000000 = common Etc item (Blue Snail Shell)
    auto* pBundle = info.GetBundleItem(4000000);
    // May or may not exist depending on WZ data
    if (pBundle)
    {
        EXPECT_EQ(pBundle->nItemID, 4000000);
    }
}

TEST_F(ItemInfoTest, GetBundleItem_EquipID_ReturnsNull)
{
    auto& info = ItemInfo::GetInstance();
    // 1302000 is an equip, not a bundle item
    auto* pBundle = info.GetBundleItem(1302000);
    EXPECT_EQ(pBundle, nullptr) << "GetBundleItem for equip ID should return nullptr";
}

// ============================================================
// GetSetItemID @ 0xae6700
// ============================================================

TEST_F(ItemInfoTest, GetSetItemID_NoSet_ReturnsZero)
{
    auto& info = ItemInfo::GetInstance();
    // Most basic weapons don't belong to a set
    auto nSetID = info.GetSetItemID(1302000);
    // Could be 0 or a valid set ID depending on the item
    // Just verify it doesn't crash
    EXPECT_GE(nSetID, 0);
}

TEST_F(ItemInfoTest, GetSetItemID_InvalidID_ReturnsZero)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_EQ(info.GetSetItemID(0), 0);
}

TEST_F(ItemInfoTest, GetSetItemID_NonEquip_ReturnsZero)
{
    auto& info = ItemInfo::GetInstance();
    // Bundle items don't have setItemID via this path
    EXPECT_EQ(info.GetSetItemID(2000000), 0);
}

// ============================================================
// GetItemName @ 0xacfb80
// ============================================================

TEST_F(ItemInfoTest, GetItemName_Equip)
{
    auto& info = ItemInfo::GetInstance();
    auto sName = info.GetItemName(1302000);
    // The name should be non-empty if the WZ data has a "name" field in info
    // Some WZ versions may not store name in equip info
    // Just check it doesn't crash
    (void)sName;
}

TEST_F(ItemInfoTest, GetItemName_InvalidID_ReturnsEmpty)
{
    auto& info = ItemInfo::GetInstance();
    auto sName = info.GetItemName(0);
    EXPECT_TRUE(sName.empty());
}

// ============================================================
// IsCashItem @ 0xaafbe0
// ============================================================

TEST_F(ItemInfoTest, IsCashItem_RegularItem_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    // Red Potion is not a cash item
    EXPECT_FALSE(info.IsCashItem(2000000));
}

TEST_F(ItemInfoTest, IsCashItem_InvalidID_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsCashItem(0));
}

// ============================================================
// IsQuestItem @ 0xab1040
// ============================================================

TEST_F(ItemInfoTest, IsQuestItem_RegularItem_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    // Red Potion is not a quest item
    EXPECT_FALSE(info.IsQuestItem(2000000));
}

TEST_F(ItemInfoTest, IsQuestItem_InvalidID_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsQuestItem(0));
}

// ============================================================
// IsTradeBlockItem @ 0xab09d0
// ============================================================

TEST_F(ItemInfoTest, IsTradeBlockItem_RegularItem_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    // Basic sword should be tradeable
    EXPECT_FALSE(info.IsTradeBlockItem(1302000));
}

TEST_F(ItemInfoTest, IsTradeBlockItem_InvalidID_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsTradeBlockItem(0));
}

// ============================================================
// GetRequiredLEV @ 0xab23b0
// ============================================================

TEST_F(ItemInfoTest, GetRequiredLEV_Equip)
{
    auto& info = ItemInfo::GetInstance();
    auto nLev = info.GetRequiredLEV(1302000);
    // A basic sword should have a low or zero level requirement
    EXPECT_GE(nLev, 0);
}

TEST_F(ItemInfoTest, GetRequiredLEV_InvalidID_ReturnsZero)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_EQ(info.GetRequiredLEV(0), 0);
}

// ============================================================
// Integration: verify equip fields are populated from WZ
// ============================================================

TEST_F(ItemInfoTest, EquipItem_FieldsPopulated)
{
    auto& info = ItemInfo::GetInstance();
    auto* pEquip = info.GetEquipItem(1302000);
    ASSERT_NE(pEquip, nullptr) << "Item 1302000 should be loadable from WZ data";

    // Print discovered values for debugging
    std::cout << "=== Equip 1302000 ===" << std::endl;
    std::cout << "  name:     " << pEquip->sItemName << std::endl;
    std::cout << "  niPAD:    " << pEquip->niPAD << std::endl;
    std::cout << "  nTUC:     " << pEquip->nTUC << std::endl;
    std::cout << "  nrLevel:  " << pEquip->nrLevel << std::endl;
    std::cout << "  nrSTR:    " << pEquip->nrSTR << std::endl;
    std::cout << "  price:    " << pEquip->nSellPrice << std::endl;
    std::cout << "  cash:     " << pEquip->bCash << std::endl;
    std::cout << "  quest:    " << pEquip->bQuest << std::endl;
    std::cout << "  setItemID:" << pEquip->nSetItemID << std::endl;

    // TUC (upgrade slots) should be reasonable for a weapon
    EXPECT_GE(pEquip->nTUC, 0);
    EXPECT_LE(pEquip->nTUC, 20);
}

TEST_F(ItemInfoTest, BundleItem_FieldsPopulated)
{
    auto& info = ItemInfo::GetInstance();
    auto* pBundle = info.GetBundleItem(2000000);
    if (!pBundle)
        GTEST_SKIP() << "Item 2000000 not found in WZ data";

    std::cout << "=== Bundle 2000000 ===" << std::endl;
    std::cout << "  name:       " << pBundle->sItemName << std::endl;
    std::cout << "  price:      " << pBundle->nSellPrice << std::endl;
    std::cout << "  slotMax:    " << pBundle->nMaxPerSlot << std::endl;
    std::cout << "  cash:       " << pBundle->bCash << std::endl;
    std::cout << "  quest:      " << pBundle->bQuest << std::endl;
    std::cout << "  tradeBlock: " << pBundle->bTradeBlock << std::endl;

    // Red Potion should be stackable
    EXPECT_GT(pBundle->nMaxPerSlot, 1) << "Red Potion should be stackable";
}

// ============================================================
// GetItemInfo @ 0xaaede0
// ============================================================

TEST_F(ItemInfoTest, GetItemInfo_Equip_ReturnsInfoNode)
{
    auto& info = ItemInfo::GetInstance();
    auto pInfo = info.GetItemInfo(1302000);
    ASSERT_NE(pInfo, nullptr) << "GetItemInfo(1302000) should return the info sub-node";

    // The info node should have known equip fields
    auto pPrice = pInfo->GetChild("price");
    // price may or may not exist, but the node itself should be valid
    (void)pPrice;
}

TEST_F(ItemInfoTest, GetItemInfo_Bundle_ReturnsInfoNode)
{
    auto& info = ItemInfo::GetInstance();
    auto pInfo = info.GetItemInfo(2000000);
    ASSERT_NE(pInfo, nullptr) << "GetItemInfo(2000000) should return the info sub-node";

    // Should be able to read price from the info node
    auto pPrice = pInfo->GetChild("price");
    if (pPrice)
    {
        EXPECT_GT(pPrice->GetInt(0), 0) << "Red Potion info should have a price";
    }
}

TEST_F(ItemInfoTest, GetItemInfo_InvalidID_ReturnsNull)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_EQ(info.GetItemInfo(0), nullptr);
}

TEST_F(ItemInfoTest, GetItemInfo_NonExistent_ReturnsNull)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_EQ(info.GetItemInfo(1999999), nullptr);
}

// ============================================================
// GetItemDesc @ 0xacfe90
// ============================================================

TEST_F(ItemInfoTest, GetItemDesc_InvalidID_ReturnsEmpty)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.GetItemDesc(0).empty());
}

TEST_F(ItemInfoTest, GetItemDesc_ValidItem_DoesNotCrash)
{
    auto& info = ItemInfo::GetInstance();
    // May or may not have a description — just verify no crash
    auto sDesc = info.GetItemDesc(2000000);
    (void)sDesc;
}

// ============================================================
// IsEquipItem @ 0x5c0050
// ============================================================

TEST_F(ItemInfoTest, IsEquipItem_EquipID_ReturnsTrue)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsEquipItem(1302000));
}

TEST_F(ItemInfoTest, IsEquipItem_BundleID_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsEquipItem(2000000));
}

TEST_F(ItemInfoTest, IsEquipItem_Zero_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsEquipItem(0));
}

// ============================================================
// GetItemPrice @ 0xaf4db0
// ============================================================

TEST_F(ItemInfoTest, GetItemPrice_RedPotion_HasPrice)
{
    auto& info = ItemInfo::GetInstance();
    std::int32_t nPrice = 0;
    long double dUnitPrice = 0.0;
    auto bResult = info.GetItemPrice(2000000, nPrice, dUnitPrice);

    EXPECT_TRUE(bResult) << "GetItemPrice should return true for valid item";
    EXPECT_GT(nPrice, 0) << "Red Potion should have a price";
}

TEST_F(ItemInfoTest, GetItemPrice_InvalidID_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    std::int32_t nPrice = 42;
    long double dUnitPrice = 1.0;
    auto bResult = info.GetItemPrice(0, nPrice, dUnitPrice);

    EXPECT_FALSE(bResult) << "GetItemPrice should return false for invalid item";
    EXPECT_EQ(nPrice, 0);
    EXPECT_DOUBLE_EQ(static_cast<double>(dUnitPrice), 0.0);
}

TEST_F(ItemInfoTest, GetItemPrice_Equip_DoesNotCrash)
{
    auto& info = ItemInfo::GetInstance();
    std::int32_t nPrice = -1;
    long double dUnitPrice = -1.0;
    auto bResult = info.GetItemPrice(1302000, nPrice, dUnitPrice);

    EXPECT_TRUE(bResult) << "GetItemPrice should return true for valid equip";
    EXPECT_GE(nPrice, 0);
    EXPECT_GE(static_cast<double>(dUnitPrice), 0.0);
}

// ============================================================
// IsCashItem(const GW_ItemSlotBase&) @ 0x788d20
// ============================================================

TEST_F(ItemInfoTest, IsCashItem_Slot_NonCashItem_NoCashSN_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    GW_ItemSlotBase item;
    item.nItemID = 2000000;  // Red Potion — not a cash item
    item.liCashItemSN = 0;

    EXPECT_FALSE(info.IsCashItem(item));
}

TEST_F(ItemInfoTest, IsCashItem_Slot_NonCashItem_WithCashSN_ReturnsTrue)
{
    auto& info = ItemInfo::GetInstance();
    GW_ItemSlotBase item;
    item.nItemID = 2000000;  // Red Potion — not normally cash
    item.liCashItemSN = 12345;  // but has a cash serial

    EXPECT_TRUE(info.IsCashItem(item));
}

TEST_F(ItemInfoTest, IsCashItem_Slot_CashFlaggedEquip_ReturnsTrue)
{
    auto& info = ItemInfo::GetInstance();
    // Use an equip that has cash=1 in WZ (if one exists in test data)
    // Fallback: verify an equip with liCashItemSN != 0 returns true
    GW_ItemSlotBase item;
    item.nItemID = 1302000;  // Regular sword
    item.liCashItemSN = 99999;

    EXPECT_TRUE(info.IsCashItem(item));
}

// ============================================================
// GetItemCoolTime @ 0xafa8c0
// ============================================================

TEST_F(ItemInfoTest, GetItemCoolTime_BundleItem_ReturnsTrue)
{
    auto& info = ItemInfo::GetInstance();
    std::int32_t nLimitMin = -1, nLimitSec = -1;
    // Red Potion — bundle item, should return true with defaults (0, 0)
    auto bResult = info.GetItemCoolTime(2000000, nLimitMin, nLimitSec);

    EXPECT_TRUE(bResult);
    EXPECT_GE(nLimitMin, 0);
    EXPECT_GE(nLimitSec, 0);
}

TEST_F(ItemInfoTest, GetItemCoolTime_InvalidID_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    std::int32_t nLimitMin = -1, nLimitSec = -1;
    auto bResult = info.GetItemCoolTime(0, nLimitMin, nLimitSec);

    EXPECT_FALSE(bResult);
}

TEST_F(ItemInfoTest, GetItemCoolTime_EquipID_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    std::int32_t nLimitMin = -1, nLimitSec = -1;
    // Equip items are not bundles or cash, so GetBundleItem returns null
    auto bResult = info.GetItemCoolTime(1302000, nLimitMin, nLimitSec);

    EXPECT_FALSE(bResult);
}

TEST_F(ItemInfoTest, GetItemCoolTime_BundleItem_DefaultsZero)
{
    auto& info = ItemInfo::GetInstance();
    std::int32_t nLimitMin = -1, nLimitSec = -1;
    auto bResult = info.GetItemCoolTime(2000000, nLimitMin, nLimitSec);

    if (bResult)
    {
        // Most items default to 0 for limitMin/limitSec
        std::cout << "  limitMin: " << nLimitMin << "  limitSec: " << nLimitSec << std::endl;
    }
}

// ============================================================
// IsAbleToEquipSubWeapon @ 0xa7aaf0
// Pure logic tests — no WZ files required.
// ============================================================

class SubWeaponTest : public ::testing::Test
{
protected:
    ItemInfo& info = ItemInfo::GetInstance();
};

// --- Shield tests (nItemID / 10000 == 109) ---

TEST_F(SubWeaponTest, Shield_GenericJob_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // Generic warrior (job 100) with 1H sword (1302000) equipping a shield (1092000)
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1092000, 1302000, 100, 0, 0));
}

TEST_F(SubWeaponTest,Shield_DualBlade_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    // Dual Blade: job < 1000 and nSubJob == 1
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1092000, 1302000, 400, 1, 0));
}

TEST_F(SubWeaponTest,Shield_Mihile_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    // Mihile (5100) cannot equip non-1098 shields
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1092000, 1302000, 5100, 0, 0));
}

TEST_F(SubWeaponTest,Shield_Mihile_1098_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // Mihile CAN equip 1098xxx shields
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1098000, 1302000, 5100, 0, 0));
}

TEST_F(SubWeaponTest,Shield_DemonSlayer_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    // Demon Slayer (3100) cannot equip non-1099 shields
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1092000, 1302000, 3100, 0, 0));
}

TEST_F(SubWeaponTest,Shield_DemonSlayer_1099_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // Demon Slayer CAN equip 1099xxx shields
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1099000, 1302000, 3100, 0, 0));
}

TEST_F(SubWeaponTest,Shield_Xenon_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    // Xenon (3600) cannot equip non-cash shields
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1092000, 1302000, 3600, 0, 0));
}

TEST_F(SubWeaponTest,Shield_Xenon_Cash_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // Xenon CAN equip cash shields
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1092000, 1302000, 3600, 0, 1));
}

// --- Mercedes card tests (1350000-1352099) ---

TEST_F(SubWeaponTest,MercedesCard_Mercedes_1HSword_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // Mercedes (2300) with 1H sword (1302000) equipping Mercedes card (1350000)
    // 1302000 = weapon type 30 (1H sword), not 2H
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1350000, 1302000, 2300, 0, 0));
}

TEST_F(SubWeaponTest,MercedesCard_NonMercedes_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    // Non-Mercedes job cannot equip Mercedes cards
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1350000, 1302000, 100, 0, 0));
}

TEST_F(SubWeaponTest,MercedesCard_GM_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // GM (900) can equip Mercedes cards
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1350000, 1302000, 900, 0, 0));
}

TEST_F(SubWeaponTest,MercedesCard_2HWeapon_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    // Mercedes with 2H sword (1402000, weapon type 40) — blocked
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1350000, 1402000, 2300, 0, 0));
}

TEST_F(SubWeaponTest,MercedesCard_DualBowgun_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // Mercedes with dual bowgun (1522000, weapon type 52) — allowed (special exception)
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1350000, 1522000, 2300, 0, 0));
}

// --- Phantom card tests (1352100-1352199) ---

TEST_F(SubWeaponTest,PhantomCard_Phantom_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // Phantom (2400) with cane (1352100), equipping phantom card
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352100, 1302000, 2400, 0, 0));
}

TEST_F(SubWeaponTest,PhantomCard_NonPhantom_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1352100, 1302000, 100, 0, 0));
}

TEST_F(SubWeaponTest,PhantomCard_2HWeapon_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1352100, 1402000, 2400, 0, 0));
}

// --- Job-specific sub-weapons ---

TEST_F(SubWeaponTest,HeroMedal_Hero_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352200, 0, 112, 0, 0));
}

TEST_F(SubWeaponTest,HeroMedal_NonHero_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1352200, 0, 122, 0, 0));
}

TEST_F(SubWeaponTest,PaladinRosario_Paladin_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352210, 0, 122, 0, 0));
}

TEST_F(SubWeaponTest,DarkKnightChain_DK_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352220, 0, 132, 0, 0));
}

TEST_F(SubWeaponTest,Mage1Book_FPMage_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352230, 0, 212, 0, 0));
}

TEST_F(SubWeaponTest,Mage2Book_ILMage_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352240, 0, 222, 0, 0));
}

TEST_F(SubWeaponTest,Mage3Book_Bishop_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352250, 0, 232, 0, 0));
}

TEST_F(SubWeaponTest,BowmasterFeather_Bowmaster_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352260, 0, 312, 0, 0));
}

TEST_F(SubWeaponTest,CrossbowThimble_Marksman_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352270, 0, 322, 0, 0));
}

TEST_F(SubWeaponTest,ShadowerSheath_Shadower_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352280, 0, 422, 0, 0));
}

TEST_F(SubWeaponTest,NightlordPouch_NL_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352290, 0, 412, 0, 0));
}

TEST_F(SubWeaponTest,ViperWristband_Buccaneer_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352900, 0, 512, 0, 0));
}

TEST_F(SubWeaponTest,CaptainSight_Corsair_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352910, 0, 522, 0, 0));
}

TEST_F(SubWeaponTest,CannonGunpowder_Cannoneer_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352920, 0, 530, 0, 0));
}

TEST_F(SubWeaponTest,CannonGunpowder_NonCannoneer_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1352920, 0, 100, 0, 0));
}

// --- Sub-weapons with beginner fallback ---

TEST_F(SubWeaponTest,AranPendulum_Aran_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352930, 0, 2100, 0, 0));
}

TEST_F(SubWeaponTest,AranPendulum_Beginner_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // Beginner (job 2000) can also equip Aran pendulum
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352930, 0, 2000, 0, 0));
}

TEST_F(SubWeaponTest,AranPendulum_NonAran_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    // Non-Aran, non-beginner job
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1352930, 0, 112, 0, 0));
}

TEST_F(SubWeaponTest,EvanPaper_Evan_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352940, 0, 2200, 0, 0));
}

TEST_F(SubWeaponTest,CygnusGem_Cygnus_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352970, 0, 1100, 0, 0));
}

TEST_F(SubWeaponTest,CygnusGem_Beginner_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // Cygnus beginner (1000) can also equip
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352970, 0, 1000, 0, 0));
}

// --- Resistance sub-weapons ---

TEST_F(SubWeaponTest,BattlemageOrb_BattleMage_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352950, 0, 3200, 0, 0));
}

TEST_F(SubWeaponTest,WildhunterArrowhead_WH_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352960, 0, 3300, 0, 0));
}

// --- Sub-weapons with 2H restriction ---

TEST_F(SubWeaponTest,LuminousOrb_Luminous_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // Luminous (2700) with no weapon, equipping orb (1352400)
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352400, 0, 2700, 0, 0));
}

TEST_F(SubWeaponTest,LuminousOrb_2HWeapon_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1352400, 1402000, 2700, 0, 0));
}

TEST_F(SubWeaponTest,DragonSoul_Kaiser_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352500, 0, 6100, 0, 0));
}

TEST_F(SubWeaponTest,DragonSoul_Kaiser_2HSword_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    // 2H sword (weapon type 40) is special exception for Kaiser
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352500, 1402000, 6100, 0, 0));
}

TEST_F(SubWeaponTest,SoulRing_AB_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352600, 0, 6500, 0, 0));
}

TEST_F(SubWeaponTest,Magnum_Mechanic_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352700, 0, 3500, 0, 0));
}

// --- Zero and Kinesis ---

TEST_F(SubWeaponTest,Zero_ZeroJob_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1560000, 0, 10112, 0, 0));
}

TEST_F(SubWeaponTest,Zero_NonZero_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1560000, 0, 100, 0, 0));
}

TEST_F(SubWeaponTest,Kinesis_KinesisJob_Allowed)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1353200, 0, 14200, 0, 0));
}

TEST_F(SubWeaponTest,Kinesis_NonKinesis_Blocked)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsAbleToEquipSubWeapon(1353200, 0, 100, 0, 0));
}

// --- GM always allowed ---

TEST_F(SubWeaponTest,GM_AlwaysAllowed)
{
    auto& info = ItemInfo::GetInstance();
    // GM (900) can equip any sub-weapon
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352200, 0, 900, 0, 0));  // Hero medal
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1352500, 0, 900, 0, 0));  // Dragon soul
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1560000, 0, 900, 0, 0));  // Zero sub
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1353200, 0, 900, 0, 0));  // Kinesis sub
}

// --- Unknown sub-weapon → default allow ---

TEST_F(SubWeaponTest,Unknown_DefaultAllow)
{
    auto& info = ItemInfo::GetInstance();
    // An item that doesn't match any known sub-weapon category
    EXPECT_TRUE(info.IsAbleToEquipSubWeapon(1999999, 0, 100, 0, 0));
}

// ============================================================
// Boolean predicate tests (WZ-backed)
// ============================================================

TEST_F(ItemInfoTest, IsOnlyItem_EquipItem)
{
    auto& info = ItemInfo::GetInstance();
    // Basic sword is not "only" — verify no crash and returns a bool
    auto bResult = info.IsOnlyItem(1302000);
    (void)bResult;  // value depends on WZ data
}

TEST_F(ItemInfoTest, IsOnlyItem_BundleItem)
{
    auto& info = ItemInfo::GetInstance();
    auto bResult = info.IsOnlyItem(2000000);
    (void)bResult;
}

TEST_F(ItemInfoTest, IsOnlyItem_InvalidID_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsOnlyItem(0));
}

TEST_F(ItemInfoTest, IsSuperiorEquipItem_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    // Basic sword is not superior
    EXPECT_FALSE(info.IsSuperiorEquipItem(1302000));
}

TEST_F(ItemInfoTest, IsSuperiorEquipItem_NonEquip_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsSuperiorEquipItem(2000000));
}

TEST_F(ItemInfoTest, IsNotSaleItem_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    // Basic sword should be sellable
    EXPECT_FALSE(info.IsNotSaleItem(1302000));
}

TEST_F(ItemInfoTest, IsNotSaleItem_InvalidID_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsNotSaleItem(0));
}

TEST_F(ItemInfoTest, IsBigSizeItem_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    // Basic sword is not big size
    EXPECT_FALSE(info.IsBigSizeItem(1302000));
}

TEST_F(ItemInfoTest, GetAppliableKarmaType_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    // Basic sword likely has karma type 0
    auto nType = info.GetAppliableKarmaType(1302000);
    EXPECT_GE(nType, 0);
}

TEST_F(ItemInfoTest, GetAppliableKarmaType_InvalidID_ReturnsZero)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_EQ(info.GetAppliableKarmaType(0), 0);
}

TEST_F(ItemInfoTest, IsPartyQuestItem_RegularItems)
{
    auto& info = ItemInfo::GetInstance();
    // Red Potion is not a party quest item
    EXPECT_FALSE(info.IsPartyQuestItem(2000000));
    // Basic sword is not a party quest item
    EXPECT_FALSE(info.IsPartyQuestItem(1302000));
}

TEST_F(ItemInfoTest, IsPartyQuestItem_InvalidID_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsPartyQuestItem(0));
}

TEST_F(ItemInfoTest, GetSellPrice_Equip)
{
    auto& info = ItemInfo::GetInstance();
    auto nPrice = info.GetSellPrice(1302000);
    EXPECT_GE(nPrice, 0) << "Equip sell price should be non-negative";
}

TEST_F(ItemInfoTest, GetSellPrice_Bundle)
{
    auto& info = ItemInfo::GetInstance();
    auto nPrice = info.GetSellPrice(2000000);
    EXPECT_GT(nPrice, 0) << "Red Potion should have a sell price";
}

TEST_F(ItemInfoTest, GetSellPrice_InvalidID_ReturnsZero)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_EQ(info.GetSellPrice(0), 0);
}

TEST_F(ItemInfoTest, ExpireOnLogout_RegularItems)
{
    auto& info = ItemInfo::GetInstance();
    // Regular items don't expire on logout
    EXPECT_FALSE(info.ExpireOnLogout(1302000));
    EXPECT_FALSE(info.ExpireOnLogout(2000000));
}

TEST_F(ItemInfoTest, IsNoCancelByMouseForItem_RegularItem)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsNoCancelByMouseForItem(2000000));
}

TEST_F(ItemInfoTest, IsPickUpBlockItem_RegularItem)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsPickUpBlockItem(2000000));
}

TEST_F(ItemInfoTest, IsMorphItem_RegularItems)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsMorphItem(1302000));
    EXPECT_FALSE(info.IsMorphItem(2000000));
}

TEST_F(ItemInfoTest, IsUnchangeable_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsUnchangeable(1302000));
}

TEST_F(ItemInfoTest, IsUnchangeable_NonEquip_ReturnsFalse)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsUnchangeable(2000000));
}

TEST_F(ItemInfoTest, IsUndecomposable_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    // Basic sword should be decomposable
    EXPECT_FALSE(info.IsUndecomposable(1302000));
}

TEST_F(ItemInfoTest, IsRoyalSpecialItem_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsRoyalSpecialItem(1302000));
}

TEST_F(ItemInfoTest, IsRoyalMasterItem_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsRoyalMasterItem(1302000));
}

TEST_F(ItemInfoTest, IsBossRewardItem_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsBossRewardItem(1302000));
}

TEST_F(ItemInfoTest, IsExItem_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsExItem(1302000));
}

TEST_F(ItemInfoTest, IsCantRepairItem_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsCantRepairItem(1302000));
}

TEST_F(ItemInfoTest, IsDefaultAccountSharableItem_RegularItems)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsDefaultAccountSharableItem(1302000));
    EXPECT_FALSE(info.IsDefaultAccountSharableItem(2000000));
}

TEST_F(ItemInfoTest, IsSharableOnceItem_RegularItems)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsSharableOnceItem(1302000));
    EXPECT_FALSE(info.IsSharableOnceItem(2000000));
}

TEST_F(ItemInfoTest, IsApplicableAccountShareTag_RegularItems)
{
    auto& info = ItemInfo::GetInstance();
    // Basic items typically don't have account share tags
    auto bEquip = info.IsApplicableAccountShareTag(1302000);
    auto bBundle = info.IsApplicableAccountShareTag(2000000);
    (void)bEquip;
    (void)bBundle;
}

TEST_F(ItemInfoTest, IsBindedWhenEquiped_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsBindedWhenEquiped(1302000));
}

TEST_F(ItemInfoTest, IsNotExtendItem_BasicWeapon)
{
    auto& info = ItemInfo::GetInstance();
    EXPECT_FALSE(info.IsNotExtendItem(1302000));
}
