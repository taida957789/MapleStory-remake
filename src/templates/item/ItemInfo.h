#pragma once

#include "ItemInfoTypes.h"

#include "util/Point.h"
#include "util/Singleton.h"

#include <array>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ms
{

// Forward declarations
class GW_ItemSlotBase;

namespace Additional
{
    struct SKILL {
        std::int32_t nSkillID;
        std::int32_t nSLV;
    };
    struct MOBCATEGORY;
    struct ELEMBOOST;
    struct CRITICAL;
    struct BOSS;
    struct MOBDIE;
    struct HPMPCHANGE;
    struct STAT_INC;
    struct KNOCKBACK;
    template<typename T> struct TCond;
}

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
class ItemInfo final : public Singleton<ItemInfo>
{
    friend class Singleton<ItemInfo>;

    struct KarmaScissorsItem {
        std::int32_t nItemID;
        std::int32_t nKarmaKey;
    };
    struct ProtectOnDieItem {
        std::int32_t nItemID;
        std::int32_t nRecoveryRate;
    };
    struct ItemPotCureItem {
        std::int32_t nItemID;
        std::int32_t nCureProb;
    };
    struct ItemPotCreateItem {
        std::int32_t nItemID;
        std::int32_t nLifeID;
    };
    struct DecomposerInstallItem {
        std::int32_t nItemID;
        std::int32_t nDecomposableItemLevel;
    };
    struct EquipSlotLevelMinusItem {
        std::int32_t nItemID;
        std::int32_t nAddTime;
        std::int32_t nMaxDays;
        std::int32_t nMinusLevel;
        std::list<std::int16_t> aSelectedSlot;
    };
    struct DyeingItem {
        std::int32_t nItemID;
        std::array<std::int32_t, 10> aDyeingPossibilityItem;
    };
    struct DressUpClothesItem {
        std::int32_t nItemID;
        std::int32_t nClothesID;
        bool bNotConsume;
        std::uint8_t nSkillEffectID;
    };
    struct AreaBuffItem {
        std::int32_t nItemID;
        std::int32_t nStateChageItemID;
        std::int32_t tTime;
        Rect rcAffectedArea;
        std::int32_t nTotalProp;
        std::array<std::int32_t, 39> aEmotionProp;
    };
    struct BitsCaseItem {
        std::int32_t nItemID;
        std::int32_t nSlotCount;
        std::int32_t nSlotPerLine;
    };
    struct GachaponAggScope {
        std::int32_t nMinType;
        std::int32_t nMaxType;
    };
    struct GachaponGaugeCharge {
        std::int32_t nProp;
        std::int32_t nEventProp;
        std::int32_t nValue;
    };
    struct GachaponItemInfo {
        std::list<GachaponAggScope> aAbleUsingAggScope;
        std::int32_t bBonus;
        std::int32_t bReplacedProb;
        std::int32_t bNoGradeResult;
        std::int32_t bSelfSelectReward;
        std::int32_t nFixedSelectReward;
        std::int32_t nSucessNpcID;
        std::list<std::string> aMsg;
        std::array<std::int32_t, 4> aFinalconfirmInfo;
        std::int32_t nGaugenQRID;
        std::int32_t nGaugeChargeTotalProp;
        std::list<GachaponGaugeCharge> aGaugeCharge;
    };
    struct ItemSkill {
        std::int32_t nSkillID;
        std::int32_t nSkillLevel;
        std::int32_t bAutoRunOnlyTown;
    };
    struct CoupleChairItem {
        std::int32_t nItemID;
        std::int32_t nDistanceX;
        std::int32_t nDistanceY;
        std::int32_t nMaxDiff;
        std::int32_t nDirection;
    };
    struct GroupEffectInfo {
        std::int32_t nEffectID;
        std::int32_t nGroupID;
        std::int32_t bOneToOne;
        std::int32_t nCompleteCount;
        std::int32_t nEffectCount;
        std::int32_t nDistanceX;
        std::int32_t nDistanceY;
        std::vector<std::int32_t> anItemID;
    };
    struct LevelInfo {
        std::int32_t nLevel;
        std::int32_t nLevelUpType;
        std::int32_t nLevelUpValue;
    };
    struct Recovery;  // TODO: define when decompiled
    struct GrowthOption {
        std::list<std::int32_t> anLevelUpTypePool;
        std::list<std::shared_ptr<LevelInfo>> apLevelInfo;
        std::int32_t bLevelUpByPoint;
        std::int32_t bFixLevel;
        std::int32_t nType;
    };
    struct LevelInfoRandomStat {
        struct LevelInfoAbility
        {
            std::string sDesc;
            std::shared_ptr<Recovery> pRecovery;
            std::list<std::shared_ptr<ItemSkill>> lpItemSkill;
            std::map<std::int32_t, std::int32_t> mSkillLevelBonus;
            std::map<std::int32_t, std::int32_t> mEquipmentSkill;
        };
        std::int32_t nExpRate;
        std::int32_t nExpPoint;
        std::int32_t nExpDecPoint;
        std::int32_t nApplyCount;
        std::list<std::shared_ptr<LevelInfoRandomStat>> lpIncStat;
        std::map<std::int32_t, std::shared_ptr<LevelInfoAbility>> mpAbility;
    };
    struct PieceItemInfo {
        std::int32_t nRewardItemID;
        std::int32_t nCompleteCount;
        std::string sUIPath;
        std::list<std::int32_t> anFixedItemID;
    };
    struct paramEquipStat {
        std::int32_t nJob;
        std::int32_t nGender;
        std::int32_t nLevel;
        std::int32_t nIncReq;
        std::int32_t nReduceReq;
        std::int32_t nSTR;
        std::int32_t nDEX;
        std::int32_t nINT;
        std::int32_t nLUK;
        std::int32_t nPvPGrade;
        std::int32_t nPOP;
    };
    struct CoreSpec {
        std::int32_t nShape;
        std::int32_t nCategory;
        bool bNotConsume;
        std::vector<std::int32_t> anAllowedMapID;
        std::int32_t nMobRate;
        std::int32_t nMobLevel;
        std::int32_t nMobHPRate;
        std::int32_t nMobAttackRate;
        std::int32_t nMobDefenseRate;
        std::int32_t nPartyExpRate;
        std::uint32_t dwAddMob;
        std::vector<std::pair<std::int32_t, std::vector<Point<std::int32_t>>>> aAddMobPos;
        std::string sRewardDesc;
        std::int32_t nRewardType;
        std::vector<std::uint32_t> aReward;
        bool bDropRareEquip;
        std::int32_t nDropRate;
        std::int32_t nDropRateHerb;
        std::int32_t nDropRateMineral;
        std::string sAddMissionDesc;
        std::int32_t nAddMissionQuestID;
        std::int32_t nAddMissionMapID;
        std::int32_t nMobRateSpecial;
        std::int32_t nPartyExpRateSpecial;
        std::int32_t nDropRateSpecial;
        std::string sChangeMobDesc;
        std::string sChangeMob;
        std::string sChangeBackGrndDesc;
        std::uint32_t dwChangeBackGrnd;
        std::string sChangeBgmDesc;
        std::string sChangeBgm;
        std::int32_t nSkinCategory;
    };
    struct CoreItem {
        std::int32_t nItemID;
        CoreSpec coreSpec;
    };
    struct DayOfWeekItemStat {
        std::int32_t nDOWIMDR;
    };
    struct Addition {
        std::shared_ptr<Additional::TCond<Additional::SKILL>> pSkill;
        std::shared_ptr<Additional::TCond<Additional::MOBCATEGORY>> pMobCategory;
        std::shared_ptr<Additional::TCond<Additional::ELEMBOOST>> pElemBoost;
        std::shared_ptr<Additional::TCond<Additional::CRITICAL>> pCritical;
        std::shared_ptr<Additional::TCond<Additional::BOSS>> pBoss;
        std::shared_ptr<Additional::TCond<Additional::MOBDIE>> pMobDie;
        std::shared_ptr<Additional::TCond<Additional::HPMPCHANGE>> pHpMpChange;
        std::shared_ptr<Additional::TCond<Additional::STAT_INC>> pStatInc;
        std::shared_ptr<Additional::TCond<Additional::KNOCKBACK>> pKnockback;
    };
    struct TextEquipParam {
        std::int32_t nTextEquipColor;
        std::int32_t nTextEquipOffsetX;
        std::int32_t nTextEquipOffsetY;
        std::int32_t nTextEquipFontSize;
        std::int32_t nTextEquipAreaX;
        std::int32_t nTextEquipAreaY;
    };
    struct VariableStat {
        float nPAD;
        float nMAD;
        float nPDD;
        float nMDD;
        float nACC;
        float nEVA;
        float nSTR;
        float nDEX;
        float nLUK;
        float nINT;
        float nMHP;
        float nMMP;
    };
    struct FixedOption {
        std::int32_t nOption;
        std::int32_t nLevel;
    };
    struct EquipItem
    {
        // --- Identity ---
        std::int32_t nItemID{};
        std::int32_t bTimeLimited{};
        std::int32_t bAbilityTimeLimited{};
        std::string sItemName;
        std::string sUOL;

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
        std::string sReplaceMsg;
        std::int32_t nReplacePeriod{};

        // --- Price / Cash ---
        std::int32_t nSellPrice{};
        std::int32_t bCash{};

        // --- Upgrade slots ---
        std::int32_t nTUC{};

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
        std::int32_t nBDR{};
        std::int32_t nIMDR{};
        std::int32_t nDamR{};
        std::int32_t nStatR{};
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
        std::shared_ptr<GrowthOption> pGrowth;

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

        // --- Set item ---
        std::int32_t nSetItemID{};
        std::int32_t nJokerToSetItem{};
        std::int32_t nGroupEffectID{};

        // --- Sound / Emotion ---
        std::string sEquipedSound;
        std::int32_t nEquippedEmotion{};

        // --- Description ---
        std::string sDesc;

        // --- Skill data ---
        std::list<std::shared_ptr<ItemSkill>> lpItemSkill;
        std::map<std::int32_t, std::int32_t> mSkillLevelBonus;
        std::list<std::int32_t> lnOnlyUpgradeID;

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
    struct SetOption;  // TODO: define when decompiled
    struct SetEffect {
        std::int32_t nCash;
        std::int32_t nSetID;
        std::list<std::int32_t> alItemList[32];
        std::list<std::int32_t> alMechanicItemList[5];
    };
    struct SetEffectStat {
        std::int16_t niSTR;
        std::int16_t niDEX;
        std::int16_t niINT;
        std::int16_t niLUK;
        std::int16_t niAllStat;
        std::int16_t niMaxHP;
        std::int16_t niMaxMP;
        std::int16_t niMaxHPr;
        std::int16_t niMaxMPr;
        std::int16_t niPAD;
        std::int16_t niMAD;
        std::int16_t niPDD;
        std::int16_t niMDD;
        std::int16_t niACC;
        std::int16_t niEVA;
        std::int16_t niCraft;
        std::int16_t niSpeed;
        std::int16_t niJump;
        std::int32_t nKnockback;
        std::int16_t niPVPDamage;
        std::int16_t niPQExpR;
        std::list<std::shared_ptr<ItemSkill>> lpItemSkill;
        std::list<std::shared_ptr<ItemSkill>> lpActiveSkill;
        std::map<std::int32_t, std::int32_t> mSkillLevelBonus;
        std::map<std::uint32_t, std::int32_t> mOptionToMob;
        std::map<std::string, std::int32_t> mOptionToMobDesc;
        std::array<std::shared_ptr<SetOption>, 10> pOption;
    };
    struct SetItemInfo {
        std::int32_t nSetItemID;
        std::vector<std::int32_t> nItemID;
        std::int32_t bParts;
        std::vector<std::list<std::int32_t>> alParts;
        std::int32_t bExpandToolTip;
        std::vector<std::string> asPartsName;
        std::vector<std::string> asTypeName;
        std::array<std::shared_ptr<SetEffectStat>, 32> pEffect;
        std::string sSetItemName;
        std::int32_t nSetCompleteCount;
        std::u16string sCompleteEffectLink;
        std::string sWeaponDesc;
        std::string sSubWeaponDesc;
    };
    struct BagInfo {
        std::int32_t nItemID;
        std::int32_t nSlotCount;
        std::int32_t nBagType;
        std::int32_t nSlotPerLine;
    };
    struct GatheringToolItem {
        std::int32_t nItemID;
        std::string sAct;
        std::int32_t nIncSpeed;
        std::int32_t nIncSkillLevel;
        std::int32_t nReqSkillLevel;
        std::int32_t nIncNumProb;
        std::int32_t nIncNum;
        std::u16string sToolEffectUOL;
    };
    struct ExtendExpireDateItem {
        std::int32_t nItemID;
        std::int32_t nExtendSeconds;
        std::int32_t nMaxExtendDays;
        std::int32_t bEternity;
    };
    struct ExpiredProtectingItem {
        std::int32_t nItemID;
        std::int32_t nProtectDays;
        std::int32_t nMaxDays;
    };
    struct RecipeOpenItem {
        std::int32_t nItemID;
        std::int32_t nRecipeID;
        std::int32_t nReqSkillLevel;
        std::int32_t nRecipeValidDay;
        std::int32_t nRecipeUseCount;
    };
    struct SetAction {
        std::string sCommand;
        std::string bsActionName;
        std::array<std::int32_t, 32> aItem;
    };
    struct SetTowerChair {
        std::int32_t nSetTowerChairID;
        std::list<std::int32_t> aItemID;
    };
    struct PetFoodItem {
        std::int32_t nItemID;
        std::int32_t niRepleteness;
        std::int32_t niTameness;
        std::list<std::uint32_t> ldwPet;
    };
    struct BridleItem {
        std::uint32_t dwTargetMobID;
        std::int32_t nItemID;
        Rect rc;
        std::int32_t nCreateItemID;
        std::int32_t nCreateItemPeriod;
        std::int32_t nCatchPercentageHP;
        std::int32_t nBridleMsgType;
        float fBridleProb;
        float fBridleProbAdj;
        std::uint32_t tUseDelay;
        std::string sDeleyMsg;
        std::string sNoMobMsg;
    };
    struct BundleItem
    {
        struct UseMesoChair
        {
            std::int32_t nUseMeso;
            std::int32_t nUseMesoTick;
            std::int32_t nUseMesoMax;
            std::int32_t nUseMesoSaveQr;
        };

        // --- Identity ---
        std::string sItemName;
        std::int32_t nItemID{};
        std::int32_t bTimeLimited{};

        // --- Restrictions ---
        std::int32_t bOnly{};
        std::int32_t bTradeBlock{};
        std::int32_t nAppliableKarmaType{};
        std::int32_t bAccountShareTagApplicable{};
        std::int32_t bNotSale{};
        std::int32_t bBigSize{};
        std::int32_t bExpireOnLogout{};
        std::int32_t bNoCancelMouse{};
        bool bNoPickupByPet{};

        // --- Stats ---
        std::int32_t nPAD{};
        std::int32_t nRequiredLEV{};
        std::list<std::uint32_t> lReqField;
        std::int32_t nReqFieldS{};
        std::int32_t nReqFieldE{};
        std::int32_t nSellPrice{};
        long double dSellUnitPrice{};
        bool bAutoPrice{};

        // --- Cash ---
        std::int32_t bCash{};
        std::int32_t bNoCancel{};

        // --- Quest ---
        std::int32_t bQuest{};
        std::int32_t bPartyQuest{};

        // --- Stack limits ---
        std::int16_t nMaxPerSlot{};
        std::int16_t nMax{};

        // --- Quest / Monster ---
        std::int32_t nReqQuestOnProgress{};
        std::int32_t nLevel{};
        std::int32_t nMCType{};
        std::int32_t nQuestID{};
        std::int32_t bUpdateExp{};
        std::int32_t nMobID{};
        std::int32_t bMonsterBookCard{};

        // --- Replace ---
        std::int32_t nReplaceItemID{};
        std::string sReplaceMsg;
        std::int32_t nReplacePeriod{};

        // --- Account sharing ---
        std::int32_t bAccountSharable{};
        std::int32_t bSharableOnce{};
        std::string sCantAccountSharableToolTip;
        std::string sCanAccountSharableToolTip;
        std::map<std::int32_t, std::string> mLvUpWarning;

        // --- Emotion ---
        std::int32_t nEmotion{};

        // --- Job restriction maps ---
        std::map<std::int32_t, std::int32_t> mCantAccountSharableJob;
        std::map<std::int32_t, std::int32_t> mCanAccountSharableJob;
        std::map<std::int32_t, std::int32_t> mCanUseJob;

        // --- Type ---
        std::int32_t nBagType{};
        std::int32_t bUseBinded{};

        // --- Level / Time limits ---
        std::int32_t nLimitMin{};
        std::int32_t nExpMinLev{};
        std::int32_t nExpMaxLev{};
        std::int32_t nLimitSec{};
        std::int32_t nPointCost{};

        // --- EXP ---
        std::int32_t bRelaxEXP{};
        std::int32_t nBonusEXPRate{};
        std::int32_t nCharismaEXP{};
        std::int32_t nInsightEXP{};
        std::int32_t nWillEXP{};
        std::int32_t nCraftEXP{};
        std::int32_t nSenseEXP{};
        std::int32_t nCharmEXP{};

        // --- Nick skill ---
        std::int32_t bNickSkillTimeLimited{};
        std::int32_t nNickSkill{};

        // --- Reward / Enchant ---
        std::int32_t nRewardItemID{};
        std::int32_t nEnchantSkill{};
        std::uint64_t nEndUseDate{};

        // --- Soul ---
        std::int32_t nSoulItemType{};
        std::uint32_t dwSummonSoulMobID{};

        // --- Bonus ---
        std::int32_t bBonusStage{};
        std::int32_t bMorphItem{};

        // --- Chair ---
        UseMesoChair stUseMesoChair{};
    };

public:
    ~ItemInfo() override = default;

    // --- Core Lookup (implemented) ---
    [[nodiscard]] auto GetEquipItem(std::int32_t nItemID) -> const EquipItem*;   // @ 0xae54c0
    [[nodiscard]] auto GetBundleItem(std::int32_t nItemID) -> const BundleItem*; // @ 0xaf9310

    // --- Query Helpers (implemented) ---
    [[nodiscard]] auto GetSetItemID(std::int32_t nItemID) -> std::int32_t;       // @ 0xae6700
    [[nodiscard]] auto GetItemName(std::int32_t nItemID) -> std::string;         // @ 0xacfb80
    [[nodiscard]] auto IsCashItem(std::int32_t nItemID) -> bool;                 // @ 0xaafbe0
    [[nodiscard]] auto IsQuestItem(std::int32_t nItemID) -> bool;                // @ 0xab1040
    [[nodiscard]] auto IsTradeBlockItem(std::int32_t nItemID) -> bool;           // @ 0xab09d0
    [[nodiscard]] auto GetRequiredLEV(std::int32_t nItemID) -> std::int32_t;     // @ 0xab23b0

    // --- Not yet implemented (address table) ---
    // GetItemProp(long)const                    @ 0xaae510
    // GetItemInfo(long)                         @ 0xaaede0
    // GetItemSlot(long, int)                    @ 0xae6c00
    // GetItemDesc(long)                         @ 0xacfe90
    // IsEquipItem(long)                         @ 0x5c0050
    // IsCashItem(GW_ItemSlotBase*)              @ 0x788d20
    // IsAbleToEquip(...)                        @ 0xaea9e0
    // CalcEquipItemQuality(ZRef<GW_ItemSlotBase>) @ 0xaed3a0
    // IterateItemInfo(void)                     @ 0xafb5d0
    // GetItemPrice(long, long&, double&)        @ 0xaf4db0
    // GetItemCoolTime(long, long&, long&)       @ 0xafa8c0

private:
    ItemInfo() = default;

    // Non-copyable, non-movable (inherited from Singleton)
    ItemInfo(const ItemInfo&) = delete;
    auto operator=(const ItemInfo&) -> ItemInfo& = delete;
    ItemInfo(ItemInfo&&) = delete;
    auto operator=(ItemInfo&&) -> ItemInfo& = delete;

    // --- WZ Registration (private) ---
    auto RegisterEquipItemInfo(std::int32_t nItemID, const std::string& sUOL) -> std::shared_ptr<EquipItem>;   // @ 0xad9ca0

    // ============================================================
    // Member variables — from constructor @ 0xafad70
    // Using std::map / std::shared_ptr as stand-ins for ZMap / ZRef
    // until those containers are implemented.
    // ============================================================

    // --- Item data caches ---
    std::map<std::int32_t, std::shared_ptr<EquipItem>> m_mEquipItem;
    std::map<std::int32_t, std::shared_ptr<BundleItem>> m_mBundleItem;
    std::map<std::int32_t, std::shared_ptr<GrowthOption>> m_mGrowthOptionItem;

    // --- Item string / map string tables ---
    std::map<std::int32_t, std::map<std::string, std::string>> m_mItemString;
    std::map<std::uint32_t, std::map<std::string, std::string>> m_mMapString;

    // --- Item ID set ---
    std::set<std::int32_t> m_sItemID;

    // --- Set item system ---
    std::map<std::int32_t, std::shared_ptr<SetItemInfo>> m_mSetItemInfo;
    std::list<SetEffect> m_lSetItemEffect;
    std::list<SetAction> m_lSetItemAction;

    // --- Specialized item registries ---
    std::map<std::int32_t, std::shared_ptr<PieceItemInfo>> m_mPieceItemInfo;
    std::map<std::int32_t, std::shared_ptr<SetTowerChair>> m_mSetTowerChairInfo;
    std::map<std::int32_t, std::int32_t> m_mSetTowerChairItemInfo;
    std::map<std::int32_t, std::shared_ptr<PetFoodItem>> m_mPetFoodItem;
    std::map<std::int32_t, std::shared_ptr<BridleItem>> m_mBridleItem;
    std::map<std::int32_t, std::shared_ptr<ExtendExpireDateItem>> m_mExtendExpireDateItem;
    std::map<std::int32_t, std::shared_ptr<ExpiredProtectingItem>> m_mExpiredProtectingItem;
    std::map<std::int32_t, std::shared_ptr<ProtectOnDieItem>> m_mProtectOnDieItem;
    std::map<std::int32_t, std::shared_ptr<KarmaScissorsItem>> m_mKarmaScissorsItem;
    std::map<std::int32_t, std::shared_ptr<BagInfo>> m_mBagItem;
    std::map<std::int32_t, std::shared_ptr<GatheringToolItem>> m_mGatheringToolItem;
    std::map<std::int32_t, std::shared_ptr<RecipeOpenItem>> m_mRecipeOpenItem;
    std::map<std::int32_t, std::shared_ptr<ItemPotCreateItem>> m_mItemPotCreateItem;
    std::map<std::int32_t, std::shared_ptr<ItemPotCureItem>> m_mItemPotCureItem;
    std::map<std::int32_t, std::shared_ptr<DecomposerInstallItem>> m_mDecomposerInstallItem;
    std::map<std::int32_t, std::shared_ptr<EquipSlotLevelMinusItem>> m_mEquipSlotLevelMinusItem;
    std::map<std::int32_t, std::shared_ptr<DyeingItem>> m_mDyeingItem;
    std::map<std::int32_t, std::shared_ptr<DressUpClothesItem>> m_mDressUpClothesItem;
    std::map<std::int32_t, std::shared_ptr<DressUpClothesItem>> m_mDressUpClothesItemByClothesID;
    std::map<std::int32_t, std::shared_ptr<CoreItem>> m_mCoreItem;
    std::map<std::int32_t, std::shared_ptr<AreaBuffItem>> m_mAreaBuffItem;
    std::map<std::int32_t, std::shared_ptr<BitsCaseItem>> m_mBitsCaseItem;
    std::map<std::int32_t, std::shared_ptr<GachaponItemInfo>> m_mGachaponItemInfo;
    std::map<std::int32_t, std::shared_ptr<CoupleChairItem>> m_mCoupleChairItem;
    std::map<std::int32_t, std::shared_ptr<GroupEffectInfo>> m_mGroupEffectInfo;

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
