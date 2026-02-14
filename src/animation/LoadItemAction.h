#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

class ActionFrame;
class WzProperty;

/**
 * @brief Load character equipment sprites for a given action
 *
 * Based on load_item_action from the decompiled client.
 * Loads a single equipment item's sprites and merges them into the
 * provided ActionFrame array via ActionFrame::Merge.
 *
 * This is the core function for character equipment rendering —
 * called once per equipped item per action to layer sprites onto
 * body animations.
 *
 * @param nAction       Action code (CharacterAction enum value)
 * @param nJob          Character job code
 * @param nID           Equipment item ID
 * @param aFrame        Output frame array (already sized for body)
 * @param nWeaponStickerID  Weapon sticker item ID (0 = none)
 * @param nVehicleID    Vehicle ID (0 = none)
 * @param nGhostIndex   Ghost sub-index for ghost actions (132-139)
 * @param bCapEquip     Whether a cap is equipped
 * @param bGatherEquip  Whether a gather tool is equipped
 * @param bDrawElfEar   Whether to draw elf ear pieces
 * @param nLarknessState Luminous larkness state skill ID
 * @param bCashCape     Whether it's a cash cape item
 * @param nMixHairID    Mix hair item ID (0 = none)
 * @param nMixPercent   Mix percentage (0-100, 100 = no mix)
 * @param bCapExtendFrame Whether to use cap extended frames
 */
void LoadItemAction(
    std::int32_t nAction,
    std::int32_t nJob,
    std::int32_t nID,
    std::vector<ActionFrame>& aFrame,
    std::int32_t nWeaponStickerID = 0,
    std::int32_t nVehicleID = 0,
    std::int32_t nGhostIndex = 0,
    bool bCapEquip = false,
    bool bGatherEquip = false,
    bool bDrawElfEar = false,
    std::int32_t nLarknessState = 0,
    bool bCashCape = false,
    std::int32_t nMixHairID = 0,
    std::int32_t nMixPercent = 100,
    bool bCapExtendFrame = false);

/**
 * @brief Load equipment sprites using extended frame data
 *
 * Based on load_item_action_extend_frame from the decompiled client.
 * Uses the item's weeklyImg to determine extended frame count,
 * replicates base frames to fill, then loads equipment sprites
 * into the extended frame array.
 *
 * @param pProp         WZ property containing extended frame data (weeklyImg)
 * @param nAction       Action code (CharacterAction enum value)
 * @param nJob          Character job code
 * @param nID           Equipment item ID
 * @param aFrame        Output frame array (may be resized)
 * @param nWeaponStickerID  Weapon sticker item ID (0 = none)
 * @param nVehicleID    Vehicle ID (0 = none)
 * @param nGhostIndex   Ghost sub-index for ghost actions (132-139)
 * @param bCapEquip     Whether a cap is equipped
 * @param bGatherEquip  Whether a gather tool is equipped
 * @param bDrawElfEar   Whether to draw elf ear pieces
 * @param nLarknessState Luminous larkness state skill ID
 * @param bCashCape     Whether it's a cash cape item
 * @param nMixHairID    Mix hair item ID (0 = none)
 * @param nMixPercent   Mix percentage (0-100, 100 = no mix)
 */
void LoadItemActionExtendFrame(
    const std::shared_ptr<WzProperty>& pProp,
    std::int32_t nAction,
    std::int32_t nJob,
    std::int32_t nID,
    std::vector<ActionFrame>& aFrame,
    std::int32_t nWeaponStickerID = 0,
    std::int32_t nVehicleID = 0,
    std::int32_t nGhostIndex = 0,
    bool bCapEquip = false,
    bool bGatherEquip = false,
    bool bDrawElfEar = false,
    std::int32_t nLarknessState = 0,
    bool bCashCape = false,
    std::int32_t nMixHairID = 0,
    std::int32_t nMixPercent = 100);

} // namespace ms
