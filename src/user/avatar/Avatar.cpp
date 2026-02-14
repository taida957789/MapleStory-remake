#include "Avatar.h"

#include "animation/ActionData.h"
#include "animation/ActionMan.h"
#include "animation/AnimationDisplayer.h"
#include "animation/CharacterActionFrameEntry.h"
#include "animation/TamingMobActionFrameEntry.h"
#include "app/Application.h"
#include "app/WvsContext.h"
#include "constants/ActionConstants.h"
#include "constants/ActionHelpers.h"
#include "constants/FieldConstants.h"
#include "constants/JobConstants.h"
#include "constants/WeaponConstants.h"
#include "graphics/Gr2DVector.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DCanvas.h"
#include "graphics/WzGr2DLayer.h"
#include "graphics/WzGr2DTypes.h"
#include "templates/morph/MorphTemplate.h"
#include "user/UserLocal.h"
#include "util/Logger.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <list>

namespace ms
{

// ============================================================================
// Init (public) — from decompiled CAvatar::Init @ 0x6030d0
// ============================================================================

void Avatar::Init(
    const AvatarLook& al,
    std::int32_t nMoveAction,
    std::shared_ptr<Gr2DVector> pOrigin,
    std::shared_ptr<WzGr2DLayer> pOverlay,
    std::int32_t z,
    std::int32_t x, std::int32_t y,
    std::int32_t nScale,
    std::int32_t nDefaultEmotion)
{
    m_nScale = nScale;
    Init(std::move(pOrigin), x, y, std::move(pOverlay), z);
    m_avatarLook = al;
    m_nMoveAction = nMoveAction;
    m_nDefaultEmotion = nDefaultEmotion;
    NotifyAvatarModified(false);
}

// ============================================================================
// Init (protected) — from decompiled CAvatar::Init @ 0x5edc20
//
// Creates the entire origin vector hierarchy and rendering layer tree.
//
// Origin hierarchy:
//   pOrigin(param)
//     └─ [optional wrapper if x,y != 0]
//          └─ m_pRawOrigin
//               └─ m_pOrigin (RelMove 0,0)
//                    ├─ m_pBodyOrigin (RelMove 0,0)
//                    │    ├─ m_pFaceOrigin
//                    │    ├─ m_pMuzzleOrigin
//                    │    ├─ m_pHandOrigin
//                    │    └─ m_pTailOrigin
//                    ├─ m_pTMNavelOrigin
//                    ├─ m_pTMHeadOrigin
//                    ├─ m_pTMMuzzleOrigin
//                    └─ m_pCubeOrigin
//
// Layer hierarchy (overlay chain):
//   pOverlay(param)
//     └─ m_pLayerUnderFace (z=0, origin=m_pBodyOrigin)
//          ├─ m_pLayerTransparent (z=2, origin=m_pBodyOrigin)
//          ├─ m_pLayerFace (z=1, origin=m_pFaceOrigin)
//          ├─ m_pLayerOverFace (z=2, origin=m_pBodyOrigin)
//          ├─ m_pLayerShadowPartner (z=-2, origin=m_pBodyOrigin)
//          ├─ m_pLayerUnderCharacter (z=-1, origin=m_pOrigin)
//          ├─ m_pLayerOverCharacter (z=3, origin=m_pOrigin)
//          ├─ m_pLayerJaguarCannon (z=3, origin=m_pOrigin)
//          └─ m_pLayerOverlay (z=0, no scale)
// ============================================================================

void Avatar::Init(
    std::shared_ptr<Gr2DVector> pOrigin,
    std::int32_t x, std::int32_t y,
    std::shared_ptr<WzGr2DLayer> pOverlay,
    std::int32_t z)
{
    auto& gr = WzGr2D::GetInstance();
    const std::uint32_t nScaleFilter = (m_nScale != 100) ? 2u : 0u;

    // --- Optional origin wrapper for non-zero offset ---
    if (x != 0 || y != 0)
    {
        auto pWrapper = std::make_shared<Gr2DVector>();
        pWrapper->PutOrigin(pOrigin.get());
        pWrapper->RelMove(x, y);
        pOrigin = std::move(pWrapper);
    }

    // --- m_pRawOrigin ---
    m_pRawOrigin = pOrigin;

    // --- m_pOrigin (child of pOrigin) ---
    m_pOrigin = std::make_shared<Gr2DVector>();
    m_pOrigin->PutOrigin(pOrigin.get());
    m_pOrigin->RelMove(0, 0);

    // --- m_pBodyOrigin (child of m_pOrigin) ---
    m_pBodyOrigin = std::make_shared<Gr2DVector>();
    m_pBodyOrigin->PutOrigin(m_pOrigin.get());
    m_pBodyOrigin->RelMove(0, 0);

    // --- m_pFaceOrigin (child of m_pBodyOrigin) ---
    m_pFaceOrigin = std::make_shared<Gr2DVector>();
    m_pFaceOrigin->PutOrigin(m_pBodyOrigin.get());

    // --- m_pMuzzleOrigin (child of m_pBodyOrigin) ---
    m_pMuzzleOrigin = std::make_shared<Gr2DVector>();
    m_pMuzzleOrigin->PutOrigin(m_pBodyOrigin.get());

    // --- m_pHandOrigin (child of m_pBodyOrigin) ---
    m_pHandOrigin = std::make_shared<Gr2DVector>();
    m_pHandOrigin->PutOrigin(m_pBodyOrigin.get());

    // --- m_pTailOrigin (child of m_pBodyOrigin) ---
    m_pTailOrigin = std::make_shared<Gr2DVector>();
    m_pTailOrigin->PutOrigin(m_pBodyOrigin.get());

    // --- m_pTMNavelOrigin (child of m_pOrigin) ---
    m_pTMNavelOrigin = std::make_shared<Gr2DVector>();
    m_pTMNavelOrigin->PutOrigin(m_pOrigin.get());

    // --- m_pTMHeadOrigin (child of m_pOrigin) ---
    m_pTMHeadOrigin = std::make_shared<Gr2DVector>();
    m_pTMHeadOrigin->PutOrigin(m_pOrigin.get());

    // --- m_pTMMuzzleOrigin (child of m_pOrigin) ---
    m_pTMMuzzleOrigin = std::make_shared<Gr2DVector>();
    m_pTMMuzzleOrigin->PutOrigin(m_pOrigin.get());

    // --- m_pCubeOrigin (child of m_pOrigin) ---
    m_pCubeOrigin = std::make_shared<Gr2DVector>();
    m_pCubeOrigin->PutOrigin(m_pOrigin.get());

    // ========== Layer creation ==========

    // --- m_pLayerUnderFace (z=0, origin=m_pBodyOrigin) ---
    m_pLayerUnderFace = gr.CreateLayer(0, 0, 0, 0, 0, nullptr, nScaleFilter);
    m_pLayerUnderFace->PutOrigin(m_pBodyOrigin.get());
    m_pLayerUnderFace->put_color(0xFFFFFFFFu);
    if (pOverlay)
    {
        m_pLayerUnderFace->put_overlay(pOverlay);
        m_pLayerUnderFace->put_z(z);
    }

    // --- m_pLayerTransparent (z=2, origin=m_pBodyOrigin, overlay=m_pLayerUnderFace) ---
    m_pLayerTransparent = gr.CreateLayer(0, 0, 0, 0, 2, nullptr, nScaleFilter);
    m_pLayerTransparent->PutOrigin(m_pBodyOrigin.get());
    m_pLayerTransparent->put_color(0xFFFFFFFFu);
    m_pLayerTransparent->put_overlay(m_pLayerUnderFace);

    // --- m_pLayerFace (z=1, origin=m_pFaceOrigin, overlay=m_pLayerUnderFace) ---
    m_pLayerFace = gr.CreateLayer(0, 0, 0, 0, 1, nullptr, nScaleFilter);
    m_pLayerFace->PutOrigin(m_pFaceOrigin.get());
    m_pLayerFace->put_overlay(m_pLayerUnderFace);
    m_pLayerFace->put_color(0xFFFFFFFFu);

    // --- m_pLayerOverFace (z=2, origin=m_pBodyOrigin, overlay=m_pLayerUnderFace) ---
    m_pLayerOverFace = gr.CreateLayer(0, 0, 0, 0, 2, nullptr, nScaleFilter);
    m_pLayerOverFace->PutOrigin(m_pBodyOrigin.get());
    m_pLayerOverFace->put_overlay(m_pLayerUnderFace);
    m_pLayerOverFace->put_color(0xFFFFFFFFu);

    // --- m_pLayerShadowPartner (z=-2, origin=m_pBodyOrigin, overlay=m_pLayerUnderFace) ---
    m_pLayerShadowPartner = gr.CreateLayer(0, 0, 0, 0, -2, nullptr, nScaleFilter);
    m_pLayerShadowPartner->PutOrigin(m_pBodyOrigin.get());
    m_pLayerShadowPartner->put_overlay(m_pLayerUnderFace);
    m_pLayerShadowPartner->put_color(0x00FFFFFFu);

    // --- Initialize shadow partner action arrays (1310 entries each, for 2 slots) ---
    // Original: ZArray<ZList<ZRef<SHADOWPARTNERACTIONFRAMEENTRY>>> with 1310 entries
    // In our code, ActionInfo doesn't have aSPAction arrays — this is handled differently.
    // TODO: Add shadow partner action frame support if needed.

    // --- m_pLayerUnderCharacter (z=-1, origin=m_pOrigin, overlay=m_pLayerUnderFace) ---
    m_pLayerUnderCharacter = gr.CreateLayer(0, 0, 0, 0, -1, nullptr, nScaleFilter);
    m_pLayerUnderCharacter->PutOrigin(m_pOrigin.get());
    m_pLayerUnderCharacter->put_overlay(m_pLayerUnderFace);
    m_pLayerUnderCharacter->put_color(0x00FFFFFFu);

    // --- m_pLayerOverCharacter (z=3, origin=m_pOrigin, overlay=m_pLayerUnderFace) ---
    m_pLayerOverCharacter = gr.CreateLayer(0, 0, 0, 0, 3, nullptr, nScaleFilter);
    m_pLayerOverCharacter->PutOrigin(m_pOrigin.get());
    m_pLayerOverCharacter->put_overlay(m_pLayerUnderFace);
    m_pLayerOverCharacter->put_color(0x00FFFFFFu);

    // --- m_pLayerJaguarCannon (z=3, origin=m_pOrigin, overlay=m_pLayerUnderFace) ---
    m_pLayerJaguarCannon = gr.CreateLayer(0, 0, 0, 0, 3, nullptr, nScaleFilter);
    m_pLayerJaguarCannon->PutOrigin(m_pOrigin.get());
    m_pLayerJaguarCannon->put_overlay(m_pLayerUnderFace);
    m_pLayerJaguarCannon->put_color(0x00FFFFFFu);

    // --- m_pLayerOverlay (z=0, overlay=m_pLayerUnderFace, no scale flag) ---
    m_pLayerOverlay = gr.CreateLayer(0, 0, 0, 0, 0, nullptr, 0);
    m_pLayerOverlay->put_overlay(m_pLayerUnderFace);

    // ========== Position snapshot ==========

    std::int32_t px = 0, py = 0;
    if (pOrigin)
        pOrigin->GetSnapshot(&px, &py, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    m_ptPosPrev.x.Put(px);
    m_ptPosPrev.y.Put(py);
    m_ptPos.x.Put(m_ptPosPrev.x.Get());
    m_ptPos.y.Put(m_ptPosPrev.y.Get());

    // ========== Clear wing/tail layers ==========

    m_pLayerDefaultWing.reset();
    m_pLayerKaiserWing.reset();
    m_pLayerKaiserTail.reset();

    // ========== Reset final state ==========

    std::memset(m_aOnlyAvatarHairEquipForced.data(), 0xFF,
                m_aOnlyAvatarHairEquipForced.size() * sizeof(std::int32_t));
    m_ptForcedMove = {0, 0};
    m_nBattlePvPPAvatar = 0;
}

// ============================================================================
// NotifyAvatarModified
// ============================================================================

void Avatar::NotifyAvatarModified(bool bResetAction)
{
    OnAvatarModified();

    if (bResetAction)
        PrepareActionLayer(6, 120, 0, 0);
}

// Invalid/sentinel action (used when mechanic mode suppresses an action)
inline constexpr auto kActionInvalid = static_cast<CharacterAction>(-1);

// Wild Hunter jaguar riding vehicle IDs (from RIDING_WILDHUNTER_JAGUAR .rdata)
inline constexpr std::array<std::int32_t, 9> kRidingWildHunterJaguar = {
    1932015, 1932030, 1932031, 1932032, 1932033,
    1932036, 1932100, 1932149, 1932215,
};

// ============================================================================
// Avatar morph checks
// ============================================================================

bool Avatar::IsMonsterMorphed() const
{
    // TODO: implement full monster morph check
    return false;
}

bool Avatar::IsSuperMan() const
{
    return m_dwMorphTemplateID && MorphTemplate::IsSuperMan(m_dwMorphTemplateID);
}

bool Avatar::IsIceKnight() const
{
    return m_dwMorphTemplateID && MorphTemplate::IsIceKnight(m_dwMorphTemplateID);
}

bool Avatar::IsKaiserDragon() const
{
    // TODO: implement
    return false;
}

// ============================================================================
// Sub-dispatcher stubs
// ============================================================================

auto Avatar::MoveAction2RawActionForMonsterJob(
    MoveActionType /*nAction*/,
    bool /*bRandom*/
) const -> CharacterAction
{
    // TODO: implement for jobs 13000/13100
    return CharacterAction::Walk1;
}

auto Avatar::MoveAction2RawActionForBattlePvP(
    MoveActionType /*nAction*/,
    std::int32_t /*nBattlePvPAvatar*/
) const -> CharacterAction
{
    // TODO: implement BattlePvP action mapping
    return CharacterAction::Walk1;
}

// ============================================================================
// GetOneTimeAction (from decompiled CAvatar::GetOneTimeAction)
// Remaps m_nOneTimeAction based on morph/mechanic/riding state.
// ============================================================================

namespace
{

/// Shared remap table used by both SuperMan and KaiserDragon morphs.
/// Returns the remapped action, or Walk1 if no mapping found (caller checks).
auto RemapMorphAction(CharacterAction act) -> CharacterAction
{
    using CA = CharacterAction;
    switch (act)
    {
    case CA::Swingt1:                                   return CA::Stabo1;
    case CA::Swingt3:                                   return CA::Stabo2;
    case CA::Stabo1:                                    return CA::Stabtf;
    case CA::Stabo2:                                    return CA::Shoot1;
    case CA::Shoot1:                                    return CA::Stabof;
    case CA::Pronestab:                                 return CA::Stabt2;
    case CA::Shootf:                                    return CA::Stabt1;
    case CA::Alert2:                                    return CA::Alert;
    case CA::Alert3:                                    return CA::Swingo1;
    case CA::Alert4:                                    return CA::Swingo2;
    case CA::Alert5:                                    return CA::Stand2;
    case CA::Ladder2:                                   return CA::Fly1;
    case CA::Rope2:                                     return CA::Jump;
    case CA::Somersault:                                return CA::Rope;
    case CA::Straight:                                  return CA::Dead;
    case CA::Eburster:                                  return CA::Prone;
    case CA::Backspin:                                  return CA::RuneAttack;
    case CA::Eorb:                                      return CA::Swingd1;
    case CA::Screw:                                     return CA::Blink;
    case CA::Doubleupper:                               return CA::Rune;
    case CA::Dragonstrike:                              return CA::Stabd1;
    case CA::Doublefire:                                return CA::Ladder;
    case CA::Edrain:                                    return CA::Magicheal;
    case CA::Recovery:                                  return CA::Swingdb1;
    case CA::Fist:                                      return CA::Swingd2;
    case CA::Timeleap:                                  return CA::Pronestab;
    case CA::Wave:                                      return CA::Swingdb2;
    case CA::StormBreak:                                return CA::Sit;
    case CA::ArrowRain:                                 return CA::Swingc1;
    default:                                            return CA::Walk1;  // sentinel: no match
    }
}

} // anonymous namespace

// ============================================================================
// GetCurrentAction (from decompiled CAvatar::GetCurrentAction)
// ============================================================================

auto Avatar::GetCurrentAction(std::int32_t* pnDir, bool bRandom) const
    -> CharacterAction
{
    auto nAction = MoveAction2RawAction(m_nMoveAction, pnDir, bRandom);

    // Forced move overrides with IonThruster (519)
    if (m_ptForcedMove.x || m_ptForcedMove.y)
        nAction = CharacterAction::IonThruster;

    // Monster jobs (13000/13100): use m_nChangeMoveAction if set
    auto nJob = m_avatarLook.nJob;
    if ((nJob == 13000 || nJob == 13100) && !bRandom && m_nChangeMoveAction != -1)
        nAction = static_cast<CharacterAction>(m_nChangeMoveAction);

    auto nOneTime = GetOneTimeAction();
    if (static_cast<std::int32_t>(nOneTime) >= 0)
        return nOneTime;

    return nAction;
}

// ============================================================================
// SetVisibleMan (from decompiled CAvatar::SetVisibleMan)
// Sets color on all avatar layers to show/hide the character.
// ============================================================================

void Avatar::SetVisibleMan(bool bVisible)
{
    auto color = bVisible ? 0xFFFFFFFFu : 0x00FFFFFFu;

    if (m_pLayerUnderFace)
        m_pLayerUnderFace->put_color(color);
    if (m_pLayerOverFace)
        m_pLayerOverFace->put_color(color);
    if (m_pLayerFace)
        m_pLayerFace->put_color(color);
    if (m_pLayerDefaultWing)
        m_pLayerDefaultWing->put_color(color);
    if (m_pLayerKaiserWing)
        m_pLayerKaiserWing->put_color(color);
    if (m_pLayerKaiserTail)
        m_pLayerKaiserTail->put_color(color);
}

// ============================================================================
// IsGroupEffectON (from decompiled CAvatar::IsGroupEffectON)
// Returns true when the avatar is standing idle (for group effects).
// ============================================================================

auto Avatar::IsGroupEffectON() const -> bool
{
    if (m_tAlertRemain > 0)
        return false;
    if (m_dwMorphTemplateID)
        return false;
    if (m_nRidingVehicleID > 0)
        return false;

    auto nAction = GetCurrentAction(nullptr, false);
    return nAction >= CharacterAction::Stand1 && nAction <= CharacterAction::Stand2;
}

// ============================================================================
// SetMoveAction (from decompiled CAvatar::SetMoveAction)
// ============================================================================

// Additional resistance riding vehicle IDs not in the jaguar array
inline constexpr std::int32_t kResistanceRiding1 = 1932051;
inline constexpr std::int32_t kResistanceRiding2 = 1932085;

void Avatar::SetMoveAction(std::int32_t nMA, std::int32_t bReload)
{
    // Check if riding a Wild Hunter jaguar or resistance riding vehicle
    bool bRidingJaguar = std::find(
        kRidingWildHunterJaguar.begin(),
        kRidingWildHunterJaguar.end(),
        m_nRidingVehicleID
    ) != kRidingWildHunterJaguar.end();

    if (bRidingJaguar
        || m_nRidingVehicleID == kResistanceRiding1
        || m_nRidingVehicleID == kResistanceRiding2)
    {
        SetResistanceRidingMoveAction(nMA, bReload);
    }
    else
    {
        if (!bReload && nMA == m_nMoveAction)
            return;

        m_nMoveAction = nMA;

        // (nMA & ~1) == 0x12 → MoveActionType::Dead (9 << 1 = 18 = 0x12)
        if ((nMA & ~1) == 0x12)
            ResetOneTimeAction();

        // If now in dead state, clear special morphs (1200/1201)
        if ((m_nMoveAction & ~1) == 0x12)
        {
            if (m_dwMorphTemplateID == 1200 || m_dwMorphTemplateID == 1201)
            {
                m_dwMorphTemplateID = 0;
                PrepareFaceLayer(-1);
            }
        }

        if (static_cast<std::int32_t>(GetOneTimeAction()) <= -1)
            PrepareActionLayer(6, 120, 0, 0);
    }

    // --- Emotion clear on prone/pronestab ---
    auto nCurrentAction = static_cast<std::int32_t>(GetCurrentAction(nullptr, false));

    if (m_dwMorphTemplateID)
    {
        // Morphed: Swingof(8) = morphed prone, Stabt2(20) = morphed pronestab
        if (nCurrentAction == static_cast<std::int32_t>(CharacterAction::Swingof)
            || nCurrentAction == static_cast<std::int32_t>(CharacterAction::Stabt2))
        {
            if (m_nEmotion == 8)
                SetEmotion(0, -1);
        }
    }
    else
    {
        // Non-morphed: Prone(25) or any pronestab action
        if (nCurrentAction == static_cast<std::int32_t>(CharacterAction::Prone)
            || is_pronestab_action(nCurrentAction))
        {
            if (m_nEmotion == 8)
                SetEmotion(0, -1);
        }
    }

    // Remove group effect if not standing idle
    if (!IsGroupEffectON())
        RemoveGroupEffect();
}

// ============================================================================
// PrepareActionLayer (from decompiled CAvatar::PrepareActionLayer)
//
// The main entry point for preparing all rendering layers for the current
// avatar action. Handles morph/riding dispatch, frame loading, delay
// calculation, canvas layer insertion, origin vector setup, and flip.
// ============================================================================

void Avatar::PrepareActionLayer(
    std::int32_t nActionSpeed,
    std::int32_t nWalkSpeed,
    std::int32_t bKeyDown,
    std::int32_t nGatherToolID)
{
    // --- Elf ear state ---
    bool bDrawElfEar = m_avatarLook.bDrawElfEar;
    if (m_bForcingAppearance)
        bDrawElfEar = m_bDrawElfEarForced;

    // Clamp walk speed to minimum 70
    if (nWalkSpeed <= 70)
        nWalkSpeed = 70;

    // If delayed load, clear and return
    if (m_bDelayedLoad)
    {
        ClearActionLayer(0);
        return;
    }

    // --- Pinkbean job action remapping (13000/13100) ---
    auto nJob = m_avatarLook.nJob;
    m_nSaveOneTimeActionForPinkbean = -1;

    if (nJob == 13000 || nJob == 13100)
    {
        auto nOneTime = static_cast<std::int32_t>(m_nOneTimeAction);
        // If one-time action is valid and NOT a PB action (981..1050)
        if (nOneTime >= 0
            && static_cast<std::uint32_t>(nOneTime - 981) > 0x45u)
        {
            m_nSaveOneTimeActionForPinkbean = nOneTime;
            m_nOneTimeAction = static_cast<CharacterAction>(
                get_change_pinkbean_action(nOneTime));
        }
    }

    // If NOT a PB job but has PB-range one-time action, clear it
    if (nJob != 13000 && nJob != 13100
        && static_cast<std::uint32_t>(
               static_cast<std::int32_t>(m_nOneTimeAction) - 981) <= 0x45u)
    {
        m_nOneTimeAction = static_cast<CharacterAction>(-1);
    }

    // --- Determine current action ---
    std::int32_t nDir = 0;
    auto nMoveRawAction = static_cast<std::int32_t>(
        MoveAction2RawAction(m_nMoveAction, &nDir, true));

    if (m_ptForcedMove.x || m_ptForcedMove.y)
        nMoveRawAction = static_cast<std::int32_t>(CharacterAction::IonThruster);

    m_nChangeMoveAction = -1;

    std::int32_t nAction;
    if (static_cast<std::int32_t>(GetOneTimeAction()) <= -1)
    {
        nAction = nMoveRawAction;
    }
    else
    {
        nAction = static_cast<std::int32_t>(GetOneTimeAction());
    }

    // --- Clear riding for certain actions ---
    // Dead(32), Pvpko(295), Pvpko2(296), BattlePvP dead actions,
    // PBdead(1013), PBdead2(1014), setitem3/4(856,857), dance actions
    if (nAction == 32 || nAction == 295 || nAction == 296
        || is_battle_pvp_dead_action(nAction)
        || nAction == 1013 || nAction == 1014
        || (static_cast<std::uint32_t>(nAction - 856) <= 1u)
        || is_dance_action(static_cast<CharacterAction>(nAction)))
    {
        bool bWasMechanic = (m_nRidingVehicleID == 1932016);
        m_nRidingVehicleID = 0;
        if (bWasMechanic)
            SetMechanicHUE(0, 1);
        m_nRidingChairID = 0;
    }

    // Pvpko/Pvpko2 clear morph
    if (nAction == 295 || nAction == 296)
        m_dwMorphTemplateID = 0;

    // --- Validate action range ---
    if (nAction >= 0)
    {
        if (m_dwMorphTemplateID)
        {
            // Morphed: only actions < 72 are valid for morph (the morph remapping range)
            if (nAction >= 72)
            {
                // Check if KaiserDragon — special case falls through to action 1
                if (!MorphTemplate::IsKaiserDragon(m_dwMorphTemplateID))
                    return;
                nAction = 1; // Stand2
            }
        }
        else if (nAction >= static_cast<std::int32_t>(ACTIONDATA_COUNT))
        {
            return;
        }
    }
    else
    {
        if (!m_dwMorphTemplateID || !MorphTemplate::IsKaiserDragon(m_dwMorphTemplateID))
            return;
        nAction = 1;
    }

    // --- Select action info slot ---
    // Slot 0 for move actions, slot 1 for one-time actions
    auto& ai = m_aiAction[static_cast<std::int32_t>(GetOneTimeAction()) > -1 ? 1 : 0];

    // --- Morph dispatch ---
    if (m_dwMorphTemplateID)
    {
        PrepareMorphActionLayer(nAction, nDir, nActionSpeed, bKeyDown);
        return;
    }

    // --- Kaiser hue handling ---
    if (m_nKaiserMorphRotateHueExtern > 0
        || m_nKaiserMorphRotateHueInnner > 0
        || m_bKaiserMorphPrimiumBlack)
    {
        if (m_pLayerUnderFace)
        {
            // Set blend mode and color channels for Kaiser morph hue
            m_pLayerUnderFace->put_blend(32);
            // Reset color channels
        }
    }

    // --- Riding dispatch ---
    if (IsRidingEx())
    {
        PrepareCharacterActionLayer(nActionSpeed, nWalkSpeed, bKeyDown, nGatherToolID);
        auto nVehicle = m_nRidingVehicleID;
        auto nOneTimeForRiding = GetOneTimeAction();
        if (IsAbleTamingMobOneTimeAction(nOneTimeForRiding, nVehicle))
            m_nTamingMobOneTimeAction = static_cast<std::int32_t>(nOneTimeForRiding);
        PrepareTamingMobActionLayer(nActionSpeed, nWalkSpeed, bKeyDown);
        PrepareJaguarCannonLayer();
        FixCharacterPosition();
        return;
    }

    // --- Check if frame data is already cached ---
    bool bCached;
    if (is_vehicle(m_nRidingVehicleID))
        bCached = ai.HasTamingMobAction(nAction);
    else
        bCached = ai.HasAction(nAction);

    if (!bCached)
    {
        // --- Prepare parameters for loading ---
        std::int32_t aAvatarHairEquip[32]{};
        GetModifiedAvatarHairEquip(aAvatarHairEquip);

        // Handle forced appearance
        if (m_bForcingAppearance && m_nAvatarFaceForced)
            m_avatarLook.nFace = m_nAvatarFaceForced;

        auto nSkin = m_avatarLook.nSkin;
        if (m_bForcingAppearance && m_nAvatarSkinForced > -1)
            nSkin = m_nAvatarSkinForced;

        // Weapon sticker
        auto nWeaponStickerID = m_avatarLook.nWeaponStickerID;
        if (m_nForcedMoveAction != -1 && m_nForcedMoveAction == nAction)
            nWeaponStickerID = 1702224;
        if (m_nForcedStandAction != -1 && m_nForcedStandAction == nAction)
            nWeaponStickerID = 1702224;
        if (m_bForcingAppearance
            && get_weapon_type(m_aOnlyAvatarHairEquipForced[11]) != 0)
            nWeaponStickerID = 0;

        auto nMixedHairColor = m_avatarLook.nMixedHairColor;
        auto nMixHairPercent = m_avatarLook.nMixHairPercent;

        if (m_bForcingAppearance && GetRolePlayingCharacterIndex() >= 3)
        {
            nWeaponStickerID = 0;
            nMixedHairColor = 0;
            nMixHairPercent = 0;
        }

        // Custom riding set
        auto aCustomRiding = m_aCustomRiding;
        LoadCustomRidingSet(m_nRidingVehicleID, aCustomRiding);

        auto nRidingForLoad = m_bSitAction ? 0 : m_nRidingVehicleID;

        // --- Load character action frames ---
        // TODO: Call CActionMan::LoadCharacterAction with full parameters
        // For now, create empty entries so the cache has something
        if (ai.aaAction.find(nAction) == ai.aaAction.end())
            ai.aaAction[nAction] = {};

        // --- Load taming mob action frames ---
        auto nLoadAction = (m_nSaveOneTimeActionForPinkbean != -1)
            ? m_nSaveOneTimeActionForPinkbean : nAction;
        // TODO: Call CActionMan::LoadTamingMobAction
        if (ai.aaTamingMobAction.find(nAction) == ai.aaTamingMobAction.end())
            ai.aaTamingMobAction[nAction] = {};

        (void)nSkin;
        (void)nWeaponStickerID;
        (void)nRidingForLoad;
        (void)nMixedHairColor;
        (void)nMixHairPercent;
        (void)nLoadAction;
        (void)nGatherToolID;
        (void)bDrawElfEar;
    }

    // --- Get action data ---
    const auto* pActionData = (nAction >= 0
        && static_cast<std::size_t>(nAction) < ACTIONDATA_COUNT)
        ? &s_aCharacterActionData[static_cast<std::size_t>(nAction)]
        : nullptr;

    auto nRepeatFrame = (pActionData && pActionData->m_nRepeatFrame > 0)
        ? pActionData->m_nRepeatFrame : 0;

    // --- Determine frame count ---
    std::int32_t nFrameCount = 0;
    const auto& charFrames = ai.aaAction[nAction];
    const auto& tmobFrames = ai.aaTamingMobAction[nAction];

    if (!is_vehicle(m_nRidingVehicleID) || m_bSitAction)
        nFrameCount = static_cast<std::int32_t>(charFrames.size());
    else
        nFrameCount = static_cast<std::int32_t>(tmobFrames.size());

    // --- Calculate frame delays ---
    ai.aFrameDelay.assign(static_cast<std::size_t>(nFrameCount), 0);
    ai.tTotFrameDelay = 0;

    if (nFrameCount == 0)
    {
        LOG_WARN("PrepareActionLayer: Invalid action {} (frame count 0, riding={}, sit={})",
                 nAction, is_vehicle(m_nRidingVehicleID), m_bSitAction);
    }

    if (is_walk_action(nAction))
    {
        // Walk actions: delay = 100 * tDelay / nWalkSpeed
        for (std::int32_t i = 0; i < nFrameCount; ++i)
        {
            std::int32_t tDelay = 150;
            if (!is_vehicle(m_nRidingVehicleID) || m_bSitAction)
            {
                if (i < static_cast<std::int32_t>(charFrames.size()) && charFrames[static_cast<std::size_t>(i)])
                    tDelay = charFrames[static_cast<std::size_t>(i)]->tDelay;
            }
            else
            {
                if (i < static_cast<std::int32_t>(tmobFrames.size()) && tmobFrames[static_cast<std::size_t>(i)])
                    tDelay = tmobFrames[static_cast<std::size_t>(i)]->tDelay;
            }
            ai.aFrameDelay[static_cast<std::size_t>(i)] = 100 * tDelay / nWalkSpeed;
            ai.tTotFrameDelay += ai.aFrameDelay[static_cast<std::size_t>(i)];
        }
        m_tWalkDelay = ai.tTotFrameDelay;
    }
    else
    {
        // Clamp action speed to [2, 10]
        if (nActionSpeed < 2)
            nActionSpeed = 2;
        else if (nActionSpeed > 10)
            nActionSpeed = 10;

        for (std::int32_t i = 0; i < nFrameCount; ++i)
        {
            std::int32_t tDelay = 150;
            if (is_vehicle(m_nRidingVehicleID) && !m_bSitAction)
            {
                if (i < static_cast<std::int32_t>(tmobFrames.size()) && tmobFrames[static_cast<std::size_t>(i)])
                    tDelay = tmobFrames[static_cast<std::size_t>(i)]->tDelay;
            }
            else
            {
                if (i < static_cast<std::int32_t>(charFrames.size()) && charFrames[static_cast<std::size_t>(i)])
                    tDelay = charFrames[static_cast<std::size_t>(i)]->tDelay;
            }

            // Apply forced additional delay rate (BattlePvP)
            std::int32_t nAdjusted = 0;
            if (m_dForcedAddActionDelayRate != 0.0L)
            {
                if (tDelay != 0)
                    nAdjusted = static_cast<std::int32_t>(
                        static_cast<double>(tDelay) / 100.0
                        * static_cast<double>(m_dForcedAddActionDelayRate));
                nAdjusted += tDelay;
            }

            // BattlePvP range (1051..1115) — use raw delay unless basic attack
            if (static_cast<std::uint32_t>(nAction - 1051) <= 0x64u
                && !is_battle_pvp_basic_attack_action(nAction))
            {
                nAdjusted = tDelay;
            }

            // Compute final delay
            std::int32_t nFinalDelay;
            if (nAdjusted != 0)
                nFinalDelay = nAdjusted;
            else
                nFinalDelay = tDelay * (nActionSpeed + 10) / 16;

            ai.aFrameDelay[static_cast<std::size_t>(i)] = nFinalDelay;
            ai.tTotFrameDelay += nFinalDelay;

            // Keydown hold: set infinite delay for held shoot frames
            if (bKeyDown && is_shoot_action(nAction)
                && IsActionHold(nAction, i))
            {
                ai.aFrameDelay[static_cast<std::size_t>(i)] = 0x7FFFFFFF;
            }
        }
    }

    // --- Remove canvases from all layers ---
    if (m_pLayerUnderFace)
        m_pLayerUnderFace->RemoveAllCanvases();
    if (m_pLayerOverFace)
        m_pLayerOverFace->RemoveAllCanvases();
    if (m_pLayerUnderCharacter)
        m_pLayerUnderCharacter->RemoveAllCanvases();
    if (m_pLayerOverCharacter)
        m_pLayerOverCharacter->RemoveAllCanvases();
    if (m_pLayerJaguarCannon)
        m_pLayerJaguarCannon->RemoveAllCanvases();

    // --- Initialize frame playback state ---
    ai.nCurFrameIndex = 0;
    ai.tCurFrameRemain = 0;
    if (nFrameCount > 0 && !ai.aFrameDelay.empty())
        ai.tCurFrameRemain = ai.aFrameDelay[0];

    // --- Insert canvases into layers ---
    if (is_vehicle(m_nRidingVehicleID) && !m_bSitAction)
    {
        // Riding: insert taming mob canvases into under/over character layers
        for (std::int32_t i = 0; i < static_cast<std::int32_t>(tmobFrames.size()); ++i)
        {
            const auto& frame = tmobFrames[static_cast<std::size_t>(i)];
            if (!frame)
                continue;

            if (m_pLayerUnderCharacter && frame->pCanvasUnderCharacter)
            {
                ApplyScaleAndOffset(m_pLayerUnderCharacter, frame->pCanvasUnderCharacter, i);
                m_pLayerUnderCharacter->InsertCanvas(frame->pCanvasUnderCharacter, 100);
            }
            if (m_pLayerOverCharacter && frame->pCanvasOverCharacter)
            {
                ApplyScaleAndOffset(m_pLayerOverCharacter, frame->pCanvasOverCharacter, i);
                m_pLayerOverCharacter->InsertCanvas(frame->pCanvasOverCharacter, 100);
            }
        }

        // Also insert character canvases into under/over face layers
        for (std::int32_t i = 0; i < static_cast<std::int32_t>(charFrames.size()); ++i)
        {
            const auto& frame = charFrames[static_cast<std::size_t>(i)];
            if (!frame)
                continue;

            if (m_pLayerUnderFace && frame->pCanvasUnderFace)
            {
                ApplyScaleAndOffset(m_pLayerUnderFace, frame->pCanvasUnderFace, i);
                m_pLayerUnderFace->InsertCanvas(frame->pCanvasUnderFace, 100);
            }
            if (m_pLayerOverFace && frame->pCanvasOverFace)
            {
                ApplyScaleAndOffset(m_pLayerOverFace, frame->pCanvasOverFace, i);
                m_pLayerOverFace->InsertCanvas(frame->pCanvasOverFace, 100);
            }
        }

        // Set alpha on under/over character layers
        auto nAlpha = ai.aAlpha.count(nAction) ? ai.aAlpha[nAction] : 255;
        if (m_pLayerUnderCharacter)
        {
            auto* pAlphaVec = m_pLayerUnderCharacter->get_alpha();
            if (pAlphaVec)
                pAlphaVec->RelMove(nAlpha, 0);
        }
        if (m_pLayerOverCharacter)
        {
            auto* pAlphaVec = m_pLayerOverCharacter->get_alpha();
            if (pAlphaVec)
                pAlphaVec->RelMove(nAlpha, 0);
        }
    }
    else
    {
        // Not riding: check if under-character has non-zero alpha color
        if (m_pLayerUnderCharacter)
        {
            auto color = m_pLayerUnderCharacter->get_color();
            if ((color & 0xFF000000u) != 0)
            {
                // Set color to transparent for under/over character
                m_pLayerUnderCharacter->put_color(0x00FFFFFF);
                if (m_pLayerOverCharacter)
                    m_pLayerOverCharacter->put_color(0x00FFFFFF);
            }
        }

        // Insert character canvases into under/over face layers
        for (std::int32_t i = 0; i < static_cast<std::int32_t>(charFrames.size()); ++i)
        {
            const auto& frame = charFrames[static_cast<std::size_t>(i)];
            if (!frame)
                continue;

            if (m_pLayerUnderFace && frame->pCanvasUnderFace)
            {
                ApplyScaleAndOffset(m_pLayerUnderFace, frame->pCanvasUnderFace, i);
                m_pLayerUnderFace->InsertCanvas(frame->pCanvasUnderFace, 100);
            }
            if (m_pLayerOverFace && frame->pCanvasOverFace)
            {
                ApplyScaleAndOffset(m_pLayerOverFace, frame->pCanvasOverFace, i);
                m_pLayerOverFace->InsertCanvas(frame->pCanvasOverFace, 100);
            }
        }
    }

    // --- Setup origin vectors ---
    // Get current frame entry for origin point extraction
    const CharacterActionFrameEntry* pCurFrame = nullptr;
    if (!charFrames.empty() && ai.nCurFrameIndex >= 0
        && static_cast<std::size_t>(ai.nCurFrameIndex) < charFrames.size())
    {
        pCurFrame = charFrames[static_cast<std::size_t>(ai.nCurFrameIndex)].get();
    }

    // Get current taming mob frame entry
    const TamingMobActionFrameEntry* pCurTMFrame = nullptr;
    if (!tmobFrames.empty() && ai.nCurFrameIndex >= 0
        && static_cast<std::size_t>(ai.nCurFrameIndex) < tmobFrames.size())
    {
        pCurTMFrame = tmobFrames[static_cast<std::size_t>(ai.nCurFrameIndex)].get();
    }

    // Determine direction/flip
    m_bFlip = (nDir != 0);

    // Face origin
    if (m_pFaceOrigin && pCurFrame)
    {
        auto fx = m_bFlip ? -pCurFrame->ptBrow.x : pCurFrame->ptBrow.x;
        m_pFaceOrigin->RelMove(fx, pCurFrame->ptBrow.y);
    }

    // Body origin
    if (m_pBodyOrigin && pCurFrame)
    {
        auto bx = m_bFlip ? -pCurFrame->ptNavel.x : pCurFrame->ptNavel.x;
        m_pBodyOrigin->RelMove(bx, pCurFrame->ptNavel.y);
    }

    // Update body rel move
    if (pCurFrame)
    {
        auto brx = m_bFlip ? -pCurFrame->ptNavel.x : pCurFrame->ptNavel.x;
        m_ptBodyRelMove.x = brx;
        m_ptBodyRelMove.y = pCurFrame->ptNavel.y;
    }

    // Muzzle layer flip
    if (!is_vehicle(m_nRidingVehicleID) && m_pLayerMuzzle)
        m_pLayerMuzzle->put_flip(m_bFlip ? 1 : 0);

    // Muzzle origin
    if (m_pMuzzleOrigin && pCurFrame)
    {
        auto mx = m_bFlip ? -pCurFrame->ptMuzzle.x : pCurFrame->ptMuzzle.x;
        m_pMuzzleOrigin->RelMove(mx, pCurFrame->ptMuzzle.y);
    }

    // Hand origin
    if (m_pHandOrigin && pCurFrame)
    {
        auto hx = m_bFlip ? -pCurFrame->ptHand.x : pCurFrame->ptHand.x;
        m_pHandOrigin->RelMove(hx, pCurFrame->ptHand.y);
    }

    // Tail origin
    if (m_pTailOrigin && pCurFrame)
    {
        auto tx = m_bFlip ? -pCurFrame->ptTail.x : pCurFrame->ptTail.x;
        m_pTailOrigin->RelMove(tx, pCurFrame->ptTail.y);
    }

    // Taming mob origins (riding vehicle)
    if (is_vehicle(m_nRidingVehicleID) && pCurTMFrame)
    {
        // TM navel origin
        if (m_pTMNavelOrigin)
        {
            auto nx = m_bFlip ? -pCurTMFrame->ptNavel.x : pCurTMFrame->ptNavel.x;
            m_pTMNavelOrigin->RelMove(nx, pCurTMFrame->ptNavel.y);
        }

        // TM head origin
        if (m_pTMHeadOrigin)
        {
            auto hx = m_bFlip ? -pCurTMFrame->ptHead.x : pCurTMFrame->ptHead.x;
            m_pTMHeadOrigin->RelMove(hx, pCurTMFrame->ptHead.y);
        }

        // TM muzzle origin
        if (m_pTMMuzzleOrigin)
        {
            auto mx = m_bFlip ? -pCurTMFrame->ptMuzzle.x : pCurTMFrame->ptMuzzle.x;
            m_pTMMuzzleOrigin->RelMove(mx, pCurTMFrame->ptMuzzle.y);
        }
    }

    // --- Riding emotion ---
    // TODO: Check IsEmotionRiding via CItemInfo

    // --- Final origin move (identity) ---
    if (m_pOrigin)
        m_pOrigin->RelMove(0, 0);

    // --- Mechanic rocket booster handling ---
    if (m_nRidingVehicleID == 1932016)
    {
        auto nMechMode = m_nMechanicMode ? m_nMechanicMode : m_nPrevMechanicMode;
        if (nMechMode == 35111003)
        {
            // Actions that need rocket booster: Ladder(30-31), Rope(67-68), tank actions(975-976)
            bool bNeedsRocket = false;
            if ((nAction >= 30 && nAction <= 31)
                || (nAction >= 67 && nAction <= 68)
                || (nAction >= 975 && nAction <= 976))
            {
                bNeedsRocket = true;
            }

            if (bNeedsRocket)
            {
                LoadMechanicRocket();
            }
            else
            {
                // Clear rocket booster layer
                m_pLayerRocketBooster.reset();
            }
        }
    }

    // --- Monster job (13000/13100) repeat frame ---
    if ((m_avatarLook.nJob == 13000 || m_avatarLook.nJob == 13100)
        && nAction == 1013)
    {
        ai.nRepeatFrame = nRepeatFrame;
    }
    else
    {
        ai.nRepeatFrame = 0;
    }

    // --- Group effect check ---
    if (!IsGroupEffectON())
        RemoveGroupEffect();

    // --- BattlePvP invisible ---
    if (m_dwBattlePvPInvisibleAction)
        SetVisibleMan(false);
}

// ============================================================================
// Stubs
// ============================================================================

void Avatar::SetResistanceRidingMoveAction(
    [[maybe_unused]] std::int32_t nMA,
    [[maybe_unused]] std::int32_t bReload)
{
    // TODO: implement resistance riding move action handling
}

// ============================================================================
// is_trans_faceacc (from decompiled is_trans_faceacc @ 0x60e330)
// Returns true if the face accessory is transparent (DemonSlayer/Xenon).
// ============================================================================

namespace
{

auto is_trans_faceacc(std::int32_t nItemID) -> bool
{
    return nItemID == 1012104 || nItemID == 1012289;
}

/// Stub for CWvsContext::m_bInGameDirectionMode check.
auto is_in_game_direction_mode() -> bool
{
    // TODO: implement when CWvsContext is available
    return false;
}

} // anonymous namespace

// ============================================================================
// RegisterNextBlink (from decompiled CAvatar::RegisterNextBlink @ 0x5e7900)
// Schedules the next eye blink at a random time 2000-5000ms from now.
// ============================================================================

void Avatar::RegisterNextBlink()
{
    m_bBlinking = false;
    auto delay = std::rand() % 3000;
    m_tNextBlink = static_cast<std::int32_t>(
        Application::GetInstance().GetUpdateTime()) + delay + 2000;
}

// ============================================================================
// PrepareFaceLayer (from decompiled CAvatar::PrepareFaceLayer @ 0x5f4200)
//
// Loads and inserts face/emotion canvases into the face rendering layer.
// Handles Larkness face overrides, Albatross face recoloring, FaceOff,
// forced appearance, DemonSlayer/Xenon default face accessories,
// and animation mode (repeat/emotion timer/blink).
// ============================================================================

void Avatar::PrepareFaceLayer(std::int32_t tDuration)
{
    if (m_bDelayedLoad)
        return;

    auto nJob = m_avatarLook.nJob;
    auto nFace = m_avatarLook.nFace;
    auto face = nFace;

    // --- Larkness (light/dark) face override ---
    if (nJob / 100 == 27 || nJob == 2004)
    {
        auto state = m_nLarknessState;
        if (state == 20040217)
        {
            face = (m_avatarLook.nGender != 0) ? 21066 : 20071;
        }
        else if (state >= 20040219 && state <= 20040220)
        {
            face = (m_avatarLook.nGender != 0) ? 21067 : 20072;
        }
    }

    // --- Albatross face color override ---
    if (m_pAlbatrossInfo)
    {
        if (m_pAlbatrossInfo->bApplied && m_pAlbatrossInfo->nFaceColor >= 0)
        {
            // Strip the color digit (hundreds place) to get the base face
            auto stripped = nFace + 100 * (10 * (nFace / 1000) - nFace / 100);
            if (stripped == 20089 || stripped == 21086)
            {
                auto recolored = stripped + 100 * m_pAlbatrossInfo->nFaceColor;
                if (ActionMan::GetInstance().IsGettableImgEntry(recolored))
                    face = recolored;
            }
        }
    }

    // --- FaceOff override (in game direction mode only) ---
    if (m_bFaceOff && is_in_game_direction_mode())
        face = m_nFaceOffItemID;

    // --- Forced appearance face override ---
    auto bForcingAppearance = m_bForcingAppearance;
    if (bForcingAppearance && m_nAvatarFaceForced)
        face = m_nAvatarFaceForced;

    // --- Skin with forced override ---
    auto nSkin = m_avatarLook.nSkin;
    if (bForcingAppearance && m_nAvatarSkinForced > -1)
        nSkin = m_nAvatarSkinForced;

    // --- Face accessory (DemonSlayer/Xenon default) ---
    auto nFaceAcc = m_avatarLook.anHairEquip[2];

    if (m_avatarLook.nDemonSlayerDefFaceAcc
        && (!nFaceAcc || is_trans_faceacc(nFaceAcc)))
    {
        nFaceAcc = m_avatarLook.nDemonSlayerDefFaceAcc;
    }

    if (m_avatarLook.nXenonDefFaceAcc
        && (!nFaceAcc || is_trans_faceacc(nFaceAcc)))
    {
        nFaceAcc = m_avatarLook.nXenonDefFaceAcc;
    }

    // --- Forced face accessory override ---
    bool bIgnoreInvisibleFace = false;
    if (bForcingAppearance)
    {
        if (m_aAvatarHairEquipForced[2] != -1)
            nFaceAcc = m_aAvatarHairEquipForced[2];

        bIgnoreInvisibleFace = (GetRolePlayingCharacterIndex() != -1);
    }

    // --- Load face look canvases ---
    std::list<std::shared_ptr<WzGr2DCanvas>> lpEmotion;
    ActionMan::GetInstance().LoadFaceLook(
        nSkin, face, m_nEmotion, nFaceAcc,
        lpEmotion, nJob, bIgnoreInvisibleFace);

    // --- Clear existing face layer canvases ---
    if (m_pLayerFace)
        m_pLayerFace->RemoveAllCanvases();

    // --- Insert emotion canvases into face layer ---
    for (const auto& pCanvas : lpEmotion)
    {
        if (!pCanvas || !m_pLayerFace)
            continue;

        auto nDelay = pCanvas->GetDelay();
        m_pLayerFace->InsertCanvas(pCanvas, nDelay);
    }

    // --- Animation mode ---
    if (face == 20071 || face == 21066 || face == 20072 || face == 21067)
    {
        // Larkness faces → loop animation
        if (m_pLayerFace)
            m_pLayerFace->Animate(Gr2DAnimationType::Repeat);
    }
    else if (m_nEmotion != 0)
    {
        // Emotion active → set emotion end timer, loop animation
        auto tNow = static_cast<std::int32_t>(
            Application::GetInstance().GetUpdateTime());
        m_tEmotionEnd = tDuration + tNow;

        if (m_pLayerFace)
            m_pLayerFace->Animate(Gr2DAnimationType::Repeat);
    }
    else
    {
        // Default → register next eye blink
        RegisterNextBlink();
    }
}

void Avatar::SetEmotion(std::int32_t nEmotion, std::int32_t nDuration)
{
    if (m_dwMorphTemplateID)
        return;
    if (m_bHideAction)
        return;

    // If this is the local user's avatar, check for Attract buff
    if (auto* pLocal = UserLocal::GetInstancePtr())
    {
        if (this == static_cast<Avatar*>(pLocal))
        {
            auto& stat = WvsContext::GetInstance().GetSecondaryStat();
            if (stat._ZtlSecureGet_nAttract_()
                && stat._ZtlSecureGet_rAttract_() == 188)
            {
                return;
            }
        }
    }

    // Block vomit emotion (8) during prone/pronestab actions
    auto nCurrentAction = GetCurrentAction(nullptr, false);
    if (nCurrentAction == CharacterAction::Prone
        || is_pronestab_action(static_cast<std::int32_t>(nCurrentAction)))
    {
        if (nEmotion == 8)
            return;
    }

    // Valid emotion range: 0–38
    if (static_cast<std::uint32_t>(nEmotion) > 0x26)
        return;

    m_nEmotion = nEmotion;

    if (m_bDelayedLoad)
        return;

    PrepareFaceLayer(nDuration);

    // Format path: "Etc/EmotionEffect/<emotion_name>"
    const auto& sName = ActionMan::GetEmotionName(nEmotion);
    auto sPath = "Etc/EmotionEffect/" + sName;

    // Play the emotion effect animation
    AnimationDisplayer::GetInstance().Effect_General(
        sPath,
        (m_nMoveAction & 1) == 0,
        m_pOrigin,
        0, 0,
        m_pLayerUnderFace,
        3, 0);
}

void Avatar::ClearActionLayer([[maybe_unused]] std::int32_t nSlot)
{
    // TODO: clear all cached frame data for the given slot
}

void Avatar::PrepareMorphActionLayer(
    [[maybe_unused]] std::int32_t nAction,
    [[maybe_unused]] std::int32_t nDir,
    [[maybe_unused]] std::int32_t nActionSpeed,
    [[maybe_unused]] std::int32_t bKeyDown)
{
    // TODO: implement morph action layer preparation
}

void Avatar::PrepareCharacterActionLayer(
    [[maybe_unused]] std::int32_t nActionSpeed,
    [[maybe_unused]] std::int32_t nWalkSpeed,
    [[maybe_unused]] std::int32_t bKeyDown,
    [[maybe_unused]] std::int32_t nGatherToolID)
{
    // TODO: implement character action layer preparation for riding
}

void Avatar::PrepareTamingMobActionLayer(
    [[maybe_unused]] std::int32_t nActionSpeed,
    [[maybe_unused]] std::int32_t nWalkSpeed,
    [[maybe_unused]] std::int32_t bKeyDown)
{
    // TODO: implement taming mob action layer preparation
}

void Avatar::PrepareJaguarCannonLayer()
{
    // TODO: implement jaguar cannon layer for Wild Hunter
}

void Avatar::FixCharacterPosition()
{
    // TODO: implement character position fix after riding layer setup
}

void Avatar::SetMechanicHUE(
    [[maybe_unused]] std::int32_t nHUE,
    [[maybe_unused]] std::int32_t bForce)
{
    // TODO: implement mechanic HUE color change
}

void Avatar::LoadMechanicRocket()
{
    // TODO: implement mechanic rocket booster layer loading
}

void Avatar::ApplyScaleAndOffset(
    [[maybe_unused]] std::shared_ptr<WzGr2DLayer>& pDstLayer,
    [[maybe_unused]] const std::shared_ptr<WzGr2DCanvas>& pSrcCanvas,
    [[maybe_unused]] std::int32_t nFrameIndex)
{
    // TODO: apply scale and offset from canvas to layer frame
}

void Avatar::GetModifiedAvatarHairEquip(std::int32_t (&aOut)[32]) const
{
    // Copy base equipment array, apply forced overrides
    for (std::size_t i = 0; i < 32; ++i)
    {
        aOut[i] = m_avatarLook.anHairEquip[i];
        if (m_bForcingAppearance && m_aAvatarHairEquipForced[i] != 0)
            aOut[i] = m_aAvatarHairEquipForced[i];
    }
}

void Avatar::LoadCustomRidingSet(
    [[maybe_unused]] std::int32_t nRidingVehicleID,
    [[maybe_unused]] std::vector<std::int32_t>& aCustomRiding)
{
    // TODO: load custom riding equipment set for the vehicle
}

bool Avatar::IsRidingEx() const
{
    // Riding-ex: has a vehicle and the vehicle IS a taming mob type
    return is_vehicle(m_nRidingVehicleID);
}

void Avatar::SetRidingEmotion(
    [[maybe_unused]] std::int32_t nVehicleID,
    [[maybe_unused]] CharacterAction nAction)
{
    // TODO: implement riding emotion from vehicle template
}

// ============================================================================
// GetOneTimeAction (from decompiled CAvatar::GetOneTimeAction)
// ============================================================================

auto Avatar::GetOneTimeAction() const -> CharacterAction
{
    using CA = CharacterAction;
    const auto act = m_nOneTimeAction;

    // --- SuperMan morph ---
    if (IsSuperMan())
    {
        // High-range actions (> ArrowRain)
        switch (act)
        {
        case CA::Shockwave:     return CA::Siege2Prone;
        case CA::Demolition:    return CA::PronestabJaguar;
        case CA::Snatch:        return CA::Alert2;
        case CA::WindSpear:     return CA::Alert3;
        case CA::WindShot:      return CA::Alert4;
        case CA::Fly2:          return CA::Magicattack2;
        case CA::Fly2Move:      return CA::Magicattackf;
        case CA::Fly2Skill:     return CA::Tired;
        default: break;
        }

        // Shared morph remap table
        auto remapped = RemapMorphAction(act);
        if (remapped != CA::Walk1)
            return remapped;
        // Fall through to LABEL_39
    }

    // --- IceKnight morph ---
    if (IsIceKnight())
    {
        switch (act)
        {
        case CA::IceKnightAttack1:  return CA::Alert5;
        case CA::IceKnightAttack2:  return CA::Alert6;
        case CA::IceKnightSmash:    return CA::Alert7;
        case CA::IceKnightJump:     return CA::Ladder2;
        case CA::IceKnightTempest:  return CA::Rope2;
        case CA::IceKnightChop:     return CA::Shoot6;
        case CA::IceKnightPanic:    return CA::Magic1;
        default: break;
        }
        // Fall through to LABEL_48
    }

    // --- Mechanic tank mode ---
    std::int32_t nMechanicMode;
    if (m_nRidingVehicleID != 1932016 || m_nMechanicMode)
        nMechanicMode = m_nMechanicMode;
    else
        nMechanicMode = m_nPrevMechanicMode;

    if (nMechanicMode == 35111003)
    {
        switch (act)
        {
        case CA::Ladder:
        case CA::Ladder2:           return CA::TankLadder;
        case CA::Rope:
        case CA::Rope2:             return CA::TankRope;
        case CA::Alert2:
        case CA::Alert3:
        case CA::Alert4:            return kActionInvalid;
        case CA::RocketBoosterStart:return CA::TankRboosterPre;
        case CA::RocketBoosterEnd:  return CA::TankRboosterAfter;
        case CA::MechanicBooster:   return CA::Siege2Stand;
        case CA::Msummon:           return CA::TankMsummon;
        case CA::Msummon2:          return CA::TankMsummon2;
        case CA::MechanicRush:      return CA::TankMrush;
        default: break;
        }
        // Fall through to LABEL_63
    }

    // --- KaiserDragon morph ---
    if (IsKaiserDragon())
    {
        // Actions that map to Stand2
        switch (act)
        {
        case CA::Alert:
        case CA::Rune:
        case CA::RuneAttack:
        case CA::Alert5:            return CA::Stand2;
        default: break;
        }

        // Swingt1/Swingt2/Swingtf all → Stabo1
        if (act == CA::Swingt2 || act == CA::Swingtf)
            return CA::Stabo1;

        // Shared morph remap table
        auto remapped = RemapMorphAction(act);
        if (remapped != CA::Walk1)
            return remapped;

        // PhantomBlow → Dead
        if (act == CA::PhantomBlow)
            return CA::Dead;

        // BombExplosion, MaxForce0, MaxForce1 → Dead
        if (act == CA::BombExplosion || act == CA::MaxForce0 || act == CA::MaxForce1)
            return CA::Dead;

        // Kaiser-specific remaps
        switch (act)
        {
        case CA::DragonUpper:       return CA::FinishattackLink2;
        case CA::Impwave:           return CA::FinishattackLink;
        case CA::BurstUp:
        case CA::RegainStr:
        case CA::Soulcharge:        return CA::Swingo1;
        case CA::DragonSlash0:      return CA::Deathblow;
        case CA::ExtraKnockBack:    return CA::Shoot3;
        case CA::ChainPulling:      return CA::Shoot5;
        case CA::FlyingSword:       return CA::Quadblow;
        case CA::WingBeat:          return CA::Shoot4;
        case CA::PrestoPassing:     return CA::Tripleblow;
        case CA::EnterTheDragon:    return CA::Magicattack1;
        case CA::Medusa:            return CA::Shootdb1;
        case CA::GigaSlasher:       return CA::Swingc2;
        case CA::DkEarthquake0:     return CA::Shootf;
        case CA::DkEarthquake1:     return CA::Shotc1;
        case CA::Prominence:        return CA::Finishblow;
        case CA::Fly2:              return CA::Magicattack2;
        case CA::Fly2Move:          return CA::Magicattackf;
        case CA::Fly2Skill:         return CA::Tired;
        default: break;
        }
        // Fall through to LABEL_89
    }

    // --- Wild Hunter jaguar riding check ---
    bool bRidingJaguar = std::find(
        kRidingWildHunterJaguar.begin(),
        kRidingWildHunterJaguar.end(),
        m_nRidingVehicleID
    ) != kRidingWildHunterJaguar.end();

    if (!bRidingJaguar)
    {
        // Not riding jaguar: remap wild hunter actions
        if (act == CA::ExtendMagazine)
            return CA::Alert2;
        if (act == CA::SilentRampage)
            return CA::Alert2;
        if (act == CA::AssistantHuntingUnit)
            return CA::WhDrillContainer;
        if (act == CA::Wildbeast || act == CA::Howling)
            return CA::Alert2;
    }

    // --- General morphed check ---
    if (m_dwMorphTemplateID)
    {
        if (act == CA::ResurrectionNew)
            return CA::Walk2;
        if (act == CA::TitanWireaction)
            return CA::Stand2;
    }

    return m_nOneTimeAction;
}

// ============================================================================
// MoveAction2RawActionForMechanic
// ============================================================================

auto Avatar::MoveAction2RawActionForMechanic(
    MoveActionType nAction,
    std::int32_t nMechanicMode
) const -> CharacterAction
{
    if (nMechanicMode != 35111003)
        return CharacterAction::Siege2Stand;

    switch (nAction)
    {
    case MoveActionType::Walk:
        return CharacterAction::Siege2Walk;
    case MoveActionType::Jump:
        return CharacterAction::TankJump;
    case MoveActionType::Prone:
        return CharacterAction::Siege2Prone;
    case MoveActionType::Ladder:
        return CharacterAction::TankLadder;
    case MoveActionType::Rope:
        return CharacterAction::TankRope;
    case MoveActionType::RocketBooster:
        return CharacterAction::TankRocketBooster;
    default:
        return CharacterAction::Siege2Stand;
    }
}

// ============================================================================
// Morph dispatch helper — shared logic for morphed avatar action mapping
// ============================================================================

namespace
{

/// Flags controlling which MoveActionType entries map to morph actions.
/// Different morph types have slightly different sets.
struct MorphFlags
{
    bool bAlertAsSeparate{false};  // Alert → Stand2 (vs grouped with Stand)
    bool bSitAsSeparate{false};    // Sit → Swingtf (vs grouped with Stand)
    bool bRocketAsJump{false};     // RocketBooster grouped with Jump
    bool bFly2Grouped{false};      // Fly2/Fly2Move grouped with Fly1
    bool bFly2Separate{false};     // Fly2 → Magicattack2, Fly2Move → Magicattackf
};

auto MorphDispatch(MoveActionType action, const MorphFlags& f) -> CharacterAction
{
    switch (action)
    {
    case MoveActionType::Stand:
        return CharacterAction::Walk2;

    case MoveActionType::Alert:
        return f.bAlertAsSeparate ? CharacterAction::Stand2 : CharacterAction::Walk2;

    case MoveActionType::Sit:
        if (f.bSitAsSeparate)
            return CharacterAction::Swingtf;
        return CharacterAction::Walk2;

    case MoveActionType::Jump:
        return CharacterAction::Stand1;

    case MoveActionType::RocketBooster:
        return f.bRocketAsJump ? CharacterAction::Stand1 : CharacterAction::Walk1;

    case MoveActionType::Prone:
        return CharacterAction::Swingof;

    case MoveActionType::Fly1:
        return CharacterAction::Swingo3;

    case MoveActionType::Fly2:
        if (f.bFly2Grouped)
            return CharacterAction::Swingo3;
        if (f.bFly2Separate)
            return CharacterAction::Magicattack2;
        return CharacterAction::Walk1;

    case MoveActionType::Fly2Move:
        if (f.bFly2Grouped)
            return CharacterAction::Swingo3;
        if (f.bFly2Separate)
            return CharacterAction::Magicattackf;
        return CharacterAction::Walk1;

    case MoveActionType::Ladder:
        return CharacterAction::Swingt1;

    case MoveActionType::Rope:
        return CharacterAction::Swingt2;

    case MoveActionType::Dead:
        return CharacterAction::Swingt3;

    default:
        return CharacterAction::Walk1;
    }
}

} // anonymous namespace

// ============================================================================
// MoveAction2RawAction (from decompiled CAvatar::MoveAction2RawAction)
// ============================================================================

auto Avatar::MoveAction2RawAction(
    std::int32_t nMA,
    std::int32_t* pnDir,
    bool bRandom
) const -> CharacterAction
{
    // Direction is lowest bit; action is shifted right by 1
    if (pnDir)
        *pnDir = nMA & 1;

    const auto action = static_cast<MoveActionType>(nMA >> 1);

    // --- Dead action special cases ---
    if (action == MoveActionType::Dead)
    {
        if (is_pvp_field())
        {
            if (m_dwMorphTemplateID && MorphTemplate::IsIceKnight(m_dwMorphTemplateID))
                return CharacterAction::Magic2;

            if (m_nSpecialDyingAction != 0)
                return static_cast<CharacterAction>(m_nSpecialDyingAction);

            return CharacterAction::Pvpko;
        }

        // Original: if g_pStage is CField and field type is Urus → Prone
        if (is_field_type_urus())
            return CharacterAction::Prone;
    }

    // --- Determine effective mechanic mode ---
    std::int32_t nMechanicMode;
    if (m_nRidingVehicleID != 1932016 || m_nMechanicMode)
        nMechanicMode = m_nMechanicMode;
    else
        nMechanicMode = m_nPrevMechanicMode;

    // --- Morphed avatar ---
    if (m_dwMorphTemplateID)
    {
        if (IsMonsterMorphed())
        {
            // Stand/Alert/Sit grouped; Jump+RocketBooster grouped; Fly1+Fly2+Fly2Move grouped
            return MorphDispatch(action, {
                .bAlertAsSeparate = false,
                .bSitAsSeparate = false,
                .bRocketAsJump = true,
                .bFly2Grouped = true,
                .bFly2Separate = false,
            });
        }

        if (m_dwMorphTemplateID && MorphTemplate::IsHideMorphed(m_dwMorphTemplateID))
        {
            // Stand/Alert/Sit grouped; Fly2/Fly2Move separate
            return MorphDispatch(action, {
                .bAlertAsSeparate = false,
                .bSitAsSeparate = false,
                .bRocketAsJump = false,
                .bFly2Grouped = false,
                .bFly2Separate = true,
            });
        }

        if (IsSuperMan())
        {
            // Full dispatch: Alert separate, Sit separate, Fly2 separate
            return MorphDispatch(action, {
                .bAlertAsSeparate = true,
                .bSitAsSeparate = true,
                .bRocketAsJump = false,
                .bFly2Grouped = false,
                .bFly2Separate = true,
            });
        }

        if (IsIceKnight())
        {
            // Alert separate, no Sit/Fly2/Fly2Move
            return MorphDispatch(action, {
                .bAlertAsSeparate = true,
                .bSitAsSeparate = false,
                .bRocketAsJump = false,
                .bFly2Grouped = false,
                .bFly2Separate = false,
            });
        }

        if (IsKaiserDragon())
        {
            // Same as SuperMan: full dispatch
            return MorphDispatch(action, {
                .bAlertAsSeparate = true,
                .bSitAsSeparate = true,
                .bRocketAsJump = false,
                .bFly2Grouped = false,
                .bFly2Separate = true,
            });
        }

        // Unknown morph → Walk1
        return CharacterAction::Walk1;
    }

    // --- Ghost mode ---
    if (m_nGhostIndex)
    {
        switch (action)
        {
        case MoveActionType::Walk:
            return CharacterAction::GhostWalk;
        case MoveActionType::Jump:
            return CharacterAction::GhostJump;
        case MoveActionType::Prone:
            return CharacterAction::GhostPronestab;
        case MoveActionType::Fly1:
        case MoveActionType::Fly2:
        case MoveActionType::Fly2Move:
            return CharacterAction::GhostFly;
        case MoveActionType::Ladder:
            return CharacterAction::GhostLadder;
        case MoveActionType::Rope:
            return CharacterAction::GhostRope;
        case MoveActionType::Dead:
            return CharacterAction::Dead;
        case MoveActionType::Sit:
            return CharacterAction::GhostSit;
        default:
            return CharacterAction::GhostStand;
        }
    }

    // --- Normal (non-morphed, non-ghost) ---

    // Monster jobs (13000, 13100)
    auto nJob = m_avatarLook.nJob;
    if (nJob == 13000 || nJob == 13100)
        return MoveAction2RawActionForMonsterJob(action, bRandom);

    // Dance skill override
    auto nDanceState = m_nDanceState;
    if (is_dance_skill(nDanceState))
    {
        switch (nDanceState)
        {
        case 80001437: return CharacterAction::Dance2;
        case 80001438: return CharacterAction::Dance1;
        case 80001439: return CharacterAction::Dance0;
        case 80001486: return CharacterAction::Dance3;
        case 80001512: return CharacterAction::Dance4;
        case 80001513: return CharacterAction::Dance5;
        case 80001514: return CharacterAction::Dance6;
        case 80001515: return CharacterAction::Dance7;
        case 80001516: return CharacterAction::Dance8;
        case 80001573: return CharacterAction::DanceStarplanet0;
        case 80001574: return CharacterAction::DanceStarplanet1;
        case 80001575: return CharacterAction::DanceStarplanet2;
        case 80001576: return CharacterAction::DanceStarplanet3;
        case 80001577: return CharacterAction::DanceStarplanet4;
        case 80001578: return CharacterAction::DanceStarplanet5;
        case 80001603: return CharacterAction::DanceStarplanetEvent0;
        case 80001604: return CharacterAction::DanceStarplanetEvent1;
        case 80001605: return CharacterAction::DanceStarplanetEvent2;
        case 80001606: return CharacterAction::DanceStarplanetEvent3;
        case 80001607: return CharacterAction::DanceStarplanetEvent4;
        case 80001608: return CharacterAction::DanceStarplanetEvent5;
        default:       return CharacterAction::Walk1;
        }
    }

    // Mechanic mode dispatch
    if (nMechanicMode && nMechanicMode != 35001002)
        return MoveAction2RawActionForMechanic(action, nMechanicMode);

    // BattlePvP avatar dispatch
    if (m_nBattlePvPPAvatar)
        return MoveAction2RawActionForBattlePvP(action, m_nBattlePvPPAvatar);

    // Pose-based replacement (only for Stand/Alert when no riding/replaced-stand)
    if (m_nPose
        && !m_nReplacedStandAction
        && !m_nRidingVehicleID
        && (action == MoveActionType::Stand || action == MoveActionType::Alert))
    {
        auto weaponType = get_weapon_type(m_nWeaponItemID);
        auto replaced = get_replaced_action_by_pose(m_nPose, weaponType);
        if (replaced != CharacterAction::Walk1)
            return replaced;
    }

    // Standard action mapping
    switch (action)
    {
    case MoveActionType::Walk:
    {
        auto forced = m_nForcedMoveAction;
        if (forced != -1 && !m_nRidingVehicleID)
            return static_cast<CharacterAction>(forced);
        // walkType == 1 → Walk1(0), otherwise → Walk2(1)
        return m_nWalkType != 1 ? CharacterAction::Walk2 : CharacterAction::Walk1;
    }

    case MoveActionType::Stand:
    {
        auto forced = m_nForcedStandAction;
        if (forced != -1 && !m_nRidingVehicleID)
            return static_cast<CharacterAction>(forced);

        auto replacedStand = m_nReplacedStandAction;
        if (replacedStand && !m_nRidingVehicleID)
            return get_replaced_stand_action(replacedStand, m_nStandType);

        // standType == 1 → Stand1(2), otherwise → Stand2(3)
        return m_nStandType != 1 ? CharacterAction::Stand2 : CharacterAction::Stand1;
    }

    case MoveActionType::Jump:
        return CharacterAction::Jump;

    case MoveActionType::Alert:
        return CharacterAction::Alert;

    case MoveActionType::Prone:
        return CharacterAction::Prone;

    case MoveActionType::Fly1:
        if (is_kinesis_job(m_avatarLook.nJob) && m_nIsNewFlyingSkillID == 142111010)
            return CharacterAction::KinesisPsychicMove;
        return CharacterAction::Fly1;

    case MoveActionType::Ladder:
        return CharacterAction::Ladder;

    case MoveActionType::Rope:
        return CharacterAction::Rope;

    case MoveActionType::Dead:
        return CharacterAction::Dead;

    case MoveActionType::Sit:
        return CharacterAction::Sit;

    case MoveActionType::Fly2:
        if (is_kinesis_job(m_avatarLook.nJob) && m_nIsNewFlyingSkillID == 142111010)
            return CharacterAction::KinesisPsychicMove;
        return CharacterAction::Fly2;

    case MoveActionType::Fly2Move:
        if (is_kinesis_job(m_avatarLook.nJob) && m_nIsNewFlyingSkillID == 142111010)
            return CharacterAction::KinesisPsychicMove;
        return CharacterAction::Fly2Move;

    case MoveActionType::Dash2:
        return CharacterAction::HustleDash;

    case MoveActionType::RocketBooster:
        return CharacterAction::RocketBooster;

    case MoveActionType::Backwalk:
        return CharacterAction::Backward;

    case MoveActionType::Bladestance:
        return CharacterAction::RpHayatoSlashstance;

    case MoveActionType::Fevermode:
        return CharacterAction::RpAyameFeverMode;

    default:
        return CharacterAction::Walk1;
    }
}

} // namespace ms
