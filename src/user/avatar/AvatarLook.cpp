#include "AvatarLook.h"

#include "network/InPacket.h"

namespace ms
{

void AvatarLook::Initialize()
{
    nGender = 0;
    nSkin = 0;
    nFace = 0;
    nWeaponStickerID = 0;
    nWeaponID = 0;
    nSubWeaponID = 0;
    nJob = 0;
    bDrawElfEar = false;
    nDemonSlayerDefFaceAcc = 0;
    nXenonDefFaceAcc = 0;
    bIsZeroBetaLook = false;
    nMixedHairColor = 0;
    nMixHairPercent = 0;
    anHairEquip.fill(0);
    anPetID.fill(0);
}

void AvatarLook::Decode([[maybe_unused]] InPacket& iPacket)
{
    // TODO: decode AvatarLook fields from packet
    // Original: AvatarLook::Decode(this, iPacket)
}

} // namespace ms
