#include "GW_CashItemOption.h"

#include "network/InPacket.h"
#include "network/OutPacket.h"

namespace ms
{

auto GW_CashItemOption::GetCashItemOptionGroup(std::int32_t nOption) -> std::int32_t
{
    switch (nOption / 1000)
    {
    case 21:
    case 22:
        return 0;
    case 31:
    case 32:
        return 1;
    case 11:
    case 12:
    case 13:
    case 14:
        return 2;
    case 43:
    case 44:
        return 3;
    case 41:
    case 42:
        return 4;
    default:
        return -1;
    }
}

void GW_CashItemOption::Decode(InPacket& iPacket, GW_CashItemOption& option)
{
    iPacket.DecodeBuffer(reinterpret_cast<std::uint8_t*>(&option.liCashItemSN), 8);
    iPacket.DecodeBuffer(reinterpret_cast<std::uint8_t*>(&option.ftExpireDate), 8);
    option.nGrade = iPacket.Decode4();
    for (auto& opt : option.anOption)
        opt = iPacket.Decode4();
}

void GW_CashItemOption::Encode(OutPacket& oPacket) const
{
    oPacket.EncodeBuffer(reinterpret_cast<const std::uint8_t*>(&liCashItemSN), 8);
    oPacket.EncodeBuffer(reinterpret_cast<const std::uint8_t*>(&ftExpireDate), 8);
    oPacket.Encode4(nGrade);
    for (auto opt : anOption)
        oPacket.Encode4(opt);
}

} // namespace ms
