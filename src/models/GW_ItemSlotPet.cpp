#include "GW_ItemSlotPet.h"

#include "network/InPacket.h"
#include "network/OutPacket.h"
#include "templates/item/ItemInfo.h"
#include "wz/WzProperty.h"

#include <cstring>

namespace ms
{

// === BackwardUpdateCashItem ===

void GW_ItemSlotPet::BackwardUpdateCashItem(GW_ItemSlotBase* pOther)
{
    auto* pSrc = static_cast<GW_ItemSlotPet*>(pOther);

    nLevel.Put(pSrc->nLevel.Get());
    nTameness.Put(pSrc->nTameness.Get());
    nRepleteness.Put(pSrc->nRepleteness.Get());
    nPetAttribute.Put(pSrc->nPetAttribute.Get());
    nRemainLife.Put(pSrc->nRemainLife.Get());
    nAttribute.Put(pSrc->nAttribute.Get());
    dateDead = pSrc->dateDead;
    nActiveState.Put(pSrc->nActiveState.Get());
    nAutoBuffSkill.Put(pSrc->nAutoBuffSkill.Get());
    nPetHue.Put(pSrc->nPetHue.Get());
    nGiantRate.Put(pSrc->nGiantRate.Get());
    usPetSkill.Put(pSrc->usPetSkill.Get());
    std::memcpy(sPetName.data(), pSrc->sPetName.data(), sPetName.size());
}

// === Serialization ===

void GW_ItemSlotPet::RawDecode(InPacket& iPacket)
{
    GW_ItemSlotBase::RawDecode(iPacket);

    iPacket.DecodeBuffer(reinterpret_cast<std::uint8_t*>(sPetName.data()), 13);
    nLevel.Put(static_cast<std::uint8_t>(iPacket.Decode1()));
    nTameness.Put(iPacket.Decode2());
    nRepleteness.Put(static_cast<std::uint8_t>(iPacket.Decode1()));
    iPacket.DecodeBuffer(reinterpret_cast<std::uint8_t*>(&dateDead), 8);
    nPetAttribute.Put(iPacket.Decode2());
    usPetSkill.Put(static_cast<std::uint16_t>(iPacket.Decode2()));
    nRemainLife.Put(iPacket.Decode4());
    nAttribute.Put(iPacket.Decode2());
    nActiveState.Put(static_cast<std::uint8_t>(iPacket.Decode1()));
    nAutoBuffSkill.Put(iPacket.Decode4());
    nPetHue.Put(iPacket.Decode4());
    nGiantRate.Put(iPacket.Decode2());
}

void GW_ItemSlotPet::RawEncode(OutPacket& oPacket, bool bForInternal)
{
    GW_ItemSlotBase::RawEncode(oPacket, bForInternal);

    oPacket.EncodeBuffer(reinterpret_cast<const std::uint8_t*>(sPetName.data()), 13);
    oPacket.Encode1(static_cast<std::int8_t>(nLevel.Get()));
    oPacket.Encode2(nTameness.Get());
    oPacket.Encode1(static_cast<std::int8_t>(nRepleteness.Get()));
    oPacket.EncodeBuffer(reinterpret_cast<const std::uint8_t*>(&dateDead), 8);
    oPacket.Encode2(nPetAttribute.Get());
    oPacket.Encode2(static_cast<std::int16_t>(usPetSkill.Get()));
    oPacket.Encode4(nRemainLife.Get());
    oPacket.Encode2(nAttribute.Get());
    oPacket.Encode1(static_cast<std::int8_t>(nActiveState.Get()));
    oPacket.Encode4(nAutoBuffSkill.Get());
    oPacket.Encode4(nPetHue.Get());
    oPacket.Encode2(nGiantRate.Get());
}

// === Set item ===

auto GW_ItemSlotPet::IsSetItem() -> std::int32_t
{
    return GetSetItemID() != 0;
}

auto GW_ItemSlotPet::GetSetItemID() -> std::int32_t
{
    auto& info = ItemInfo::GetInstance();
    auto pInfo = info.GetItemInfo(static_cast<std::int32_t>(nItemID));
    if (!pInfo)
        return 0;
    auto pChild = pInfo->GetChild("setItemID");
    return pChild ? pChild->GetInt(0) : 0;
}

auto GW_ItemSlotPet::IsAllowedOverlappedSet() -> bool
{
    auto& info = ItemInfo::GetInstance();
    auto pInfo = info.GetItemInfo(static_cast<std::int32_t>(nItemID));
    if (!pInfo)
        return false;
    auto pChild = pInfo->GetChild("allowOverlappedSet");
    return pChild ? pChild->GetInt(0) != 0 : false;
}

auto GW_ItemSlotPet::IsDead() const -> bool
{
    auto& info = ItemInfo::GetInstance();
    auto pInfo = info.GetItemInfo(static_cast<std::int32_t>(nItemID));
    if (pInfo)
    {
        auto pLimited = pInfo->GetChild("limitedLife");
        if (pLimited && pLimited->GetInt(0))
            return nRemainLife.Get() <= 0;
        auto pImmortal = pInfo->GetChild("permanent");
        if (pImmortal && pImmortal->GetInt(0))
            return false;
    }
    return dateDead >= kDbDate20790101;
}

auto GW_ItemSlotPet::IsDeadByDate() const -> bool
{
    auto& info = ItemInfo::GetInstance();
    auto pInfo = info.GetItemInfo(static_cast<std::int32_t>(nItemID));
    if (pInfo)
    {
        auto pImmortal = pInfo->GetChild("permanent");
        if (pImmortal && pImmortal->GetInt(0))
            return false;
    }
    return dateDead >= kDbDate20790101;
}

} // namespace ms
