#include "GW_ItemSlotEquip.h"

#include "network/InPacket.h"
#include "network/OutPacket.h"
#include "templates/item/ItemInfo.h"

#include <algorithm>
#include <cstring>

namespace ms
{

// === Title ===

void GW_ItemSlotEquip::SetItemTitle(const std::string& s)
{
    auto len = std::min(s.size(), sTitle.size() - 1);
    std::memcpy(sTitle.data(), s.data(), len);
    sTitle[len] = '\0';
}

// === Look / Grade ===

auto GW_ItemSlotEquip::GetLookItemID() -> std::int32_t
{
    auto nOpt5 = option.nOption5.Get();
    if (nOpt5 == 0)
        return static_cast<std::int32_t>(nItemID);
    auto nID = static_cast<std::int32_t>(nItemID);
    return 10000 * (nID / 10000) + nOpt5 % 10000;
}

auto GW_ItemSlotEquip::GetAdditionalGrade() -> std::int32_t
{
    auto nOpt4 = option.nOption4.Get();
    if (nOpt4 >= 10)
        return nOpt4 / 10000;
    return nOpt4;
}

// === Growth / PS enchant ===

auto GW_ItemSlotEquip::GetGrowthEnchantID() -> std::int32_t
{
    auto nVal = item.nGrowthEnchant.Get();
    if (nVal == 0)
        return 0;
    if (nVal <= 100)
        return nVal + 2048499;
    return nVal - 100 + 2048499;
}

void GW_ItemSlotEquip::SetGrowthEnchantID(std::int32_t nGrowthEnchantID, std::int32_t nLevelUpType)
{
    if (nGrowthEnchantID != 0 && item.nLevel.Get() == 0)
    {
        item.nGrowthEnchant.Put(static_cast<std::uint8_t>(nGrowthEnchantID + 13));
        item.nLevel.Put(1);
        item.nLevelUpType.Put(static_cast<std::uint8_t>(nLevelUpType));
    }
}

auto GW_ItemSlotEquip::GetPSEnchantID() -> std::int32_t
{
    auto nVal = item.nPSEnchant.Get();
    if (nVal == 0)
        return 0;
    if (nVal <= 100)
        return nVal + 2048599;
    return nVal - 100 + 2048599;
}

// === Cuttable ===

auto GW_ItemSlotEquip::DecCuttableCount() -> bool
{
    if (item.nCuttable.Get() == 0)
        return false;
    item.nCuttable.Put(item.nCuttable.Get() - 1);
    return true;
}

// === Grade manipulation ===

void GW_ItemSlotEquip::SetReleased(std::int32_t bReleased)
{
    auto nGrade = option.nGrade.Get();
    if (bReleased)
        option.nGrade.Put(nGrade | EquipGradeFlag::Released);
    else
        option.nGrade.Put(static_cast<std::uint8_t>(nGrade & ~EquipGradeFlag::Released));
}

void GW_ItemSlotEquip::SetAdditionalReleased(std::int32_t bReleased)
{
    auto nGrade = option.nGrade.Get();
    if (bReleased)
        option.nGrade.Put(static_cast<std::uint8_t>(nGrade & ~EquipGradeFlag::AdditionalNotReleased));
    else
        option.nGrade.Put(nGrade | EquipGradeFlag::AdditionalNotReleased);
}

auto GW_ItemSlotEquip::GetCubeExOptLv() const -> std::int32_t
{
    if (item.nAttribute.Get() & EquipAttr::CubeExOpt2)
        return 2;
    return (item.nAttribute.Get() & EquipAttr::CubeExOpt1) >> 10;
}

// === Potential options ===

auto GW_ItemSlotEquip::GetPotentialOption(std::int32_t nIdx) const -> std::uint16_t
{
    switch (nIdx)
    {
    case 0: return option.nOption1.Get();
    case 1: return option.nOption2.Get();
    case 2: return option.nOption3.Get();
    case 3: return option.nOption4.Get();
    case 4: return option.nOption6.Get();
    case 5: return option.nOption7.Get();
    default: return 0;
    }
}

void GW_ItemSlotEquip::SetPotentialOption(std::int32_t nIdx, std::uint16_t usOption)
{
    switch (nIdx)
    {
    case 0: option.nOption1.Put(usOption); break;
    case 1: option.nOption2.Put(usOption); break;
    case 2: option.nOption3.Put(usOption); break;
    case 3: option.nOption4.Put(usOption); break;
    case 4: option.nOption6.Put(usOption); break;
    case 5: option.nOption7.Put(usOption); break;
    default: break;
    }
}

void GW_ItemSlotEquip::ResetItemGrade()
{
    SetReleased(0);
    option.nGrade.Put(option.nGrade.Get() & 0xF0); // clear grade, keep flags
    for (std::int32_t i = 0; i < 3; ++i)
        SetPotentialOption(i, 0);
}

void GW_ItemSlotEquip::ResetAdditionalGrade()
{
    option.nGrade.Put(static_cast<std::uint8_t>(option.nGrade.Get() & ~EquipGradeFlag::AdditionalNotReleased));
    for (std::int32_t i = 3; i < 6; ++i)
        SetPotentialOption(i, 0);
}

void GW_ItemSlotEquip::ResetSoulSocketAndOption()
{
    option.nSoulOption.Put(0);
    option.nSoulOptionID.Put(0);
    option.nSoulSocketID.Put(0);
}

// === Serialization ===

void GW_ItemSlotEquip::RawDecode(InPacket& iPacket)
{
    GW_ItemSlotBase::RawDecode(iPacket);
    item.Decode(iPacket);
    SetItemTitle(iPacket.DecodeStr());

    // Potential / options (note: nOption5 is decoded after nOption6, nOption7)
    option.nGrade.Put(static_cast<std::uint8_t>(iPacket.Decode1()));
    option.nCHUC.Put(static_cast<std::uint8_t>(iPacket.Decode1()));
    option.nOption1.Put(static_cast<std::uint16_t>(iPacket.Decode2()));
    option.nOption2.Put(static_cast<std::uint16_t>(iPacket.Decode2()));
    option.nOption3.Put(static_cast<std::uint16_t>(iPacket.Decode2()));
    option.nOption4.Put(static_cast<std::uint16_t>(iPacket.Decode2()));
    option.nOption6.Put(static_cast<std::uint16_t>(iPacket.Decode2()));
    option.nOption7.Put(static_cast<std::uint16_t>(iPacket.Decode2()));
    option.nOption5.Put(static_cast<std::uint16_t>(iPacket.Decode2()));

    // Serial number (only for non-cash items)
    if (liCashItemSN)
        liSN = 0;
    else
        iPacket.DecodeBuffer(reinterpret_cast<std::uint8_t*>(&liSN), 8);

    iPacket.DecodeBuffer(reinterpret_cast<std::uint8_t*>(&ftEquipped), 8);
    nPrevBonusExpRate = iPacket.Decode4();

    GW_CashItemOption::Decode(iPacket, cashItemOption);

    // Soul options
    option.nSoulOptionID.Put(iPacket.Decode2());
    option.nSoulSocketID.Put(iPacket.Decode2());
    option.nSoulOption.Put(static_cast<std::uint16_t>(iPacket.Decode2()));
}

void GW_ItemSlotEquip::RawEncode(OutPacket& oPacket, bool bToClient)
{
    GW_ItemSlotBase::RawEncode(oPacket, bToClient);
    item.Encode(oPacket);
    oPacket.EncodeStr(GetItemTitle());

    oPacket.Encode1(static_cast<std::int8_t>(option.nGrade.Get()));
    oPacket.Encode1(static_cast<std::int8_t>(option.nCHUC.Get()));

    // Potential lines 1-3: hide unreleased potentials from client
    if (bToClient && !IsReleased() && GetItemGrade())
    {
        oPacket.Encode2(0);
        oPacket.Encode2(0);
        oPacket.Encode2(0);
    }
    else
    {
        oPacket.Encode2(static_cast<std::int16_t>(option.nOption1.Get()));
        oPacket.Encode2(static_cast<std::int16_t>(option.nOption2.Get()));
        oPacket.Encode2(static_cast<std::int16_t>(option.nOption3.Get()));
    }

    // Additional potential lines 4,6,7: hide unreleased additional potentials
    if (!bToClient || IsAdditionalReleased() || option.nOption4.Get() < 10000u)
    {
        oPacket.Encode2(static_cast<std::int16_t>(option.nOption4.Get()));
        oPacket.Encode2(static_cast<std::int16_t>(option.nOption6.Get()));
        oPacket.Encode2(static_cast<std::int16_t>(option.nOption7.Get()));
    }
    else
    {
        oPacket.Encode2(static_cast<std::int16_t>(option.nOption4.Get() / 10000));
        oPacket.Encode2(0);
        oPacket.Encode2(0);
    }

    // Look change option (always sent)
    oPacket.Encode2(static_cast<std::int16_t>(option.nOption5.Get()));

    // Serial number (only for non-cash items)
    if (!liCashItemSN)
        oPacket.EncodeBuffer(reinterpret_cast<const std::uint8_t*>(&liSN), 8);

    oPacket.EncodeBuffer(reinterpret_cast<const std::uint8_t*>(&ftEquipped), 8);
    oPacket.Encode4(nPrevBonusExpRate);

    cashItemOption.Encode(oPacket);

    // Soul options
    oPacket.Encode2(option.nSoulOptionID.Get());
    oPacket.Encode2(option.nSoulSocketID.Get());
    oPacket.Encode2(static_cast<std::int16_t>(option.nSoulOption.Get()));
}

// === Set item ===

auto GW_ItemSlotEquip::IsSetItem() -> std::int32_t
{
    return GetSetItemID() != 0;
}

auto GW_ItemSlotEquip::GetSetItemID() -> std::int32_t
{
    auto& info = ItemInfo::GetInstance();
    auto* pEquip = info.GetEquipItem(nItemID);
    return pEquip ? pEquip->nSetItemID : 0;
}

} // namespace ms
