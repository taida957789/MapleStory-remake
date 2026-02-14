#include "LoadItemAction.h"

#include "ActionData.h"
#include "ActionFrame.h"
#include "ActionMan.h"
#include "CharacterImgEntry.h"
#include "constants/EquipDataPath.h"
#include "constants/WeaponConstants.h"
#include "graphics/WzGr2DCanvas.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

#include <memory>
#include <string>

namespace ms
{

namespace
{

// === Color mix stub ===
// Full pixel blending deferred; returns original canvas unchanged.
auto GetColorMixCopy(
    const std::shared_ptr<WzGr2DCanvas>& pCanvas,
    [[maybe_unused]] const std::shared_ptr<WzGr2DCanvas>& pCanvasMix,
    [[maybe_unused]] std::int32_t nMixPercent) -> std::shared_ptr<WzGr2DCanvas>
{
    return pCanvas;
}

// === Helper: wrap WzCanvas in WzGr2DCanvas ===
auto MakeGr2DCanvas(const std::shared_ptr<WzProperty>& pProp)
    -> std::shared_ptr<WzGr2DCanvas>
{
    if (!pProp)
        return nullptr;
    auto wzCanvas = pProp->GetCanvas();
    if (!wzCanvas)
        return nullptr;
    return std::make_shared<WzGr2DCanvas>(wzCanvas);
}

// === sUOL flag check ===
// Returns true if the action is a special UOL action (dance, starplanet,
// spinoff, hideBody, BattlePvP).
auto IsUOLAction(std::int32_t nAction) -> bool
{
    // dance (874-882)
    if (nAction >= 874 && nAction <= 882)
        return true;
    // starplanet dance (940-945)
    if (nAction >= 940 && nAction <= 945)
        return true;
    // starplanet event dance (946-951)
    if (nAction >= 946 && nAction <= 951)
        return true;
    // spinoff guitar (1156-1159)
    if (nAction >= 1156 && nAction <= 1159)
        return true;
    // hideBody (980)
    if (nAction == 980)
        return true;
    // BattlePvP (1051-1151)
    if (nAction >= 1051 && nAction <= 1151)
        return true;
    return false;
}

// === pPropMix flag check ===
// Returns true if this item type uses the special face/accessory/mix path.
auto IsMixableItem(std::int32_t nID) -> bool
{
    // Cape range (12000-12999 in decompiled is nID/10000==1, but plan says
    // face/hair/accessory/skin/cap)
    auto prefix = nID / 10000;

    // Face (20000-29999)
    if (prefix == 2)
        return true;
    // Hair (30000-49999)
    if (prefix == 3 || prefix == 4)
        return true;
    // Cap (1000000-1009999)
    if (prefix == 100)
        return true;
    // Accessory
    if (is_accessory(nID))
        return true;

    return false;
}

// === ResolveAction ===
// Resolves action code for special cases (BattlePvP, UOL items).
// Returns resolved action code. Sets bHideHead if in Leemalnyun range.
auto ResolveAction(
    std::int32_t nAction,
    [[maybe_unused]] std::int32_t nID,
    bool bIsUOL,
    bool bIsMixable,
    bool& bHideHead) -> std::int32_t
{
    // BattlePvP Leemalnyun range check (1139-1151)
    bHideHead = (nAction >= 1139 && nAction <= 1151);

    // If both sUOL and pPropMix flags are set, remap the action
    if (bIsUOL && bIsMixable)
    {
        if (nAction >= 1051 && nAction <= 1151)
        {
            // BattlePvP: remap to standard action
            action_mapping_for_battle_pvp(nAction);
        }
        else
        {
            // Dance/starplanet/spinoff/hideBody: force Stand1
            nAction = 2; // CharacterAction::Stand1
        }
    }

    return nAction;
}

// === ResolveWeaponSticker ===
// If a weapon sticker is equipped, overrides pImg with the sticker's
// weapon-type-specific image property.
void ResolveWeaponSticker(
    std::int32_t nWeaponStickerID,
    std::int32_t nID,
    bool bGatherEquip,
    std::shared_ptr<WzProperty>& pImg)
{
    if (nWeaponStickerID == 0)
        return;
    if (nWeaponStickerID / 100000 != 17)
        return;
    if (bGatherEquip)
        return;

    auto sPath = get_equip_data_path(nWeaponStickerID);
    if (sPath.empty())
        return;

    auto& resMan = WzResMan::GetInstance();
    auto pStickerRoot = resMan.GetProperty(sPath);
    if (!pStickerRoot)
        return;

    auto pInfo = pStickerRoot->GetChild("info");
    if (!pInfo)
        return;

    // Weekly handling
    auto pWeekly = pInfo->GetChild("weekly");
    if (pWeekly && pWeekly->GetInt(0) != 0)
    {
        // Weekly rotation — load day-specific WZ
        // TODO: implement weekly rotation path lookup
        // For now, use the base sticker node
    }

    // Navigate to weapon type sub-property
    auto nWeaponType = get_weapon_type(nID);
    auto pWeaponNode = pStickerRoot->GetChild(std::to_string(nWeaponType));
    if (pWeaponNode)
    {
        pImg = pWeaponNode;
    }
}

// === BuildPiecedImgAction ===
// For pieced actions, build a synthetic property container by looking up
// each piece's action/frame in the item's WZ data.
auto BuildPiecedImgAction(
    const std::shared_ptr<WzProperty>& pImg,
    std::int32_t nAction,
    std::int32_t nID) -> std::shared_ptr<WzProperty>
{
    auto& actionMan = ActionMan::GetInstance();
    const auto* pActionData = actionMan.GetActionData(nAction);
    if (!pActionData || !pActionData->m_bPieced || pActionData->m_aPieces.empty())
        return nullptr;

    // Create a synthetic property to hold the assembled pieces
    auto pImgAction = std::make_shared<WzProperty>("pieced");
    pImgAction->SetLoaded();

    auto isFace = (nID >= 20000 && nID <= 29999);
    std::int32_t pieceIdx = 0;

    for (const auto& piece : pActionData->m_aPieces)
    {
        std::shared_ptr<WzProperty> pPieceAction;

        if (!isFace)
        {
            auto sActionName = actionMan.GetActionName(piece.m_nAction);
            if (!sActionName.empty())
            {
                pPieceAction = pImg->GetChild(sActionName);
            }
        }

        if (!pPieceAction)
        {
            ++pieceIdx;
            continue;
        }

        // Get the specific frame
        auto pFrame = pPieceAction->GetChild(std::to_string(piece.m_nFrameIdx));
        if (pFrame)
        {
            // Insert as numbered child in synthetic property
            auto pChild = std::make_shared<WzProperty>(std::to_string(pieceIdx));
            pChild->SetLoaded();
            // Copy children from the frame into our synthetic child
            for (const auto& [name, child] : pFrame->GetChildren())
            {
                pChild->AddChild(child);
            }
            pImgAction->AddChild(pChild);
        }

        ++pieceIdx;
    }

    if (pImgAction->GetChildCount() == 0)
        return nullptr;

    return pImgAction;
}

// === GetImgAction ===
// Look up the action property in the item's WZ data.
// For ghost actions (132-139), navigates to the ghost sub-index.
auto GetImgAction(
    const std::shared_ptr<WzProperty>& pImg,
    std::int32_t nAction,
    std::int32_t nGhostIndex) -> std::shared_ptr<WzProperty>
{
    auto& actionMan = ActionMan::GetInstance();
    auto sActionName = actionMan.GetActionName(nAction);
    if (sActionName.empty())
        return nullptr;

    auto pImgAction = pImg->GetChild(sActionName);
    if (!pImgAction)
        return nullptr;

    // Ghost actions (132-139): navigate to ghost sub-index
    if (nAction >= 132 && nAction <= 139)
    {
        pImgAction = pImgAction->GetChild(std::to_string(nGhostIndex));
    }

    return pImgAction;
}

// === GetImgActionMix ===
// Load mix hair's action property if mix hair ID is specified.
auto GetImgActionMix(
    std::int32_t nMixHairID,
    std::int32_t nAction,
    std::int32_t nGhostIndex) -> std::shared_ptr<WzProperty>
{
    if (nMixHairID == 0)
        return nullptr;

    auto& actionMan = ActionMan::GetInstance();
    auto pMixEntry = actionMan.GetCharacterImgEntry(nMixHairID);
    if (!pMixEntry || !pMixEntry->m_pImg)
        return nullptr;

    return GetImgAction(pMixEntry->m_pImg, nAction, nGhostIndex);
}

// === AllocateFaceFrames ===
// For face items (2000-2999), compute and resize the frame array.
void AllocateFaceFrames(
    const std::shared_ptr<WzProperty>& pImgAction,
    std::vector<ActionFrame>& aFrame)
{
    // Read subAvatarAction
    auto pSubAvatar = pImgAction->GetChild("subAvatarAction");
    auto frameCount = static_cast<std::int32_t>(pImgAction->GetChildCount());

    if (pSubAvatar)
    {
        auto sSubAvatar = pSubAvatar->GetString("");
        if (!sSubAvatar.empty())
            --frameCount;
    }

    // Check repeat
    auto pRepeat = pImgAction->GetChild("repeat");
    if (pRepeat && pRepeat->GetInt(0) != 0)
        --frameCount;

    if (frameCount > 0)
    {
        aFrame.resize(static_cast<std::size_t>(frameCount));
    }
}

// === LoadFaceAccessorySprites ===
// Special path for face/accessory/mix items in UOL actions.
// Always reads frame "0" and merges into each output frame.
void LoadFaceAccessorySprites(
    const std::shared_ptr<WzProperty>& pImgAction,
    const std::shared_ptr<WzProperty>& pImgActionMix,
    const std::shared_ptr<CharacterImgEntry>& pImgEntry,
    std::vector<ActionFrame>& aFrame,
    std::int32_t nJob,
    bool bHideHead,
    bool bDrawElfEar,
    bool bCapEquip,
    std::int32_t nMixPercent)
{
    // Always use frame "0" for face/accessory UOL actions
    auto pProp = pImgAction->GetChild("0");
    if (!pProp)
        return;

    std::shared_ptr<WzProperty> pPropMix;
    if (pImgActionMix)
        pPropMix = pImgActionMix->GetChild("0");

    for (std::size_t frameIdx = 0; frameIdx < aFrame.size(); ++frameIdx)
    {
        if (bHideHead)
            continue;

        // Enumerate children of the frame property
        for (const auto& [sChildName, pChild] : pProp->GetChildren())
        {
            // Skip "ear" unless elf ear drawing enabled
            if (sChildName == "ear" && !bDrawElfEar)
                continue;

            std::shared_ptr<WzGr2DCanvas> pCanvas;
            std::shared_ptr<WzGr2DCanvas> pCanvasMix;

            if (sChildName == "hairShade")
            {
                // hairShade is a sub-property — get canvas from child "0"
                auto pHairShade = pProp->GetChild(sChildName);
                if (pHairShade)
                {
                    pCanvas = MakeGr2DCanvas(pHairShade->GetChild("0"));
                    if (pPropMix)
                    {
                        auto pMixHairShade = pPropMix->GetChild(sChildName);
                        if (pMixHairShade)
                            pCanvasMix = MakeGr2DCanvas(pMixHairShade->GetChild("0"));
                    }
                }
            }
            else
            {
                // backHairBelowCap: skip if no cap equipped
                if (sChildName == "backHairBelowCap" && !bCapEquip)
                    continue;

                // Direct canvas from child
                pCanvas = MakeGr2DCanvas(pChild);
                if (pPropMix)
                {
                    auto pMixChild = pPropMix->GetChild(sChildName);
                    pCanvasMix = MakeGr2DCanvas(pMixChild);
                }
            }

            // Apply color mix if needed
            if (pCanvas && nMixPercent != 100 && pCanvasMix)
            {
                pCanvas = GetColorMixCopy(pCanvas, pCanvasMix, nMixPercent);
            }

            // Merge into frame
            if (pCanvas)
            {
                aFrame[frameIdx].Merge(
                    pImgEntry->m_sISlot,
                    pImgEntry->m_sVSlot,
                    pCanvas,
                    nJob,
                    pChild);
            }
        }
    }
}

// === LoadEquipmentSprites ===
// General equipment sprite loading path.
// Iterates frames in pImgAction and merges child canvases into aFrame.
void LoadEquipmentSprites(
    const std::shared_ptr<WzProperty>& pImg,
    const std::shared_ptr<WzProperty>& pImgAction,
    const std::shared_ptr<WzProperty>& pImgActionMix,
    const std::shared_ptr<CharacterImgEntry>& pImgEntry,
    std::vector<ActionFrame>& aFrame,
    std::int32_t nAction,
    std::int32_t nJob,
    std::int32_t nID,
    bool bDrawElfEar,
    bool bCapEquip,
    bool bCashCape,
    std::int32_t nLarknessState,
    std::int32_t nVehicleID,
    bool bCapExtendFrame,
    std::int32_t nMixPercent)
{
    auto& actionMan = ActionMan::GetInstance();
    auto nWeaponType = get_weapon_type(nID);

    for (const auto& [sFrameName, pFrameNode] : pImgAction->GetChildren())
    {
        // Parse frame index
        std::int32_t nFrameIdx = 0;
        try
        {
            nFrameIdx = std::stoi(sFrameName);
        }
        catch (...)
        {
            continue; // Skip non-numeric children (subAvatarAction, repeat, etc.)
        }

        if (nFrameIdx < 0 || static_cast<std::size_t>(nFrameIdx) >= aFrame.size())
            continue;

        auto pFrame = pFrameNode;
        if (!pFrame)
            continue;

        // Get mix frame if available
        std::shared_ptr<WzProperty> pFrameMix;
        if (pImgActionMix)
            pFrameMix = pImgActionMix->GetChild(sFrameName);

        // vehicleDefaultFrame + extendFrame redirect
        if (pImgEntry->m_pVehicleDefaultFrame
            && pImgEntry->m_bExtendFrame
            && nVehicleID != 0)
        {
            auto sActionName = actionMan.GetActionName(nAction);
            auto pVehDefault = pImgEntry->m_pVehicleDefaultFrame->GetChild(sActionName);
            if (pVehDefault && pVehDefault->GetInt(0) != 0)
            {
                auto pDefault = pImg->GetChild("default");
                if (pDefault)
                    pFrame = pDefault;
                else
                    continue;
            }
        }

        // extendFrame + bCapExtendFrame redirect
        if (pImgEntry->m_bExtendFrame && bCapExtendFrame)
        {
            auto pDefault = pImg->GetChild("default");
            if (pDefault)
                pFrame = pDefault;
            else
                continue;
        }

        // Enumerate children within the frame
        bool bEffect = false;

        for (const auto& [sChildName, pChildNode] : pFrame->GetChildren())
        {
            // "effect" flag
            if (sChildName == "effect")
            {
                bEffect = true;
                continue;
            }

            // "weapon"/"weaponL" raging blow skip (frames 4-11)
            if ((sChildName == "weapon" || sChildName == "weaponL")
                && (nAction == 418 || nAction == 419
                    || nAction == 420 || nAction == 421)) // RagingBlow1-4
            {
                if (nFrameIdx >= 4 && nFrameIdx <= 11)
                    continue;
            }

            // "weapon"/"weaponL" noWeapon skip
            if ((sChildName == "weapon" || sChildName == "weaponL")
                && nWeaponType != 0)
            {
                const auto* pActionData = actionMan.GetActionData(nAction);
                if (pActionData
                    && static_cast<std::size_t>(nFrameIdx) < pActionData->m_aPieces.size()
                    && pActionData->m_aPieces[static_cast<std::size_t>(nFrameIdx)].m_bNoWeapon)
                {
                    continue;
                }
            }

            // "weapon"/"weaponL" ladder/rope kaiser/davenger skip
            if ((sChildName == "weapon" || sChildName == "weaponL")
                && (nAction == 30 || nAction == 67  // Ladder, Ladder2
                    || nAction == 31 || nAction == 68) // Rope, Rope2
                && (nWeaponType == 23 || nWeaponType == 40))
            {
                if (is_kaiser_job(nJob))
                    continue;
                if (is_davenger_job(nJob) && !bCashCape)
                    continue;
            }

            // "weapon2" handling
            if (sChildName == "weapon2")
            {
                const auto* pActionData = actionMan.GetActionData(nAction);
                bool bWeapon2Piece = false;
                if (pActionData
                    && static_cast<std::size_t>(nFrameIdx) < pActionData->m_aPieces.size())
                {
                    bWeapon2Piece = pActionData->m_aPieces[
                        static_cast<std::size_t>(nFrameIdx)].m_bWeapon2;
                }

                // Skip weapon2 unless weapon2 piece flag set or luminous weapon
                if (!bWeapon2Piece && nWeaponType != 21) // 21 = shining rod
                    continue;
            }

            // Luminous larkness remapping
            if (is_luminous_job(nJob) && nWeaponType == 21)
            {
                std::string sExpected = bEffect ? "effect" : "weapon";

                switch (nLarknessState)
                {
                case 20040216: sExpected += "1"; break; // light
                case 20040217: sExpected += "2"; break; // dark
                case 20040219: // equilibrium 1
                case 20040220: sExpected += "3"; break; // equilibrium 2
                default: break;
                }

                if (sChildName != sExpected)
                    continue;
            }

            // "ear" skip
            if (sChildName == "ear" && !bDrawElfEar)
                continue;

            // Extract canvas
            std::shared_ptr<WzGr2DCanvas> pCanvas;
            std::shared_ptr<WzGr2DCanvas> pCanvasMix;
            std::shared_ptr<WzProperty> pSpriteProp;

            if (sChildName == "hairShade")
            {
                // hairShade is a sub-property — get canvas from child "0"
                auto pHairShade = pFrame->GetChild(sChildName);
                if (pHairShade)
                {
                    auto pHairShadeFrame = pHairShade->GetChild("0");
                    pCanvas = MakeGr2DCanvas(pHairShadeFrame);
                    pSpriteProp = pHairShadeFrame;

                    if (pFrameMix)
                    {
                        auto pMixHS = pFrameMix->GetChild(sChildName);
                        if (pMixHS)
                            pCanvasMix = MakeGr2DCanvas(pMixHS->GetChild("0"));
                    }
                }
            }
            else
            {
                // backHairBelowCap: skip if no cap equipped
                if (sChildName == "backHairBelowCap" && !bCapEquip)
                    continue;

                // Direct canvas from child
                pCanvas = MakeGr2DCanvas(pChildNode);
                pSpriteProp = pChildNode;

                if (pFrameMix)
                {
                    auto pMixChild = pFrameMix->GetChild(sChildName);
                    pCanvasMix = MakeGr2DCanvas(pMixChild);
                }
            }

            // Apply color mix
            if (pCanvas && nMixPercent != 100 && pCanvasMix)
            {
                pCanvas = GetColorMixCopy(pCanvas, pCanvasMix, nMixPercent);
            }

            // Merge into frame
            if (pCanvas)
            {
                aFrame[static_cast<std::size_t>(nFrameIdx)].Merge(
                    pImgEntry->m_sISlot,
                    pImgEntry->m_sVSlot,
                    pCanvas,
                    nJob,
                    pSpriteProp);
            }
        }
    }
}

} // anonymous namespace

// === Main function ===

void LoadItemAction(
    std::int32_t nAction,
    std::int32_t nJob,
    std::int32_t nID,
    std::vector<ActionFrame>& aFrame,
    std::int32_t nWeaponStickerID,
    std::int32_t nVehicleID,
    std::int32_t nGhostIndex,
    bool bCapEquip,
    bool bGatherEquip,
    bool bDrawElfEar,
    std::int32_t nLarknessState,
    bool bCashCape,
    std::int32_t nMixHairID,
    std::int32_t nMixPercent,
    bool bCapExtendFrame)
{
    auto& actionMan = ActionMan::GetInstance();

    // Step 1: Determine UOL and mixable flags
    bool bIsUOL = IsUOLAction(nAction);
    bool bIsMixable = IsMixableItem(nID);

    // Step 2: Resolve action (BattlePvP remapping, UOL+mix -> Stand1)
    bool bHideHead = false;
    nAction = ResolveAction(nAction, nID, bIsUOL, bIsMixable, bHideHead);

    // Step 3: Load character image entry
    auto pImgEntry = actionMan.GetCharacterImgEntry(nID);
    if (!pImgEntry || !pImgEntry->m_pImg)
        return;

    auto pImg = pImgEntry->m_pImg;

    // Step 4: Resolve weapon sticker (may override pImg)
    ResolveWeaponSticker(nWeaponStickerID, nID, bGatherEquip, pImg);

    // Step 5: Get action property — pieced or normal path
    const auto* pActionData = actionMan.GetActionData(nAction);
    std::shared_ptr<WzProperty> pImgAction;

    if (pActionData && pActionData->m_bPieced)
    {
        pImgAction = BuildPiecedImgAction(pImg, nAction, nID);
    }
    else
    {
        pImgAction = GetImgAction(pImg, nAction, nGhostIndex);
    }

    if (!pImgAction)
        return;

    // Step 6: Get mix action property
    auto pImgActionMix = GetImgActionMix(nMixHairID, nAction, nGhostIndex);

    // Step 7: For face items, allocate frames
    bool bIsFace = (nID >= 20000 && nID <= 29999);
    if (bIsFace)
    {
        AllocateFaceFrames(pImgAction, aFrame);
    }

    // Step 8: Load sprites — face/accessory UOL path or general equipment path
    if (bIsUOL && bIsMixable)
    {
        LoadFaceAccessorySprites(
            pImgAction, pImgActionMix, pImgEntry,
            aFrame, nJob, bHideHead, bDrawElfEar,
            bCapEquip, nMixPercent);
    }
    else
    {
        LoadEquipmentSprites(
            pImg, pImgAction, pImgActionMix, pImgEntry,
            aFrame, nAction, nJob, nID,
            bDrawElfEar, bCapEquip, bCashCape,
            nLarknessState, nVehicleID, bCapExtendFrame,
            nMixPercent);
    }
}

void LoadItemActionExtendFrame(
    const std::shared_ptr<WzProperty>& pProp,
    std::int32_t nAction,
    std::int32_t nJob,
    std::int32_t nID,
    std::vector<ActionFrame>& aFrame,
    [[maybe_unused]] std::int32_t nWeaponStickerID,
    std::int32_t nVehicleID,
    std::int32_t nGhostIndex,
    bool bCapEquip,
    bool bGatherEquip,
    bool bDrawElfEar,
    std::int32_t nLarknessState,
    bool bCashCape,
    std::int32_t nMixHairID,
    std::int32_t nMixPercent)
{
    if (!pProp)
        return;

    auto& actionMan = ActionMan::GetInstance();

    // Look up the action property within pProp (weeklyImg)
    auto sActionName = actionMan.GetActionName(nAction);
    if (sActionName.empty())
        return;

    auto pPropAction = pProp->GetChild(sActionName);
    if (!pPropAction)
        return;

    // Get extended frame count from WZ
    auto nExFrameCount = static_cast<std::int32_t>(pPropAction->GetChildCount());
    if (nExFrameCount <= 0)
        return;

    // Get current frame count
    auto nCurrentCount = static_cast<std::int32_t>(aFrame.size());
    if (nCurrentCount <= 0)
        return;

    // If current frame count already >= extended count, nothing to do
    if (static_cast<std::uint32_t>(nCurrentCount)
        > static_cast<std::uint32_t>(nExFrameCount))
        return;

    // Build extended frame array by replicating base frames
    auto nMultipleCount = nExFrameCount / nCurrentCount;
    std::vector<ActionFrame> aExtendFrame(static_cast<std::size_t>(nExFrameCount));

    for (std::int32_t i = 0; i < nExFrameCount; ++i)
    {
        auto nSrcIdx = i / nMultipleCount;
        if (nSrcIdx < nCurrentCount)
            aExtendFrame[static_cast<std::size_t>(i)] =
                aFrame[static_cast<std::size_t>(nSrcIdx)];
    }

    // Dead → Jump for loading
    auto nLoadAction = nAction;
    if (nAction == 32) // ACT_DEAD
        nLoadAction = 28; // ACT_JUMP

    // Load item action into extended frames
    LoadItemAction(
        nLoadAction, nJob, nID, aExtendFrame,
        /*nWeaponStickerID=*/0,
        nVehicleID, nGhostIndex,
        bCapEquip, bGatherEquip, bDrawElfEar,
        nLarknessState, bCashCape,
        nMixHairID, nMixPercent,
        /*bCapExtendFrame=*/false);

    // Copy extended frames back to aFrame
    aFrame = std::move(aExtendFrame);
}

} // namespace ms
