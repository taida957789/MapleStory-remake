#include "GW_ItemSlotBundle.h"

#include "network/InPacket.h"
#include "network/OutPacket.h"

#include <algorithm>
#include <cstring>

namespace ms
{

// === BackwardUpdateCashItem ===

void GW_ItemSlotBundle::BackwardUpdateCashItem(GW_ItemSlotBase* pOther)
{
    auto* pSrc = static_cast<GW_ItemSlotBundle*>(pOther);
    nAttribute.Put(pSrc->nAttribute.Get());
}

// === Serialization ===

void GW_ItemSlotBundle::RawDecode(InPacket& iPacket)
{
    GW_ItemSlotBase::RawDecode(iPacket);

    nNumber.Put(static_cast<std::uint16_t>(iPacket.Decode2()));
    auto sStr = iPacket.DecodeStr();
    std::memcpy(sTitle.data(), sStr.c_str(),
                std::min(sStr.size() + 1, sTitle.size()));
    nAttribute.Put(iPacket.Decode2());

    auto nCategory = static_cast<std::int32_t>(nItemID) / 10000;
    if (nCategory == 207 || nCategory == 233)
        iPacket.DecodeBuffer(reinterpret_cast<std::uint8_t*>(&liSN), 8);
    else
        liSN = 0;
}

void GW_ItemSlotBundle::RawEncode(OutPacket& oPacket, bool bForInternal)
{
    GW_ItemSlotBase::RawEncode(oPacket, bForInternal);

    oPacket.Encode2(static_cast<std::int16_t>(nNumber.Get()));
    oPacket.EncodeStr(std::string(sTitle.data()));
    oPacket.Encode2(nAttribute.Get());

    auto nCategory = static_cast<std::int32_t>(nItemID) / 10000;
    if (nCategory == 207 || nCategory == 233)
        oPacket.EncodeBuffer(reinterpret_cast<const std::uint8_t*>(&liSN), 8);
}

// === Title ===

void GW_ItemSlotBundle::SetItemTitle(const std::string& s)
{
    auto len = std::min(s.size(), sTitle.size() - 1);
    std::memcpy(sTitle.data(), s.data(), len);
    sTitle[len] = '\0';
}

// === Binded ===

auto GW_ItemSlotBundle::IsBindedItem() -> std::int32_t
{
    auto nCategory = static_cast<std::int32_t>(nItemID) / 10000;
    return (nCategory == 265 || nCategory == 308 || nCategory == 433 || nCategory == 223)
        && (nAttribute.Get() & BundleAttr::Binded) != 0;
}

void GW_ItemSlotBundle::SetBinded()
{
    auto nCategory = static_cast<std::int32_t>(nItemID) / 10000;
    if (nCategory == 265 || nCategory == 308 || nCategory == 433 || nCategory == 223)
        nAttribute.Put(nAttribute.Get() | BundleAttr::Binded);
}

} // namespace ms
