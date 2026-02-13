#include "LoadCharacterAction.h"

#include "ActionFrame.h"
#include "LoadItemAction.h"
#include "constants/ActionHelpers.h"
#include "enums/BodyPart.h"
#include "enums/CharacterAction.h"

namespace ms
{

void LoadCharacterAction(
    std::int32_t nAction,
    std::int32_t nSkin,
    std::int32_t nJob,
    const std::int32_t (&aEquips)[kBodyPartCount],
    std::vector<ActionFrame>& aFrame,
    std::int32_t nWeaponStickerID,
    bool bDrawElfEar,
    bool bInvisibleCashCape,
    bool bZigzag,
    bool bRemoveBody)
{
    constexpr std::int32_t kSkinBase = 2000;
    constexpr std::int32_t kFaceBase = 12000;

    // Validate weapon sticker
    if (!is_weapon_sticker_item(nWeaponStickerID))
    {
        nWeaponStickerID = 0;
    }

    // Compute body item ID
    auto nSkinItem = nSkin + kSkinBase;
    if (bRemoveBody ||
        nAction == static_cast<std::int32_t>(CharacterAction::HideBody))
    {
        nSkinItem = 0;
    }

    // Compute face item ID
    auto nFaceItem = nSkin + kFaceBase;

    // 1. Load body sprites
    LoadItemAction(
        nAction, nJob, nSkinItem, aFrame,
        /*nWeaponStickerID=*/0,
        /*nVehicleID=*/0,
        /*nGhostIndex=*/0,
        /*bCapEquip=*/false,
        /*bGatherEquip=*/false,
        /*bDrawElfEar=*/bDrawElfEar,
        /*nLarknessState=*/0,
        /*bCashCape=*/bInvisibleCashCape,
        /*nMixHairID=*/0,
        /*nMixPercent=*/0,
        /*bCapExtendFrame=*/false);

    // 2. Load face sprites
    LoadItemAction(
        nAction, nJob, nFaceItem, aFrame,
        /*nWeaponStickerID=*/0,
        /*nVehicleID=*/0,
        /*nGhostIndex=*/0,
        /*bCapEquip=*/false,
        /*bGatherEquip=*/false,
        /*bDrawElfEar=*/bDrawElfEar,
        /*nLarknessState=*/0,
        /*bCashCape=*/bInvisibleCashCape,
        /*nMixHairID=*/0,
        /*nMixPercent=*/0,
        /*bCapExtendFrame=*/false);

    // 3. Equipment loop — iterate all body part slots
    auto eAction = static_cast<CharacterAction>(nAction);
    for (std::int32_t i = 0; i < kBodyPartCount; ++i)
    {
        auto ePart = static_cast<BodyPart>(i);

        // Skip logic per body part
        switch (ePart)
        {
        case BodyPart::BP_WEAPON:
            if (is_weapon_hide_action(eAction))
                continue;
            if (eAction == CharacterAction::Supercannon)
                continue;
            break;

        case BodyPart::BP_TAMINGMOB:
        case BodyPart::BP_SADDLE:
        case BodyPart::BP_MOBEQUIP:
            continue;

        case BodyPart::BP_SHIELD:
            if (eAction == CharacterAction::Pvpko)
                continue;
            break;

        default:
            break;
        }

        // Dead action → use Jump for equipment
        auto nRealAction = nAction;
        if (eAction == CharacterAction::Dead)
        {
            nRealAction = static_cast<std::int32_t>(CharacterAction::Jump);
        }

        // Skip empty equipment slots
        auto nEquipID = aEquips[i];
        if (nEquipID == 0)
            continue;

        // Weapon sticker: only pass for weapon slot
        auto nStickerForSlot =
            (ePart == BodyPart::BP_WEAPON) ? nWeaponStickerID : 0;

        if (ePart == BodyPart::BP_HAIR)
        {
            // Hair: skip if invisible cash cape
            if (bInvisibleCashCape)
                continue;

            LoadItemAction(
                nRealAction, nJob, nEquipID, aFrame,
                /*nWeaponStickerID=*/nStickerForSlot,
                /*nVehicleID=*/0,
                /*nGhostIndex=*/0,
                /*bCapEquip=*/false,
                /*bGatherEquip=*/false,
                /*bDrawElfEar=*/bDrawElfEar,
                /*nLarknessState=*/0,
                /*bCashCape=*/bInvisibleCashCape,
                /*nMixHairID=*/0,
                /*nMixPercent=*/0,
                /*bCapExtendFrame=*/false);
        }
        else if (ePart == BodyPart::BP_CAP)
        {
            // Cap: skip if removeBody; use mixPercent=100
            if (bRemoveBody)
                continue;

            LoadItemAction(
                nRealAction, nJob, nEquipID, aFrame,
                /*nWeaponStickerID=*/nStickerForSlot,
                /*nVehicleID=*/0,
                /*nGhostIndex=*/0,
                /*bCapEquip=*/false,
                /*bGatherEquip=*/false,
                /*bDrawElfEar=*/bDrawElfEar,
                /*nLarknessState=*/0,
                /*bCashCape=*/bInvisibleCashCape,
                /*nMixHairID=*/0,
                /*nMixPercent=*/100,
                /*bCapExtendFrame=*/false);
        }
        else
        {
            // All other equipment: skip if removeBody
            if (bRemoveBody)
                continue;

            LoadItemAction(
                nRealAction, nJob, nEquipID, aFrame,
                /*nWeaponStickerID=*/nStickerForSlot,
                /*nVehicleID=*/0,
                /*nGhostIndex=*/0,
                /*bCapEquip=*/false,
                /*bGatherEquip=*/false,
                /*bDrawElfEar=*/bDrawElfEar,
                /*nLarknessState=*/0,
                /*bCashCape=*/bInvisibleCashCape,
                /*nMixHairID=*/0,
                /*nMixPercent=*/0,
                /*bCapExtendFrame=*/false);
        }
    }

    // 4. Zigzag mirroring: [0,1,2,3] → [0,1,2,3,2,1,0]
    if (bZigzag && aFrame.size() > 1)
    {
        auto nOrigSize = aFrame.size();
        aFrame.reserve(nOrigSize * 2 - 1);
        for (auto i = static_cast<std::ptrdiff_t>(nOrigSize) - 2; i >= 0; --i)
        {
            aFrame.push_back(aFrame[static_cast<std::size_t>(i)]);
        }
    }
}

} // namespace ms
