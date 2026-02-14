#include "GW_ItemSlotBase.h"

#include "GW_ItemSlotBundle.h"
#include "GW_ItemSlotEquip.h"
#include "GW_ItemSlotPet.h"
#include "network/InPacket.h"
#include "network/OutPacket.h"

namespace ms
{

// === Serialization ===

void GW_ItemSlotBase::RawDecode(InPacket& iPacket)
{
    nItemID = iPacket.Decode4();
    if (iPacket.Decode1())
        iPacket.DecodeBuffer(reinterpret_cast<std::uint8_t*>(&liCashItemSN), 8);
    else
        liCashItemSN = 0;
    iPacket.DecodeBuffer(reinterpret_cast<std::uint8_t*>(&dateExpire), 8);
    nBagIndex = iPacket.Decode4();
}

void GW_ItemSlotBase::RawEncode(OutPacket& oPacket, bool /*bForInternal*/)
{
    oPacket.Encode4(static_cast<std::int32_t>(nItemID));
    auto bCash = liCashItemSN != 0;
    oPacket.Encode1(static_cast<std::int8_t>(bCash));
    if (bCash)
        oPacket.EncodeBuffer(reinterpret_cast<const std::uint8_t*>(&liCashItemSN), 8);
    oPacket.EncodeBuffer(reinterpret_cast<const std::uint8_t*>(&dateExpire), 8);
    oPacket.Encode4(nBagIndex);
}

auto GW_ItemSlotBase::CreateItem(std::int32_t nType) -> std::shared_ptr<GW_ItemSlotBase>
{
    switch (nType)
    {
    case GW_ItemSlotEquip_TYPE:
        return std::make_shared<GW_ItemSlotEquip>();
    case GW_ItemSlotBundle_TYPE:
        return std::make_shared<GW_ItemSlotBundle>();
    case GW_ItemSlotPet_TYPE:
        return std::make_shared<GW_ItemSlotPet>();
    default:
        return nullptr;
    }
}

auto GW_ItemSlotBase::IsBagOpened() const -> bool
{
    auto nID = static_cast<std::int32_t>(nItemID);
    auto nCategory = nID / 10000;
    if ((nCategory == 265 || nCategory == 308 || nCategory == 433) && nBagIndex > -1)
    {
        std::int32_t nMaxBag;
        auto nType = nID / 1000000;
        switch (nType)
        {
        case 2: // Consume
        case 3: // Setup
            nMaxBag = 2;
            break;
        case 4: // Etc
            nMaxBag = 7;
            break;
        default:
            nMaxBag = 0;
            break;
        }
        return nBagIndex < nMaxBag;
    }
    return false;
}

} // namespace ms
