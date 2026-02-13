#pragma once

#include "DayOfWeekItemStat.h"
#include "FixedOption.h"

#include "util/ZXString.h"

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

// Forward declarations for pointer-only members
struct GROWTHOPTION;
struct ItemSkill;
struct Addition;
struct VariableStat;

/// Equipment item data (original size: 0x2F8 = 760 bytes)
/// Decompiled from: constructor @ 0xaa4b80, assignment @ 0xabada0
struct EquipItem
{
    // --- Identity ---
    std::int32_t nItemID{};
    std::int32_t bTimeLimited{};
    std::int32_t bAbilityTimeLimited{};
    ZXString<char> sItemName;
    std::string sUOL;                      // Ztl_bstr_t in original

    // --- Required stats ---
    std::int32_t nrSTR{};
    std::int32_t nrINT{};
    std::int32_t nrDEX{};
    std::int32_t nrLUK{};
    std::int32_t nrPOP{};
    std::int32_t nrJob{};
    std::int32_t nrSpecJob{};
    std::int32_t nrLevel{};
    std::int32_t nrMobLevel{};
    std::int32_t nrPvPGrade{};

    // --- Replacement ---
    std::int32_t nReplaceItemID{};
    ZXString<char> sReplaceMsg;
    std::int32_t nReplacePeriod{};

    // --- Price / Cash ---
    std::int32_t nSellPrice{};
    std::int32_t bCash{};

    // --- Upgrade slots ---
    std::int32_t nTUC{};                   // Total Upgrade Count

    // --- Stat increments ---
    std::int32_t niSTR{};
    std::int32_t niDEX{};
    std::int32_t niINT{};
    std::int32_t niLUK{};
    std::int32_t niMaxHP{};
    std::int32_t niMaxMP{};
    std::int32_t niMaxHPr{};
    std::int32_t niMaxMPr{};
    std::int32_t niPAD{};
    std::int32_t niMAD{};
    std::int32_t niPDD{};
    std::int32_t niMDD{};
    std::int32_t niACC{};
    std::int32_t niEVA{};
    std::int32_t niCraft{};
    std::int32_t niSpeed{};
    std::int32_t niJump{};
    std::int32_t niSwim{};
    std::int32_t niFatigue{};

    // --- Time-limited stat increments ---
    std::int32_t niTLSTR{};
    std::int32_t niTLDEX{};
    std::int32_t niTLINT{};
    std::int32_t niTLLUK{};
    std::int32_t niTLMaxHP{};
    std::int32_t niTLMaxMP{};
    std::int32_t niTLPAD{};
    std::int32_t niTLMAD{};
    std::int32_t niTLPDD{};
    std::int32_t niTLMDD{};
    std::int32_t niTLACC{};
    std::int32_t niTLEVA{};
    std::int32_t niTLCraft{};
    std::int32_t niTLSpeed{};
    std::int32_t niTLJump{};
    std::int32_t nTLBDR{};
    std::int32_t nTLIMDR{};
    std::int32_t nTLDamR{};
    std::int32_t nTLStatR{};

    // --- Day-of-week stats ---
    std::int32_t bDayOfWeekItemStat{};
    std::array<DayOfWeekItemStat, 7> aDayOfWeekItemStat{};

    // --- PVP / Bonus damage ---
    std::int32_t niPVPDamage{};
    std::int32_t niReduceReq{};
    std::int32_t niIncReq{};
    std::int32_t nBDR{};                   // Boss Damage Rate
    std::int32_t nIMDR{};                  // Ignore Monster DEF Rate
    std::int32_t nDamR{};                  // Damage Rate
    std::int32_t nStatR{};                 // Stat Rate
    std::int32_t nCuttable{};

    // --- Special flags ---
    std::int32_t bExItem{};
    std::int32_t bBossReward{};
    std::int32_t nExGrade{};
    std::int32_t bNoMoveToLocker{};
    std::int32_t nKnockback{};

    // --- Recovery / Movement ---
    double dRecovery{};
    double dFs{};
    std::int32_t nSwim{};

    // --- Taming mob / Vehicle ---
    std::int32_t nTamingMob{};
    std::int32_t nVehicleDoubleJumpLevel{};
    std::int32_t nVehicleGlideLevel{};
    std::int32_t nVehicleNewFlyingLevel{};
    std::int32_t nVehicleSkillIsTown{};
    std::int32_t nVehicleNaviFlyingLevel{};

    // --- Android ---
    std::int32_t nAndroid{};
    std::int32_t nAndroidGrade{};

    // --- Personality EXP ---
    std::int32_t nCharismaEXP{};
    std::int32_t nInsightEXP{};
    std::int32_t nWillEXP{};
    std::int32_t nCraftEXP{};
    std::int32_t nSenseEXP{};
    std::int32_t nCharmEXP{};
    std::int32_t nCashForceCharmEXP{};

    // --- Party / Alliance bonuses ---
    std::int32_t nBestFriendPartyBonusExp{};
    std::int32_t nBloodAllianceExpRate{};
    std::int32_t nBloodAlliancePartyExpRate{};

    // --- Grade / Quest ---
    std::int32_t nMinGrade{};
    std::int32_t bQuest{};
    std::int32_t bPartyQuest{};

    // --- Restrictions ---
    std::int32_t bOnly{};
    std::int32_t bOnlyEquip{};
    std::int32_t bSuperiorEqp{};
    std::int32_t bTradeBlock{};
    std::int32_t nAppliableKarmaType{};
    std::int32_t bAccountShareTagApplicable{};
    std::int32_t bNotSale{};
    std::int32_t bBigSize{};
    std::int32_t bExpireOnLogout{};
    std::int32_t bBindedWhenEquiped{};
    std::uint32_t dwSpecialID{};
    std::int32_t bNotExtend{};
    std::int32_t bAccountSharable{};
    std::int32_t bSharableOnce{};
    std::int32_t bUnChangeable{};
    std::uint32_t dwAfterimageFlag{};
    std::int32_t bJewelCraft{};
    std::int32_t bScope{};
    std::int32_t bMorphItem{};
    std::int32_t bUndecomposable{};

    // --- Pet template flag (CFlag<512> = 16 x int32) ---
    std::array<std::uint32_t, 16> uPetTemplateFlag{};

    // --- Growth ---
    std::shared_ptr<GROWTHOPTION> pGrowth;

    // --- Potential ---
    std::int32_t bEpic{};
    std::int32_t bFixedPotential{};
    std::int32_t nFixedGrade{};
    std::int32_t nFixedOptionCnt{};
    std::array<FixedOption, 6> aFixedOption{};
    std::int32_t nDisableFieldType{};
    std::int32_t nSetGrade{};
    std::int32_t nFixedOptionLevel{};
    std::int32_t nCubeExBaseOptionLevel{};
    std::int32_t bNoPotential{};
    std::int32_t bSpecialGrade{};
    std::int32_t bRandItemVariation{};
    std::int32_t nReissueBan{};

    // --- Destruction / Enhancement ---
    std::int32_t bNotDestroy{};
    std::int32_t bAlwaysGradeUpgrade{};
    std::int32_t bAlwaysInchantSuccess{};
    std::int32_t bSellingOneMeso{};
    std::int32_t nBitsSlot{};

    // --- Ring ---
    std::int32_t nRingOptionSkill{};
    std::int32_t nRingOptionSkillLevel{};

    // --- Royal ---
    std::int32_t bRoyalSpecial{};
    std::int32_t bRoyalMaster{};

    // --- Text equip ---
    std::int32_t bText{};
    // TextEquipParam omitted until needed

    // --- Set item ---
    std::int32_t nSetItemID{};
    std::int32_t nJokerToSetItem{};
    std::int32_t nGroupEffectID{};

    // --- Sound / Emotion ---
    std::string sEquipedSound;             // Ztl_bstr_t in original
    std::int32_t nEquippedEmotion{};

    // --- Description ---
    ZXString<char> sDesc;

    // --- Skill data ---
    // ZList<ZRef<ItemSkill>> lpItemSkill — simplified:
    std::vector<std::shared_ptr<ItemSkill>> lpItemSkill;
    // ZMap<long,long,long> mSkillLevelBonus — simplified:
    std::map<std::int32_t, std::int32_t> mSkillLevelBonus;
    // ZList<long> lnOnlyUpgradeID — simplified:
    std::vector<std::int32_t> lnOnlyUpgradeID;

    // --- Bonus EXP / Taming ---
    std::vector<std::pair<std::int32_t, std::int32_t>> aBonusExpRate;
    std::vector<std::int32_t> aTamingMobItem;

    // --- Equip drop ---
    std::int32_t nEquipDropRate{};
    std::int32_t nEquipDropFieldStart{};
    std::int32_t nEquipDropFieldEnd{};
    std::int32_t nEquipDropExceptMobStart{};
    std::int32_t nEquipDropExceptMobEnd{};

    // --- Misc ---
    std::int32_t nAttackCountInc{};
    std::int32_t nLookChangeType{};

    // --- Addition / Variable stat ---
    std::shared_ptr<Addition> pAddition;
    std::shared_ptr<VariableStat> pVariableStat;

    // --- Durability ---
    std::int32_t nDurability{};
    std::int32_t bCantRepair{};
};

} // namespace ms
