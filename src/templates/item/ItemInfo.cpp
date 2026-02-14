#include "ItemInfo.h"

#include "constants/EquipDataPath.h"
#include "models/GW_ItemSlotBase.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

#include <algorithm>
#include <cstdio>

namespace ms
{

namespace
{

/// Format item ID as 8-digit zero-padded string
auto format_item_id(std::int32_t nItemID) -> std::string
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%08d", nItemID);
    return buf;
}

/// Resolve bundle item ID to WZ path.
/// Pattern: "Item/{Consume,Install,Etc,Cash}/0{prefix}.img/0{itemID}"
auto get_bundle_data_path(std::int32_t nItemID) -> std::string
{
    auto nType = helper::GetItemType(nItemID);

    const char* sCategory = nullptr;
    switch (nType)
    {
    case helper::kConsume: sCategory = "Consume"; break;
    case helper::kInstall: sCategory = "Install"; break;
    case helper::kEtc:     sCategory = "Etc";     break;
    case helper::kCash:    sCategory = "Cash";    break;
    default: return {};
    }

    auto sPrefix = format_item_id((nItemID / 10000) * 10000);

    // e.g. "Item/Consume/0200.img/02000000"
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d", nItemID / 10000);

    return std::string("Item/") + sCategory + "/" + buf + ".img/" + sPrefix;
}

/// Read an int child property, return default if child doesn't exist
auto get_child_int(const std::shared_ptr<WzProperty>& pProp, const std::string& sName, std::int32_t nDefault = 0) -> std::int32_t
{
    auto pChild = pProp->GetChild(sName);
    return pChild ? pChild->GetInt(nDefault) : nDefault;
}

/// Read a double child property, return default if child doesn't exist
auto get_child_double(const std::shared_ptr<WzProperty>& pProp, const std::string& sName, double dDefault = 0.0) -> double
{
    auto pChild = pProp->GetChild(sName);
    return pChild ? pChild->GetDouble(dDefault) : dDefault;
}

/// Read a string child property, return default if child doesn't exist
auto get_child_string(const std::shared_ptr<WzProperty>& pProp, const std::string& sName, const std::string& sDefault = "") -> std::string
{
    auto pChild = pProp->GetChild(sName);
    return pChild ? pChild->GetString(sDefault) : sDefault;
}

/// Resolve pet item ID to WZ path.
/// Pattern: "Item/Pet/{nItemID:07d}.img"
/// StringPool 0x2AFA = "Item/Pet/%07d.img"
auto get_pet_data_path(std::int32_t nItemID) -> std::string
{
    char buf[32];
    std::snprintf(buf, sizeof(buf), "Item/Pet/%07d.img", nItemID);
    return buf;
}

} // anonymous namespace

// ============================================================
// GetItemProp @ 0xaae510
// Returns the WZ property node for any item ID.
// For equips: the .img root (e.g. "Character/Weapon/01302000.img")
// For bundles: the item sub-node (e.g. "Item/Consume/0200.img/02000000")
// For pets: the .img root (e.g. "Item/Pet/5000000.img")
// ============================================================
auto ItemInfo::GetItemProp(std::int32_t nItemID) const -> std::shared_ptr<WzProperty>
{
    auto& wzResMan = WzResMan::GetInstance();
    auto nType = helper::GetItemType(nItemID);

    if (nType == helper::kEquip)
    {
        // Equip items and face/hair: resolve via equip data path
        auto sPath = get_equip_data_path(nItemID);
        if (sPath.empty())
            return nullptr;
        return wzResMan.GetProperty(sPath);
    }

    // Pet items (cash type, slot type 3): "Item/Pet/%07d.img"
    if (nType == helper::kCash && nItemID / 10000 > 3)
    {
        auto sPath = get_pet_data_path(nItemID);
        auto pProp = wzResMan.GetProperty(sPath);
        if (pProp)
            return pProp;
        // Fall through to bundle path if pet path fails
    }

    // Bundle items (Consume/Install/Etc/Cash)
    auto sPath = get_bundle_data_path(nItemID);
    if (sPath.empty())
        return nullptr;
    return wzResMan.GetProperty(sPath);
}

// ============================================================
// RegisterEquipItemInfo @ 0xad9ca0
// Loads equip data from WZ and populates EquipItem struct
// ============================================================
auto ItemInfo::RegisterEquipItemInfo(std::int32_t nItemID, const std::string& sUOL) -> std::shared_ptr<EquipItem>
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty(sUOL);
    if (!pProp)
        return nullptr;

    auto pInfo = pProp->GetChild("info");
    if (!pInfo)
        return nullptr;

    auto pEquip = std::make_shared<EquipItem>();
    pEquip->nItemID = nItemID;
    pEquip->sUOL = sUOL;

    // --- Identity ---
    pEquip->bTimeLimited       = get_child_int(pInfo, "timeLimited");
    pEquip->bAbilityTimeLimited = get_child_int(pInfo, "abilityTimeLimited");
    pEquip->sItemName          = get_child_string(pInfo, "name");

    // --- Required stats ---
    pEquip->nrSTR   = get_child_int(pInfo, "reqSTR");
    pEquip->nrDEX   = get_child_int(pInfo, "reqDEX");
    pEquip->nrINT   = get_child_int(pInfo, "reqINT");
    pEquip->nrLUK   = get_child_int(pInfo, "reqLUK");
    pEquip->nrPOP   = get_child_int(pInfo, "reqPOP");
    pEquip->nrJob   = get_child_int(pInfo, "reqJob");
    pEquip->nrLevel = get_child_int(pInfo, "reqLevel");

    // --- Price / Cash ---
    pEquip->nSellPrice = get_child_int(pInfo, "price");
    pEquip->bCash      = get_child_int(pInfo, "cash");

    // --- Upgrade slots ---
    pEquip->nTUC = get_child_int(pInfo, "tuc");

    // --- Stat increments ---
    pEquip->niSTR   = get_child_int(pInfo, "incSTR");
    pEquip->niDEX   = get_child_int(pInfo, "incDEX");
    pEquip->niINT   = get_child_int(pInfo, "incINT");
    pEquip->niLUK   = get_child_int(pInfo, "incLUK");
    pEquip->niMaxHP = get_child_int(pInfo, "incMHP");
    pEquip->niMaxMP = get_child_int(pInfo, "incMMP");
    pEquip->niPAD   = get_child_int(pInfo, "incPAD");
    pEquip->niMAD   = get_child_int(pInfo, "incMAD");
    pEquip->niPDD   = get_child_int(pInfo, "incPDD");
    pEquip->niMDD   = get_child_int(pInfo, "incMDD");
    pEquip->niACC   = get_child_int(pInfo, "incACC");
    pEquip->niEVA   = get_child_int(pInfo, "incEVA");
    pEquip->niCraft = get_child_int(pInfo, "incCraft");
    pEquip->niSpeed = get_child_int(pInfo, "incSpeed");
    pEquip->niJump  = get_child_int(pInfo, "incJump");

    // --- Special flags ---
    pEquip->bQuest      = get_child_int(pInfo, "quest");
    pEquip->bOnly       = get_child_int(pInfo, "only");
    pEquip->bOnlyEquip  = get_child_int(pInfo, "onlyEquip");
    pEquip->bTradeBlock = get_child_int(pInfo, "tradeBlock");
    pEquip->bNotSale    = get_child_int(pInfo, "notSale");
    pEquip->bAccountSharable = get_child_int(pInfo, "accountSharable");
    pEquip->bExpireOnLogout  = get_child_int(pInfo, "expireOnLogout");
    pEquip->bSuperiorEqp     = get_child_int(pInfo, "superiorEqp");

    // --- Set / Group ---
    pEquip->nSetItemID      = get_child_int(pInfo, "setItemID");
    pEquip->nJokerToSetItem = get_child_int(pInfo, "jokerToSetItem");

    // --- Damage ---
    pEquip->nBDR   = get_child_int(pInfo, "bdR");
    pEquip->nIMDR  = get_child_int(pInfo, "imdR");
    pEquip->nDamR  = get_child_int(pInfo, "damR");
    pEquip->nStatR = get_child_int(pInfo, "statR");

    // --- Vehicle ---
    pEquip->nTamingMob = get_child_int(pInfo, "tamingMob");

    // --- Recovery / Movement ---
    pEquip->dRecovery = get_child_double(pInfo, "recovery");
    pEquip->dFs       = get_child_double(pInfo, "fs");

    // --- Knockback ---
    pEquip->nKnockback = get_child_int(pInfo, "knockback");

    // --- Description ---
    pEquip->sDesc = get_child_string(pInfo, "desc");

    return pEquip;
}

// ============================================================
// GetEquipItem @ 0xae54c0
// Cache-or-load pattern: check cache, load from WZ on miss
// ============================================================
auto ItemInfo::GetEquipItem(std::int32_t nItemID) -> const EquipItem*
{
    // Check cache first
    auto it = m_mEquipItem.find(nItemID);
    if (it != m_mEquipItem.end())
        return it->second.get();

    // Resolve WZ path
    auto sPath = get_equip_data_path(nItemID);
    if (sPath.empty())
        return nullptr;

    // Load from WZ
    auto pEquip = RegisterEquipItemInfo(nItemID, sPath);
    if (!pEquip)
        return nullptr;

    // Insert into cache and return
    auto [inserted, _] = m_mEquipItem.emplace(nItemID, std::move(pEquip));
    return inserted->second.get();
}

// ============================================================
// GetBundleItem @ 0xaf9310
// Cache-or-load pattern for bundle (consume/install/etc/cash) items
// ============================================================
auto ItemInfo::GetBundleItem(std::int32_t nItemID) -> const BundleItem*
{
    // Check cache first
    auto it = m_mBundleItem.find(nItemID);
    if (it != m_mBundleItem.end())
        return it->second.get();

    // Resolve WZ path
    auto sPath = get_bundle_data_path(nItemID);
    if (sPath.empty())
        return nullptr;

    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty(sPath);
    if (!pProp)
        return nullptr;

    auto pInfo = pProp->GetChild("info");
    if (!pInfo)
        return nullptr;

    auto pBundle = std::make_shared<BundleItem>();
    pBundle->nItemID = nItemID;

    // --- Identity ---
    pBundle->sItemName    = get_child_string(pInfo, "name");
    pBundle->bTimeLimited = get_child_int(pInfo, "timeLimited");

    // --- Price / Cash ---
    pBundle->nSellPrice = get_child_int(pInfo, "price");
    pBundle->bCash      = get_child_int(pInfo, "cash");

    // --- Stack limits ---
    pBundle->nMaxPerSlot = static_cast<std::int16_t>(get_child_int(pInfo, "slotMax"));

    // --- Required ---
    pBundle->nRequiredLEV = get_child_int(pInfo, "reqLevel");
    pBundle->nPAD         = get_child_int(pInfo, "pad");

    // --- Flags ---
    pBundle->bQuest      = get_child_int(pInfo, "quest");
    pBundle->bOnly       = get_child_int(pInfo, "only");
    pBundle->bTradeBlock = get_child_int(pInfo, "tradeBlock");
    pBundle->bNotSale    = get_child_int(pInfo, "notSale");
    pBundle->bAccountSharable = get_child_int(pInfo, "accountSharable");
    pBundle->bExpireOnLogout  = get_child_int(pInfo, "expireOnLogout");

    // --- Type ---
    pBundle->nBagType = get_child_int(pInfo, "bagType");

    // --- Level / Time limits ---
    // Original @ 0xAF57A0: only read for Use items (type 2), category 414,
    // or get_etc_cash_item_type() == 7. Values clamped to >= 0.
    auto nCategory = nItemID / 10000;
    if (helper::GetItemType(nItemID) == helper::kConsume || nCategory == 414
        /* TODO: || get_etc_cash_item_type(nItemID) == 7 */)
    {
        pBundle->nLimitMin = std::max(0, get_child_int(pInfo, "limitMin"));
        pBundle->nLimitSec = std::max(0, get_child_int(pInfo, "limitSec"));
    }

    // Insert into cache and return
    auto [inserted, _] = m_mBundleItem.emplace(nItemID, std::move(pBundle));
    return inserted->second.get();
}

// ============================================================
// GetSetItemID @ 0xae6700
// ============================================================
auto ItemInfo::GetSetItemID(std::int32_t nItemID) -> std::int32_t
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* pEquip = GetEquipItem(nItemID);
        return pEquip ? pEquip->nSetItemID : 0;
    }
    return 0;
}

// ============================================================
// GetItemName @ 0xacfb80
// ============================================================
auto ItemInfo::GetItemName(std::int32_t nItemID) -> std::string
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* pEquip = GetEquipItem(nItemID);
        return pEquip ? pEquip->sItemName : std::string{};
    }

    auto* pBundle = GetBundleItem(nItemID);
    return pBundle ? pBundle->sItemName : std::string{};
}

// ============================================================
// IsCashItem @ 0xaafbe0
// ============================================================
auto ItemInfo::IsCashItem(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* pEquip = GetEquipItem(nItemID);
        return pEquip ? pEquip->bCash != 0 : false;
    }

    auto* pBundle = GetBundleItem(nItemID);
    return pBundle ? pBundle->bCash != 0 : false;
}

// ============================================================
// IsQuestItem @ 0xab1040
// ============================================================
auto ItemInfo::IsQuestItem(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* pEquip = GetEquipItem(nItemID);
        return pEquip ? pEquip->bQuest != 0 : false;
    }

    auto* pBundle = GetBundleItem(nItemID);
    return pBundle ? pBundle->bQuest != 0 : false;
}

// ============================================================
// IsTradeBlockItem @ 0xab09d0
// ============================================================
auto ItemInfo::IsTradeBlockItem(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* pEquip = GetEquipItem(nItemID);
        return pEquip ? pEquip->bTradeBlock != 0 : false;
    }

    auto* pBundle = GetBundleItem(nItemID);
    return pBundle ? pBundle->bTradeBlock != 0 : false;
}

// ============================================================
// GetRequiredLEV @ 0xab23b0
// ============================================================
auto ItemInfo::GetRequiredLEV(std::int32_t nItemID) -> std::int32_t
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* pEquip = GetEquipItem(nItemID);
        return pEquip ? pEquip->nrLevel : 0;
    }

    auto* pBundle = GetBundleItem(nItemID);
    return pBundle ? pBundle->nRequiredLEV : 0;
}

// ============================================================
// GetItemInfo @ 0xaaede0
// Returns the "info" sub-property for any item ID.
// Special case: category 910 items return the prop node directly
// (their .img root IS the info node).
// ============================================================
auto ItemInfo::GetItemInfo(std::int32_t nItemID) const -> std::shared_ptr<WzProperty>
{
    auto pProp = GetItemProp(nItemID);
    if (!pProp)
        return nullptr;

    // Category 910 items: prop root is the info node itself
    if (nItemID / 10000 == 910)
        return pProp;

    return pProp->GetChild("info");
}

// ============================================================
// GetItemDesc @ 0xacfe90
// Returns item description string.
// Original calls GetItemString(nItemID, StringPool(0x9A8="desc"))
// which reads from m_mItemString table. We approximate by reading
// from WZ info node until GetItemString is implemented.
// ============================================================
auto ItemInfo::GetItemDesc(std::int32_t nItemID) -> std::string
{
    // TODO: replace with GetItemString(nItemID, "desc") when implemented
    if (helper::IsEquipItemID(nItemID))
    {
        auto* pEquip = GetEquipItem(nItemID);
        return pEquip ? pEquip->sDesc : std::string{};
    }

    auto pInfo = GetItemInfo(nItemID);
    if (!pInfo)
        return {};
    return get_child_string(pInfo, "desc");
}

// ============================================================
// IsEquipItem @ 0x5c0050
// Returns true if GetEquipItem succeeds (item exists as equip).
// ============================================================
auto ItemInfo::IsEquipItem(std::int32_t nItemID) -> bool
{
    return GetEquipItem(nItemID) != nullptr;
}

// ============================================================
// GetItemPrice @ 0xaf4db0
// Reads price, unitPrice, and autoPrice from WZ info node.
// Original returns int (1=success, 0=no item info).
// Also checks CSpecialServerMan override first (TODO: not yet implemented).
// ============================================================
auto ItemInfo::GetItemPrice(std::int32_t nItemID, std::int32_t& nPrice, long double& dUnitPrice) -> bool
{
    nPrice = 0;
    dUnitPrice = 0.0;

    auto pInfo = GetItemInfo(nItemID);
    if (!pInfo)
        return false;

    // TODO: CSpecialServerMan::GetSellItemPrice override check goes here

    nPrice = get_child_int(pInfo, "price");
    dUnitPrice = static_cast<long double>(get_child_double(pInfo, "unitPrice"));

    if (nPrice == 0 && get_child_int(pInfo, "autoPrice"))
    {
        // Read "lv" from WZ info, look up price by category and level
        auto nLv = get_child_int(pInfo, "lv");
        auto itCategory = m_mItemSellPriceByLv.find(nItemID / 10000);
        if (itCategory != m_mItemSellPriceByLv.end())
        {
            auto itPrice = itCategory->second.find(nLv);
            if (itPrice != itCategory->second.end())
                nPrice = itPrice->second;
        }
    }

    return true;
}

// ============================================================
// IsCashItem(const GW_ItemSlotBase&) @ 0x788d20
// Overload that checks the item ID first, then falls back to
// checking the cash-item serial number on the item slot.
// ============================================================
auto ItemInfo::IsCashItem(const GW_ItemSlotBase& item) -> bool
{
    if (IsCashItem(static_cast<std::int32_t>(item.nItemID)))
        return true;
    return item.liCashItemSN != 0;
}

// ============================================================
// GetItemCoolTime @ 0xafa8c0
// Reads limitMin / limitSec for an item. Cash items read
// directly from WZ info; non-cash read from BundleItem cache.
// Returns true if both values are non-negative.
// ============================================================
auto ItemInfo::GetItemCoolTime(std::int32_t nItemID, std::int32_t& nLimitMin, std::int32_t& nLimitSec) -> bool
{
    if (helper::GetItemType(nItemID) == helper::kCash)
    {
        auto pInfo = GetItemInfo(nItemID);
        if (!pInfo)
            return false;
        nLimitMin = get_child_int(pInfo, "limitMin");
        nLimitSec = get_child_int(pInfo, "limitSec");
    }
    else
    {
        auto* pBundle = GetBundleItem(nItemID);
        if (!pBundle)
            return false;
        nLimitMin = pBundle->nLimitMin;
        nLimitSec = pBundle->nLimitSec;
    }
    return nLimitMin >= 0 && nLimitSec >= 0;
}

} // namespace ms
