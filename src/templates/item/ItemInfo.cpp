#include "ItemInfo.h"

#include "constants/EquipDataPath.h"
#include "constants/JobConstants.h"
#include "constants/WeaponConstants.h"
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

    // --- Requirements (extended) ---
    pEquip->nrSpecJob   = get_child_int(pInfo, "reqSpecJob");
    pEquip->nrMobLevel  = get_child_int(pInfo, "reqMobLevel");
    pEquip->nrPvPGrade  = get_child_int(pInfo, "reqPvPGrade");

    // --- Replacement ---
    pEquip->nReplaceItemID = get_child_int(pInfo, "replaceItemId");
    pEquip->sReplaceMsg    = get_child_string(pInfo, "replaceMsg");
    pEquip->nReplacePeriod = get_child_int(pInfo, "replacePeriod");

    // --- Additional stat increments ---
    pEquip->niMaxHPr  = get_child_int(pInfo, "incMHPr");
    pEquip->niMaxMPr  = get_child_int(pInfo, "incMMPr");
    pEquip->niSwim    = get_child_int(pInfo, "incSwim");
    pEquip->niFatigue = get_child_int(pInfo, "incFatigue");

    // --- PVP / Bonus damage ---
    pEquip->niPVPDamage = get_child_int(pInfo, "incPVPDamage");
    pEquip->niReduceReq = get_child_int(pInfo, "reduceReq");
    pEquip->niIncReq    = get_child_int(pInfo, "incReq");

    // --- Special flags ---
    pEquip->nCuttable            = get_child_int(pInfo, "cuttable", 255);
    pEquip->bExItem              = get_child_int(pInfo, "exItem");
    pEquip->bBossReward          = get_child_int(pInfo, "bossReward");
    pEquip->nExGrade             = get_child_int(pInfo, "exGrade");
    pEquip->bNoMoveToLocker      = get_child_int(pInfo, "noMoveToLocker");
    pEquip->bBigSize             = get_child_int(pInfo, "bigSize");
    pEquip->bBindedWhenEquiped   = get_child_int(pInfo, "bindOnEquip");
    pEquip->dwSpecialID          = static_cast<std::uint32_t>(get_child_int(pInfo, "specialID"));
    pEquip->bNotExtend           = get_child_int(pInfo, "notExtend");
    pEquip->bUnChangeable        = get_child_int(pInfo, "unchangeable");
    pEquip->dwAfterimageFlag     = static_cast<std::uint32_t>(get_child_int(pInfo, "afterImage"));
    pEquip->bJewelCraft          = get_child_int(pInfo, "jewelCraft");
    pEquip->bScope               = get_child_int(pInfo, "scope");
    pEquip->bMorphItem           = get_child_int(pInfo, "morphItem");

    // --- Trade restrictions ---
    pEquip->nAppliableKarmaType        = get_child_int(pInfo, "karmaType");
    pEquip->bAccountShareTagApplicable = get_child_int(pInfo, "accountShareTag");
    pEquip->bSharableOnce              = get_child_int(pInfo, "sharableOnce");

    // --- Destruction / Enhancement ---
    pEquip->bUndecomposable       = get_child_int(pInfo, "undecomposable");
    pEquip->bNotDestroy           = get_child_int(pInfo, "notDestroy");
    pEquip->bAlwaysGradeUpgrade   = get_child_int(pInfo, "alwaysGradeUpgrade");
    pEquip->bAlwaysInchantSuccess = get_child_int(pInfo, "alwaysInchantSuccess");
    pEquip->bSellingOneMeso       = get_child_int(pInfo, "sellingOneMeso");
    pEquip->nBitsSlot             = get_child_int(pInfo, "bitsSlot", -1);

    // --- Potential ---
    pEquip->bEpic                  = get_child_int(pInfo, "epic");
    pEquip->bFixedPotential        = get_child_int(pInfo, "fixedPotential");
    pEquip->nFixedGrade            = get_child_int(pInfo, "fixedGrade");
    pEquip->nSetGrade              = get_child_int(pInfo, "setGrade");
    pEquip->nFixedOptionLevel      = get_child_int(pInfo, "fixedOptionLevel");
    pEquip->nCubeExBaseOptionLevel = get_child_int(pInfo, "cubeExBaseOptionLevel");
    pEquip->bNoPotential           = get_child_int(pInfo, "noPotential");
    pEquip->bSpecialGrade          = get_child_int(pInfo, "specialGrade");
    pEquip->bRandItemVariation     = get_child_int(pInfo, "randItemVariation");
    pEquip->nReissueBan            = get_child_int(pInfo, "reissueBan");
    pEquip->nDisableFieldType      = get_child_int(pInfo, "disableFieldType", -1);

    // --- Royal / Text ---
    pEquip->bRoyalSpecial = get_child_int(pInfo, "royalSpecial");
    pEquip->bRoyalMaster  = get_child_int(pInfo, "royalMaster");
    pEquip->bText         = get_child_int(pInfo, "textTag");

    // --- Ring / Group / Sound / Emotion ---
    pEquip->nRingOptionSkill      = get_child_int(pInfo, "ringOptionSkill");
    pEquip->nRingOptionSkillLevel = get_child_int(pInfo, "ringOptionSkillLv");
    pEquip->nGroupEffectID        = get_child_int(pInfo, "groupEffectID", -1);
    pEquip->sEquipedSound         = get_child_string(pInfo, "equipedSound");
    pEquip->nEquippedEmotion      = get_child_int(pInfo, "equipEmotion");

    // --- Android ---
    pEquip->nAndroid      = get_child_int(pInfo, "android");
    pEquip->nAndroidGrade = get_child_int(pInfo, "androidGrade");

    // --- Personality EXP ---
    pEquip->nCharismaEXP       = get_child_int(pInfo, "charismaEXP");
    pEquip->nInsightEXP        = get_child_int(pInfo, "insightEXP");
    pEquip->nWillEXP           = get_child_int(pInfo, "willEXP");
    pEquip->nCraftEXP          = get_child_int(pInfo, "craftEXP");
    pEquip->nSenseEXP          = get_child_int(pInfo, "senseEXP");
    pEquip->nCharmEXP          = get_child_int(pInfo, "charmEXP");
    pEquip->nCashForceCharmEXP = get_child_int(pInfo, "cashForceCharmEXP", -1);

    // --- Party bonuses ---
    pEquip->nBestFriendPartyBonusExp   = get_child_int(pInfo, "bestFriendPartyBonusExp");
    pEquip->nBloodAllianceExpRate      = get_child_int(pInfo, "bloodAllianceExpRate");
    pEquip->nBloodAlliancePartyExpRate = get_child_int(pInfo, "bloodAlliancePartyExpRate");

    // --- Vehicle ---
    pEquip->nVehicleDoubleJumpLevel  = get_child_int(pInfo, "vehicleDoubleJump");
    pEquip->nVehicleGlideLevel       = get_child_int(pInfo, "vehicleGlide");
    pEquip->nVehicleNewFlyingLevel   = get_child_int(pInfo, "vehicleNewFlying");
    pEquip->nVehicleSkillIsTown      = get_child_int(pInfo, "vehicleSkillIsTown");
    pEquip->nVehicleNaviFlyingLevel  = get_child_int(pInfo, "vehicleNaviFlying");

    // --- Movement / Swim / Durability ---
    pEquip->nSwim       = get_child_int(pInfo, "swim", 100);
    pEquip->nDurability = get_child_int(pInfo, "durability", -1);
    pEquip->bCantRepair = get_child_int(pInfo, "cantRepair");

    // --- Grade / Quest ---
    pEquip->bPartyQuest = get_child_int(pInfo, "pquest");
    pEquip->nMinGrade   = get_child_int(pInfo, "minGrade");

    // --- Misc ---
    pEquip->nAttackCountInc = get_child_int(pInfo, "attackCountInc");
    pEquip->nLookChangeType = get_child_int(pInfo, "lookChangeType");

    // --- Time-limited stats (only load when bAbilityTimeLimited is set) ---
    if (pEquip->bAbilityTimeLimited) {
        pEquip->niTLSTR   = get_child_int(pInfo, "incTLSTR");
        pEquip->niTLDEX   = get_child_int(pInfo, "incTLDEX");
        pEquip->niTLINT   = get_child_int(pInfo, "incTLINT");
        pEquip->niTLLUK   = get_child_int(pInfo, "incTLLUK");
        pEquip->niTLMaxHP = get_child_int(pInfo, "incTLMHP");
        pEquip->niTLMaxMP = get_child_int(pInfo, "incTLMMP");
        pEquip->niTLPAD   = get_child_int(pInfo, "incTLPAD");
        pEquip->niTLMAD   = get_child_int(pInfo, "incTLMAD");
        pEquip->niTLPDD   = get_child_int(pInfo, "incTLPDD");
        pEquip->niTLMDD   = get_child_int(pInfo, "incTLMDD");
        pEquip->niTLACC   = get_child_int(pInfo, "incTLACC");
        pEquip->niTLEVA   = get_child_int(pInfo, "incTLEVA");
        pEquip->niTLCraft = get_child_int(pInfo, "incTLCraft");
        pEquip->niTLSpeed = get_child_int(pInfo, "incTLSpeed");
        pEquip->niTLJump  = get_child_int(pInfo, "incTLJump");
        pEquip->nTLBDR    = get_child_int(pInfo, "TLBDR");
        pEquip->nTLIMDR   = get_child_int(pInfo, "TLIMDR");
        pEquip->nTLDamR   = get_child_int(pInfo, "TLDamR");
        pEquip->nTLStatR  = get_child_int(pInfo, "TLStatR");
    }

    // --- Description ---
    pEquip->sDesc = get_child_string(pInfo, "desc");

    // ============================================================
    // Complex sub-property loaders (require child iteration)
    // ============================================================

    // --- aFixedOption (option sub-property, max 6) ---
    if (auto pOption = pInfo->GetChild("option")) {
        std::int32_t nIdx = 0;
        for (const auto& [sKey, pOpt] : pOption->GetChildren()) {
            if (nIdx >= 6) break;
            if (!pOpt) continue;
            pEquip->aFixedOption[nIdx].nOption = get_child_int(pOpt, "option");
            pEquip->aFixedOption[nIdx].nLevel  = get_child_int(pOpt, "level");
            ++nIdx;
        }
        pEquip->nFixedOptionCnt = nIdx;
    }

    // --- lpItemSkill (skill list from epic sub-tree) ---
    if (auto pEpic = pInfo->GetChild("epic")) {
        if (auto pSkill = pEpic->GetChild("skill")) {
            for (const auto& [sKey, pEntry] : pSkill->GetChildren()) {
                if (!pEntry) continue;
                auto pIS = std::make_shared<ItemSkill>();
                pIS->nSkillID         = get_child_int(pEntry, "id");
                pIS->nSkillLevel      = get_child_int(pEntry, "level");
                pIS->bAutoRunOnlyTown = get_child_int(pEntry, "autoRunOnlyTown");
                pEquip->lpItemSkill.push_back(std::move(pIS));
            }
        }

        // --- mSkillLevelBonus ---
        if (auto pBonus = pEpic->GetChild("skillLevelBonus")) {
            for (const auto& [sKey, pEntry] : pBonus->GetChildren()) {
                if (!pEntry) continue;
                auto nSkillID = get_child_int(pEntry, "id");
                auto nLevel   = get_child_int(pEntry, "level");
                pEquip->mSkillLevelBonus[nSkillID] = nLevel;
            }
        }
    }

    // --- lnOnlyUpgradeID ---
    if (auto pOnly = pInfo->GetChild("onlyUpgrade")) {
        for (const auto& [sKey, pEntry] : pOnly->GetChildren()) {
            if (!pEntry) continue;
            pEquip->lnOnlyUpgradeID.push_back(pEntry->GetInt());
        }
    }

    // --- aBonusExpRate ---
    if (auto pBonusExp = pInfo->GetChild("bonusExp")) {
        for (const auto& [sKey, pEntry] : pBonusExp->GetChildren()) {
            if (!pEntry) continue;
            auto nTermStart = get_child_int(pEntry, "termStart");
            auto nIncExpR   = get_child_int(pEntry, "incExpR");
            pEquip->aBonusExpRate.emplace_back(nTermStart, nIncExpR);
            auto nTermEnd = get_child_int(pEntry, "termEnd");
            if (nTermEnd)
                pEquip->aBonusExpRate.emplace_back(nTermEnd, 0);
        }
        // Sort by second (rate) then by first (term) — matches original PairSecondLess then PairFirstLess
        std::stable_sort(pEquip->aBonusExpRate.begin(), pEquip->aBonusExpRate.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });
        std::stable_sort(pEquip->aBonusExpRate.begin(), pEquip->aBonusExpRate.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
    }

    // --- aTamingMobItem (for items where nItemID/10000 == 191) ---
    if (nItemID / 10000 == 191) {
        for (const auto& [sKey, pEntry] : pProp->GetChildren()) {
            if (sKey == "info") continue;
            auto nTamingID = std::stoi(sKey);
            if (nTamingID / 10000 == 190)
                pEquip->aTamingMobItem.push_back(nTamingID);
        }
    }

    // --- uPetTemplateFlag (CFlag<512> for pet equips) ---
    if (nItemID / 10000 == 180) {
        auto nSub = nItemID % 1000;
        if (nSub < 100 || (nItemID - 1800000 >= 2300 && nItemID - 1800000 < 3000)) {
            for (const auto& [sKey, pEntry] : pProp->GetChildren()) {
                if (sKey == "info") continue;
                auto nPetID = std::stoi(sKey);
                auto nBit = nPetID % 1000;
                if (nBit >= 0 && nBit < 512)
                    pEquip->uPetTemplateFlag[nBit / 32] |= (1u << (31 - (nBit & 0x1F)));
            }
        }
    }

    // --- DayOfWeekItemStat ---
    if (auto pDOW = pInfo->GetChild("dayOfWeek")) {
        pEquip->bDayOfWeekItemStat = 1;
        for (const auto& [sKey, pDay] : pDOW->GetChildren()) {
            if (!pDay) continue;
            auto nDay = std::stoi(sKey);
            if (nDay >= 0 && nDay < 7)
                pEquip->aDayOfWeekItemStat[nDay].nDOWIMDR = get_child_int(pDay, "imdR");
        }
    }

    // --- VariableStat ---
    if (auto pVar = pInfo->GetChild("variableStat")) {
        auto pVS = std::make_shared<VariableStat>();
        pVS->nPAD = static_cast<float>(get_child_double(pVar, "incPAD"));
        pVS->nMAD = static_cast<float>(get_child_double(pVar, "incMAD"));
        pVS->nPDD = static_cast<float>(get_child_double(pVar, "incPDD"));
        pVS->nMDD = static_cast<float>(get_child_double(pVar, "incMDD"));
        pVS->nACC = static_cast<float>(get_child_double(pVar, "incACC"));
        pVS->nEVA = static_cast<float>(get_child_double(pVar, "incEVA"));
        pVS->nSTR = static_cast<float>(get_child_double(pVar, "incSTR"));
        pVS->nDEX = static_cast<float>(get_child_double(pVar, "incDEX"));
        pVS->nLUK = static_cast<float>(get_child_double(pVar, "incLUK"));
        pVS->nINT = static_cast<float>(get_child_double(pVar, "incINT"));
        pVS->nMHP = static_cast<float>(get_child_double(pVar, "incMHP"));
        pVS->nMMP = static_cast<float>(get_child_double(pVar, "incMMP"));
        pEquip->pVariableStat = std::move(pVS);
    }

    // --- TextEquipParam ---
    if (pEquip->bText) {
        if (auto pText = pInfo->GetChild("text")) {
            auto pTP = std::make_shared<TextEquipParam>();
            pTP->nTextEquipColor    = get_child_int(pText, "textColor", -1);
            pTP->nTextEquipOffsetX  = get_child_int(pText, "textOffsetX", 7);
            pTP->nTextEquipOffsetY  = get_child_int(pText, "textOffsetY", 7);
            pTP->nTextEquipFontSize = get_child_int(pText, "textFontSize", 11);
            pTP->nTextEquipAreaX    = get_child_int(pText, "textAreaX", 68);
            pTP->nTextEquipAreaY    = get_child_int(pText, "textAreaY", 25);
            pEquip->pTextEquipParam = std::move(pTP);
        }
    }

    // --- GrowthOption ---
    if (auto pGrowth = pInfo->GetChild("growth")) {
        auto pGO = std::make_shared<GrowthOption>();
        pGO->nType           = get_child_int(pGrowth, "type");
        pGO->bLevelUpByPoint = get_child_int(pGrowth, "levelUpByPoint");
        pGO->bFixLevel       = get_child_int(pGrowth, "fixLevel");
        if (auto pPool = pGrowth->GetChild("levelUpTypePool")) {
            for (const auto& [k, v] : pPool->GetChildren()) {
                if (v) pGO->anLevelUpTypePool.push_back(v->GetInt());
            }
        }
        if (auto pLevels = pGrowth->GetChild("level")) {
            for (const auto& [k, pLI] : pLevels->GetChildren()) {
                if (!pLI) continue;
                auto p = std::make_shared<LevelInfo>();
                p->nLevel        = get_child_int(pLI, "level");
                p->nLevelUpType  = get_child_int(pLI, "levelUpType");
                p->nLevelUpValue = get_child_int(pLI, "levelUpValue");
                pGO->apLevelInfo.push_back(std::move(p));
            }
        }
        pEquip->pGrowth = std::move(pGO);
    }

    // --- EquipDrop (from pInfo sub-tree) ---
    if (auto pDrop = pInfo->GetChild("equipDrop")) {
        pEquip->nEquipDropRate           = get_child_int(pDrop, "rate");
        pEquip->nEquipDropFieldStart     = get_child_int(pDrop, "fieldStart");
        pEquip->nEquipDropFieldEnd       = get_child_int(pDrop, "fieldEnd");
        pEquip->nEquipDropExceptMobStart = get_child_int(pDrop, "exceptMobStart");
        pEquip->nEquipDropExceptMobEnd   = get_child_int(pDrop, "exceptMobEnd");
    }

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

    // --- Stats / Requirements (extended) ---
    pBundle->dSellUnitPrice = static_cast<long double>(get_child_double(pInfo, "unitPrice"));
    pBundle->bAutoPrice     = get_child_int(pInfo, "autoPrice") != 0;
    pBundle->nMax           = static_cast<std::int16_t>(get_child_int(pInfo, "maxCount"));

    // --- Restrictions ---
    pBundle->bNoCancelMouse = get_child_int(pInfo, "noCancelMouse") != 0;
    pBundle->bNoCancel      = get_child_int(pInfo, "noCancel");
    pBundle->bNoPickupByPet = get_child_int(pInfo, "notPickupByPet") != 0;

    // --- Trade ---
    pBundle->nAppliableKarmaType       = get_child_int(pInfo, "karmaType");
    pBundle->bAccountShareTagApplicable = get_child_int(pInfo, "accountShareTag");
    pBundle->bSharableOnce             = get_child_int(pInfo, "sharableOnce");
    pBundle->bUseBinded                = get_child_int(pInfo, "useTradeBlock");

    // --- Quest / Monster ---
    pBundle->bPartyQuest         = get_child_int(pInfo, "pquest");
    pBundle->nReqQuestOnProgress = get_child_int(pInfo, "reqQuestOnProgress");
    pBundle->nLevel              = get_child_int(pInfo, "lv");
    pBundle->nMCType             = get_child_int(pInfo, "mcType");
    pBundle->nQuestID            = get_child_int(pInfo, "questId");
    pBundle->bUpdateExp          = get_child_int(pInfo, "exp");
    pBundle->nMobID              = get_child_int(pInfo, "mobId");
    pBundle->bMonsterBookCard    = get_child_int(pInfo, "monsterBook");

    // --- Replace ---
    pBundle->nReplaceItemID = get_child_int(pInfo, "replaceItemId");
    pBundle->sReplaceMsg    = get_child_string(pInfo, "replaceMsg");
    pBundle->nReplacePeriod = get_child_int(pInfo, "replacePeriod");

    // --- Account sharing ---
    pBundle->sCantAccountSharableToolTip = get_child_string(pInfo, "cantAccountSharable");
    pBundle->sCanAccountSharableToolTip  = get_child_string(pInfo, "canAccountSharable");

    // --- EXP / Level / Time ---
    pBundle->nExpMinLev    = get_child_int(pInfo, "minLev");
    pBundle->nExpMaxLev    = get_child_int(pInfo, "maxLev");
    pBundle->nPointCost    = get_child_int(pInfo, "pointCost");
    pBundle->bRelaxEXP     = get_child_int(pInfo, "relaxEXP");
    pBundle->nBonusEXPRate = get_child_int(pInfo, "bonusEXPRate");

    // --- Personality EXP ---
    pBundle->nCharismaEXP = get_child_int(pInfo, "charismaEXP");
    pBundle->nInsightEXP  = get_child_int(pInfo, "insightEXP");
    pBundle->nWillEXP     = get_child_int(pInfo, "willEXP");
    pBundle->nCraftEXP    = get_child_int(pInfo, "craftEXP");
    pBundle->nSenseEXP    = get_child_int(pInfo, "senseEXP");
    pBundle->nCharmEXP    = get_child_int(pInfo, "charmEXP");

    // --- Nick skill / Reward / Enchant ---
    pBundle->bNickSkillTimeLimited = get_child_int(pInfo, "nickSkillTimeLimited");
    pBundle->nNickSkill            = get_child_int(pInfo, "nickSkill");
    pBundle->nRewardItemID         = get_child_int(pInfo, "rewardItemID");
    pBundle->nEnchantSkill         = get_child_int(pInfo, "enchantSkill");
    pBundle->nEndUseDate           = static_cast<std::uint64_t>(get_child_int(pInfo, "endUseDate"));

    // --- Soul / Bonus / Emotion ---
    pBundle->nSoulItemType      = get_child_int(pInfo, "soulItemType");
    pBundle->dwSummonSoulMobID  = static_cast<std::uint32_t>(get_child_int(pInfo, "summonSoulMobID"));
    pBundle->bBonusStage        = get_child_int(pInfo, "bonusStageItem");
    pBundle->bMorphItem         = get_child_int(pInfo, "morphItem");
    pBundle->nEmotion           = get_child_int(pInfo, "emotion");

    // --- Chair (UseMesoChair sub-fields) ---
    if (auto pMesoChair = pInfo->GetChild("mesoChair")) {
        pBundle->stUseMesoChair.nUseMeso       = get_child_int(pMesoChair, "useMeso");
        pBundle->stUseMesoChair.nUseMesoTick   = get_child_int(pMesoChair, "useMesoTick");
        pBundle->stUseMesoChair.nUseMesoMax    = get_child_int(pMesoChair, "useMesoMax");
        pBundle->stUseMesoChair.nUseMesoSaveQr = get_child_int(pMesoChair, "useMesoSaveQr");
    }

    // --- Type ---
    pBundle->nBagType = get_child_int(pInfo, "bagType");

    // ============================================================
    // Complex sub-property loaders (require child iteration)
    // ============================================================

    // --- lReqField (list of required field/map IDs) ---
    if (auto pReqField = pInfo->GetChild("reqField")) {
        for (const auto& [sKey, pEntry] : pReqField->GetChildren()) {
            if (!pEntry) continue;
            pBundle->lReqField.push_back(static_cast<std::uint32_t>(pEntry->GetInt()));
        }
    }
    pBundle->nReqFieldS = get_child_int(pInfo, "reqFieldStart");
    pBundle->nReqFieldE = get_child_int(pInfo, "reqFieldEnd");

    // --- Job restriction maps ---
    auto load_job_map = [](const std::shared_ptr<WzProperty>& pNode,
                           std::map<std::int32_t, std::int32_t>& mOut) {
        if (!pNode) return;
        for (const auto& [sKey, pEntry] : pNode->GetChildren()) {
            if (!pEntry) continue;
            mOut[std::stoi(sKey)] = pEntry->GetInt();
        }
    };
    load_job_map(pInfo->GetChild("cantAccountSharableJob"), pBundle->mCantAccountSharableJob);
    load_job_map(pInfo->GetChild("canAccountSharableJob"),  pBundle->mCanAccountSharableJob);
    load_job_map(pInfo->GetChild("canUseJob"),              pBundle->mCanUseJob);

    // --- mLvUpWarning ---
    if (auto pWarn = pInfo->GetChild("lvUpWarning")) {
        for (const auto& [sKey, pEntry] : pWarn->GetChildren()) {
            if (!pEntry) continue;
            pBundle->mLvUpWarning[std::stoi(sKey)] = pEntry->GetString();
        }
    }

    // --- Level / Time limits ---
    // Original @ 0xAF57A0: only read for Use items (type 2), category 414,
    // or get_etc_cash_item_type() == 7. Values clamped to >= 0.
    auto nCategory = nItemID / 10000;
    if (helper::GetItemType(nItemID) == helper::kConsume || nCategory == 414
        || helper::get_etc_cash_item_type(nItemID) == 7)
    {
        pBundle->nLimitMin = std::max(0, get_child_int(pInfo, "limitMin"));
        pBundle->nLimitSec = std::max(0, get_child_int(pInfo, "limitSec"));
    }

    // Insert into cache and return
    auto [inserted, _] = m_mBundleItem.emplace(nItemID, std::move(pBundle));
    return inserted->second.get();
}

// ============================================================
// IterateItemInfo @ 0xafb5d0
// Master initialization function. Loads all item data tables
// from WZ files. Currently implements item string loading;
// other sub-loaders will be added as needed.
// ============================================================
auto ItemInfo::IterateItemInfo() -> bool
{
    LoadItemSellPriceByLv();
    IterateMapString();
    IterateItemString();
    IterateSkillCastItem();
    IterateItemNameForScanner();
    RegisterSetItemEffect();
    RegisterSetItemInfo();
    RegisterGroupEffectInfo();
    RegisterSetItemAction();
    RegisterPieceItemInfo();
    RegisterGachaponItemInfo();
    IterateCashBundleItem();
    IterateBridleItem();
    IterateExtendExpireDateItem();
    IterateBagItem();
    IterateCashItemTag();
    RegisterPremiumMapTransferBasicMap();
    RegisterExclusiveEquipInfo();
    IterateEquipSlotLevelMinusItem();
    IterateCoreItem();
    IterateBitsCaseItem();
    RegisterSetTowerChairInfo();
    RegisterEventNameTagString();
    LoadRequirePoint();
    LoadItemIDSet();
    return true;
}

// ============================================================
// IterateItemString @ 0xae3ee0  (no-arg)
// Loads item strings from 6 WZ String archives into
// m_mItemString. Each archive contains item names, descs, etc.
// ============================================================
void ItemInfo::IterateItemString()
{
    auto& wzResMan = WzResMan::GetInstance();

    // StringPool 0x185A..0x185F
    static const char* const sPaths[] = {
        "String/Eqp.img",      // 0x185A
        "String/Consume.img",  // 0x185B
        "String/Ins.img",      // 0x185C
        "String/Cash.img",     // 0x185D
        "String/Pet.img",      // 0x185E
        "String/Etc.img",      // 0x185F
    };

    for (auto* sPath : sPaths)
    {
        auto pProp = wzResMan.GetProperty(sPath);
        if (pProp)
            IterateItemString(pProp);
    }
}

// ============================================================
// IterateItemString @ 0xad4890  (recursive, with WzProperty)
// Recursively walks a WZ String property tree.
// Intermediate nodes (directories like "Eqp/Hat") are recursed.
// Leaf nodes (item IDs with string children) are inserted into
// m_mItemString as {nItemID -> {"name" -> "...", "desc" -> "..."}}.
// ============================================================
void ItemInfo::IterateItemString(const std::shared_ptr<WzProperty>& pProp)
{
    if (!pProp)
        return;

    auto& wzResMan = WzResMan::GetInstance();

    for (const auto& [sChildName, pChild] : pProp->GetChildren())
    {
        if (!pChild)
            continue;

        if (pChild->HasChildren())
        {
            // Check if this is a leaf item node (children are string values)
            // or an intermediate directory (children have their own children).
            // Peek at the first child to decide: if the first child has no
            // children itself, this is a leaf item node.
            bool bLeaf = false;
            for (const auto& [sGrandName, pGrandchild] : pChild->GetChildren())
            {
                (void)sGrandName;
                bLeaf = !pGrandchild || !pGrandchild->HasChildren();
                break;
            }

            if (bLeaf)
            {
                // Leaf item node: parse item ID from name
                std::int32_t nItemID = 0;
                try {
                    nItemID = std::stoi(sChildName);
                } catch (...) {
                    continue;
                }

                // Insert all string key-value pairs for this item
                auto& mStrings = m_mItemString[nItemID];
                for (const auto& [sKey, pValue] : pChild->GetChildren())
                {
                    if (pValue)
                        mStrings[sKey] = pValue->GetString();
                }
            }
            else
            {
                // Intermediate directory: recurse deeper
                IterateItemString(pChild);
            }
        }
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// LoadItemSellPriceByLv @ 0xaf50d0
// Reads "Item/ItemSellPriceStandard.img" and populates the
// two-level sell price map: category → level → price.
// ============================================================
void ItemInfo::LoadItemSellPriceByLv()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Item/ItemSellPriceStandard.img");
    if (!pProp)
        return;

    for (const auto& [sCategoryName, pCategory] : pProp->GetChildren())
    {
        if (!pCategory)
            continue;

        std::int32_t nCategory = 0;
        try { nCategory = std::stoi(sCategoryName); } catch (...) { continue; }

        for (const auto& [sLevelName, pLevel] : pCategory->GetChildren())
        {
            if (!pLevel)
                continue;

            std::int32_t nLevel = 0;
            try { nLevel = std::stoi(sLevelName); } catch (...) { continue; }

            m_mItemSellPriceByLv[nCategory][nLevel] = pLevel->GetInt();
        }
    }
}

// ============================================================
// IterateMapString @ 0xae4a50  (no-arg)
// Loads map strings from "String/Map.img" into m_mMapString.
// StringPool 0xECE = "String/Map.img"
// ============================================================
void ItemInfo::IterateMapString()
{
    auto& wzResMan = WzResMan::GetInstance();
    m_bReleaseMapString = false;

    auto pProp = wzResMan.GetProperty("String/Map.img");
    if (pProp)
        IterateMapString(pProp);
}

// ============================================================
// IterateMapString @ 0xad62c0  (recursive, with WzProperty)
// Same structure as IterateItemString(pProp) but populates
// m_mMapString (uint32_t key → map of string key-value pairs).
// ============================================================
void ItemInfo::IterateMapString(const std::shared_ptr<WzProperty>& pProp)
{
    if (!pProp)
        return;

    auto& wzResMan = WzResMan::GetInstance();

    for (const auto& [sChildName, pChild] : pProp->GetChildren())
    {
        if (!pChild || !pChild->HasChildren())
            continue;

        // Leaf vs directory check (same heuristic as IterateItemString)
        bool bLeaf = false;
        for (const auto& [sGrandName, pGrandchild] : pChild->GetChildren())
        {
            (void)sGrandName;
            bLeaf = !pGrandchild || !pGrandchild->HasChildren();
            break;
        }

        if (bLeaf)
        {
            std::uint32_t nMapID = 0;
            try { nMapID = static_cast<std::uint32_t>(std::stoul(sChildName)); } catch (...) { continue; }

            auto& mStrings = m_mMapString[nMapID];
            for (const auto& [sKey, pValue] : pChild->GetChildren())
            {
                if (pValue)
                    mStrings[sKey] = pValue->GetString();
            }
        }
        else
        {
            IterateMapString(pChild);
        }
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// IterateSkillCastItem @ 0xaa9b60
// Reads "Item/Consume/0252.img" and maps each item's
// skillId → itemID into m_mSkillID_CastItemID.
// ============================================================
void ItemInfo::IterateSkillCastItem()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Item/Consume/0252.img");
    if (!pProp)
        return;

    for (const auto& [sItemName, pItem] : pProp->GetChildren())
    {
        if (!pItem)
            continue;

        std::int32_t nItemID = 0;
        try { nItemID = std::stoi(sItemName); } catch (...) { continue; }

        auto pInfo = pItem->GetChild("info");
        if (!pInfo)
            continue;

        auto nSkillID = get_child_int(pInfo, "skillId");
        if (nSkillID != 0)
            m_mSkillID_CastItemID[nSkillID] = nItemID;
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// LoadItemIDSet @ 0xacbc90
// Populates m_sItemID from all item IDs in m_mItemString.
// Must be called after IterateItemString.
// ============================================================
void ItemInfo::LoadItemIDSet()
{
    for (const auto& [nItemID, mStrings] : m_mItemString)
    {
        (void)mStrings;
        m_sItemID.insert(nItemID);
    }
}

// ============================================================
// RegisterPremiumMapTransferBasicMap @ 0xab4c30
// Reads item 5041001 (premium teleport rock) to populate
// m_mPremiumMapTransferBasicMap with basic allowed map IDs.
// ============================================================
void ItemInfo::RegisterPremiumMapTransferBasicMap()
{
    auto pProp = GetItemProp(5041001);
    if (!pProp)
        return;

    auto pInfo = pProp->GetChild("info");
    if (!pInfo)
        return;

    auto pBasic = pInfo->GetChild("basic");
    if (!pBasic)
        return;

    for (const auto& [sName, pChild] : pBasic->GetChildren())
    {
        if (!pChild)
            continue;

        auto nMapID = static_cast<std::uint32_t>(pChild->GetInt());
        m_mPremiumMapTransferBasicMap[nMapID] = 1;
    }
}

// ============================================================
// LoadRequirePoint @ 0xad3080
// Reads inner ability require point tables from
// "Etc/InnerAbility.img/OptionRequirePoint".
// ============================================================
void ItemInfo::LoadRequirePoint()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Etc/InnerAbility.img");
    if (!pProp)
        return;

    auto pRequirePoint = pProp->GetChild("OptionRequirePoint");
    if (!pRequirePoint)
        return;

    // Load rate points
    auto pRate = pRequirePoint->GetChild("Rate");
    if (pRate)
    {
        for (const auto& [sName, pChild] : pRate->GetChildren())
        {
            if (pChild)
                m_vRequireRatePoint.push_back(pChild->GetInt());
        }
    }

    // Load ability points
    auto pAbility = pRequirePoint->GetChild("Ability");
    if (pAbility)
    {
        for (const auto& [sName, pChild] : pAbility->GetChildren())
        {
            if (pChild)
                m_vRequireAbilityPoint.push_back(pChild->GetInt());
        }
    }
}

// ============================================================
// RegisterSetItemEffect @ 0xac4a00
// Reads "Effect/SetEff.img" and populates m_lSetItemEffect
// with set item visual effect definitions.
// ============================================================
void ItemInfo::RegisterSetItemEffect()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Effect/SetEff.img");
    if (!pProp)
        return;

    for (const auto& [sSetName, pSet] : pProp->GetChildren())
    {
        if (!pSet)
            continue;

        SetEffect eff{};
        eff.nCash = get_child_int(pSet, "cash");

        std::int32_t nSetID = 0;
        try { nSetID = std::stoi(sSetName); } catch (...) { continue; }
        eff.nSetID = nSetID;

        // Load item lists per slot count (0..31)
        for (int i = 0; i < 32; ++i)
        {
            auto pSlot = pSet->GetChild(std::to_string(i));
            if (!pSlot)
                continue;
            for (const auto& [sIdx, pItem] : pSlot->GetChildren())
            {
                if (pItem)
                    eff.alItemList[i].push_back(pItem->GetInt());
            }
        }

        // Load mechanic item lists
        auto pMech = pSet->GetChild("Mechanic");
        if (pMech)
        {
            for (int i = 0; i < 5; ++i)
            {
                auto pSlot = pMech->GetChild(std::to_string(i));
                if (!pSlot)
                    continue;
                for (const auto& [sIdx, pItem] : pSlot->GetChildren())
                {
                    if (pItem)
                        eff.alMechanicItemList[i].push_back(pItem->GetInt());
                }
            }
        }

        m_lSetItemEffect.push_back(std::move(eff));
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// RegisterGroupEffectInfo @ 0xac33b0
// Reads "Etc/GroupEffectInfo.img" and populates
// m_mGroupEffectInfo with group effect definitions.
// ============================================================
void ItemInfo::RegisterGroupEffectInfo()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Etc/GroupEffectInfo.img");
    if (!pProp)
        return;

    for (const auto& [sName, pGroup] : pProp->GetChildren())
    {
        if (!pGroup)
            continue;

        std::int32_t nEffectID = 0;
        try { nEffectID = std::stoi(sName); } catch (...) { continue; }

        auto pInfo = std::make_shared<GroupEffectInfo>();
        pInfo->nEffectID = nEffectID;
        pInfo->nGroupID = get_child_int(pGroup, "groupID");
        pInfo->bOneToOne = get_child_int(pGroup, "oneToOne");
        pInfo->nCompleteCount = get_child_int(pGroup, "completeCount");
        pInfo->nEffectCount = get_child_int(pGroup, "effectCount");
        pInfo->nDistanceX = get_child_int(pGroup, "distanceX");
        pInfo->nDistanceY = get_child_int(pGroup, "distanceY");

        auto pItems = pGroup->GetChild("itemID");
        if (pItems)
        {
            for (const auto& [sIdx, pItem] : pItems->GetChildren())
            {
                if (pItem)
                    pInfo->anItemID.push_back(pItem->GetInt());
            }
        }

        m_mGroupEffectInfo[nEffectID] = std::move(pInfo);
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// RegisterSetItemAction @ 0xacff40
// Reads "Etc/SetItemActionKR.img" and populates
// m_lSetItemAction with set item action definitions.
// ============================================================
void ItemInfo::RegisterSetItemAction()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Etc/SetItemActionKR.img");
    if (!pProp)
        return;

    for (const auto& [sName, pAction] : pProp->GetChildren())
    {
        if (!pAction)
            continue;

        SetAction action{};
        action.sCommand = get_child_string(pAction, "command");
        action.bsActionName = get_child_string(pAction, "actionName");

        for (std::size_t i = 0; i < 32; ++i)
        {
            action.aItem[i] = get_child_int(pAction, std::to_string(i));
        }

        m_lSetItemAction.push_back(std::move(action));
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// RegisterPieceItemInfo @ 0xaf9470
// Reads "Etc/PieceItemInfo.img" and populates
// m_mPieceItemInfo with piece (collection) item data.
// ============================================================
void ItemInfo::RegisterPieceItemInfo()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Etc/PieceItemInfo.img");
    if (!pProp)
        return;

    for (const auto& [sName, pPiece] : pProp->GetChildren())
    {
        if (!pPiece)
            continue;

        std::int32_t nItemID = 0;
        try { nItemID = std::stoi(sName); } catch (...) { continue; }

        auto pInfo = std::make_shared<PieceItemInfo>();
        pInfo->nRewardItemID = get_child_int(pPiece, "rewardItemID");
        pInfo->nCompleteCount = get_child_int(pPiece, "completeCount");
        pInfo->sUIPath = get_child_string(pPiece, "uiPath");

        auto pFixed = pPiece->GetChild("fixedItemID");
        if (pFixed)
        {
            for (const auto& [sIdx, pItem] : pFixed->GetChildren())
            {
                if (pItem)
                    pInfo->anFixedItemID.push_back(pItem->GetInt());
            }
        }

        m_mPieceItemInfo[nItemID] = std::move(pInfo);
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// RegisterSetTowerChairInfo @ 0xac2a10
// Reads "Etc/SetTowerChairInfo.img" and populates
// m_mSetTowerChairInfo and m_mSetTowerChairItemInfo.
// ============================================================
void ItemInfo::RegisterSetTowerChairInfo()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Etc/SetTowerChairInfo.img");
    if (!pProp)
        return;

    for (const auto& [sName, pSet] : pProp->GetChildren())
    {
        if (!pSet)
            continue;

        std::int32_t nSetID = 0;
        try { nSetID = std::stoi(sName); } catch (...) { continue; }

        auto pInfo = std::make_shared<SetTowerChair>();
        pInfo->nSetTowerChairID = nSetID;

        auto pItems = pSet->GetChild("itemID");
        if (pItems)
        {
            for (const auto& [sIdx, pItem] : pItems->GetChildren())
            {
                if (!pItem)
                    continue;
                auto nItemID = pItem->GetInt();
                pInfo->aItemID.push_back(nItemID);
                m_mSetTowerChairItemInfo[nItemID] = nSetID;
            }
        }

        m_mSetTowerChairInfo[nSetID] = std::move(pInfo);
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// RegisterExclusiveEquipInfo @ 0xae5690
// Reads "Etc/ExclusiveEquip.img" and populates
// m_mExclusiveEquip and related exclusive equip maps.
// ============================================================
void ItemInfo::RegisterExclusiveEquipInfo()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Etc/ExclusiveEquip.img");
    if (!pProp)
        return;

    for (const auto& [sName, pGroup] : pProp->GetChildren())
    {
        if (!pGroup)
            continue;

        std::int32_t nGroupID = 0;
        try { nGroupID = std::stoi(sName); } catch (...) { continue; }

        auto sGroupName = get_child_string(pGroup, "name");
        auto sGroupCategory = get_child_string(pGroup, "category");
        auto sGroupString = get_child_string(pGroup, "string");

        auto pItems = pGroup->GetChild("item");
        if (pItems)
        {
            for (const auto& [sIdx, pItem] : pItems->GetChildren())
            {
                if (!pItem)
                    continue;
                auto nItemID = pItem->GetInt();
                m_mExclusiveEquip[nItemID] = nGroupID;
                m_mExclusiveEquipName[nItemID] = sGroupName;
                m_mExclusiveEquipCategory[nItemID] = sGroupCategory;
                m_mExclusiveEquipString[nItemID] = sGroupString;
            }
        }
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// RegisterEventNameTagString @ 0xab8cb0
// Reads "Etc/EventNameTag.img" and populates
// m_aaEventNameTagString with event name tag arrays.
// ============================================================
void ItemInfo::RegisterEventNameTagString()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Etc/EventNameTag.img");
    if (!pProp)
        return;

    for (const auto& [sName, pGroup] : pProp->GetChildren())
    {
        if (!pGroup)
            continue;

        std::vector<std::string> aNames;
        for (const auto& [sIdx, pName] : pGroup->GetChildren())
        {
            if (pName)
                aNames.push_back(pName->GetString());
        }

        m_aaEventNameTagString.push_back(std::move(aNames));
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// IterateBridleItem @ 0xacdc50
// Reads "Item/Consume/0227.img" and populates
// m_mBridleItem with monster capture item data.
// ============================================================
void ItemInfo::IterateBridleItem()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Item/Consume/0227.img");
    if (!pProp)
        return;

    for (const auto& [sItemName, pItem] : pProp->GetChildren())
    {
        if (!pItem)
            continue;

        std::int32_t nItemID = 0;
        try { nItemID = std::stoi(sItemName); } catch (...) { continue; }

        auto pInfo = pItem->GetChild("info");
        if (!pInfo)
            continue;

        auto pBridle = std::make_shared<BridleItem>();
        pBridle->nItemID = nItemID;
        pBridle->dwTargetMobID = static_cast<std::uint32_t>(get_child_int(pInfo, "mob"));
        pBridle->nCreateItemID = get_child_int(pInfo, "createItem");
        pBridle->nCreateItemPeriod = get_child_int(pInfo, "createPeriod");
        pBridle->nCatchPercentageHP = get_child_int(pInfo, "mobHP");
        pBridle->nBridleMsgType = get_child_int(pInfo, "bridleMsgType");
        pBridle->fBridleProb = static_cast<float>(get_child_double(pInfo, "bridleProb"));
        pBridle->fBridleProbAdj = static_cast<float>(get_child_double(pInfo, "bridleProbAdj"));
        pBridle->tUseDelay = static_cast<std::uint32_t>(get_child_int(pInfo, "useDelay"));
        pBridle->sDeleyMsg = get_child_string(pInfo, "delayMsg");
        pBridle->sNoMobMsg = get_child_string(pInfo, "noMobMsg");

        // Parse rect
        pBridle->rc.left = get_child_int(pInfo, "left");
        pBridle->rc.top = get_child_int(pInfo, "top");
        pBridle->rc.right = get_child_int(pInfo, "right");
        pBridle->rc.bottom = get_child_int(pInfo, "bottom");

        m_mBridleItem[nItemID] = std::move(pBridle);
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// IterateExtendExpireDateItem @ 0xab37c0
// Reads "Item/Cash/0550.img" and populates
// m_mExtendExpireDateItem.
// ============================================================
void ItemInfo::IterateExtendExpireDateItem()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Item/Cash/0550.img");
    if (!pProp)
        return;

    for (const auto& [sItemName, pItem] : pProp->GetChildren())
    {
        if (!pItem)
            continue;

        std::int32_t nItemID = 0;
        try { nItemID = std::stoi(sItemName); } catch (...) { continue; }

        auto pInfo = pItem->GetChild("info");
        if (!pInfo)
            continue;

        auto pExtend = std::make_shared<ExtendExpireDateItem>();
        pExtend->nItemID = nItemID;
        pExtend->nExtendSeconds = get_child_int(pInfo, "extendSeconds");
        pExtend->nMaxExtendDays = get_child_int(pInfo, "maxExtendDays");
        pExtend->bEternity = get_child_int(pInfo, "eternity");

        m_mExtendExpireDateItem[nItemID] = std::move(pExtend);
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// IterateBagItem @ 0xab3de0
// Reads bag item data from 3 WZ archives and populates
// m_mBagItem.
// ============================================================
void ItemInfo::IterateBagItem()
{
    auto& wzResMan = WzResMan::GetInstance();

    static const char* const sPaths[] = {
        "Item/Consume/0033.img",
        "Item/Install/0303.img",
        "Item/Etc/0433.img",
    };

    for (auto* sPath : sPaths)
    {
        auto pProp = wzResMan.GetProperty(sPath);
        if (!pProp)
            continue;

        for (const auto& [sItemName, pItem] : pProp->GetChildren())
        {
            if (!pItem)
                continue;

            std::int32_t nItemID = 0;
            try { nItemID = std::stoi(sItemName); } catch (...) { continue; }

            auto pInfo = pItem->GetChild("info");
            if (!pInfo)
                continue;

            auto pBag = std::make_shared<BagInfo>();
            pBag->nItemID = nItemID;
            pBag->nSlotCount = get_child_int(pInfo, "slotCount");
            pBag->nBagType = get_child_int(pInfo, "bagType");
            pBag->nSlotPerLine = get_child_int(pInfo, "slotPerLine");

            m_mBagItem[nItemID] = std::move(pBag);
        }
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// IterateBitsCaseItem @ 0xabe750
// Reads "Item/Install/0309.img" and populates
// m_mBitsCaseItem with bits-case item data.
// ============================================================
void ItemInfo::IterateBitsCaseItem()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Item/Install/0309.img");
    if (!pProp)
        return;

    for (const auto& [sItemName, pItem] : pProp->GetChildren())
    {
        if (!pItem)
            continue;

        std::int32_t nItemID = 0;
        try { nItemID = std::stoi(sItemName); } catch (...) { continue; }

        auto pInfo = pItem->GetChild("info");
        if (!pInfo)
            continue;

        auto pBits = std::make_shared<BitsCaseItem>();
        pBits->nItemID = nItemID;
        pBits->nSlotCount = get_child_int(pInfo, "slotCount");
        pBits->nSlotPerLine = get_child_int(pInfo, "slotPerLine");

        m_mBitsCaseItem[nItemID] = std::move(pBits);
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// IterateCoreItem @ 0xaec0a0
// Reads "Item/Install/0360.img" and populates
// m_mCoreItem with core item data.
// ============================================================
void ItemInfo::IterateCoreItem()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Item/Install/0360.img");
    if (!pProp)
        return;

    for (const auto& [sItemName, pItem] : pProp->GetChildren())
    {
        if (!pItem)
            continue;

        std::int32_t nItemID = 0;
        try { nItemID = std::stoi(sItemName); } catch (...) { continue; }

        auto pInfo = pItem->GetChild("info");
        if (!pInfo)
            continue;

        auto pCore = std::make_shared<CoreItem>();
        pCore->nItemID = nItemID;

        auto& cs = pCore->coreSpec;
        cs.nShape = get_child_int(pInfo, "shape");
        cs.nCategory = get_child_int(pInfo, "category");
        cs.bNotConsume = get_child_int(pInfo, "notConsume") != 0;
        cs.nMobRate = get_child_int(pInfo, "mobRate");
        cs.nMobLevel = get_child_int(pInfo, "mobLevel");
        cs.nMobHPRate = get_child_int(pInfo, "mobHPRate");
        cs.nMobAttackRate = get_child_int(pInfo, "mobAttackRate");
        cs.nMobDefenseRate = get_child_int(pInfo, "mobDefenseRate");
        cs.nPartyExpRate = get_child_int(pInfo, "partyExpRate");
        cs.dwAddMob = static_cast<std::uint32_t>(get_child_int(pInfo, "addMob"));
        cs.sRewardDesc = get_child_string(pInfo, "rewardDesc");
        cs.nRewardType = get_child_int(pInfo, "rewardType");
        cs.bDropRareEquip = get_child_int(pInfo, "dropRareEquip") != 0;
        cs.nDropRate = get_child_int(pInfo, "dropRate");
        cs.nDropRateHerb = get_child_int(pInfo, "dropRateHerb");
        cs.nDropRateMineral = get_child_int(pInfo, "dropRateMineral");
        cs.sAddMissionDesc = get_child_string(pInfo, "addMissionDesc");
        cs.nAddMissionQuestID = get_child_int(pInfo, "addMissionQuestID");
        cs.nAddMissionMapID = get_child_int(pInfo, "addMissionMapID");
        cs.nMobRateSpecial = get_child_int(pInfo, "mobRateSpecial");
        cs.nPartyExpRateSpecial = get_child_int(pInfo, "partyExpRateSpecial");
        cs.nDropRateSpecial = get_child_int(pInfo, "dropRateSpecial");
        cs.sChangeMobDesc = get_child_string(pInfo, "changeMobDesc");
        cs.sChangeMob = get_child_string(pInfo, "changeMob");
        cs.sChangeBackGrndDesc = get_child_string(pInfo, "changeBackGrndDesc");
        cs.dwChangeBackGrnd = static_cast<std::uint32_t>(get_child_int(pInfo, "changeBackGrnd"));
        cs.sChangeBgmDesc = get_child_string(pInfo, "changeBgmDesc");
        cs.sChangeBgm = get_child_string(pInfo, "changeBgm");
        cs.nSkinCategory = get_child_int(pInfo, "skinCategory");

        // Load allowed map IDs
        auto pMaps = pInfo->GetChild("allowedMap");
        if (pMaps)
        {
            for (const auto& [sIdx, pMap] : pMaps->GetChildren())
            {
                if (pMap)
                    cs.anAllowedMapID.push_back(pMap->GetInt());
            }
        }

        // Load rewards
        auto pRewards = pInfo->GetChild("reward");
        if (pRewards)
        {
            for (const auto& [sIdx, pReward] : pRewards->GetChildren())
            {
                if (pReward)
                    cs.aReward.push_back(static_cast<std::uint32_t>(pReward->GetInt()));
            }
        }

        m_mCoreItem[nItemID] = std::move(pCore);
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// IterateEquipSlotLevelMinusItem @ 0xac5750
// Reads "Item/Consume/0292.img" and populates
// m_mEquipSlotLevelMinusItem.
// ============================================================
void ItemInfo::IterateEquipSlotLevelMinusItem()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Item/Consume/0292.img");
    if (!pProp)
        return;

    for (const auto& [sItemName, pItem] : pProp->GetChildren())
    {
        if (!pItem)
            continue;

        std::int32_t nItemID = 0;
        try { nItemID = std::stoi(sItemName); } catch (...) { continue; }

        auto pInfo = pItem->GetChild("info");
        if (!pInfo)
            continue;

        auto pSlotItem = std::make_shared<EquipSlotLevelMinusItem>();
        pSlotItem->nItemID = nItemID;
        pSlotItem->nAddTime = get_child_int(pInfo, "addTime");
        pSlotItem->nMaxDays = get_child_int(pInfo, "maxDays");
        pSlotItem->nMinusLevel = get_child_int(pInfo, "minusLevel");

        auto pSlots = pInfo->GetChild("selectedSlot");
        if (pSlots)
        {
            for (const auto& [sIdx, pSlot] : pSlots->GetChildren())
            {
                if (pSlot)
                    pSlotItem->aSelectedSlot.push_back(static_cast<std::int16_t>(pSlot->GetInt()));
            }
        }

        m_mEquipSlotLevelMinusItem[nItemID] = std::move(pSlotItem);
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// IterateCashItemTag @ 0xad1a90
// Reads "String/CashItemSearch.img" and populates
// m_CashItemTag with tag → item ID list mappings.
// ============================================================
void ItemInfo::IterateCashItemTag()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("String/CashItemSearch.img");
    if (!pProp)
        return;

    for (const auto& [sItemName, pItem] : pProp->GetChildren())
    {
        if (!pItem)
            continue;

        std::int32_t nItemID = 0;
        try { nItemID = std::stoi(sItemName); } catch (...) { continue; }

        for (const auto& [sKey, pTag] : pItem->GetChildren())
        {
            if (!pTag)
                continue;
            auto sTag = pTag->GetString();
            if (!sTag.empty())
                m_CashItemTag[sTag].push_back(nItemID);
        }
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// IterateCashBundleItem @ 0xacd240
// Iterates cash bundle items from "Item/Cash" archives
// and ensures they are registered in the BundleItem cache.
// ============================================================
void ItemInfo::IterateCashBundleItem()
{
    auto& wzResMan = WzResMan::GetInstance();

    // Cash items span category 050x through 056x
    static const char* const sPaths[] = {
        "Item/Cash/0501.img",
        "Item/Cash/0502.img",
        "Item/Cash/0503.img",
        "Item/Cash/0504.img",
        "Item/Cash/0505.img",
        "Item/Cash/0506.img",
        "Item/Cash/0507.img",
        "Item/Cash/0508.img",
        "Item/Cash/0509.img",
        "Item/Cash/0510.img",
        "Item/Cash/0512.img",
        "Item/Cash/0513.img",
        "Item/Cash/0514.img",
        "Item/Cash/0515.img",
        "Item/Cash/0517.img",
        "Item/Cash/0520.img",
        "Item/Cash/0521.img",
        "Item/Cash/0522.img",
        "Item/Cash/0528.img",
        "Item/Cash/0530.img",
        "Item/Cash/0533.img",
        "Item/Cash/0536.img",
        "Item/Cash/0537.img",
        "Item/Cash/0539.img",
        "Item/Cash/0545.img",
        "Item/Cash/0546.img",
        "Item/Cash/0547.img",
        "Item/Cash/0549.img",
        "Item/Cash/0550.img",
        "Item/Cash/0551.img",
        "Item/Cash/0552.img",
        "Item/Cash/0553.img",
        "Item/Cash/0554.img",
        "Item/Cash/0556.img",
        "Item/Cash/0557.img",
        "Item/Cash/0561.img",
        "Item/Cash/0562.img",
        "Item/Cash/0564.img",
    };

    for (auto* sPath : sPaths)
    {
        auto pProp = wzResMan.GetProperty(sPath);
        if (!pProp)
            continue;

        for (const auto& [sItemName, pItem] : pProp->GetChildren())
        {
            if (!pItem)
                continue;

            std::int32_t nItemID = 0;
            try { nItemID = std::stoi(sItemName); } catch (...) { continue; }

            // Ensure the bundle item is loaded into cache
            (void)GetBundleItem(nItemID);
        }
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// IterateItemNameForScanner @ 0xad0cf0
// Loads item names from all String archives and NoScan data
// from "Etc/ScanBlock.img" for the item scanner.
// ============================================================
void ItemInfo::IterateItemNameForScanner()
{
    if (m_bItemScannerInfoLoaded)
        return;

    m_bItemScannerInfoLoaded = true;
    auto& wzResMan = WzResMan::GetInstance();

    // Load names from all string archives
    static const char* const sPaths[] = {
        "String/Eqp.img",
        "String/Consume.img",
        "String/Ins.img",
        "String/Cash.img",
        "String/Pet.img",
        "String/Etc.img",
    };

    for (auto* sPath : sPaths)
    {
        auto pProp = wzResMan.GetProperty(sPath);
        if (pProp)
            IterateItemNameForScanner(pProp);
    }

    // Load no-scan items
    auto pScanBlock = wzResMan.GetProperty("Etc/ScanBlock.img");
    if (pScanBlock)
        LoadNoScanItem(pScanBlock);

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// IterateItemNameForScanner (recursive, with WzProperty)
// Walks WZ tree and appends {nItemID, sName} to
// m_lItemNameForScanner for all items that have a "name" child.
// ============================================================
void ItemInfo::IterateItemNameForScanner(const std::shared_ptr<WzProperty>& pProp)
{
    if (!pProp)
        return;

    for (const auto& [sChildName, pChild] : pProp->GetChildren())
    {
        if (!pChild || !pChild->HasChildren())
            continue;

        // Leaf vs directory check
        bool bLeaf = false;
        for (const auto& [sGrandName, pGrandchild] : pChild->GetChildren())
        {
            (void)sGrandName;
            bLeaf = !pGrandchild || !pGrandchild->HasChildren();
            break;
        }

        if (bLeaf)
        {
            std::int32_t nItemID = 0;
            try { nItemID = std::stoi(sChildName); } catch (...) { continue; }

            auto pName = pChild->GetChild("name");
            if (pName)
            {
                ItemName item;
                item.nItemID = nItemID;
                item.sItemName = pName->GetString();
                m_lItemNameForScanner.push_back(std::move(item));
            }
        }
        else
        {
            IterateItemNameForScanner(pChild);
        }
    }
}

// ============================================================
// LoadNoScanItem
// Reads "Etc/ScanBlock.img" and populates m_mNoScanItem
// with items that should not appear in scanner results.
// ============================================================
void ItemInfo::LoadNoScanItem(const std::shared_ptr<WzProperty>& pProp)
{
    if (!pProp)
        return;

    for (const auto& [sName, pChild] : pProp->GetChildren())
    {
        if (!pChild)
            continue;

        auto nItemID = pChild->GetInt();
        if (nItemID != 0)
            m_mNoScanItem[nItemID] = 1;
    }
}

// ============================================================
// RegisterSetItemInfo @ 0xaf1540
// Reads "Etc/SetItemInfo.img" and populates m_mSetItemInfo
// with complete set item definitions including effects.
// This is a complex function with many sub-properties.
// ============================================================
void ItemInfo::RegisterSetItemInfo()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Etc/SetItemInfo.img");
    if (!pProp)
        return;

    for (const auto& [sSetName, pSet] : pProp->GetChildren())
    {
        if (!pSet)
            continue;

        std::int32_t nSetItemID = 0;
        try { nSetItemID = std::stoi(sSetName); } catch (...) { continue; }

        auto pInfo = std::make_shared<SetItemInfo>();
        pInfo->nSetItemID = nSetItemID;
        pInfo->sSetItemName = get_child_string(pSet, "setItemName");
        pInfo->nSetCompleteCount = get_child_int(pSet, "completeCount");
        pInfo->bExpandToolTip = get_child_int(pSet, "expandToolTip");
        pInfo->bParts = get_child_int(pSet, "parts");
        pInfo->sWeaponDesc = get_child_string(pSet, "weaponDesc");
        pInfo->sSubWeaponDesc = get_child_string(pSet, "subWeaponDesc");

        // Load item IDs
        auto pItemIDs = pSet->GetChild("ItemID");
        if (pItemIDs)
        {
            for (const auto& [sIdx, pItem] : pItemIDs->GetChildren())
            {
                if (pItem)
                    pInfo->nItemID.push_back(pItem->GetInt());
            }
        }

        // Load effects per set count
        auto pEffect = pSet->GetChild("Effect");
        if (pEffect)
        {
            for (const auto& [sCount, pEff] : pEffect->GetChildren())
            {
                std::int32_t nCount = 0;
                try { nCount = std::stoi(sCount); } catch (...) { continue; }
                if (nCount < 0 || nCount >= 32)
                    continue;
                auto nIdx = static_cast<std::size_t>(nCount);

                auto pStat = std::make_shared<SetEffectStat>();
                pStat->niSTR = static_cast<std::int16_t>(get_child_int(pEff, "incSTR"));
                pStat->niDEX = static_cast<std::int16_t>(get_child_int(pEff, "incDEX"));
                pStat->niINT = static_cast<std::int16_t>(get_child_int(pEff, "incINT"));
                pStat->niLUK = static_cast<std::int16_t>(get_child_int(pEff, "incLUK"));
                pStat->niAllStat = static_cast<std::int16_t>(get_child_int(pEff, "incAllStat"));
                pStat->niMaxHP = static_cast<std::int16_t>(get_child_int(pEff, "incMHP"));
                pStat->niMaxMP = static_cast<std::int16_t>(get_child_int(pEff, "incMMP"));
                pStat->niMaxHPr = static_cast<std::int16_t>(get_child_int(pEff, "incMHPr"));
                pStat->niMaxMPr = static_cast<std::int16_t>(get_child_int(pEff, "incMMPr"));
                pStat->niPAD = static_cast<std::int16_t>(get_child_int(pEff, "incPAD"));
                pStat->niMAD = static_cast<std::int16_t>(get_child_int(pEff, "incMAD"));
                pStat->niPDD = static_cast<std::int16_t>(get_child_int(pEff, "incPDD"));
                pStat->niMDD = static_cast<std::int16_t>(get_child_int(pEff, "incMDD"));
                pStat->niACC = static_cast<std::int16_t>(get_child_int(pEff, "incACC"));
                pStat->niEVA = static_cast<std::int16_t>(get_child_int(pEff, "incEVA"));
                pStat->niCraft = static_cast<std::int16_t>(get_child_int(pEff, "incCraft"));
                pStat->niSpeed = static_cast<std::int16_t>(get_child_int(pEff, "incSpeed"));
                pStat->niJump = static_cast<std::int16_t>(get_child_int(pEff, "incJump"));
                pStat->nKnockback = get_child_int(pEff, "knockback");
                pStat->niPVPDamage = static_cast<std::int16_t>(get_child_int(pEff, "incPVPDamage"));
                pStat->niPQExpR = static_cast<std::int16_t>(get_child_int(pEff, "incPQExpR"));

                pInfo->pEffect[nIdx] = std::move(pStat);
            }
        }

        // Load parts
        if (pInfo->bParts)
        {
            auto pParts = pSet->GetChild("Parts");
            if (pParts)
            {
                for (const auto& [sIdx, pPart] : pParts->GetChildren())
                {
                    if (!pPart)
                        continue;

                    std::list<std::int32_t> aPartItems;
                    for (const auto& [sPartIdx, pPartItem] : pPart->GetChildren())
                    {
                        if (pPartItem)
                            aPartItems.push_back(pPartItem->GetInt());
                    }
                    pInfo->alParts.push_back(std::move(aPartItems));
                }
            }
        }

        // Load type/part names
        auto pTypeName = pSet->GetChild("typeName");
        if (pTypeName)
        {
            for (const auto& [sIdx, pName] : pTypeName->GetChildren())
            {
                if (pName)
                    pInfo->asTypeName.push_back(pName->GetString());
            }
        }

        auto pPartsName = pSet->GetChild("partsName");
        if (pPartsName)
        {
            for (const auto& [sIdx, pName] : pPartsName->GetChildren())
            {
                if (pName)
                    pInfo->asPartsName.push_back(pName->GetString());
            }
        }

        m_mSetItemInfo[nSetItemID] = std::move(pInfo);
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// RegisterGachaponItemInfo @ 0xae0a80
// Reads "Etc/incubatorInfo.img" and populates
// m_mGachaponItemInfo with gachapon definitions.
// ============================================================
void ItemInfo::RegisterGachaponItemInfo()
{
    auto& wzResMan = WzResMan::GetInstance();
    auto pProp = wzResMan.GetProperty("Etc/incubatorInfo.img");
    if (!pProp)
        return;

    for (const auto& [sName, pGacha] : pProp->GetChildren())
    {
        if (!pGacha)
            continue;

        std::int32_t nItemID = 0;
        try { nItemID = std::stoi(sName); } catch (...) { continue; }

        auto pInfo = std::make_shared<GachaponItemInfo>();
        pInfo->bBonus = get_child_int(pGacha, "bonus");
        pInfo->bReplacedProb = get_child_int(pGacha, "replacedProb");
        pInfo->bNoGradeResult = get_child_int(pGacha, "noGradeResult");
        pInfo->bSelfSelectReward = get_child_int(pGacha, "selfSelectReward");
        pInfo->nFixedSelectReward = get_child_int(pGacha, "fixedSelectReward");
        pInfo->nSucessNpcID = get_child_int(pGacha, "sucessNpcID");
        pInfo->nGaugenQRID = get_child_int(pGacha, "gaugenQRID");
        pInfo->nGaugeChargeTotalProp = get_child_int(pGacha, "gaugeChargeTotalProp");

        // Load agg scope
        auto pAggScope = pGacha->GetChild("ableUsingAggScope");
        if (pAggScope)
        {
            for (const auto& [sIdx, pScope] : pAggScope->GetChildren())
            {
                if (!pScope)
                    continue;
                GachaponAggScope scope;
                scope.nMinType = get_child_int(pScope, "minType");
                scope.nMaxType = get_child_int(pScope, "maxType");
                pInfo->aAbleUsingAggScope.push_back(scope);
            }
        }

        // Load messages
        auto pMsg = pGacha->GetChild("msg");
        if (pMsg)
        {
            for (const auto& [sIdx, pMsgItem] : pMsg->GetChildren())
            {
                if (pMsgItem)
                    pInfo->aMsg.push_back(pMsgItem->GetString());
            }
        }

        // Load final confirm info
        auto pConfirm = pGacha->GetChild("finalconfirmInfo");
        if (pConfirm)
        {
            for (std::size_t i = 0; i < 4; ++i)
            {
                pInfo->aFinalconfirmInfo[i] = get_child_int(pConfirm, std::to_string(i));
            }
        }

        // Load gauge charge
        auto pGaugeCharge = pGacha->GetChild("gaugeCharge");
        if (pGaugeCharge)
        {
            for (const auto& [sIdx, pCharge] : pGaugeCharge->GetChildren())
            {
                if (!pCharge)
                    continue;
                GachaponGaugeCharge charge;
                charge.nProp = get_child_int(pCharge, "prop");
                charge.nEventProp = get_child_int(pCharge, "eventProp");
                charge.nValue = get_child_int(pCharge, "value");
                pInfo->aGaugeCharge.push_back(charge);
            }
        }

        m_mGachaponItemInfo[nItemID] = std::move(pInfo);
    }

    wzResMan.FlushCachedObjects(0);
}

// ============================================================
// GetItemString @ 0xacbb70
// Looks up a string field for a given item ID from the
// m_mItemString table (populated during item string loading).
// Used by GetItemName("name"), GetItemDesc("desc"), etc.
// ============================================================
auto ItemInfo::GetItemString(std::int32_t nItemID, const std::string& sKey) -> std::string
{
    auto itItem = m_mItemString.find(nItemID);
    if (itItem == m_mItemString.end())
        return {};

    auto itKey = itItem->second.find(sKey);
    if (itKey == itItem->second.end())
        return {};

    return itKey->second;
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
// Original: GetItemString(nItemID, StringPool(0xA7C="name"))
// ============================================================
auto ItemInfo::GetItemName(std::int32_t nItemID) -> std::string
{
    return GetItemString(nItemID, "name");
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
// Original: GetItemString(nItemID, StringPool(0x9A8="desc"))
// ============================================================
auto ItemInfo::GetItemDesc(std::int32_t nItemID) -> std::string
{
    return GetItemString(nItemID, "desc");
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

// ============================================================
// IsAbleToEquipSubWeapon @ 0xa7aaf0
// Determines if a character can equip a sub-weapon (shield slot)
// given their job, equipped main weapon, and the sub-weapon item.
// ============================================================
auto ItemInfo::IsAbleToEquipSubWeapon(std::int32_t nItemID, std::int32_t nEquippedWeaponID,
                                      std::int32_t nJob, std::int16_t nSubJob,
                                      std::int32_t bCash) const -> bool
{
    if (nItemID / 10000 == 135)
    {
        // Mercedes cards: 1350000-1352099
        if (nItemID - 1350000 < 2100)
        {
            auto nWeaponType = get_weapon_type(nEquippedWeaponID);
            return (((nWeaponType < 40 || nWeaponType > 53)
                    && nWeaponType != 56 && nWeaponType != 57 && nWeaponType != 58)
                    || get_weapon_type(nEquippedWeaponID) == 52)
                && (nJob / 100 == 23 || nJob == 2002 || nJob == 900);
        }
        // Phantom cards: 1352100-1352199
        if (nItemID - 1350000 < 2200)
        {
            auto nWeaponType = get_weapon_type(nEquippedWeaponID);
            if ((nWeaponType >= 40 && nWeaponType <= 53)
                || nWeaponType == 56 || nWeaponType == 57 || nWeaponType == 58)
                return false;
            return is_phantom_job(nJob) || nJob == 900;
        }
    }

    // Shields: 109xxxx
    if (nItemID / 10000 == 109)
    {
        return (nItemID / 1000 == 1098 || bCash || !is_michael_job(nJob))
            && (nItemID / 1000 == 1099 || bCash || !is_dslayer_job_born(nJob))
            && !is_dual_job_born(nJob, nSubJob)
            && (bCash || !is_res_hybrid_job(nJob));
    }

    // Luminous orb (is_orb = Phantom Card range 1352400-1352499)
    if (is_orb(nItemID))
    {
        if (is_two_hand_weapon(nEquippedWeaponID))
            return false;
        return is_luminous_job(nJob) || nJob == 900;
    }

    // Xenon energy sword: nItemID/100 == 13530
    if (nItemID / 100 == 13530)
    {
        if (is_two_hand_weapon(nEquippedWeaponID))
            return false;
        return is_res_hybrid_job(nJob) || nJob == 900;
    }

    // Kaiser dragon soul
    if (is_dragon_soul(nItemID))
    {
        if (!is_two_hand_weapon(nEquippedWeaponID) || get_weapon_type(nEquippedWeaponID) == 40)
            return is_kaiser_job(nJob) || nJob == 900;
        return false;
    }

    // Angelic Buster soul ring
    if (is_soulring(nItemID))
    {
        if (is_two_hand_weapon(nEquippedWeaponID))
            return false;
        return is_angelic_burster_job(nJob) || nJob == 900;
    }

    // Job-specific sub-weapons (no two-hand restriction, job || GM pattern)
    if (is_magnum(nItemID))
        return is_mechanic_job(nJob) || nJob == 900;
    if (is_hero_medal(nItemID))
        return is_hero_job(nJob) || nJob == 900;
    if (is_paladin_rosario(nItemID))
        return is_paladin_job(nJob) || nJob == 900;
    if (is_darknight_chain(nItemID))
        return is_darkknight_job(nJob) || nJob == 900;
    if (is_mage1_book(nItemID))
        return is_mage1_job(nJob) || nJob == 900;
    if (is_mage2_book(nItemID))
        return is_mage2_job(nJob) || nJob == 900;
    if (is_mage3_book(nItemID))
        return is_mage3_job(nJob) || nJob == 900;
    if (is_bowmaster_feather(nItemID))
        return is_bowmaster_job(nJob) || nJob == 900;
    if (is_crossbow_thimble(nItemID))
        return is_crossbow_job(nJob) || nJob == 900;
    if (is_shadower_sheath(nItemID))
        return is_shadower_job(nJob) || nJob == 900;
    if (is_nightlord_pouch(nItemID))
        return is_nightlord_job(nJob) || nJob == 900;
    if (is_viper_wristband(nItemID))
        return is_viper_job(nJob) || nJob == 900;
    if (is_captain_sight(nItemID))
        return is_captain_job(nJob) || nJob == 900;

    // Cannoneer gunpowder (job || GM pattern)
    if (is_cannon_gunpowder(nItemID))
        return is_cannonshooter_job(nJob) || nJob == 900;

    // Job-specific with beginner fallback (job || GM || beginner pattern)
    if (is_aran_pendulum(nItemID))
        return is_aran_job(nJob) || nJob == 900 || is_beginner_job(nJob);
    if (is_evan_paper(nItemID))
        return is_evan_job(nJob) || nJob == 900 || is_beginner_job(nJob);

    // Resistance sub-weapons (job || GM pattern)
    if (is_battlemage_orb(nItemID))
        return is_bmage_job(nJob) || nJob == 900;
    if (is_wildhunter_arrowhead(nItemID))
        return is_wildhunter_job(nJob) || nJob == 900;

    // Cygnus gem (job || GM || beginner pattern)
    if (is_cygnus_gem(nItemID))
        return is_cygnus_job(nJob) || nJob == 900 || is_beginner_job(nJob);

    // Zero / Kinesis sub-weapons (job || GM pattern)
    if (is_zero_sub_weapon_item(nItemID))
        return is_zero_job(nJob) || nJob == 900;
    if (is_kiness_sub_weapon_item(nItemID))
        return is_kinesis_job(nJob) || nJob == 900;

    // Unknown sub-weapon type: allow
    return true;
}

// ============================================================
// IsOnlyItem @ 0xab04d0  (Pattern A: equip+bundle)
// ============================================================
auto ItemInfo::IsOnlyItem(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* p = GetEquipItem(nItemID);
        return p ? p->bOnly != 0 : false;
    }
    auto* p = GetBundleItem(nItemID);
    return p ? p->bOnly != 0 : false;
}

// ============================================================
// IsOnlyEquipItem @ 0xab0610  (Pattern B: equip-only)
// ============================================================
auto ItemInfo::IsOnlyEquipItem(std::int32_t nItemID) -> bool
{
    if (!helper::IsEquipItemID(nItemID)) return false;
    auto* p = GetEquipItem(nItemID);
    return p ? p->bOnlyEquip != 0 : false;
}

// ============================================================
// IsSuperiorEquipItem @ 0xab0750  (Pattern B: equip-only)
// ============================================================
auto ItemInfo::IsSuperiorEquipItem(std::int32_t nItemID) -> bool
{
    if (!helper::IsEquipItemID(nItemID)) return false;
    auto* p = GetEquipItem(nItemID);
    return p ? p->bSuperiorEqp != 0 : false;
}

// ============================================================
// IsNoCancelByMouseForItem @ 0xab0890  (Pattern C: bundle-only)
// ============================================================
auto ItemInfo::IsNoCancelByMouseForItem(std::int32_t nItemID) -> bool
{
    auto* p = GetBundleItem(nItemID);
    return p ? p->bNoCancelMouse : false;
}

// ============================================================
// IsNotSaleItem @ 0xab0ec0  (Pattern A: equip+bundle)
// ============================================================
auto ItemInfo::IsNotSaleItem(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* p = GetEquipItem(nItemID);
        return p ? p->bNotSale != 0 : false;
    }
    auto* p = GetBundleItem(nItemID);
    return p ? p->bNotSale != 0 : false;
}

// ============================================================
// IsDefaultAccountSharableItem @ 0xab0b10  (Pattern A: equip+bundle)
// ============================================================
auto ItemInfo::IsDefaultAccountSharableItem(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* p = GetEquipItem(nItemID);
        return p ? p->bAccountSharable != 0 : false;
    }
    auto* p = GetBundleItem(nItemID);
    return p ? p->bAccountSharable != 0 : false;
}

// ============================================================
// IsSharableOnceItem @ 0xab0c50  (Pattern A: equip+bundle)
// ============================================================
auto ItemInfo::IsSharableOnceItem(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* p = GetEquipItem(nItemID);
        return p ? p->bSharableOnce != 0 : false;
    }
    auto* p = GetBundleItem(nItemID);
    return p ? p->bSharableOnce != 0 : false;
}

// ============================================================
// IsCantRepairItem @ 0xab0d90  (Pattern B: equip-only)
// ============================================================
auto ItemInfo::IsCantRepairItem(std::int32_t nItemID) -> bool
{
    if (!helper::IsEquipItemID(nItemID)) return false;
    auto* p = GetEquipItem(nItemID);
    return p ? p->bCantRepair != 0 : false;
}

// ============================================================
// IsPartyQuestItem @ 0xab1180  (Pattern A: equip+bundle)
// ============================================================
auto ItemInfo::IsPartyQuestItem(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* p = GetEquipItem(nItemID);
        return p ? p->bPartyQuest != 0 : false;
    }
    auto* p = GetBundleItem(nItemID);
    return p ? p->bPartyQuest != 0 : false;
}

// ============================================================
// IsPickUpBlockItem @ 0xab12c0  (Pattern C: bundle-only)
// ============================================================
auto ItemInfo::IsPickUpBlockItem(std::int32_t nItemID) -> bool
{
    auto* p = GetBundleItem(nItemID);
    return p ? p->bNoPickupByPet : false;
}

// ============================================================
// IsBindedWhenEquiped @ 0xab1830  (Pattern B: equip-only)
// ============================================================
auto ItemInfo::IsBindedWhenEquiped(std::int32_t nItemID) -> bool
{
    if (!helper::IsEquipItemID(nItemID)) return false;
    auto* p = GetEquipItem(nItemID);
    return p ? p->bBindedWhenEquiped != 0 : false;
}

// ============================================================
// IsNotExtendItem @ 0xab1a90  (Pattern B: equip-only)
// ============================================================
auto ItemInfo::IsNotExtendItem(std::int32_t nItemID) -> bool
{
    if (!helper::IsEquipItemID(nItemID)) return false;
    auto* p = GetEquipItem(nItemID);
    return p ? p->bNotExtend != 0 : false;
}

// ============================================================
// ExpireOnLogout @ 0xab1bd0  (Pattern A: equip+bundle)
// ============================================================
auto ItemInfo::ExpireOnLogout(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* p = GetEquipItem(nItemID);
        return p ? p->bExpireOnLogout != 0 : false;
    }
    auto* p = GetBundleItem(nItemID);
    return p ? p->bExpireOnLogout != 0 : false;
}

// ============================================================
// IsUnchangeable @ 0xab0110  (Pattern B: equip-only)
// ============================================================
auto ItemInfo::IsUnchangeable(std::int32_t nItemID) -> bool
{
    if (!helper::IsEquipItemID(nItemID)) return false;
    auto* p = GetEquipItem(nItemID);
    return p ? p->bUnChangeable != 0 : false;
}

// ============================================================
// IsUndecomposable @ 0xab0250  (Pattern B: equip-only)
// ============================================================
auto ItemInfo::IsUndecomposable(std::int32_t nItemID) -> bool
{
    if (!helper::IsEquipItemID(nItemID)) return false;
    auto* p = GetEquipItem(nItemID);
    return p ? p->bUndecomposable != 0 : false;
}

// ============================================================
// IsRoyalSpecialItem @ 0xaafd20  (Pattern B: equip-only)
// ============================================================
auto ItemInfo::IsRoyalSpecialItem(std::int32_t nItemID) -> bool
{
    if (!helper::IsEquipItemID(nItemID)) return false;
    auto* p = GetEquipItem(nItemID);
    return p ? p->bRoyalSpecial != 0 : false;
}

// ============================================================
// IsRoyalMasterItem @ 0xaafe70  (Pattern B: equip-only)
// ============================================================
auto ItemInfo::IsRoyalMasterItem(std::int32_t nItemID) -> bool
{
    if (!helper::IsEquipItemID(nItemID)) return false;
    auto* p = GetEquipItem(nItemID);
    return p ? p->bRoyalMaster != 0 : false;
}

// ============================================================
// IsApplicableAccountShareTag @ 0xafa090  (Pattern A: equip+bundle)
// ============================================================
auto ItemInfo::IsApplicableAccountShareTag(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* p = GetEquipItem(nItemID);
        return p ? p->bAccountShareTagApplicable != 0 : false;
    }
    auto* p = GetBundleItem(nItemID);
    return p ? p->bAccountShareTagApplicable != 0 : false;
}

// ============================================================
// IsPickUpBlockByPetItem @ 0xafa0e0  (Pattern C: bundle-only)
// ============================================================
auto ItemInfo::IsPickUpBlockByPetItem(std::int32_t nItemID) -> bool
{
    auto* p = GetBundleItem(nItemID);
    return p ? p->bNoPickupByPet : false;
}

// ============================================================
// GetAppliableKarmaType @ 0xafa040  (Pattern A: returns int)
// ============================================================
auto ItemInfo::GetAppliableKarmaType(std::int32_t nItemID) -> std::int32_t
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* p = GetEquipItem(nItemID);
        return p ? p->nAppliableKarmaType : 0;
    }
    auto* p = GetBundleItem(nItemID);
    return p ? p->nAppliableKarmaType : 0;
}

// ============================================================
// IsBigSizeItem  (Pattern A: equip+bundle)
// ============================================================
auto ItemInfo::IsBigSizeItem(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* p = GetEquipItem(nItemID);
        return p ? p->bBigSize != 0 : false;
    }
    auto* p = GetBundleItem(nItemID);
    return p ? p->bBigSize != 0 : false;
}

// ============================================================
// IsBossRewardItem  (Pattern B: equip-only)
// ============================================================
auto ItemInfo::IsBossRewardItem(std::int32_t nItemID) -> bool
{
    if (!helper::IsEquipItemID(nItemID)) return false;
    auto* p = GetEquipItem(nItemID);
    return p ? p->bBossReward != 0 : false;
}

// ============================================================
// IsExItem  (Pattern B: equip-only)
// ============================================================
auto ItemInfo::IsExItem(std::int32_t nItemID) -> bool
{
    if (!helper::IsEquipItemID(nItemID)) return false;
    auto* p = GetEquipItem(nItemID);
    return p ? p->bExItem != 0 : false;
}

// ============================================================
// IsMorphItem  (Pattern A: equip+bundle)
// ============================================================
auto ItemInfo::IsMorphItem(std::int32_t nItemID) -> bool
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* p = GetEquipItem(nItemID);
        return p ? p->bMorphItem != 0 : false;
    }
    auto* p = GetBundleItem(nItemID);
    return p ? p->bMorphItem != 0 : false;
}

// ============================================================
// GetSellPrice  (Pattern A: returns int)
// ============================================================
auto ItemInfo::GetSellPrice(std::int32_t nItemID) -> std::int32_t
{
    if (helper::IsEquipItemID(nItemID))
    {
        auto* p = GetEquipItem(nItemID);
        return p ? p->nSellPrice : 0;
    }
    auto* p = GetBundleItem(nItemID);
    return p ? p->nSellPrice : 0;
}

} // namespace ms
