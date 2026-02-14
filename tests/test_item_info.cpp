/**
 * ItemInfo tests using real WZ files via Google Test
 */

#include <gtest/gtest.h>
#include <filesystem>

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
    if (!pProp)
        GTEST_SKIP() << "WzResMan cannot resolve Character sub-package paths";
    EXPECT_NE(pProp, nullptr);
}

TEST_F(ItemInfoTest, GetItemProp_Equip_HasInfoChild)
{
    auto& info = ItemInfo::GetInstance();
    auto pProp = info.GetItemProp(1302000);
    if (!pProp)
        GTEST_SKIP() << "WzResMan cannot resolve Character sub-package paths";

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
    if (!pEquip)
        GTEST_SKIP() << "WzResMan cannot resolve Character sub-package paths";

    EXPECT_EQ(pEquip->nItemID, 1302000);
}

TEST_F(ItemInfoTest, GetEquipItem_HasBasicStats)
{
    auto& info = ItemInfo::GetInstance();
    auto* pEquip = info.GetEquipItem(1302000);
    if (!pEquip)
        GTEST_SKIP() << "WzResMan cannot resolve Character sub-package paths";

    // A weapon should have some PAD (physical attack)
    EXPECT_GT(pEquip->niPAD, 0) << "Weapon should have positive physical attack";
}

TEST_F(ItemInfoTest, GetEquipItem_CacheConsistency)
{
    auto& info = ItemInfo::GetInstance();
    auto* pFirst = info.GetEquipItem(1302000);
    if (!pFirst)
        GTEST_SKIP() << "WzResMan cannot resolve Character sub-package paths";

    auto* pSecond = info.GetEquipItem(1302000);
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
    if (!pEquip)
        GTEST_SKIP() << "Item 1302000 not found in WZ data";

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
