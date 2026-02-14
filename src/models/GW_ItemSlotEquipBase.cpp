#include "GW_ItemSlotEquipBase.h"

#include "network/InPacket.h"
#include "network/OutPacket.h"

namespace ms
{

// The equip base uses a bitmask-based encoding: a 32-bit flags word indicates
// which fields follow in the stream (only non-zero/non-default values are
// transmitted).  Two consecutive flag words cover all 37 fields.

void GW_ItemSlotEquipBase::Decode(InPacket& iPacket)
{
    // --- First bitmask (bits 0..31) ---
    auto dwFlag1 = static_cast<std::uint32_t>(iPacket.Decode4());

    nRUC.Put(         (dwFlag1 & StatFlag1::RUC)              ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    nCUC.Put(         (dwFlag1 & StatFlag1::CUC)              ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    niSTR.Put(        (dwFlag1 & StatFlag1::iSTR)             ? iPacket.Decode2() : 0);
    niDEX.Put(        (dwFlag1 & StatFlag1::iDEX)             ? iPacket.Decode2() : 0);
    niINT.Put(        (dwFlag1 & StatFlag1::iINT)             ? iPacket.Decode2() : 0);
    niLUK.Put(        (dwFlag1 & StatFlag1::iLUK)             ? iPacket.Decode2() : 0);
    niMaxHP.Put(      (dwFlag1 & StatFlag1::iMaxHP)           ? iPacket.Decode2() : 0);
    niMaxMP.Put(      (dwFlag1 & StatFlag1::iMaxMP)           ? iPacket.Decode2() : 0);
    niPAD.Put(        (dwFlag1 & StatFlag1::iPAD)             ? iPacket.Decode2() : 0);
    niMAD.Put(        (dwFlag1 & StatFlag1::iMAD)             ? iPacket.Decode2() : 0);
    niPDD.Put(        (dwFlag1 & StatFlag1::iPDD)             ? iPacket.Decode2() : 0);
    niMDD.Put(        (dwFlag1 & StatFlag1::iMDD)             ? iPacket.Decode2() : 0);
    niACC.Put(        (dwFlag1 & StatFlag1::iACC)             ? iPacket.Decode2() : 0);
    niEVA.Put(        (dwFlag1 & StatFlag1::iEVA)             ? iPacket.Decode2() : 0);
    niCraft.Put(      (dwFlag1 & StatFlag1::iCraft)           ? iPacket.Decode2() : 0);
    niSpeed.Put(      (dwFlag1 & StatFlag1::iSpeed)           ? iPacket.Decode2() : 0);
    niJump.Put(       (dwFlag1 & StatFlag1::iJump)            ? iPacket.Decode2() : 0);
    nAttribute.Put(   (dwFlag1 & StatFlag1::Attribute)        ? iPacket.Decode2() : 0);
    nLevelUpType.Put( (dwFlag1 & StatFlag1::LevelUpType)      ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    nLevel.Put(       (dwFlag1 & StatFlag1::Level)            ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    nEXP64.Put(       (dwFlag1 & StatFlag1::EXP64)            ? iPacket.Decode8() : 0);
    nDurability.Put(  (dwFlag1 & StatFlag1::Durability)       ? iPacket.Decode4() : -1);
    nIUC.Put(         (dwFlag1 & StatFlag1::IUC)              ? iPacket.Decode4() : 0);
    niPVPDamage.Put(  (dwFlag1 & StatFlag1::iPVPDamage)       ? iPacket.Decode2() : 0);
    niReduceReq.Put(  (dwFlag1 & StatFlag1::iReduceReq)       ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    nSpecialAttribute.Put((dwFlag1 & StatFlag1::SpecialAttribute) ? iPacket.Decode2() : 0);
    nDurabilityMax.Put((dwFlag1 & StatFlag1::DurabilityMax)   ? iPacket.Decode4() : -1);
    niIncReq.Put(     (dwFlag1 & StatFlag1::iIncReq)          ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    nGrowthEnchant.Put((dwFlag1 & StatFlag1::GrowthEnchant)   ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    nPSEnchant.Put(   (dwFlag1 & StatFlag1::PSEnchant)        ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    nBDR.Put(         (dwFlag1 & StatFlag1::BDR)              ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    nIMDR.Put(        (dwFlag1 & StatFlag1::IMDR)             ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);

    // --- Second bitmask (bits 0..4) ---
    auto dwFlag2 = static_cast<std::uint32_t>(iPacket.Decode4());

    nDamR.Put(        (dwFlag2 & StatFlag2::DamR)             ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    nStatR.Put(       (dwFlag2 & StatFlag2::StatR)            ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    nCuttable.Put(    (dwFlag2 & StatFlag2::Cuttable)         ? static_cast<std::uint8_t>(iPacket.Decode1()) : 0);
    nExGradeOption.Put((dwFlag2 & StatFlag2::ExGradeOption)   ? iPacket.Decode8() : 0);
    nItemState.Put(   (dwFlag2 & StatFlag2::ItemState)        ? iPacket.Decode4() : 0);
}

void GW_ItemSlotEquipBase::Encode(OutPacket& oPacket) const
{
    // --- First bitmask ---
    std::uint32_t dwFlag1 = 0;
    auto offset1 = oPacket.GetOffset();
    oPacket.Encode4(0); // placeholder

    if (nRUC.Get() != 0)             { oPacket.Encode1(static_cast<std::int8_t>(nRUC.Get()));          dwFlag1 |= StatFlag1::RUC; }
    if (nCUC.Get() != 0)             { oPacket.Encode1(static_cast<std::int8_t>(nCUC.Get()));          dwFlag1 |= StatFlag1::CUC; }
    if (niSTR.Get() != 0)            { oPacket.Encode2(niSTR.Get());            dwFlag1 |= StatFlag1::iSTR; }
    if (niDEX.Get() != 0)            { oPacket.Encode2(niDEX.Get());            dwFlag1 |= StatFlag1::iDEX; }
    if (niINT.Get() != 0)            { oPacket.Encode2(niINT.Get());            dwFlag1 |= StatFlag1::iINT; }
    if (niLUK.Get() != 0)            { oPacket.Encode2(niLUK.Get());            dwFlag1 |= StatFlag1::iLUK; }
    if (niMaxHP.Get() != 0)          { oPacket.Encode2(niMaxHP.Get());          dwFlag1 |= StatFlag1::iMaxHP; }
    if (niMaxMP.Get() != 0)          { oPacket.Encode2(niMaxMP.Get());          dwFlag1 |= StatFlag1::iMaxMP; }
    if (niPAD.Get() != 0)            { oPacket.Encode2(niPAD.Get());            dwFlag1 |= StatFlag1::iPAD; }
    if (niMAD.Get() != 0)            { oPacket.Encode2(niMAD.Get());            dwFlag1 |= StatFlag1::iMAD; }
    if (niPDD.Get() != 0)            { oPacket.Encode2(niPDD.Get());            dwFlag1 |= StatFlag1::iPDD; }
    if (niMDD.Get() != 0)            { oPacket.Encode2(niMDD.Get());            dwFlag1 |= StatFlag1::iMDD; }
    if (niACC.Get() != 0)            { oPacket.Encode2(niACC.Get());            dwFlag1 |= StatFlag1::iACC; }
    if (niEVA.Get() != 0)            { oPacket.Encode2(niEVA.Get());            dwFlag1 |= StatFlag1::iEVA; }
    if (niCraft.Get() != 0)          { oPacket.Encode2(niCraft.Get());          dwFlag1 |= StatFlag1::iCraft; }
    if (niSpeed.Get() != 0)          { oPacket.Encode2(niSpeed.Get());          dwFlag1 |= StatFlag1::iSpeed; }
    if (niJump.Get() != 0)           { oPacket.Encode2(niJump.Get());           dwFlag1 |= StatFlag1::iJump; }
    if (nAttribute.Get() != 0)       { oPacket.Encode2(nAttribute.Get());       dwFlag1 |= StatFlag1::Attribute; }
    if (nLevelUpType.Get() != 0)     { oPacket.Encode1(static_cast<std::int8_t>(nLevelUpType.Get())); dwFlag1 |= StatFlag1::LevelUpType; }
    if (nLevel.Get() != 0)           { oPacket.Encode1(static_cast<std::int8_t>(nLevel.Get()));       dwFlag1 |= StatFlag1::Level; }
    if (nEXP64.Get() != 0)           { oPacket.Encode8(nEXP64.Get());           dwFlag1 |= StatFlag1::EXP64; }
    if (nDurability.Get() != -1)     { oPacket.Encode4(nDurability.Get());      dwFlag1 |= StatFlag1::Durability; }
    if (nIUC.Get() != 0)             { oPacket.Encode4(nIUC.Get());             dwFlag1 |= StatFlag1::IUC; }
    if (niPVPDamage.Get() != 0)      { oPacket.Encode2(niPVPDamage.Get());      dwFlag1 |= StatFlag1::iPVPDamage; }
    if (niReduceReq.Get() != 0)      { oPacket.Encode1(static_cast<std::int8_t>(niReduceReq.Get()));  dwFlag1 |= StatFlag1::iReduceReq; }
    if (nSpecialAttribute.Get() != 0){ oPacket.Encode2(nSpecialAttribute.Get()); dwFlag1 |= StatFlag1::SpecialAttribute; }
    if (nDurabilityMax.Get() != -1)  { oPacket.Encode4(nDurabilityMax.Get());   dwFlag1 |= StatFlag1::DurabilityMax; }
    if (niIncReq.Get() != 0)         { oPacket.Encode1(static_cast<std::int8_t>(niIncReq.Get()));     dwFlag1 |= StatFlag1::iIncReq; }
    if (nGrowthEnchant.Get() != 0)   { oPacket.Encode1(static_cast<std::int8_t>(nGrowthEnchant.Get())); dwFlag1 |= StatFlag1::GrowthEnchant; }
    if (nPSEnchant.Get() != 0)       { oPacket.Encode1(static_cast<std::int8_t>(nPSEnchant.Get()));   dwFlag1 |= StatFlag1::PSEnchant; }
    if (nBDR.Get() != 0)             { oPacket.Encode1(static_cast<std::int8_t>(nBDR.Get()));         dwFlag1 |= StatFlag1::BDR; }
    if (nIMDR.Get() != 0)            { oPacket.Encode1(static_cast<std::int8_t>(nIMDR.Get()));        dwFlag1 |= StatFlag1::IMDR; }

    oPacket.Set4At(offset1, static_cast<std::int32_t>(dwFlag1));

    // --- Second bitmask ---
    std::uint32_t dwFlag2 = 0;
    auto offset2 = oPacket.GetOffset();
    oPacket.Encode4(0); // placeholder

    if (nDamR.Get() != 0)            { oPacket.Encode1(static_cast<std::int8_t>(nDamR.Get()));        dwFlag2 |= StatFlag2::DamR; }
    if (nStatR.Get() != 0)           { oPacket.Encode1(static_cast<std::int8_t>(nStatR.Get()));       dwFlag2 |= StatFlag2::StatR; }
    if (nCuttable.Get() != 0)        { oPacket.Encode1(static_cast<std::int8_t>(nCuttable.Get()));    dwFlag2 |= StatFlag2::Cuttable; }
    if (nExGradeOption.Get() != 0)   { oPacket.Encode8(nExGradeOption.Get());   dwFlag2 |= StatFlag2::ExGradeOption; }
    if (nItemState.Get() != 0)       { oPacket.Encode4(nItemState.Get());       dwFlag2 |= StatFlag2::ItemState; }

    oPacket.Set4At(offset2, static_cast<std::int32_t>(dwFlag2));
}

} // namespace ms
