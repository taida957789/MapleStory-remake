#pragma once

#include "util/security/ZtlSecureTear.h"

#include <array>
#include <cstdint>
#include <map>
#include <queue>
#include <set>
#include <vector>

namespace ms
{

class InPacket;
class OutPacket;

/**
 * @brief Secondary (temporary) stat storage for buff effects
 *
 * Based on SecondaryStat (__cppobj) from the original MapleStory client.
 *
 * Naming convention for buff fields:
 *   n<Name>_  - buff value (effect magnitude)
 *   r<Name>_  - reason (skill ID that granted the buff)
 *   t<Name>_  - time (remaining duration / expiry tick)
 *   m<Name>_  - extra parameter (max, multiplier, etc.)
 *   x<Name>_  - extended parameter
 *   c<Name>_  - count parameter
 *   b<Name>_  - boolean flag
 *   p<Name>_  - point / position
 *   w<Name>_  - weapon / secondary parameter
 *   s<Name>_  - sub-parameter
 *   o<Name>_  - original value
 *
 * Fields WITHOUT trailing underscore are standalone values
 * (not managed by the buff n/r/t system).
 */
class SecondaryStat
{
public:
    // === Inner types ========================================================

    struct LarknessInfo
    {
        ZtlSecureTear<int> rLarkness;
        ZtlSecureTear<int> tLarkness;

        void Decode(InPacket& iPacket);
    };

    struct StopForceAtom
    {
        std::int32_t nIdx{};
        std::int32_t nCount{};
        std::int32_t nWeaponID{};
        std::vector<std::int32_t> aAngleInfo;

        void Init();
        void Decode(InPacket& iPacket);
        void Encode(OutPacket& oPacket);
    };

    // === Basic stats ========================================================

    ZtlSecureTear<int> nSTR_;
    ZtlSecureTear<int> rSTR_;
    ZtlSecureTear<int> tSTR_;

    ZtlSecureTear<int> nINT_;
    ZtlSecureTear<int> rINT_;
    ZtlSecureTear<int> tINT_;

    ZtlSecureTear<int> nDEX_;
    ZtlSecureTear<int> rDEX_;
    ZtlSecureTear<int> tDEX_;

    ZtlSecureTear<int> nLUK_;
    ZtlSecureTear<int> rLUK_;
    ZtlSecureTear<int> tLUK_;

    // === PVP stats ==========================================================

    ZtlSecureTear<int> nPVPDamage;
    ZtlSecureTear<int> nPVPDamage_;
    ZtlSecureTear<int> rPVPDamage_;
    ZtlSecureTear<int> tPVPDamage_;

    ZtlSecureTear<int> nPVPDamageSkill_;
    ZtlSecureTear<int> rPVPDamageSkill_;
    ZtlSecureTear<int> tPVPDamageSkill_;

    ZtlSecureTear<int> nIncMaxDamage;
    ZtlSecureTear<int> nIncMaxDamage_;
    ZtlSecureTear<int> rIncMaxDamage_;
    ZtlSecureTear<int> tIncMaxDamage_;

    // === Combat stats =======================================================

    // PAD
    ZtlSecureTear<int> nPAD;
    ZtlSecureTear<int> nPAD_;
    ZtlSecureTear<int> rPAD_;
    ZtlSecureTear<int> tPAD_;
    ZtlSecureTear<int> nItemPADR;

    // PDD
    ZtlSecureTear<int> nPDD;
    ZtlSecureTear<int> nPDD_;
    ZtlSecureTear<int> rPDD_;
    ZtlSecureTear<int> tPDD_;
    ZtlSecureTear<int> nItemPDDR;

    // MAD
    ZtlSecureTear<int> nMAD;
    ZtlSecureTear<int> nMAD_;
    ZtlSecureTear<int> rMAD_;
    ZtlSecureTear<int> tMAD_;
    ZtlSecureTear<int> nItemMADR;

    // MDD
    ZtlSecureTear<int> nMDD;
    ZtlSecureTear<int> nMDD_;
    ZtlSecureTear<int> rMDD_;
    ZtlSecureTear<int> tMDD_;
    ZtlSecureTear<int> nItemMDDR;

    // ACC
    ZtlSecureTear<int> nACC;
    ZtlSecureTear<int> nACC_;
    ZtlSecureTear<int> rACC_;
    ZtlSecureTear<int> tACC_;
    ZtlSecureTear<int> nItemACCR;

    // EVA
    ZtlSecureTear<int> nEVA;
    ZtlSecureTear<int> nEVA_;
    ZtlSecureTear<int> rEVA_;
    ZtlSecureTear<int> tEVA_;
    ZtlSecureTear<int> nItemEVAR;

    // EVAR
    ZtlSecureTear<int> nEVAR_;
    ZtlSecureTear<int> rEVAR_;
    ZtlSecureTear<int> mEVAR_;
    ZtlSecureTear<int> tEVAR_;

    // Craft / Speed / Jump
    ZtlSecureTear<int> nCraft;
    ZtlSecureTear<int> nCraft_;
    ZtlSecureTear<int> rCraft_;
    ZtlSecureTear<int> tCraft_;

    ZtlSecureTear<int> nSpeed;
    ZtlSecureTear<int> nSpeed_;
    ZtlSecureTear<int> rSpeed_;
    ZtlSecureTear<int> tSpeed_;

    ZtlSecureTear<int> nJump;
    ZtlSecureTear<int> nJump_;
    ZtlSecureTear<int> rJump_;
    ZtlSecureTear<int> tJump_;

    // === Warrior / Mage buffs ===============================================

    ZtlSecureTear<int> nMagicGuard_;
    ZtlSecureTear<int> rMagicGuard_;
    ZtlSecureTear<int> tMagicGuard_;

    ZtlSecureTear<int> nDarkSight_;
    ZtlSecureTear<int> rDarkSight_;
    ZtlSecureTear<int> tDarkSight_;
    ZtlSecureTear<int> mDarkSight_;
    ZtlSecureTear<int> pDarkSight_;
    ZtlSecureTear<int> xDarkSight_;
    ZtlSecureTear<int> cDarkSight_;

    ZtlSecureTear<int> nBooster_;
    ZtlSecureTear<int> rBooster_;
    ZtlSecureTear<int> tBooster_;

    ZtlSecureTear<int> nPowerGuard_;
    ZtlSecureTear<int> rPowerGuard_;
    ZtlSecureTear<int> tPowerGuard_;

    ZtlSecureTear<int> nMechanic_;
    ZtlSecureTear<int> rMechanic_;
    ZtlSecureTear<int> tMechanic_;

    ZtlSecureTear<int> nDrawBack_;
    ZtlSecureTear<int> rDrawBack_;
    ZtlSecureTear<int> tDrawBack_;

    ZtlSecureTear<int> nMaxHP_;
    ZtlSecureTear<int> rMaxHP_;
    ZtlSecureTear<int> tMaxHP_;

    ZtlSecureTear<int> nMaxMP_;
    ZtlSecureTear<int> rMaxMP_;
    ZtlSecureTear<int> tMaxMP_;

    ZtlSecureTear<int> nInvincible_;
    ZtlSecureTear<int> rInvincible_;
    ZtlSecureTear<int> tInvincible_;

    ZtlSecureTear<int> nSoulArrow_;
    ZtlSecureTear<int> rSoulArrow_;
    ZtlSecureTear<int> tSoulArrow_;

    // === Debuffs =============================================================

    ZtlSecureTear<int> nStun_;
    ZtlSecureTear<int> rStun_;
    ZtlSecureTear<int> tStun_;

    ZtlSecureTear<int> nShock_;
    ZtlSecureTear<int> rShock_;
    ZtlSecureTear<int> tShock_;

    ZtlSecureTear<int> nPoison_;
    ZtlSecureTear<int> rPoison_;
    ZtlSecureTear<int> tPoison_;

    ZtlSecureTear<int> nSeal_;
    ZtlSecureTear<int> rSeal_;
    ZtlSecureTear<int> tSeal_;

    ZtlSecureTear<int> nDarkness_;
    ZtlSecureTear<int> rDarkness_;
    ZtlSecureTear<int> tDarkness_;

    // === Combo / Charge =====================================================

    ZtlSecureTear<int> nComboCounter_;
    ZtlSecureTear<int> rComboCounter_;
    ZtlSecureTear<int> tComboCounter_;
    ZtlSecureTear<int> mComboCounter_;

    ZtlSecureTear<int> nWeaponCharge_;
    ZtlSecureTear<int> rWeaponCharge_;
    ZtlSecureTear<int> tWeaponCharge_;

    ZtlSecureTear<int> nElementalCharge_;
    ZtlSecureTear<int> rElementalCharge_;
    ZtlSecureTear<int> tElementalCharge_;
    ZtlSecureTear<int> mElementalCharge_;
    ZtlSecureTear<int> wElementalCharge_;
    ZtlSecureTear<int> uElementalCharge_;
    ZtlSecureTear<int> zElementalCharge_;

    // === Party buffs ========================================================

    ZtlSecureTear<int> nHolySymbol_;
    ZtlSecureTear<int> rHolySymbol_;
    ZtlSecureTear<int> tHolySymbol_;
    ZtlSecureTear<int> nHolySymbolDrop_;

    ZtlSecureTear<int> nMesoUp_;
    ZtlSecureTear<int> rMesoUp_;
    ZtlSecureTear<int> tMesoUp_;

    ZtlSecureTear<int> nShadowPartner_;
    ZtlSecureTear<int> rShadowPartner_;
    ZtlSecureTear<int> tShadowPartner_;

    ZtlSecureTear<int> nPickPocket_;
    ZtlSecureTear<int> rPickPocket_;
    ZtlSecureTear<int> tPickPocket_;

    ZtlSecureTear<int> nMesoGuard_;
    ZtlSecureTear<int> rMesoGuard_;
    ZtlSecureTear<int> tMesoGuard_;

    // === Status effects =====================================================

    ZtlSecureTear<int> nThaw_;
    ZtlSecureTear<int> rThaw_;
    ZtlSecureTear<int> tThaw_;

    ZtlSecureTear<int> nWeakness_;
    ZtlSecureTear<int> rWeakness_;
    ZtlSecureTear<int> tWeakness_;

    ZtlSecureTear<int> nWeaknessMdamage_;
    ZtlSecureTear<int> rWeaknessMdamage_;
    ZtlSecureTear<int> tWeaknessMdamage_;

    ZtlSecureTear<int> nCurse_;
    ZtlSecureTear<int> rCurse_;
    ZtlSecureTear<int> tCurse_;

    ZtlSecureTear<int> nSlow_;
    ZtlSecureTear<int> rSlow_;
    ZtlSecureTear<int> tSlow_;
    ZtlSecureTear<bool> bSlowIgnoreMoveSkill_;

    // === Misc buffs =========================================================

    ZtlSecureTear<int> nArcaneAim_;
    ZtlSecureTear<int> rArcaneAim_;
    ZtlSecureTear<int> tArcaneAim_;

    ZtlSecureTear<int> nTimeBomb_;
    ZtlSecureTear<int> rTimeBomb_;
    ZtlSecureTear<int> tTimeBomb_;

    ZtlSecureTear<int> nBuffLimit_;
    ZtlSecureTear<int> rBuffLimit_;
    ZtlSecureTear<int> tBuffLimit_;

    ZtlSecureTear<int> nDisOrder_;
    ZtlSecureTear<int> rDisOrder_;
    ZtlSecureTear<int> tDisOrder_;

    ZtlSecureTear<int> nThread_;
    ZtlSecureTear<int> rThread_;
    ZtlSecureTear<int> tThread_;

    ZtlSecureTear<int> nMorph_;
    ZtlSecureTear<int> rMorph_;
    ZtlSecureTear<int> tMorph_;

    ZtlSecureTear<int> nGhost_;
    ZtlSecureTear<int> rGhost_;
    ZtlSecureTear<int> tGhost_;

    ZtlSecureTear<int> nRegen_;
    ZtlSecureTear<int> rRegen_;
    ZtlSecureTear<int> tRegen_;

    ZtlSecureTear<int> nBasicStatUp_;
    ZtlSecureTear<int> rBasicStatUp_;
    ZtlSecureTear<int> tBasicStatUp_;

    ZtlSecureTear<int> nStance_;
    ZtlSecureTear<int> rStance_;
    ZtlSecureTear<int> tStance_;

    ZtlSecureTear<int> nSharpEyes_;
    ZtlSecureTear<int> rSharpEyes_;
    ZtlSecureTear<int> tSharpEyes_;
    ZtlSecureTear<int> mSharpEyes_;

    ZtlSecureTear<int> nManaReflection_;
    ZtlSecureTear<int> rManaReflection_;
    ZtlSecureTear<int> tManaReflection_;

    ZtlSecureTear<int> nAttract_;
    ZtlSecureTear<int> rAttract_;
    ZtlSecureTear<int> tAttract_;

    ZtlSecureTear<int> nMagnet_;
    ZtlSecureTear<int> rMagnet_;
    ZtlSecureTear<int> tMagnet_;

    ZtlSecureTear<int> nMagnetArea_;
    ZtlSecureTear<int> rMagnetArea_;
    ZtlSecureTear<int> tMagnetArea_;

    ZtlSecureTear<int> nNoBulletConsume_;
    ZtlSecureTear<int> rNoBulletConsume_;
    ZtlSecureTear<int> tNoBulletConsume_;
    ZtlSecureTear<int> mNoBulletConsume_;

    ZtlSecureTear<int> nInfinity_;
    ZtlSecureTear<int> rInfinity_;
    ZtlSecureTear<int> tInfinity_;
    ZtlSecureTear<int> tUpdateInfinity_;

    ZtlSecureTear<int> nStackBuff_;
    ZtlSecureTear<int> rStackBuff_;
    ZtlSecureTear<int> tStackBuff_;
    ZtlSecureTear<int> mStackBuff_;

    ZtlSecureTear<int> nTrinity_;
    ZtlSecureTear<int> rTrinity_;
    ZtlSecureTear<int> tTrinity_;
    ZtlSecureTear<int> mTrinity_;

    ZtlSecureTear<int> nAdvancedBless_;
    ZtlSecureTear<int> rAdvancedBless_;
    ZtlSecureTear<int> tAdvancedBless_;
    ZtlSecureTear<int> xAdvancedBless_;

    ZtlSecureTear<int> nUsefulAdvancedBless_;
    ZtlSecureTear<int> rUsefulAdvancedBless_;
    ZtlSecureTear<int> tUsefulAdvancedBless_;

    ZtlSecureTear<int> nIllusionStep_;
    ZtlSecureTear<int> rIllusionStep_;
    ZtlSecureTear<int> tIllusionStep_;

    ZtlSecureTear<int> nBlind_;
    ZtlSecureTear<int> rBlind_;
    ZtlSecureTear<int> tBlind_;

    ZtlSecureTear<int> nConcentration_;
    ZtlSecureTear<int> rConcentration_;
    ZtlSecureTear<int> tConcentration_;

    ZtlSecureTear<int> nBanMap_;
    ZtlSecureTear<int> rBanMap_;
    ZtlSecureTear<int> tBanMap_;
    ZtlSecureTear<int> mBanMap_;

    ZtlSecureTear<int> nMaxLevelBuff_;
    ZtlSecureTear<int> rMaxLevelBuff_;
    ZtlSecureTear<int> tMaxLevelBuff_;

    ZtlSecureTear<int> nBarrier_;
    ZtlSecureTear<int> tBarrier_;
    ZtlSecureTear<int> rBarrier_;

    // === Dojang ==============================================================

    ZtlSecureTear<int> nDojangShield_;
    ZtlSecureTear<int> tDojangShield_;
    ZtlSecureTear<int> rDojangShield_;

    ZtlSecureTear<int> nReverseInput_;
    ZtlSecureTear<int> rReverseInput_;
    ZtlSecureTear<int> tReverseInput_;

    ZtlSecureTear<int> nDojangBerserk_;
    ZtlSecureTear<int> rDojangBerserk_;
    ZtlSecureTear<int> tDojangBerserk_;

    ZtlSecureTear<int> nDojangInvincible_;
    ZtlSecureTear<int> rDojangInvincible_;
    ZtlSecureTear<int> tDojangInvincible_;

    // === Item buffs =========================================================

    ZtlSecureTear<int> nMesoUpByItem;
    ZtlSecureTear<int> nMesoUpByItem_;
    ZtlSecureTear<int> rMesoUpByItem_;
    ZtlSecureTear<int> tMesoUpByItem_;

    ZtlSecureTear<int> nItemUpByItem;
    ZtlSecureTear<int> nItemUpByItem_;
    ZtlSecureTear<int> rItemUpByItem_;
    ZtlSecureTear<int> tItemUpByItem_;
    ZtlSecureTear<int> xItemUpByItem_;
    ZtlSecureTear<int> sItemUpByItem_;

    // === Immune / Defense ====================================================

    ZtlSecureTear<int> nRespectPImmune;
    ZtlSecureTear<int> nRespectPImmune_;
    ZtlSecureTear<int> rRespectPImmune_;
    ZtlSecureTear<int> tRespectPImmune_;

    ZtlSecureTear<int> nRespectMImmune;
    ZtlSecureTear<int> nRespectMImmune_;
    ZtlSecureTear<int> rRespectMImmune_;
    ZtlSecureTear<int> tRespectMImmune_;

    ZtlSecureTear<char> nDefenseAtt;
    ZtlSecureTear<int> nDefenseAtt_;
    ZtlSecureTear<int> rDefenseAtt_;
    ZtlSecureTear<int> tDefenseAtt_;

    ZtlSecureTear<char> nDefenseState;
    ZtlSecureTear<int> nDefenseState_;
    ZtlSecureTear<int> rDefenseState_;
    ZtlSecureTear<int> tDefenseState_;

    // === Cygnus Knights =====================================================

    ZtlSecureTear<int> nSoulMasterFinal_;
    ZtlSecureTear<int> rSoulMasterFinal_;
    ZtlSecureTear<int> tSoulMasterFinal_;

    ZtlSecureTear<int> nWindBreakerFinal_;
    ZtlSecureTear<int> rWindBreakerFinal_;
    ZtlSecureTear<int> tWindBreakerFinal_;

    ZtlSecureTear<int> nElementalReset_;
    ZtlSecureTear<int> rElementalReset_;
    ZtlSecureTear<int> tElementalReset_;

    ZtlSecureTear<int> nHideAttack_;
    ZtlSecureTear<int> rHideAttack_;
    ZtlSecureTear<int> tHideAttack_;

    ZtlSecureTear<int> nEventRate_;
    ZtlSecureTear<int> rEventRate_;
    ZtlSecureTear<int> tEventRate_;

    // === Combo skills =======================================================

    ZtlSecureTear<int> nComboAbilityBuff_;
    ZtlSecureTear<int> rComboAbilityBuff_;
    ZtlSecureTear<int> tComboAbilityBuff_;

    ZtlSecureTear<int> nComboDrain_;
    ZtlSecureTear<int> rComboDrain_;
    ZtlSecureTear<int> tComboDrain_;

    ZtlSecureTear<int> nComboBarrier_;
    ZtlSecureTear<int> rComboBarrier_;
    ZtlSecureTear<int> tComboBarrier_;

    ZtlSecureTear<int> nPartyBarrier_;
    ZtlSecureTear<int> rPartyBarrier_;
    ZtlSecureTear<int> tPartyBarrier_;

    ZtlSecureTear<int> nBodyPressure_;
    ZtlSecureTear<int> rBodyPressure_;
    ZtlSecureTear<int> tBodyPressure_;

    ZtlSecureTear<int> nRepeatEffect_;
    ZtlSecureTear<int> rRepeatEffect_;
    ZtlSecureTear<int> tRepeatEffect_;

    ZtlSecureTear<int> nExpBuffRate_;
    ZtlSecureTear<int> rExpBuffRate_;
    ZtlSecureTear<int> tExpBuffRate_;

    ZtlSecureTear<int> nStopPortion_;
    ZtlSecureTear<int> rStopPortion_;
    ZtlSecureTear<int> tStopPortion_;

    ZtlSecureTear<int> nStopMotion_;
    ZtlSecureTear<int> rStopMotion_;
    ZtlSecureTear<int> tStopMotion_;

    ZtlSecureTear<int> nFear_;
    ZtlSecureTear<int> rFear_;
    ZtlSecureTear<int> tFear_;

    ZtlSecureTear<int> nMagicShield_;
    ZtlSecureTear<int> rMagicShield_;
    ZtlSecureTear<int> tMagicShield_;
    ZtlSecureTear<int> mMagicShield_;

    ZtlSecureTear<int> nMagicResistance_;
    ZtlSecureTear<int> rMagicResistance_;
    ZtlSecureTear<int> tMagicResistance_;

    ZtlSecureTear<int> nSoulStone_;
    ZtlSecureTear<int> rSoulStone_;
    ZtlSecureTear<int> tSoulStone_;

    ZtlSecureTear<int> nReviveOnce_;
    ZtlSecureTear<int> rReviveOnce_;
    ZtlSecureTear<int> tReviveOnce_;

    ZtlSecureTear<int> nAntiMagicShell_;
    ZtlSecureTear<int> rAntiMagicShell_;
    ZtlSecureTear<int> tAntiMagicShell_;
    ZtlSecureTear<bool> bAntiMagicShell_;

    // === Flying ==============================================================

    ZtlSecureTear<int> nFlying_;
    ZtlSecureTear<int> rFlying_;
    ZtlSecureTear<int> tFlying_;

    ZtlSecureTear<int> nNewFlying_;
    ZtlSecureTear<int> rNewFlying_;
    ZtlSecureTear<int> tNewFlying_;

    ZtlSecureTear<int> nNaviFlying_;
    ZtlSecureTear<int> rNaviFlying_;
    ZtlSecureTear<int> tNaviFlying_;

    // === Frozen / Web / Enrage ==============================================

    ZtlSecureTear<int> nFrozen_;
    ZtlSecureTear<int> rFrozen_;
    ZtlSecureTear<int> tFrozen_;

    ZtlSecureTear<int> nFrozen2_;
    ZtlSecureTear<int> rFrozen2_;
    ZtlSecureTear<int> tFrozen2_;

    ZtlSecureTear<int> nWeb_;
    ZtlSecureTear<int> rWeb_;
    ZtlSecureTear<int> tWeb_;

    ZtlSecureTear<int> nEnrage_;
    ZtlSecureTear<int> rEnrage_;
    ZtlSecureTear<int> tEnrage_;

    ZtlSecureTear<int> nSuddenDeath_;
    ZtlSecureTear<int> rSuddenDeath_;
    ZtlSecureTear<int> tSuddenDeath_;
    ZtlSecureTear<int> mSuddenDeath_;

    ZtlSecureTear<int> nNotDamaged_;
    ZtlSecureTear<int> rNotDamaged_;
    ZtlSecureTear<int> tNotDamaged_;

    ZtlSecureTear<int> nFinalCut_;
    ZtlSecureTear<int> rFinalCut_;
    ZtlSecureTear<int> tFinalCut_;

    // === Wild Hunter ========================================================

    ZtlSecureTear<int> nHowlingAttackDamage_;
    ZtlSecureTear<int> rHowlingAttackDamage_;
    ZtlSecureTear<int> tHowlingAttackDamage_;

    ZtlSecureTear<int> nBeastFormDamageUp_;
    ZtlSecureTear<int> rBeastFormDamageUp_;
    ZtlSecureTear<int> tBeastFormDamageUp_;

    // === Evan dragon ========================================================

    ZtlSecureTear<int> nEMHP_;
    ZtlSecureTear<int> rEMHP_;
    ZtlSecureTear<int> tEMHP_;

    ZtlSecureTear<int> nEMMP_;
    ZtlSecureTear<int> rEMMP_;
    ZtlSecureTear<int> tEMMP_;

    ZtlSecureTear<int> nEPAD_;
    ZtlSecureTear<int> rEPAD_;
    ZtlSecureTear<int> tEPAD_;

    ZtlSecureTear<int> nEMAD_;
    ZtlSecureTear<int> rEMAD_;
    ZtlSecureTear<int> tEMAD_;

    ZtlSecureTear<int> nEPDD_;
    ZtlSecureTear<int> rEPDD_;
    ZtlSecureTear<int> tEPDD_;

    ZtlSecureTear<int> nEMDD_;
    ZtlSecureTear<int> rEMDD_;
    ZtlSecureTear<int> tEMDD_;

    // === Guard / Mine / Cyclone =============================================

    ZtlSecureTear<int> nGuard_;
    ZtlSecureTear<int> rGuard_;
    ZtlSecureTear<int> tGuard_;

    ZtlSecureTear<int> nMine_;
    ZtlSecureTear<int> rMine_;
    ZtlSecureTear<int> tMine_;

    ZtlSecureTear<int> nCyclone_;
    ZtlSecureTear<int> rCyclone_;
    ZtlSecureTear<int> tCyclone_;

    ZtlSecureTear<int> nOnCapsule_;
    ZtlSecureTear<int> rOnCapsule_;
    ZtlSecureTear<int> tOnCapsule_;

    ZtlSecureTear<int> nHowlingCritical_;
    ZtlSecureTear<int> rHowlingCritical_;
    ZtlSecureTear<int> tHowlingCritical_;

    ZtlSecureTear<int> nHowlingMaxMP_;
    ZtlSecureTear<int> rHowlingMaxMP_;
    ZtlSecureTear<int> tHowlingMaxMP_;

    ZtlSecureTear<int> nHowlingDefence_;
    ZtlSecureTear<int> rHowlingDefence_;
    ZtlSecureTear<int> tHowlingDefence_;

    ZtlSecureTear<int> nHowlingEvasion_;
    ZtlSecureTear<int> rHowlingEvasion_;
    ZtlSecureTear<int> tHowlingEvasion_;

    ZtlSecureTear<int> nConversion_;
    ZtlSecureTear<int> rConversion_;
    ZtlSecureTear<int> tConversion_;

    ZtlSecureTear<int> nRevive_;
    ZtlSecureTear<int> rRevive_;
    ZtlSecureTear<int> tRevive_;

    ZtlSecureTear<int> nPinkbeanMinibeenMove_;
    ZtlSecureTear<int> rPinkbeanMinibeenMove_;
    ZtlSecureTear<int> tPinkbeanMinibeenMove_;

    ZtlSecureTear<int> nSneak_;
    ZtlSecureTear<int> rSneak_;
    ZtlSecureTear<int> tSneak_;

    ZtlSecureTear<int> nBeastFormMaxHP_;
    ZtlSecureTear<int> rBeastFormMaxHP_;
    ZtlSecureTear<int> tBeastFormMaxHP_;

    // === Dice ================================================================

    ZtlSecureTear<int> nDice_;
    ZtlSecureTear<int> rDice_;
    ZtlSecureTear<int> tDice_;
    ZtlSecureTear<int> oDice_;
    std::array<std::int32_t, 22> aDiceInfo{};

    // === Blessing / Holy ====================================================

    ZtlSecureTear<int> nBlessingArmor_;
    ZtlSecureTear<int> rBlessingArmor_;
    ZtlSecureTear<int> tBlessingArmor_;

    ZtlSecureTear<int> nBlessingArmorIncPAD_;
    ZtlSecureTear<int> rBlessingArmorIncPAD_;
    ZtlSecureTear<int> tBlessingArmorIncPAD_;

    ZtlSecureTear<int> nHolyMagicShell_;
    ZtlSecureTear<int> rHolyMagicShell_;
    ZtlSecureTear<int> tHolyMagicShell_;
    ZtlSecureTear<int> tHolyMagicShellReUse_;

    ZtlSecureTear<int> nDamR_;
    ZtlSecureTear<int> rDamR_;
    ZtlSecureTear<int> tDamR_;

    ZtlSecureTear<int> nTeleportMasteryOn_;
    ZtlSecureTear<int> rTeleportMasteryOn_;
    ZtlSecureTear<int> tTeleportMasteryOn_;

    ZtlSecureTear<int> nCombatOrders_;
    ZtlSecureTear<int> rCombatOrders_;
    ZtlSecureTear<int> tCombatOrders_;

    // === Beholder / Dispel ==================================================

    ZtlSecureTear<int> nBeholder_;
    ZtlSecureTear<int> rBeholder_;
    ZtlSecureTear<int> tBeholder_;
    ZtlSecureTear<int> sBeholder_;
    ZtlSecureTear<int> ssBeholder_;

    ZtlSecureTear<int> nDispelItemOption_;
    ZtlSecureTear<int> rDispelItemOption_;
    ZtlSecureTear<int> tDispelItemOption_;

    ZtlSecureTear<int> nDispelItemOptionByField_;
    ZtlSecureTear<int> rDispelItemOptionByField_;
    ZtlSecureTear<int> tDispelItemOptionByField_;

    ZtlSecureTear<int> nInflation_;
    ZtlSecureTear<int> rInflation_;
    ZtlSecureTear<int> tInflation_;

    ZtlSecureTear<int> nOnixDivineProtection_;
    ZtlSecureTear<int> rOnixDivineProtection_;
    ZtlSecureTear<int> tOnixDivineProtection_;

    ZtlSecureTear<int> nBless_;
    ZtlSecureTear<int> rBless_;
    ZtlSecureTear<int> tBless_;

    ZtlSecureTear<int> nTeam_;
    ZtlSecureTear<int> rTeam_;
    ZtlSecureTear<int> tTeam_;

    ZtlSecureTear<int> nExplosion_;
    ZtlSecureTear<int> rExplosion_;
    ZtlSecureTear<int> tExplosion_;

    ZtlSecureTear<int> nDarkTornado_;
    ZtlSecureTear<int> rDarkTornado_;
    ZtlSecureTear<int> tDarkTornado_;

    ZtlSecureTear<int> nIncMaxHP_;
    ZtlSecureTear<int> rIncMaxHP_;
    ZtlSecureTear<int> tIncMaxHP_;

    ZtlSecureTear<int> nIncMaxMP_;
    ZtlSecureTear<int> rIncMaxMP_;
    ZtlSecureTear<int> tIncMaxMP_;

    // === PvP buffs ==========================================================

    ZtlSecureTear<int> nPvPScoreBonus_;
    ZtlSecureTear<int> rPvPScoreBonus_;
    ZtlSecureTear<int> tPvPScoreBonus_;

    ZtlSecureTear<int> nPvPInvincible_;
    ZtlSecureTear<int> rPvPInvincible_;
    ZtlSecureTear<int> tPvPInvincible_;

    ZtlSecureTear<int> nPvPRaceEffect_;
    ZtlSecureTear<int> rPvPRaceEffect_;
    ZtlSecureTear<int> tPvPRaceEffect_;

    ZtlSecureTear<int> nAmplifyDamage_;
    ZtlSecureTear<int> rAmplifyDamage_;
    ZtlSecureTear<int> tAmplifyDamage_;

    ZtlSecureTear<int> nIceKnight_;
    ZtlSecureTear<int> rIceKnight_;
    ZtlSecureTear<int> tIceKnight_;

    ZtlSecureTear<int> nInfinityForce_;
    ZtlSecureTear<int> rInfinityForce_;
    ZtlSecureTear<int> tInfinityForce_;

    ZtlSecureTear<int> nKeyDownTimeIgnore_;
    ZtlSecureTear<int> rKeyDownTimeIgnore_;
    ZtlSecureTear<int> tKeyDownTimeIgnore_;

    ZtlSecureTear<int> nMasterMagicOn_;
    ZtlSecureTear<int> rMasterMagicOn_;
    ZtlSecureTear<int> tMasterMagicOn_;

    // === Resistance / Awake =================================================

    ZtlSecureTear<int> nAsrR_;
    ZtlSecureTear<int> rAsrR_;
    ZtlSecureTear<int> tAsrR_;

    ZtlSecureTear<int> nAsrRByItem_;
    ZtlSecureTear<int> rAsrRByItem_;
    ZtlSecureTear<int> tAsrRByItem_;

    ZtlSecureTear<int> nTerR_;
    ZtlSecureTear<int> rTerR_;
    ZtlSecureTear<int> tTerR_;

    ZtlSecureTear<int> nAwake_;
    ZtlSecureTear<int> rAwake_;
    ZtlSecureTear<int> tAwake_;

    ZtlSecureTear<int> nDevilishPower_;
    ZtlSecureTear<int> rDevilishPower_;
    ZtlSecureTear<int> tDevilishPower_;
    ZtlSecureTear<int> tUpdateDevilishPower_;

    ZtlSecureTear<int> nDamAbsorbShield_;
    ZtlSecureTear<int> rDamAbsorbShield_;
    ZtlSecureTear<int> tDamAbsorbShield_;

    ZtlSecureTear<int> nRoulette_;
    ZtlSecureTear<int> rRoulette_;
    ZtlSecureTear<int> tRoulette_;

    ZtlSecureTear<int> nEvent_;
    ZtlSecureTear<int> rEvent_;
    ZtlSecureTear<int> tEvent_;

    ZtlSecureTear<int> nSpiritLink_;
    ZtlSecureTear<int> rSpiritLink_;
    ZtlSecureTear<int> tSpiritLink_;

    ZtlSecureTear<int> nCriticalBuff_;
    ZtlSecureTear<int> rCriticalBuff_;
    ZtlSecureTear<int> tCriticalBuff_;

    ZtlSecureTear<int> nDropRate_;
    ZtlSecureTear<int> rDropRate_;
    ZtlSecureTear<int> tDropRate_;

    ZtlSecureTear<int> nPlusExpRate_;
    ZtlSecureTear<int> rPlusExpRate_;
    ZtlSecureTear<int> tPlusExpRate_;

    // === Item effects =======================================================

    ZtlSecureTear<int> nItemInvincible_;
    ZtlSecureTear<int> rItemInvincible_;
    ZtlSecureTear<int> tItemInvincible_;

    ZtlSecureTear<int> nItemCritical_;
    ZtlSecureTear<int> rItemCritical_;
    ZtlSecureTear<int> tItemCritical_;

    ZtlSecureTear<int> nItemEvade_;
    ZtlSecureTear<int> rItemEvade_;
    ZtlSecureTear<int> tItemEvade_;

    ZtlSecureTear<int> nEvent2_;
    ZtlSecureTear<int> rEvent2_;
    ZtlSecureTear<int> tEvent2_;

    ZtlSecureTear<int> nVampiricTouch_;
    ZtlSecureTear<int> rVampiricTouch_;
    ZtlSecureTear<int> tVampiricTouch_;
    ZtlSecureTear<int> tVampiricTouchCoolTime_;

    ZtlSecureTear<int> nDDR_;
    ZtlSecureTear<int> rDDR_;
    ZtlSecureTear<int> tDDR_;

    // === Critical / Resistance increments ===================================

    ZtlSecureTear<int> nIncCriticalDamMin;
    ZtlSecureTear<int> nIncCriticalDamMin_;
    ZtlSecureTear<int> rIncCriticalDamMin_;
    ZtlSecureTear<int> tIncCriticalDamMin_;

    ZtlSecureTear<int> nIncCriticalDamMax;
    ZtlSecureTear<int> nIncCriticalDamMax_;
    ZtlSecureTear<int> rIncCriticalDamMax_;
    ZtlSecureTear<int> tIncCriticalDamMax_;

    ZtlSecureTear<int> nIncTerR;
    ZtlSecureTear<int> nIncTerR_;
    ZtlSecureTear<int> rIncTerR_;
    ZtlSecureTear<int> tIncTerR_;

    ZtlSecureTear<int> nIncAsrR;
    ZtlSecureTear<int> nIncAsrR_;
    ZtlSecureTear<int> rIncAsrR_;
    ZtlSecureTear<int> tIncAsrR_;

    // === Death / Poison marks ===============================================

    ZtlSecureTear<int> nDeathMark_;
    ZtlSecureTear<int> rDeathMark_;
    ZtlSecureTear<int> tDeathMark_;

    ZtlSecureTear<int> nPainMark_;
    ZtlSecureTear<int> rPainMark_;
    ZtlSecureTear<int> tPainMark_;

    ZtlSecureTear<int> nLapidification_;
    ZtlSecureTear<int> rLapidification_;
    ZtlSecureTear<int> tLapidification_;

    ZtlSecureTear<int> nVampDeath_;
    ZtlSecureTear<int> rVampDeath_;
    ZtlSecureTear<int> tVampDeath_;

    ZtlSecureTear<int> nVampDeathSummon_;
    ZtlSecureTear<int> rVampDeathSummon_;
    ZtlSecureTear<int> tVampDeathSummon_;

    ZtlSecureTear<int> nVenomSnake_;
    ZtlSecureTear<int> rVenomSnake_;
    ZtlSecureTear<int> tVenomSnake_;

    // === Standalone stat modifiers ==========================================

    ZtlSecureTear<int> nReduceCoolTime;
    ZtlSecureTear<int> nMPConReduce;
    ZtlSecureTear<int> nDAMreflect;

    ZtlSecureTear<int> nFixCoolTime_;
    ZtlSecureTear<int> rFixCoolTime_;
    ZtlSecureTear<int> tFixCoolTime_;

    // === Carnival ===========================================================

    ZtlSecureTear<int> nCarnivalAttack_;
    ZtlSecureTear<int> rCarnivalAttack_;
    ZtlSecureTear<int> tCarnivalAttack_;

    ZtlSecureTear<int> nCarnivalDefence_;
    ZtlSecureTear<int> rCarnivalDefence_;
    ZtlSecureTear<int> tCarnivalDefence_;

    ZtlSecureTear<int> nCarnivalExp_;
    ZtlSecureTear<int> rCarnivalExp_;
    ZtlSecureTear<int> tCarnivalExp_;

    ZtlSecureTear<int> nSlowAttack_;
    ZtlSecureTear<int> rSlowAttack_;
    ZtlSecureTear<int> tSlowAttack_;

    ZtlSecureTear<int> nPyramidEffect_;
    ZtlSecureTear<int> rPyramidEffect_;
    ZtlSecureTear<int> tPyramidEffect_;

    ZtlSecureTear<int> nHollowPointBullet_;
    ZtlSecureTear<int> rHollowPointBullet_;
    ZtlSecureTear<int> tHollowPointBullet_;

    ZtlSecureTear<int> nKillingPoint_;
    ZtlSecureTear<int> nPinkbeanRollingGrade_;

    // === KeyDown / Movement =================================================

    ZtlSecureTear<int> nKeyDownMoving_;
    ZtlSecureTear<int> rKeyDownMoving_;
    ZtlSecureTear<int> tKeyDownMoving_;

    ZtlSecureTear<int> nKeyDownAreaMoving_;
    ZtlSecureTear<int> rKeyDownAreaMoving_;
    ZtlSecureTear<int> tKeyDownAreaMoving_;

    ZtlSecureTear<int> nIgnoreTargetDEF_;
    ZtlSecureTear<int> rIgnoreTargetDEF_;
    ZtlSecureTear<int> tIgnoreTargetDEF_;
    ZtlSecureTear<int> mIgnoreTargetDEF_;

    ZtlSecureTear<int> nInvisible_;
    ZtlSecureTear<int> rInvisible_;
    ZtlSecureTear<int> tInvisible_;

    ZtlSecureTear<int> nEnrageCr_;
    ZtlSecureTear<int> rEnrageCr_;
    ZtlSecureTear<int> tEnrageCr_;

    ZtlSecureTear<int> nEnrageCrDamMin_;
    ZtlSecureTear<int> rEnrageCrDamMin_;
    ZtlSecureTear<int> tEnrageCrDamMin_;

    ZtlSecureTear<int> nBlessOfDarkness_;
    ZtlSecureTear<int> rBlessOfDarkness_;
    ZtlSecureTear<int> tBlessOfDarkness_;

    ZtlSecureTear<int> nLifeTidal_;
    ZtlSecureTear<int> rLifeTidal_;
    ZtlSecureTear<int> tLifeTidal_;
    ZtlSecureTear<int> mLifeTidal_;

    ZtlSecureTear<int> nJudgement_;
    ZtlSecureTear<int> rJudgement_;
    ZtlSecureTear<int> tJudgement_;
    ZtlSecureTear<int> xJudgement_;

    ZtlSecureTear<int> nDojangLuckyBonus_;
    ZtlSecureTear<int> rDojangLuckyBonus_;
    ZtlSecureTear<int> tDojangLuckyBonus_;

    ZtlSecureTear<int> nHitCriDamR_;
    ZtlSecureTear<int> rHitCriDamR_;
    ZtlSecureTear<int> tHitCriDamR_;

    // === Larkness ===========================================================

    ZtlSecureTear<int> nLarkness_;
    ZtlSecureTear<int> rLarkness_;
    ZtlSecureTear<int> tLarkness_;
    ZtlSecureTear<int> dgLarkness_;
    ZtlSecureTear<int> lgLarkness_;

    ZtlSecureTear<int> nSmashStack_;
    ZtlSecureTear<int> rSmashStack_;
    ZtlSecureTear<int> tSmashStack_;
    ZtlSecureTear<int> xSmashStack_;

    ZtlSecureTear<int> nReshuffleSwitch_;
    ZtlSecureTear<int> rReshuffleSwitch_;
    ZtlSecureTear<int> tReshuffleSwitch_;

    ZtlSecureTear<int> nSpecialAction_;
    ZtlSecureTear<int> rSpecialAction_;
    ZtlSecureTear<int> tSpecialAction_;

    // === Soul skills ========================================================

    ZtlSecureTear<int> nSoulGazeCriDamR_;
    ZtlSecureTear<int> rSoulGazeCriDamR_;
    ZtlSecureTear<int> tSoulGazeCriDamR_;

    ZtlSecureTear<int> nSoulRageCount_;
    ZtlSecureTear<int> rSoulRageCount_;
    ZtlSecureTear<int> tSoulRageCount_;

    ZtlSecureTear<int> nPowerTransferGauge_;
    ZtlSecureTear<int> rPowerTransferGauge_;
    ZtlSecureTear<int> tPowerTransferGauge_;

    ZtlSecureTear<int> nAffinitySlug_;
    ZtlSecureTear<int> rAffinitySlug_;
    ZtlSecureTear<int> tAffinitySlug_;

    ZtlSecureTear<int> nSoulExalt_;
    ZtlSecureTear<int> rSoulExalt_;
    ZtlSecureTear<int> tSoulExalt_;

    ZtlSecureTear<int> nHiddenPieceOn_;
    ZtlSecureTear<int> rHiddenPieceOn_;
    ZtlSecureTear<int> tHiddenPieceOn_;

    ZtlSecureTear<int> nBossShield_;
    ZtlSecureTear<int> rBossShield_;
    ZtlSecureTear<int> tBossShield_;

    // === Ignore Counter / Immune ============================================

    ZtlSecureTear<int> nIgnorePCounter_;
    ZtlSecureTear<int> rIgnorePCounter_;
    ZtlSecureTear<int> tIgnorePCounter_;

    ZtlSecureTear<int> nIgnoreAllCounter_;
    ZtlSecureTear<int> rIgnoreAllCounter_;
    ZtlSecureTear<int> tIgnoreAllCounter_;

    ZtlSecureTear<int> nIgnorePImmune_;
    ZtlSecureTear<int> rIgnorePImmune_;
    ZtlSecureTear<int> tIgnorePImmune_;

    ZtlSecureTear<int> nIgnoreAllImmune_;
    ZtlSecureTear<int> rIgnoreAllImmune_;
    ZtlSecureTear<int> tIgnoreAllImmune_;

    ZtlSecureTear<int> nRestoration_;
    ZtlSecureTear<int> rRestoration_;
    ZtlSecureTear<int> tRestoration_;

    ZtlSecureTear<int> nComboCostInc_;
    ZtlSecureTear<int> rComboCostInc_;
    ZtlSecureTear<int> tComboCostInc_;

    ZtlSecureTear<int> nChargeBuff_;
    ZtlSecureTear<int> rChargeBuff_;
    ZtlSecureTear<int> tChargeBuff_;

    ZtlSecureTear<int> nIncExpR;
    ZtlSecureTear<int> nIncPQExpR;
    ZtlSecureTear<int> nIncPADlv;
    ZtlSecureTear<int> nIncMADlv;

    ZtlSecureTear<int> nComboUnlimited_;
    ZtlSecureTear<int> rComboUnlimited_;
    ZtlSecureTear<int> tComboUnlimited_;

    ZtlSecureTear<int> nFinalJudgement_;
    ZtlSecureTear<int> rFinalJudgement_;
    ZtlSecureTear<int> tFinalJudgement_;

    ZtlSecureTear<int> nAmaranthGenerator_;
    ZtlSecureTear<int> rAmaranthGenerator_;
    ZtlSecureTear<int> tAmaranthGenerator_;

    ZtlSecureTear<int> nIncMonsterBattleCaptureRate_;
    ZtlSecureTear<int> rIncMonsterBattleCaptureRate_;
    ZtlSecureTear<int> tIncMonsterBattleCaptureRate_;

    ZtlSecureTear<int> nMHPCutR_;
    ZtlSecureTear<int> rMHPCutR_;
    ZtlSecureTear<int> tMHPCutR_;

    ZtlSecureTear<int> nMMPCutR_;
    ZtlSecureTear<int> rMMPCutR_;
    ZtlSecureTear<int> tMMPCutR_;

    ZtlSecureTear<int> nSelfWeakness_;
    ZtlSecureTear<int> rSelfWeakness_;
    ZtlSecureTear<int> tSelfWeakness_;

    ZtlSecureTear<int> nElementDarkness_;
    ZtlSecureTear<int> rElementDarkness_;
    ZtlSecureTear<int> tElementDarkness_;

    ZtlSecureTear<int> nDominion_;
    ZtlSecureTear<int> rDominion_;
    ZtlSecureTear<int> tDominion_;

    ZtlSecureTear<int> nSiphonVitality_;
    ZtlSecureTear<int> rSiphonVitality_;
    ZtlSecureTear<int> tSiphonVitality_;

    ZtlSecureTear<int> nDarknessAscension_;
    ZtlSecureTear<int> rDarknessAscension_;
    ZtlSecureTear<int> tDarknessAscension_;

    ZtlSecureTear<int> nChangeFoxMan_;
    ZtlSecureTear<int> rChangeFoxMan_;
    ZtlSecureTear<int> tChangeFoxMan_;

    ZtlSecureTear<int> nFireBarrier_;
    ZtlSecureTear<int> rFireBarrier_;
    ZtlSecureTear<int> tFireBarrier_;

    ZtlSecureTear<int> nUsingScouter_;

    // === Larkness queue =====================================================

    std::queue<LarknessInfo> qLarknessInfo;

    // === StopForceAtom ======================================================

    ZtlSecureTear<int> nStopForceAtomInfo_;
    ZtlSecureTear<int> rStopForceAtomInfo_;
    ZtlSecureTear<int> tStopForceAtomInfo_;
    StopForceAtom sStopForceAtomInfo_;

    // === MobZoneState =======================================================

    ZtlSecureTear<int> nMobZoneState_;
    ZtlSecureTear<int> rMobZoneState_;
    ZtlSecureTear<int> tMobZoneState_;
    std::set<std::uint32_t> setMobZoneState;

    // === GiveMeHeal / TouchMe / Contagion ===================================

    ZtlSecureTear<int> nGiveMeHeal_;
    ZtlSecureTear<int> rGiveMeHeal_;
    ZtlSecureTear<int> tGiveMeHeal_;

    ZtlSecureTear<int> nTouchMe_;
    ZtlSecureTear<int> rTouchMe_;
    ZtlSecureTear<int> tTouchMe_;

    ZtlSecureTear<int> nContagion_;
    ZtlSecureTear<int> rContagion_;
    ZtlSecureTear<int> tContagion_;

    // === Auras ===============================================================

    ZtlSecureTear<int> nIceAura_;
    ZtlSecureTear<int> rIceAura_;
    ZtlSecureTear<int> tIceAura_;
    ZtlSecureTear<int> cIceAura_;
    ZtlSecureTear<int> bIceAura_;

    ZtlSecureTear<int> nFireAura_;
    ZtlSecureTear<int> rFireAura_;
    ZtlSecureTear<int> tFireAura_;

    ZtlSecureTear<int> nKnightsAura_;
    ZtlSecureTear<int> rKnightsAura_;
    ZtlSecureTear<int> tKnightsAura_;
    ZtlSecureTear<int> cKnightsAura_;
    ZtlSecureTear<int> bKnightsAura_;

    ZtlSecureTear<int> nVengeanceOfAngel_;
    ZtlSecureTear<int> rVengeanceOfAngel_;
    ZtlSecureTear<int> tVengeanceOfAngel_;

    ZtlSecureTear<int> nHeavensDoor_;
    ZtlSecureTear<int> rHeavensDoor_;
    ZtlSecureTear<int> tHeavensDoor_;

    ZtlSecureTear<int> nPreparation_;
    ZtlSecureTear<int> rPreparation_;
    ZtlSecureTear<int> tPreparation_;

    ZtlSecureTear<int> nBullsEye_;
    ZtlSecureTear<int> rBullsEye_;
    ZtlSecureTear<int> tBullsEye_;

    ZtlSecureTear<int> nBleedingToxin_;
    ZtlSecureTear<int> rBleedingToxin_;
    ZtlSecureTear<int> tBleedingToxin_;

    // === PvP / Potion =======================================================

    ZtlSecureTear<int> bPvP;

    ZtlSecureTear<int> nIncEffectHPPotion;
    ZtlSecureTear<int> nIncEffectHPPotion_;
    ZtlSecureTear<int> rIncEffectHPPotion_;
    ZtlSecureTear<int> tIncEffectHPPotion_;

    ZtlSecureTear<int> nIncEffectMPPotion;
    ZtlSecureTear<int> nIncEffectMPPotion_;
    ZtlSecureTear<int> rIncEffectMPPotion_;
    ZtlSecureTear<int> tIncEffectMPPotion_;

    ZtlSecureTear<int> nIgnoreMobDamR_;
    ZtlSecureTear<int> rIgnoreMobDamR_;
    ZtlSecureTear<int> tIgnoreMobDamR_;

    ZtlSecureTear<int> nAsura_;
    ZtlSecureTear<int> rAsura_;
    ZtlSecureTear<int> tAsura_;

    ZtlSecureTear<int> nFlipTheCoin_;
    ZtlSecureTear<int> rFlipTheCoin_;
    ZtlSecureTear<int> tFlipTheCoin_;

    ZtlSecureTear<int> nUnityOfPower_;
    ZtlSecureTear<int> rUnityOfPower_;
    ZtlSecureTear<int> tUnityOfPower_;

    ZtlSecureTear<int> nStimulate_;
    ZtlSecureTear<int> rStimulate_;
    ZtlSecureTear<int> tStimulate_;

    ZtlSecureTear<int> nReturnTeleport_;
    ZtlSecureTear<int> rReturnTeleport_;
    ZtlSecureTear<int> tReturnTeleport_;

    ZtlSecureTear<int> nCapDebuff_;
    ZtlSecureTear<int> mCapDebuff_;
    ZtlSecureTear<int> rCapDebuff_;
    ZtlSecureTear<int> tCapDebuff_;

    // === Drop / Boss damage =================================================

    ZtlSecureTear<int> nDropRIncrease_;
    ZtlSecureTear<int> rDropRIncrease_;
    ZtlSecureTear<int> tDropRIncrease_;
    ZtlSecureTear<int> xDropRIncrease_;
    ZtlSecureTear<int> bDropRIncrease_;

    ZtlSecureTear<int> nIgnoreMobpdpR_;
    ZtlSecureTear<int> rIgnoreMobpdpR_;
    ZtlSecureTear<int> tIgnoreMobpdpR_;
    ZtlSecureTear<int> bIgnoreMobpdpR_;

    ZtlSecureTear<int> nBdR_;
    ZtlSecureTear<int> rBdR_;
    ZtlSecureTear<int> tBdR_;
    ZtlSecureTear<int> bBdR_;

    // === Demon / Exceed =====================================================

    ZtlSecureTear<int> nExceed_;
    ZtlSecureTear<int> rExceed_;
    ZtlSecureTear<int> tExceed_;

    ZtlSecureTear<int> nDiabolikRecovery_;
    ZtlSecureTear<int> rDiabolikRecovery_;
    ZtlSecureTear<int> tDiabolikRecovery_;

    ZtlSecureTear<int> nFinalAttackProp_;
    ZtlSecureTear<int> rFinalAttackProp_;
    ZtlSecureTear<int> tFinalAttackProp_;

    ZtlSecureTear<int> nExceedOverload_;
    ZtlSecureTear<int> rExceedOverload_;
    ZtlSecureTear<int> tExceedOverload_;

    ZtlSecureTear<int> nOverloadCount_;
    ZtlSecureTear<int> rOverloadCount_;
    ZtlSecureTear<int> tOverloadCount_;

    ZtlSecureTear<int> nBuckShot_;
    ZtlSecureTear<int> rBuckShot_;
    ZtlSecureTear<int> tBuckShot_;

    ZtlSecureTear<int> nFireBomb_;
    ZtlSecureTear<int> rFireBomb_;
    ZtlSecureTear<int> tFireBomb_;

    ZtlSecureTear<int> nSurplusSupply_;
    ZtlSecureTear<int> rSurplusSupply_;
    ZtlSecureTear<int> tSurplusSupply_;

    // === Base damage ========================================================

    ZtlSecureTear<int> nSetBaseDamage_;
    ZtlSecureTear<int> rSetBaseDamage_;
    ZtlSecureTear<int> tSetBaseDamage_;

    ZtlSecureTear<int> nSetBaseDamageByBuff_;
    ZtlSecureTear<int> rSetBaseDamageByBuff_;
    ZtlSecureTear<int> tSetBaseDamageByBuff_;

    ZtlSecureTear<int> nChillingStep_;
    ZtlSecureTear<int> rChillingStep_;
    ZtlSecureTear<int> tChillingStep_;

    ZtlSecureTear<int> nHalfstatByDebuff_;
    ZtlSecureTear<int> rHalfstatByDebuff_;
    ZtlSecureTear<int> tHalfstatByDebuff_;

    // === Cygnus element =====================================================

    ZtlSecureTear<int> nCygnusElementSkill_;
    ZtlSecureTear<int> rCygnusElementSkill_;
    ZtlSecureTear<int> tCygnusElementSkill_;

    ZtlSecureTear<int> nStrikerHyperElectric_;
    ZtlSecureTear<int> rStrikerHyperElectric_;
    ZtlSecureTear<int> tStrikerHyperElectric_;

    ZtlSecureTear<int> nEventPointAbsorb_;
    ZtlSecureTear<int> rEventPointAbsorb_;
    ZtlSecureTear<int> tEventPointAbsorb_;

    ZtlSecureTear<int> nEventAssemble_;
    ZtlSecureTear<int> rEventAssemble_;
    ZtlSecureTear<int> tEventAssemble_;

    ZtlSecureTear<int> nStormBringer_;
    ZtlSecureTear<int> rStormBringer_;
    ZtlSecureTear<int> tStormBringer_;

    ZtlSecureTear<int> nACCR_;
    ZtlSecureTear<int> rACCR_;
    ZtlSecureTear<int> tACCR_;

    ZtlSecureTear<int> nDEXR_;
    ZtlSecureTear<int> rDEXR_;
    ZtlSecureTear<int> tDEXR_;

    ZtlSecureTear<int> nAlbatross_;
    ZtlSecureTear<int> rAlbatross_;
    ZtlSecureTear<int> tAlbatross_;

    ZtlSecureTear<int> nTranslucence_;
    ZtlSecureTear<int> rTranslucence_;
    ZtlSecureTear<int> tTranslucence_;

    ZtlSecureTear<int> nPoseType_;
    ZtlSecureTear<int> rPoseType_;
    ZtlSecureTear<int> tPoseType_;
    ZtlSecureTear<int> bPoseType_;

    ZtlSecureTear<int> nLightOfSpirit_;
    ZtlSecureTear<int> rLightOfSpirit_;
    ZtlSecureTear<int> tLightOfSpirit_;

    ZtlSecureTear<int> nElementSoul_;
    ZtlSecureTear<int> rElementSoul_;
    ZtlSecureTear<int> tElementSoul_;

    ZtlSecureTear<int> nGlimmeringTime_;
    ZtlSecureTear<int> rGlimmeringTime_;
    ZtlSecureTear<int> tGlimmeringTime_;

    ZtlSecureTear<int> nTrueSight_;
    ZtlSecureTear<int> rTrueSight_;
    ZtlSecureTear<int> tTrueSight_;

    ZtlSecureTear<int> nPinkbeanYoYoStack_;
    ZtlSecureTear<int> rPinkbeanYoYoStack_;
    ZtlSecureTear<int> tPinkbeanYoYoStack_;

    ZtlSecureTear<int> nHiddenHyperLinkMaximization_;
    ZtlSecureTear<int> rHiddenHyperLinkMaximization_;
    ZtlSecureTear<int> tHiddenHyperLinkMaximization_;

    // === Stigma / Soul explosion ============================================

    ZtlSecureTear<int> nStigma_;
    ZtlSecureTear<int> rStigma_;
    ZtlSecureTear<int> tStigma_;
    ZtlSecureTear<int> bStigma_;

    ZtlSecureTear<int> nSoulExplosion_;
    ZtlSecureTear<int> rSoulExplosion_;
    ZtlSecureTear<int> wSoulExplosion_;

    std::map<std::int32_t, std::uint32_t> mBuffedByPhatomSteal;

    // === Soul MP ============================================================

    ZtlSecureTear<int> nSoulMP_;
    ZtlSecureTear<int> rSoulMP_;
    ZtlSecureTear<int> tSoulMP_;
    ZtlSecureTear<int> xSoulMP_;

    ZtlSecureTear<int> nFullSoulMP_;
    ZtlSecureTear<int> rFullSoulMP_;
    ZtlSecureTear<int> tFullSoulMP_;
    ZtlSecureTear<int> xFullSoulMP_;

    ZtlSecureTear<int> nSoulSkillDamageUp_;
    ZtlSecureTear<int> rSoulSkillDamageUp_;
    ZtlSecureTear<int> tSoulSkillDamageUp_;

    ZtlSecureTear<int> nCrossOverChain_;
    ZtlSecureTear<int> rCrossOverChain_;
    ZtlSecureTear<int> tCrossOverChain_;
    ZtlSecureTear<int> xCrossOverChain_;

    ZtlSecureTear<int> nReincarnation_;
    ZtlSecureTear<int> rReincarnation_;
    ZtlSecureTear<int> tReincarnation_;
    ZtlSecureTear<int> xReincarnation_;

    ZtlSecureTear<int> nDotBasedBuff_;
    ZtlSecureTear<int> rDotBasedBuff_;
    ZtlSecureTear<int> tDotBasedBuff_;

    ZtlSecureTear<int> nBlessEnsenble_;
    ZtlSecureTear<int> rBlessEnsenble_;
    ZtlSecureTear<int> tBlessEnsenble_;

    // === Archery / Quiver ===================================================

    ZtlSecureTear<int> nExtremeArchery_;
    ZtlSecureTear<int> rExtremeArchery_;
    ZtlSecureTear<int> tExtremeArchery_;
    ZtlSecureTear<int> xExtremeArchery_;
    ZtlSecureTear<int> bExtremeArchery_;

    ZtlSecureTear<int> nQuiverCatridge_;
    ZtlSecureTear<int> rQuiverCatridge_;
    ZtlSecureTear<int> tQuiverCatridge_;
    ZtlSecureTear<int> xQuiverCatridge_;

    ZtlSecureTear<int> nAdvancedQuiver_;
    ZtlSecureTear<int> rAdvancedQuiver_;
    ZtlSecureTear<int> tAdvancedQuiver_;

    ZtlSecureTear<int> nUserControlMob_;
    ZtlSecureTear<int> rUserControlMob_;
    ZtlSecureTear<int> tUserControlMob_;

    ZtlSecureTear<int> nShieldAttack_;
    ZtlSecureTear<int> rShieldAttack_;
    ZtlSecureTear<int> tShieldAttack_;
    ZtlSecureTear<int> xShieldAttack_;

    ZtlSecureTear<int> nSSFShootingAttack_;
    ZtlSecureTear<int> rSSFShootingAttack_;
    ZtlSecureTear<int> tSSFShootingAttack_;
    ZtlSecureTear<int> xSSFShootingAttack_;

    ZtlSecureTear<int> nArmorPiercing_;
    ZtlSecureTear<int> rArmorPiercing_;
    ZtlSecureTear<int> tArmorPiercing_;
    ZtlSecureTear<int> bArmorPiercing_;

    ZtlSecureTear<int> nImmuneBarrier_;
    ZtlSecureTear<int> rImmuneBarrier_;
    ZtlSecureTear<int> tImmuneBarrier_;
    ZtlSecureTear<int> xImmuneBarrier_;

    // === Zero ================================================================

    ZtlSecureTear<int> nZeroAuraStr_;
    ZtlSecureTear<int> rZeroAuraStr_;
    ZtlSecureTear<int> tZeroAuraStr_;
    ZtlSecureTear<int> cZeroAuraStr_;
    ZtlSecureTear<int> bZeroAuraStr_;

    ZtlSecureTear<int> nZeroAuraSpd_;
    ZtlSecureTear<int> rZeroAuraSpd_;
    ZtlSecureTear<int> tZeroAuraSpd_;
    ZtlSecureTear<int> cZeroAuraSpd_;
    ZtlSecureTear<int> bZeroAuraSpd_;

    ZtlSecureTear<int> nCriticalGrowing_;
    ZtlSecureTear<int> rCriticalGrowing_;
    ZtlSecureTear<int> tCriticalGrowing_;

    ZtlSecureTear<int> nQuickDraw_;
    ZtlSecureTear<int> rQuickDraw_;
    ZtlSecureTear<int> tQuickDraw_;

    ZtlSecureTear<int> nBowMasterConcentration_;
    ZtlSecureTear<int> rBowMasterConcentration_;
    ZtlSecureTear<int> tBowMasterConcentration_;

    // === Time / Viper =======================================================

    ZtlSecureTear<int> nTimeFastABuff_;
    ZtlSecureTear<int> rTimeFastABuff_;
    ZtlSecureTear<int> tTimeFastABuff_;

    ZtlSecureTear<int> nTimeFastBBuff_;
    ZtlSecureTear<int> rTimeFastBBuff_;
    ZtlSecureTear<int> tTimeFastBBuff_;

    ZtlSecureTear<int> nViperEnergyCharge_;

    ZtlSecureTear<int> nLimitMP_;
    ZtlSecureTear<int> rLimitMP_;
    ZtlSecureTear<int> tLimitMP_;

    ZtlSecureTear<int> nReflectDamR_;
    ZtlSecureTear<int> rReflectDamR_;
    ZtlSecureTear<int> tReflectDamR_;

    ZtlSecureTear<int> nFlareTrick_;
    ZtlSecureTear<int> rFlareTrick_;
    ZtlSecureTear<int> tFlareTrick_;

    ZtlSecureTear<int> nDamageReduce_;
    ZtlSecureTear<int> rDamageReduce_;
    ZtlSecureTear<int> tDamageReduce_;

    // === Battle PvP NPCs ====================================================

    ZtlSecureTear<int> nBattlePvP_Mike_Shield_;
    ZtlSecureTear<int> rBattlePvP_Mike_Shield_;
    ZtlSecureTear<int> tBattlePvP_Mike_Shield_;

    ZtlSecureTear<int> nBattlePvP_Mike_Bugle_;
    ZtlSecureTear<int> rBattlePvP_Mike_Bugle_;
    ZtlSecureTear<int> tBattlePvP_Mike_Bugle_;

    ZtlSecureTear<int> nBattlePvP_Helena_Mark_;
    ZtlSecureTear<int> rBattlePvP_Helena_Mark_;
    ZtlSecureTear<int> tBattlePvP_Helena_Mark_;
    ZtlSecureTear<int> cBattlePvP_Helena_Mark_;

    ZtlSecureTear<int> nBattlePvP_Helena_WindSpirit_;
    ZtlSecureTear<int> rBattlePvP_Helena_WindSpirit_;
    ZtlSecureTear<int> tBattlePvP_Helena_WindSpirit_;

    ZtlSecureTear<int> nBattlePvP_LangE_Protection_;
    ZtlSecureTear<int> rBattlePvP_LangE_Protection_;
    ZtlSecureTear<int> tBattlePvP_LangE_Protection_;

    ZtlSecureTear<int> nBattlePvP_LeeMalNyun_ScaleUp_;
    ZtlSecureTear<int> rBattlePvP_LeeMalNyun_ScaleUp_;
    ZtlSecureTear<int> tBattlePvP_LeeMalNyun_ScaleUp_;

    ZtlSecureTear<int> nBattlePvP_Revive_;
    ZtlSecureTear<int> rBattlePvP_Revive_;
    ZtlSecureTear<int> tBattlePvP_Revive_;

    // === Xenon / Angelic Burster =============================================

    ZtlSecureTear<int> nXenonAegisSystem_;
    ZtlSecureTear<int> rXenonAegisSystem_;
    ZtlSecureTear<int> tXenonAegisSystem_;

    ZtlSecureTear<int> nAngelicBursterSoulSeeker_;
    ZtlSecureTear<int> rAngelicBursterSoulSeeker_;
    ZtlSecureTear<int> tAngelicBursterSoulSeeker_;

    ZtlSecureTear<int> nHiddenPossession_;
    ZtlSecureTear<int> rHiddenPossession_;
    ZtlSecureTear<int> tHiddenPossession_;

    ZtlSecureTear<int> nNightWalkerBat_;
    ZtlSecureTear<int> rNightWalkerBat_;
    ZtlSecureTear<int> tNightWalkerBat_;

    ZtlSecureTear<int> nNightLordMark_;
    ZtlSecureTear<int> rNightLordMark_;
    ZtlSecureTear<int> tNightLordMark_;

    ZtlSecureTear<int> nWizardIgnite_;
    ZtlSecureTear<int> rWizardIgnite_;
    ZtlSecureTear<int> tWizardIgnite_;

    ZtlSecureTear<int> nGatherDropR_;
    ZtlSecureTear<int> rGatherDropR_;
    ZtlSecureTear<int> tGatherDropR_;

    ZtlSecureTear<int> nAimBox2D_;
    ZtlSecureTear<int> rAimBox2D_;
    ZtlSecureTear<int> tAimBox2D_;

    ZtlSecureTear<int> nCursorSniping_;
    ZtlSecureTear<int> rCursorSniping_;
    ZtlSecureTear<int> tCursorSniping_;

    ZtlSecureTear<int> nDebuffTolerance_;
    ZtlSecureTear<int> rDebuffTolerance_;
    ZtlSecureTear<int> tDebuffTolerance_;

    ZtlSecureTear<int> nDance_;
    ZtlSecureTear<int> rDance_;
    ZtlSecureTear<int> tDance_;

    ZtlSecureTear<int> nDotHealHPPerSecond_;
    ZtlSecureTear<int> rDotHealHPPerSecond_;
    ZtlSecureTear<int> tDotHealHPPerSecond_;
    ZtlSecureTear<int> xDotHealHPPerSecond_;

    ZtlSecureTear<int> nSpiritGuard_;
    ZtlSecureTear<int> rSpiritGuard_;
    ZtlSecureTear<int> tSpiritGuard_;

    ZtlSecureTear<int> nPreReviveOnce_;
    ZtlSecureTear<int> rPreReviveOnce_;
    ZtlSecureTear<int> tPreReviveOnce_;

    ZtlSecureTear<int> nComboTempest_;
    ZtlSecureTear<int> rComboTempest_;
    ZtlSecureTear<int> tComboTempest_;

    ZtlSecureTear<int> nEmber_;
    ZtlSecureTear<int> rEmber_;
    ZtlSecureTear<int> tEmber_;

    ZtlSecureTear<int> nBossWaitingLinesBuff_;
    ZtlSecureTear<int> rBossWaitingLinesBuff_;
    ZtlSecureTear<int> tBossWaitingLinesBuff_;

    // === Shadow ==============================================================

    ZtlSecureTear<int> nShadowServant_;
    ZtlSecureTear<int> rShadowServant_;
    ZtlSecureTear<int> tShadowServant_;

    ZtlSecureTear<int> nShadowIllusion_;
    ZtlSecureTear<int> rShadowIllusion_;
    ZtlSecureTear<int> tShadowIllusion_;

    ZtlSecureTear<int> nKnockBack_;
    ZtlSecureTear<int> rKnockBack_;
    ZtlSecureTear<int> tKnockBack_;
    ZtlSecureTear<int> bKnockBack_;

    ZtlSecureTear<int> nAddAttackCount_;
    ZtlSecureTear<int> rAddAttackCount_;
    ZtlSecureTear<int> tAddAttackCount_;

    ZtlSecureTear<int> nComplusionSlant_;
    ZtlSecureTear<int> rComplusionSlant_;
    ZtlSecureTear<int> tComplusionSlant_;

    // === Jaguar / Devil =====================================================

    ZtlSecureTear<int> nJaguarCount_;
    ZtlSecureTear<int> rJaguarCount_;
    ZtlSecureTear<int> tJaguarCount_;

    ZtlSecureTear<int> nJaguarSummoned_;
    ZtlSecureTear<int> rJaguarSummoned_;
    ZtlSecureTear<int> tJaguarSummoned_;

    ZtlSecureTear<int> nDevilCry_;
    ZtlSecureTear<int> rDevilCry_;
    ZtlSecureTear<int> tDevilCry_;

    // === Battle Mage ========================================================

    ZtlSecureTear<int> nBMageAura_;
    ZtlSecureTear<int> rBMageAura_;
    ZtlSecureTear<int> tBMageAura_;
    ZtlSecureTear<int> xBMageAura_;
    ZtlSecureTear<int> bBMageAura_;
    ZtlSecureTear<int> cBMageAura_;

    ZtlSecureTear<int> nDarkLighting_;
    ZtlSecureTear<int> rDarkLighting_;
    ZtlSecureTear<int> tDarkLighting_;

    ZtlSecureTear<int> nAttackCountX_;
    ZtlSecureTear<int> rAttackCountX_;
    ZtlSecureTear<int> tAttackCountX_;

    ZtlSecureTear<int> nBMageDeath_;
    ZtlSecureTear<int> rBMageDeath_;
    ZtlSecureTear<int> tBMageDeath_;
    ZtlSecureTear<int> xBMageDeath_;

    ZtlSecureTear<int> nBombTime_;
    ZtlSecureTear<int> rBombTime_;
    ZtlSecureTear<int> tBombTime_;

    ZtlSecureTear<int> nNoDebuff_;
    ZtlSecureTear<int> rNoDebuff_;
    ZtlSecureTear<int> tNoDebuff_;

    // === Pinkbean ===========================================================

    ZtlSecureTear<int> nPinkbeanAttackBuff_;
    ZtlSecureTear<int> rPinkbeanAttackBuff_;
    ZtlSecureTear<int> tPinkbeanAttackBuff_;
    ZtlSecureTear<int> bPinkbeanAttackBuff_;

    ZtlSecureTear<int> nPinkbeanRelax_;
    ZtlSecureTear<int> rPinkbeanRelax_;
    ZtlSecureTear<int> tPinkbeanRelax_;

    ZtlSecureTear<int> nNextAttackEnhance_;
    ZtlSecureTear<int> rNextAttackEnhance_;
    ZtlSecureTear<int> tNextAttackEnhance_;

    ZtlSecureTear<int> nRandAreaAttack_;
    ZtlSecureTear<int> rRandAreaAttack_;
    ZtlSecureTear<int> tRandAreaAttack_;
    ZtlSecureTear<int> xRandAreaAttack_;

    // === Aran ================================================================

    ZtlSecureTear<int> nAranBeyonderDamAbsorb_;
    ZtlSecureTear<int> rAranBeyonderDamAbsorb_;
    ZtlSecureTear<int> tAranBeyonderDamAbsorb_;

    ZtlSecureTear<int> nAranCombotempastOption_;
    ZtlSecureTear<int> rAranCombotempastOption_;
    ZtlSecureTear<int> tAranCombotempastOption_;

    ZtlSecureTear<int> nNautilusFinalAttack_;
    ZtlSecureTear<int> rNautilusFinalAttack_;
    ZtlSecureTear<int> tNautilusFinalAttack_;

    ZtlSecureTear<int> nViperTimeLeap_;
    ZtlSecureTear<int> rViperTimeLeap_;
    ZtlSecureTear<int> tViperTimeLeap_;

    // === Royal Guard / Michael ==============================================

    ZtlSecureTear<int> nRoyalGuardState_;
    ZtlSecureTear<int> rRoyalGuardState_;
    ZtlSecureTear<int> tRoyalGuardState_;
    ZtlSecureTear<int> xRoyalGuardState_;
    ZtlSecureTear<int> bRoyalGuardState_;

    ZtlSecureTear<int> nRoyalGuardPrepare_;
    ZtlSecureTear<int> rRoyalGuardPrepare_;
    ZtlSecureTear<int> tRoyalGuardPrepare_;

    ZtlSecureTear<int> nMichaelSoulLink_;
    ZtlSecureTear<int> rMichaelSoulLink_;
    ZtlSecureTear<int> tMichaelSoulLink_;
    ZtlSecureTear<int> xMichaelSoulLink_;
    ZtlSecureTear<int> bMichaelSoulLink_;
    ZtlSecureTear<int> cMichaelSoulLink_;
    ZtlSecureTear<int> yMichaelSoulLink_;

    ZtlSecureTear<int> nMichaelStanceLink_;
    ZtlSecureTear<int> rMichaelStanceLink_;
    ZtlSecureTear<int> tMichaelStanceLink_;

    ZtlSecureTear<int> nTriflingWhimOnOff_;
    ZtlSecureTear<int> rTriflingWhimOnOff_;
    ZtlSecureTear<int> tTriflingWhimOnOff_;

    ZtlSecureTear<int> nAddRangeOnOff_;
    ZtlSecureTear<int> rAddRangeOnOff_;
    ZtlSecureTear<int> tAddRangeOnOff_;

    // === Kinesis =============================================================

    ZtlSecureTear<int> nKinesisPsychicPoint_;
    ZtlSecureTear<int> rKinesisPsychicPoint_;
    ZtlSecureTear<int> tKinesisPsychicPoint_;

    ZtlSecureTear<int> nKinesisPsychicOver_;
    ZtlSecureTear<int> rKinesisPsychicOver_;
    ZtlSecureTear<int> tKinesisPsychicOver_;

    ZtlSecureTear<int> nKinesisPsychicShield_;
    ZtlSecureTear<int> rKinesisPsychicShield_;
    ZtlSecureTear<int> tKinesisPsychicShield_;

    ZtlSecureTear<int> nKinesisIncMastery_;
    ZtlSecureTear<int> rKinesisIncMastery_;
    ZtlSecureTear<int> tKinesisIncMastery_;

    ZtlSecureTear<int> nKinesisPsychicEnergeShield_;
    ZtlSecureTear<int> rKinesisPsychicEnergeShield_;
    ZtlSecureTear<int> tKinesisPsychicEnergeShield_;

    ZtlSecureTear<int> nDebuffActiveSkillHPCon_;
    ZtlSecureTear<int> rDebuffActiveSkillHPCon_;
    ZtlSecureTear<int> tDebuffActiveSkillHPCon_;

    ZtlSecureTear<int> nDebuffIncHP_;
    ZtlSecureTear<int> rDebuffIncHP_;
    ZtlSecureTear<int> tDebuffIncHP_;

    // === Blade / Fever ======================================================

    ZtlSecureTear<int> nBladeStance_;
    ZtlSecureTear<int> rBladeStance_;
    ZtlSecureTear<int> tBladeStance_;
    ZtlSecureTear<int> xBladeStance_;

    ZtlSecureTear<int> nFever_;
    ZtlSecureTear<int> rFever_;
    ZtlSecureTear<int> tFever_;

    ZtlSecureTear<int> nBowMasterMortalBlow_;
    ZtlSecureTear<int> rBowMasterMortalBlow_;
    ZtlSecureTear<int> tBowMasterMortalBlow_;

    ZtlSecureTear<int> nAngelicBursterSoulResonance_;
    ZtlSecureTear<int> rAngelicBursterSoulResonance_;
    ZtlSecureTear<int> tAngelicBursterSoulResonance_;

    ZtlSecureTear<int> nIgnisRore_;
    ZtlSecureTear<int> rIgnisRore_;
    ZtlSecureTear<int> tIgnisRore_;

    ZtlSecureTear<int> nRpSiksin_;
    ZtlSecureTear<int> rRpSiksin_;
    ZtlSecureTear<int> tRpSiksin_;

    ZtlSecureTear<int> nTeleportMasteryRange_;
    ZtlSecureTear<int> rTeleportMasteryRange_;
    ZtlSecureTear<int> tTeleportMasteryRange_;

    ZtlSecureTear<int> nIncMobRateDummy_;
    ZtlSecureTear<int> rIncMobRateDummy_;
    ZtlSecureTear<int> tIncMobRateDummy_;

    // === Adrenalin / Aran v2 ================================================

    ZtlSecureTear<int> nAdrenalinBoost_;
    ZtlSecureTear<int> rAdrenalinBoost_;
    ZtlSecureTear<int> tAdrenalinBoost_;
    ZtlSecureTear<int> cAdrenalinBoost_;

    ZtlSecureTear<int> nAranSmashSwing_;
    ZtlSecureTear<int> rAranSmashSwing_;
    ZtlSecureTear<int> tAranSmashSwing_;

    ZtlSecureTear<int> nAranDrain_;
    ZtlSecureTear<int> rAranDrain_;
    ZtlSecureTear<int> tAranDrain_;

    ZtlSecureTear<int> nAranBoostEndHunt_;
    ZtlSecureTear<int> rAranBoostEndHunt_;
    ZtlSecureTear<int> tAranBoostEndHunt_;

    // === Resistance Weapon ==================================================

    ZtlSecureTear<int> nRWCylinder_;
    ZtlSecureTear<int> rRWCylinder_;
    ZtlSecureTear<int> tRWCylinder_;
    ZtlSecureTear<int> bRWCylinder_;
    ZtlSecureTear<int> cRWCylinder_;

    ZtlSecureTear<int> nRWCombination_;
    ZtlSecureTear<int> rRWCombination_;
    ZtlSecureTear<int> tRWCombination_;

    ZtlSecureTear<int> nRWMagnumBlow_;
    ZtlSecureTear<int> rRWMagnumBlow_;
    ZtlSecureTear<int> tRWMagnumBlow_;
    ZtlSecureTear<int> bRWMagnumBlow_;
    ZtlSecureTear<int> xRWMagnumBlow_;

    ZtlSecureTear<int> nRWBarrier_;
    ZtlSecureTear<int> rRWBarrier_;
    ZtlSecureTear<int> tRWBarrier_;

    ZtlSecureTear<int> nRWBarrierHeal_;
    ZtlSecureTear<int> rRWBarrierHeal_;
    ZtlSecureTear<int> tRWBarrierHeal_;

    ZtlSecureTear<int> nRWMaximizeCannon_;
    ZtlSecureTear<int> rRWMaximizeCannon_;
    ZtlSecureTear<int> tRWMaximizeCannon_;

    ZtlSecureTear<int> nRWOverHeat_;
    ZtlSecureTear<int> rRWOverHeat_;
    ZtlSecureTear<int> tRWOverHeat_;

    ZtlSecureTear<int> nRWMovingEvar_;
    ZtlSecureTear<int> rRWMovingEvar_;
    ZtlSecureTear<int> tRWMovingEvar_;

    // === Temporary stat system ==============================================

    // ZRef<TemporaryStatBase<long>> aTemporaryStat[8];
    // ZArray<IndieTempStat> aIndieTempStat;
    std::map<std::int32_t, std::int32_t> mRechargeBuff;
    // ZMap<unsigned long, int, unsigned long> mBuffedForSpecMap;
};

} // namespace ms
