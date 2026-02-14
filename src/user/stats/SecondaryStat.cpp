#include "SecondaryStat.h"

#include "network/InPacket.h"
#include "network/OutPacket.h"

namespace ms
{

// ============================================================================
// LarknessInfo
// ============================================================================

auto SecondaryStat::LarknessInfo::Decode(InPacket& iPacket) -> void
{
    rLarkness = iPacket.Decode4();
    tLarkness = iPacket.Decode4();
}

// ============================================================================
// StopForceAtom
// ============================================================================

auto SecondaryStat::StopForceAtom::Init() -> void
{
    nIdx = 0;
    nCount = 0;
    nWeaponID = 0;
    aAngleInfo.clear();
}

auto SecondaryStat::StopForceAtom::Decode(InPacket& iPacket) -> void
{
    nIdx = 0;
    nCount = 0;
    nWeaponID = 0;
    aAngleInfo.clear();

    nIdx = iPacket.Decode4();
    nCount = iPacket.Decode4();
    nWeaponID = iPacket.Decode4();

    const auto nAngleCount = iPacket.Decode4();
    for (std::int32_t i = 0; i < nAngleCount; ++i)
    {
        aAngleInfo.push_back(iPacket.Decode4());
    }
}

auto SecondaryStat::StopForceAtom::Encode(OutPacket& oPacket) -> void
{
    oPacket.Encode4(nIdx);
    oPacket.Encode4(nCount);
    oPacket.Encode4(nWeaponID);

    oPacket.Encode4(static_cast<std::int32_t>(aAngleInfo.size()));
    for (const auto nAngle : aAngleInfo)
    {
        oPacket.Encode4(nAngle);
    }
}

} // namespace ms
