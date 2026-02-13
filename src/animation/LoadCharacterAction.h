#pragma once

#include "enums/BodyPart.h"

#include <cstdint>
#include <vector>

namespace ms
{

class ActionFrame;

/// Number of body part equipment slots
inline constexpr std::int32_t kBodyPartCount =
    static_cast<std::int32_t>(BodyPart::BP_COUNT);

/**
 * @brief Load all character equipment sprites for a given action
 *
 * Based on CActionMan::load_character_action from the decompiled client
 * and ActionManager.LoadCharacterActionInternal from the C# reference.
 *
 * Calls LoadItemAction for:
 * 1. Body item (nSkin + 2000)
 * 2. Face item (nSkin + 12000)
 * 3. Each equipped body part slot (hair, cap, weapons, etc.)
 *
 * @param nAction            Action code (CharacterAction enum value)
 * @param nSkin              Skin ID (0-based)
 * @param nJob               Character job code
 * @param aEquips            Equipment item IDs per body part slot (BP_COUNT entries, 0 = empty)
 * @param aFrame             Output: merged action frames
 * @param nWeaponStickerID   Weapon sticker item ID (0 = none)
 * @param bDrawElfEar        Whether to draw elf ear pieces
 * @param bInvisibleCashCape Whether an invisible cash cape is equipped
 * @param bZigzag            Whether action uses zigzag frame mirroring
 * @param bRemoveBody        Whether to remove body (e.g. full-body costumes)
 */
void LoadCharacterAction(
    std::int32_t nAction,
    std::int32_t nSkin,
    std::int32_t nJob,
    const std::int32_t (&aEquips)[kBodyPartCount],
    std::vector<ActionFrame>& aFrame,
    std::int32_t nWeaponStickerID = 0,
    bool bDrawElfEar = false,
    bool bInvisibleCashCape = false,
    bool bZigzag = false,
    bool bRemoveBody = false);

} // namespace ms
