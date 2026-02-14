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

// --- Vehicle IDs ---
inline constexpr std::int32_t kMechanicTankVehicle = 1932016;
inline constexpr std::int32_t kResistanceRiding1   = 1932051;
inline constexpr std::int32_t kResistanceRiding2   = 1932085;

// Wild Hunter jaguar riding vehicle IDs (from RIDING_WILDHUNTER_JAGUAR .rdata)
inline constexpr std::array<std::int32_t, 9> kRidingWildHunterJaguar = {
    1932015, 1932030, 1932031, 1932032, 1932033,
    1932036, 1932100, 1932149, 1932215,
};

// --- Skill IDs ---
inline constexpr std::int32_t kMechanicTankSkill      = 35111003;
inline constexpr std::int32_t kMechanicOpenPortalSkill = 35001002;

// --- Item IDs ---
inline constexpr std::int32_t kDefaultWeaponSticker    = 1702224;
inline constexpr std::int32_t kTransFaceAcc1           = 1012104;
inline constexpr std::int32_t kTransFaceAcc2           = 1012289;

// --- Dance skill IDs (skill → action mapping) ---
inline constexpr std::int32_t kSkillDance2              = 80001437;
inline constexpr std::int32_t kSkillDance1              = 80001438;
inline constexpr std::int32_t kSkillDance0              = 80001439;
inline constexpr std::int32_t kSkillDance3              = 80001486;
inline constexpr std::int32_t kSkillDance4              = 80001512;
inline constexpr std::int32_t kSkillDance5              = 80001513;
inline constexpr std::int32_t kSkillDance6              = 80001514;
inline constexpr std::int32_t kSkillDance7              = 80001515;
inline constexpr std::int32_t kSkillDance8              = 80001516;
inline constexpr std::int32_t kSkillDanceStarplanet0    = 80001573;
inline constexpr std::int32_t kSkillDanceStarplanet1    = 80001574;
inline constexpr std::int32_t kSkillDanceStarplanet2    = 80001575;
inline constexpr std::int32_t kSkillDanceStarplanet3    = 80001576;
inline constexpr std::int32_t kSkillDanceStarplanet4    = 80001577;
inline constexpr std::int32_t kSkillDanceStarplanet5    = 80001578;
inline constexpr std::int32_t kSkillDanceStarplanetEvt0 = 80001603;
inline constexpr std::int32_t kSkillDanceStarplanetEvt1 = 80001604;
inline constexpr std::int32_t kSkillDanceStarplanetEvt2 = 80001605;
inline constexpr std::int32_t kSkillDanceStarplanetEvt3 = 80001606;
inline constexpr std::int32_t kSkillDanceStarplanetEvt4 = 80001607;
inline constexpr std::int32_t kSkillDanceStarplanetEvt5 = 80001608;

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
    MoveActionType nAction,
    bool bRandom
) const -> CharacterAction
{
    CharacterAction nResult;

    switch (nAction)
    {
    case MoveActionType::Walk:
        nResult = CharacterAction::PinkbeanWalk;
        break;
    case MoveActionType::Jump:
        nResult = CharacterAction::PinkbeanJump;
        break;
    case MoveActionType::Alert:
        nResult = CharacterAction::PinkbeanAlert;
        break;
    case MoveActionType::Prone:
        nResult = CharacterAction::PinkbeanProne;
        break;
    case MoveActionType::Fly1:
    case MoveActionType::Fly2:
    case MoveActionType::Fly2Move:
        nResult = CharacterAction::PinkbeanFly;
        break;
    case MoveActionType::Ladder:
        nResult = CharacterAction::PinkbeanLadder;
        break;
    case MoveActionType::Rope:
        nResult = CharacterAction::PinkbeanRope;
        break;
    case MoveActionType::Dead:
        nResult = CharacterAction::PinkbeanDead;
        // TODO: if on character select screen, use PinkbeanDead2
        break;
    case MoveActionType::Sit:
        nResult = CharacterAction::PinkbeanSit;
        break;
    default:
        nResult = CharacterAction::PinkbeanStand;
        break;
    }

    if (!bRandom)
        return nResult;

    auto nChanged = ActionMan::GetInstance().GetRandomMoveActionChange(
        static_cast<std::int32_t>(nResult));
    m_nChangeMoveAction = nChanged;

    if (nChanged == -1)
        return nResult;

    return static_cast<CharacterAction>(nChanged);
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
    if (nAction == static_cast<std::int32_t>(CharacterAction::Dead)
        || nAction == static_cast<std::int32_t>(CharacterAction::Pvpko)
        || nAction == static_cast<std::int32_t>(CharacterAction::Pvpko2)
        || is_battle_pvp_dead_action(nAction)
        || nAction == static_cast<std::int32_t>(CharacterAction::PinkbeanDead)
        || nAction == static_cast<std::int32_t>(CharacterAction::PinkbeanDead2)
        || nAction == static_cast<std::int32_t>(CharacterAction::Setitem3)
        || nAction == static_cast<std::int32_t>(CharacterAction::Setitem4)
        || is_dance_action(static_cast<CharacterAction>(nAction)))
    {
        bool bWasMechanic = (m_nRidingVehicleID == kMechanicTankVehicle);
        m_nRidingVehicleID = 0;
        if (bWasMechanic)
            SetMechanicHUE(0, 1);
        m_nRidingChairID = 0;
    }

    // Pvpko/Pvpko2 clear morph
    if (nAction == static_cast<std::int32_t>(CharacterAction::Pvpko)
        || nAction == static_cast<std::int32_t>(CharacterAction::Pvpko2))
        m_dwMorphTemplateID = 0;

    // --- Validate action range ---
    if (nAction >= 0)
    {
        if (m_dwMorphTemplateID)
        {
            // Morphed: only actions below Magic3 are valid for morph
            if (nAction >= static_cast<std::int32_t>(CharacterAction::Magic3))
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
            nWeaponStickerID = kDefaultWeaponSticker;
        if (m_nForcedStandAction != -1 && m_nForcedStandAction == nAction)
            nWeaponStickerID = kDefaultWeaponSticker;
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
        {
            std::vector<std::shared_ptr<CharacterActionFrameEntry>> apFE;
            ActionMan::GetInstance().LoadCharacterAction(
                nAction,
                m_avatarLook.nGender,
                nSkin,
                nJob,
                aAvatarHairEquip,
                apFE,
                nWeaponStickerID,
                nRidingForLoad,
                m_bTamingMobTired,
                m_nGhostIndex,
                nGatherToolID,
                bDrawElfEar,
                m_nChangeWeaponLook,
                m_nLarknessState,
                GetPortableChairID(),
                nMixedHairColor,
                nMixHairPercent,
                m_nBattlePvPPAvatar);
            ai.aaAction[nAction] = std::move(apFE);
        }

        // --- Load taming mob action frames ---
        auto nLoadAction = (m_nSaveOneTimeActionForPinkbean != -1)
            ? m_nSaveOneTimeActionForPinkbean : nAction;
        // TODO: Call CActionMan::LoadTamingMobAction
        if (ai.aaTamingMobAction.find(nAction) == ai.aaTamingMobAction.end())
            ai.aaTamingMobAction[nAction] = {};

        (void)nLoadAction;
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

            // BattlePvP range — use raw delay unless basic attack
            if (static_cast<std::uint32_t>(
                    nAction - static_cast<std::int32_t>(CharacterAction::BattlepvpManjiWalk)) <= 0x64u
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
    if (m_nRidingVehicleID == kMechanicTankVehicle)
    {
        auto nMechMode = m_nMechanicMode ? m_nMechanicMode : m_nPrevMechanicMode;
        if (nMechMode == kMechanicTankSkill)
        {
            bool bNeedsRocket = false;
            if ((nAction >= static_cast<std::int32_t>(CharacterAction::Ladder)
                    && nAction <= static_cast<std::int32_t>(CharacterAction::Rope))
                || (nAction >= static_cast<std::int32_t>(CharacterAction::Ladder2)
                    && nAction <= static_cast<std::int32_t>(CharacterAction::Rope2))
                || (nAction >= static_cast<std::int32_t>(CharacterAction::TankRope)
                    && nAction <= static_cast<std::int32_t>(CharacterAction::TankHerbalismMechanic)))
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
        && nAction == static_cast<std::int32_t>(CharacterAction::PinkbeanDead))
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
    return nItemID == kTransFaceAcc1 || nItemID == kTransFaceAcc2;
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
    auto& pLocal = UserLocal::GetInstance();
    if (this == &pLocal)
    {
        auto& stat = WvsContext::GetInstance().GetSecondaryStat();
        if (stat.nAttract_.Get() && stat.rAttract_.Get() == 188)
            return;
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

void Avatar::ClearActionLayer(std::int32_t nSlot)
{
    auto& ai = m_aiAction[static_cast<std::size_t>(nSlot)];
    ai.aaAction.clear();
    ai.aaTamingMobAction.clear();
    ai.aAlpha.clear();
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

// ============================================================================
// IsRidingWildHunterJaguar — from CAvatar::IsRidingWildHunterJaguar @ 0x4b4420
// ============================================================================

bool Avatar::IsRidingWildHunterJaguar() const
{
    return std::find(
        kRidingWildHunterJaguar.begin(),
        kRidingWildHunterJaguar.end(),
        m_nRidingVehicleID
    ) != kRidingWildHunterJaguar.end();
}

// ============================================================================
// ConvertCharacterAction — from CAvatar::ConvertCharacterAction @ 0x5e91f0
// ============================================================================

auto Avatar::ConvertCharacterAction(std::int32_t nAction) const -> std::int32_t
{
    const auto u = static_cast<std::uint32_t>(nAction);

    if (m_nRidingVehicleID == kResistanceRiding1 || m_nRidingVehicleID == kResistanceRiding2)
    {
        if (u < 2)
            return static_cast<std::int32_t>(CharacterAction::Fly2Move);
        if (nAction == static_cast<std::int32_t>(CharacterAction::GhostWalk))
            return static_cast<std::int32_t>(CharacterAction::Fly2Move);
        if (is_battle_pvp_walk_action(nAction))
            return static_cast<std::int32_t>(CharacterAction::Fly2Move);
        if (is_stand_action(nAction))
            return static_cast<std::int32_t>(CharacterAction::Fly2Move);
        return nAction;
    }

    // General riding mount
    if ((u >= 2 && u <= 3)
        || nAction == static_cast<std::int32_t>(CharacterAction::Stand1Floating)
        || nAction == static_cast<std::int32_t>(CharacterAction::Sit)
        || nAction == static_cast<std::int32_t>(CharacterAction::GhostStand)
        || is_battle_pvp_stand_action(nAction)
        || u < 2
        || nAction == static_cast<std::int32_t>(CharacterAction::GhostWalk)
        || is_battle_pvp_walk_action(nAction)
        || nAction == static_cast<std::int32_t>(CharacterAction::Prone)
        || nAction == static_cast<std::int32_t>(CharacterAction::Siege2Prone))
    {
        return static_cast<std::int32_t>(CharacterAction::Sit);
    }

    if (IsRidingWildHunterJaguar()
        && nAction == static_cast<std::int32_t>(CharacterAction::Pronestab))
        return static_cast<std::int32_t>(CharacterAction::PronestabJaguar);

    return nAction;
}

// ============================================================================
// AvatarLayerFlip — from CAvatar::AvatarLayerFlip @ 0x5ea6e0
// ============================================================================

void Avatar::AvatarLayerFlip(std::int32_t nFlip)
{
    if (m_pLayerUnderFace)
        m_pLayerUnderFace->put_flip(nFlip);
    if (m_pLayerOverFace)
        m_pLayerOverFace->put_flip(nFlip);
    if (m_pLayerFace)
        m_pLayerFace->put_flip(nFlip);
    if (m_pLayerUnderCharacter)
        m_pLayerUnderCharacter->put_flip(nFlip);
    if (m_pLayerOverCharacter)
        m_pLayerOverCharacter->put_flip(nFlip);
    if (m_pLayerJaguarCannon)
        m_pLayerJaguarCannon->put_flip(nFlip);
    if (m_pLayerRocketBooster)
        m_pLayerRocketBooster->put_flip(nFlip);
}

// ============================================================================
// FixCharacterPosition — from CAvatar::FixCharacterPosition @ 0x5ec720
// ============================================================================

void Avatar::FixCharacterPosition()
{
    if (m_bDelayedLoad)
        return;

    std::int32_t nDir = 0;
    auto nCurrentAction = GetCurrentAction(&nDir, false);
    auto nCharAction = ConvertCharacterAction(static_cast<std::int32_t>(nCurrentAction));

    // Determine taming mob action
    auto nTMAction = (m_nTamingMobOneTimeAction >= 0)
        ? m_nTamingMobOneTimeAction
        : m_nTamingMobAction;

    // Select action info slots
    auto nOneTime = GetOneTimeAction();
    auto& aiChar = m_aiAction[static_cast<std::size_t>(
        nOneTime != static_cast<CharacterAction>(-1) ? 1 : 0)];
    auto& aiTM = m_aiAction[static_cast<std::size_t>(
        m_nTamingMobOneTimeAction >= 0 ? 1 : 0)];

    // Look up character action frames
    auto itChar = aiChar.aaAction.find(nCharAction);
    if (itChar == aiChar.aaAction.end() || itChar->second.empty())
        return;
    auto& aCharFrames = itChar->second;

    // Look up taming mob action frames
    auto itTM = aiTM.aaTamingMobAction.find(nTMAction);
    if (itTM == aiTM.aaTamingMobAction.end() || itTM->second.empty())
        return;
    auto& aTMFrames = itTM->second;

    // Get current character frame entry
    auto nCharIdx = static_cast<std::size_t>(aiChar.nCurFrameIndex);
    if (nCharIdx >= aCharFrames.size() || !aCharFrames[nCharIdx])
        return;
    auto& charFE = *aCharFrames[nCharIdx];

    // Read character brow point
    auto ptBrow = charFE.ptBrow;

    // Compute body offset from taming mob
    std::int32_t dx = 0;
    std::int32_t dy = 0;
    Point2D ptMuzzle{};
    Point2D ptHand{};
    Point2D ptNavel{};
    Point2D ptTMNavel{};
    Point2D ptTMHead{};
    Point2D ptTMMuzzle{};

    auto nTMIdx = static_cast<std::size_t>(aiTM.nCurTMFrameIndex);
    if (nTMIdx < aTMFrames.size() && aTMFrames[nTMIdx])
    {
        auto& tmFE = *aTMFrames[nTMIdx];

        dx = tmFE.ptNavel.x - charFE.ptNavel.x;
        dy = tmFE.ptNavel.y - charFE.ptNavel.y;
        m_rcTamingMobBody = tmFE.rcBody;

        ptTMNavel = tmFE.ptNavel;
        ptTMHead = tmFE.ptHead;
        ptTMMuzzle = tmFE.ptMuzzle;
        ptMuzzle = charFE.ptMuzzle;
        ptHand = charFE.ptHand;
        ptNavel = charFE.ptNavel;
    }

    // Determine flip direction
    bool bFlipRight = (nDir != 0)
        || nCharAction == static_cast<std::int32_t>(CharacterAction::Ladder)
        || nCharAction == static_cast<std::int32_t>(CharacterAction::Rope)
        || nCharAction == static_cast<std::int32_t>(CharacterAction::GhostRope)
        || nCharAction == static_cast<std::int32_t>(CharacterAction::GhostLadder)
        || is_battle_pvp_rope_action(nCharAction)
        || nCharAction == static_cast<std::int32_t>(CharacterAction::PinkbeanLadder)
        || nCharAction == static_cast<std::int32_t>(CharacterAction::PinkbeanRope)
        || nCharAction == static_cast<std::int32_t>(CharacterAction::Ladder2)
        || nCharAction == static_cast<std::int32_t>(CharacterAction::Rope2);

    if (bFlipRight)
    {
        AvatarLayerFlip(0);
        m_pFaceOrigin->RelMove(ptBrow.x, ptBrow.y);
        m_pBodyOrigin->RelMove(dx, dy);
        m_ptBodyRelMove.x = dx;
        m_ptBodyRelMove.y = dy;
        m_bFlip = false;
    }
    else
    {
        AvatarLayerFlip(1);
        m_pFaceOrigin->RelMove(-ptBrow.x, ptBrow.y);
        m_pBodyOrigin->RelMove(-dx, dy);
        m_ptBodyRelMove.x = -dx;
        m_ptBodyRelMove.y = dy;
        m_bFlip = true;
    }

    // Position remaining origins (negate x if flipped)
    auto flipX = [this](std::int32_t x) { return m_bFlip ? -x : x; };

    m_pMuzzleOrigin->RelMove(flipX(ptMuzzle.x), ptMuzzle.y);
    m_pHandOrigin->RelMove(flipX(ptHand.x), ptHand.y);
    m_pTailOrigin->RelMove(flipX(ptNavel.x), ptNavel.y);
    m_pTMNavelOrigin->RelMove(flipX(ptTMNavel.x), ptTMNavel.y);
    m_pTMHeadOrigin->RelMove(flipX(ptTMHead.x), ptTMHead.y);
    m_pTMMuzzleOrigin->RelMove(flipX(ptTMMuzzle.x), ptTMMuzzle.y);

    // Reset origin position and speed
    m_pOrigin->RelMove(0, 0);
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
    if (m_nRidingVehicleID != kMechanicTankVehicle || m_nMechanicMode)
        nMechanicMode = m_nMechanicMode;
    else
        nMechanicMode = m_nPrevMechanicMode;

    if (nMechanicMode == kMechanicTankSkill)
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
    if (nMechanicMode != kMechanicTankSkill)
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
    if (m_nRidingVehicleID != kMechanicTankVehicle || m_nMechanicMode)
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
        case kSkillDance2:              return CharacterAction::Dance2;
        case kSkillDance1:              return CharacterAction::Dance1;
        case kSkillDance0:              return CharacterAction::Dance0;
        case kSkillDance3:              return CharacterAction::Dance3;
        case kSkillDance4:              return CharacterAction::Dance4;
        case kSkillDance5:              return CharacterAction::Dance5;
        case kSkillDance6:              return CharacterAction::Dance6;
        case kSkillDance7:              return CharacterAction::Dance7;
        case kSkillDance8:              return CharacterAction::Dance8;
        case kSkillDanceStarplanet0:    return CharacterAction::DanceStarplanet0;
        case kSkillDanceStarplanet1:    return CharacterAction::DanceStarplanet1;
        case kSkillDanceStarplanet2:    return CharacterAction::DanceStarplanet2;
        case kSkillDanceStarplanet3:    return CharacterAction::DanceStarplanet3;
        case kSkillDanceStarplanet4:    return CharacterAction::DanceStarplanet4;
        case kSkillDanceStarplanet5:    return CharacterAction::DanceStarplanet5;
        case kSkillDanceStarplanetEvt0: return CharacterAction::DanceStarplanetEvent0;
        case kSkillDanceStarplanetEvt1: return CharacterAction::DanceStarplanetEvent1;
        case kSkillDanceStarplanetEvt2: return CharacterAction::DanceStarplanetEvent2;
        case kSkillDanceStarplanetEvt3: return CharacterAction::DanceStarplanetEvent3;
        case kSkillDanceStarplanetEvt4: return CharacterAction::DanceStarplanetEvent4;
        case kSkillDanceStarplanetEvt5: return CharacterAction::DanceStarplanetEvent5;
        default:       return CharacterAction::Walk1;
        }
    }

    // Mechanic mode dispatch
    if (nMechanicMode && nMechanicMode != kMechanicOpenPortalSkill)
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

// ============================================================================
// GetActionInfo — returns the active ACTIONINFO slot
// ============================================================================

auto Avatar::GetActionInfo() -> ActionInfo*
{
    const bool bOneTime = (GetOneTimeAction() > static_cast<CharacterAction>(-1));
    return &m_aiAction[bOneTime ? 1 : 0];
}

static auto get_update_time() -> std::int32_t
{
    return static_cast<std::int32_t>(Application::GetInstance().GetUpdateTime());
}

// ============================================================================
// Helper methods
// ============================================================================

void Avatar::ResetCharacterOneTimeAction()
{
    m_nOneTimeAction = static_cast<CharacterAction>(-1);
    ClearCharacterActionLayer(1);
}

void Avatar::ResetTamingMobOneTimeAction()
{
    m_nTamingMobOneTimeAction = -1;
    ClearTamingMobActionLayer(1);
}

bool Avatar::IsRidingDslayerWing() const
{
    return m_nRidingVehicleID == kResistanceRiding1
        || m_nRidingVehicleID == kResistanceRiding2;
}

void Avatar::ClearCharacterActionLayer(std::int32_t nSlot)
{
    m_aiAction[nSlot].aaAction.clear();
}

void Avatar::ClearTamingMobActionLayer(std::int32_t nSlot)
{
    m_aiAction[nSlot].aaTamingMobAction.clear();
    m_aiAction[nSlot].aAlpha.clear();
}

void Avatar::AvatarLayerRemoveCanvas(std::int32_t /*nZ*/)
{
    // TODO: remove all canvas from layers at the given z-order
}

void Avatar::LoadDarkTornado()
{
    // TODO: requires CSkillInfo::GetMobSkill(173) and
    // CAnimationDisplayer::LoadLayer — load dark tornado effect layer
    // from mob skill 173 UOL, assign to m_pLayerDarkTornado, animate on repeat.
}

auto Avatar::GetOrigin() -> std::shared_ptr<Gr2DVector>&
{
    if (m_pFakeOrigin)
        return m_pFakeOrigin;
    return m_pOrigin;
}

// ============================================================================
// CharacterFrameUpdate — frame advance for character action path
// Returns true if taming mob frame also needs update.
// Decompiled from CAvatar::CharacterFrameUpdate @ 0x5fab70
// ============================================================================

bool Avatar::CharacterFrameUpdate()
{
    std::int32_t nDir = 0;
    const auto nRawAction = GetCurrentAction(&nDir, false);
    const auto nCharAction = ConvertCharacterAction(
        static_cast<std::int32_t>(nRawAction));

    // Determine taming mob action
    std::int32_t nTMAction;
    if (m_nTamingMobOneTimeAction > -1)
        nTMAction = m_nTamingMobOneTimeAction;
    else
        nTMAction = m_nTamingMobAction;

    // Decrement frame remain by one tick (30ms)
    const bool bOneTime =
        (GetOneTimeAction() > static_cast<CharacterAction>(-1));
    m_aiAction[bOneTime].tCurFrameRemain -= 30;

    if (m_aiAction[bOneTime].tCurFrameRemain > 0
        || m_aiAction[bOneTime].bCurFrameStop)
    {
        return true;
    }

    auto* ai = &m_aiAction[bOneTime];
    auto* tmAi = &m_aiAction[m_nTamingMobOneTimeAction > -1 ? 1 : 0];

    // Rope/ladder actions: freeze frame if position hasn't changed
    const auto ca = static_cast<CharacterAction>(nCharAction);
    const bool bRopeOrLadder =
        ca == CharacterAction::Ladder
        || ca == CharacterAction::Rope
        || ca == CharacterAction::GhostLadder
        || ca == CharacterAction::GhostRope
        || is_battle_pvp_rope_action(nCharAction)
        || ca == CharacterAction::PinkbeanLadder
        || ca == CharacterAction::PinkbeanRope;

    const bool bCanAdvance = !bRopeOrLadder
        || (IsRidingDslayerWing()
            && m_ptPosPrev.y.Get() != m_ptPos.y.Get());

    if (!bCanAdvance)
    {
        ai->tCurFrameRemain = 0;
        return true;
    }

    // --- Advance frame index ---
    auto itAction = ai->aaAction.find(nCharAction);
    if (itAction == ai->aaAction.end() || itAction->second.empty())
        return true;

    auto& aAction = itAction->second;
    const auto nFrameCount = static_cast<std::int32_t>(aAction.size());

    ++ai->nCurFrameIndex;
    if (ai->nCurFrameIndex >= nFrameCount)
    {
        // One-time action completed
        if (GetOneTimeAction() > static_cast<CharacterAction>(-1)
            && !m_bRepeatOneTimeAction)
        {
            const auto nSaveAction = GetOneTimeAction();
            ResetCharacterOneTimeAction();
            ActionProcess(nSaveAction);

            if (nSaveAction == CharacterAction::Getoff
                || nSaveAction == CharacterAction::Getoff3)
            {
                return false;
            }

            PrepareCharacterActionLayer(6, 120, 0, 0);

            if (is_wildhunter_job(m_avatarLook.nJob))
            {
                if (m_nTamingMobOneTimeAction != -1)
                {
                    auto* tmSlot = &m_aiAction[
                        m_nTamingMobOneTimeAction > -1 ? 1 : 0];
                    tmSlot->bCurFrameStop = false;
                    tmSlot->tCurFrameRemain = 0;
                    tmSlot->tCurTMFrameRemain = 0;
                    PrepareJaguarCannonLayer();
                    FixCharacterPosition();
                    return true;
                }
                PrepareTamingMobActionLayer(6, 120, 0);
                PrepareJaguarCannonLayer();
            }

            FixCharacterPosition();
            return true;
        }
        ai->nCurFrameIndex = 0;
    }

    // Accumulate frame delay
    ai->tCurFrameRemain += ai->aFrameDelay[ai->nCurFrameIndex];

    // Get action data (piece table)
    const auto& ad = s_aCharacterActionData[nCharAction];

    // Validate taming mob frame data exists
    auto itTM = tmAi->aaTamingMobAction.find(nTMAction);
    if (itTM == tmAi->aaTamingMobAction.end() || itTM->second.empty())
        return true;

    // ShiftCanvas on face/shadow layers
    if (m_pLayerUnderFace) m_pLayerUnderFace->ShiftCanvas(1);
    if (m_pLayerOverFace) m_pLayerOverFace->ShiftCanvas(1);
    if (m_pLayerShadowPartner) m_pLayerShadowPartner->ShiftCanvas(1);

    // Reset body origin
    if (m_pBodyOrigin) m_pBodyOrigin->RelMove(0, 0);

    // Calculate body relative move (TM navel − character navel)
    auto* tmFrame = itTM->second[tmAi->nCurTMFrameIndex].get();
    auto* charFrame = aAction[ai->nCurFrameIndex].get();

    auto dx = tmFrame->ptNavel.x - charFrame->ptNavel.x;
    auto dy = tmFrame->ptNavel.y - charFrame->ptNavel.y;

    if (m_pLayerUnderFace && m_pLayerUnderFace->get_flip())
        dx = -dx;

    m_ptBodyRelMove.x.Put(dx);
    m_ptBodyRelMove.y.Put(dy);

    // Apply body relative move
    if (m_pBodyOrigin)
        m_pBodyOrigin->RelMove(
            m_ptBodyRelMove.x.Get(),
            m_ptBodyRelMove.y.Get());

    // Handle extended frames (piece index remapping)
    const auto nCurIdx = ai->nCurFrameIndex;
    const auto nPieceCount = static_cast<std::int32_t>(ad.m_aPieces.size());
    auto nDisplayIdx = nCurIdx;
    if (nPieceCount > 0 && ai->IsExtendFrame(nPieceCount))
        nDisplayIdx = nCurIdx / ai->GetFrameMultipleCountOf(nPieceCount);

    if (nDisplayIdx < 0 || nDisplayIdx >= nPieceCount)
        return true;

    const auto& piece = ad.m_aPieces[nDisplayIdx];

    // Set flip: base flip XOR piece flip
    if (m_pLayerUnderFace)
    {
        m_pLayerUnderFace->put_flip(m_bFlip);
        auto curFlip = m_pLayerUnderFace->get_flip();
        m_pLayerUnderFace->put_flip(piece.m_bFlip ^ curFlip);

        // Direction fix override
        if (piece.m_nDirectionFix > 0
            && piece.m_nDirectionFix - 1 != nDir)
        {
            if (piece.m_nDirectionFix == 1)
                m_pLayerUnderFace->put_flip(1);
            else if (piece.m_nDirectionFix == 2)
                m_pLayerUnderFace->put_flip(0);
        }
    }

    // Propagate flip to OverFace and Face
    if (m_pLayerUnderFace && m_pLayerOverFace)
        m_pLayerOverFace->put_flip(m_pLayerUnderFace->get_flip());
    if (m_pLayerUnderFace && m_pLayerFace)
        m_pLayerFace->put_flip(m_pLayerUnderFace->get_flip());

    // Update face origin (brow point)
    auto browX = charFrame->ptBrow.x;
    auto browY = charFrame->ptBrow.y;

    if (piece.m_nRotate == 0 && m_pLayerFace
        && m_pLayerFace->get_flip())
    {
        browX = -browX;
    }
    if (m_pFaceOrigin) m_pFaceOrigin->RelMove(browX, browY);

    // Show face: link Face alpha to UnderFace alpha, or clear
    if (m_pLayerFace)
    {
        auto* faceAlpha = m_pLayerFace->get_alpha();
        if (piece.m_bShowFace)
        {
            auto* ufAlpha = m_pLayerUnderFace
                ? m_pLayerUnderFace->get_alpha()
                : nullptr;
            if (faceAlpha && ufAlpha)
                faceAlpha->PutOrigin(ufAlpha);
        }
        else
        {
            if (faceAlpha)
                faceAlpha->PutOrigin(nullptr);
        }
    }

    // Handle rotation with flip
    if (m_pLayerFace && m_pLayerFace->get_flip() && piece.m_nRotate)
    {
        std::int32_t snapshotX = 0;
        if (m_pRawOrigin)
            m_pRawOrigin->GetSnapshot(
                &snapshotX, nullptr, nullptr, nullptr,
                nullptr, nullptr, nullptr, nullptr);
        if (m_pOrigin)
            m_pOrigin->PutFlipX(snapshotX);
    }
    if (m_pOrigin)
        m_pOrigin->PutRA(static_cast<double>(piece.m_nRotate));

    // Reset face alpha origin
    if (m_pLayerFace)
    {
        auto* fAlpha = m_pLayerFace->get_alpha();
        if (fAlpha)
            fAlpha->RelMove(0, 0);
    }

    // Muzzle layer flip + origin
    if (m_pLayerMuzzle)
        m_pLayerMuzzle->put_flip(m_bFlip);

    auto muzzleX = charFrame->ptMuzzle.x;
    auto muzzleY = charFrame->ptMuzzle.y;
    if (m_bFlip) muzzleX = -muzzleX;
    if (m_pMuzzleOrigin) m_pMuzzleOrigin->RelMove(muzzleX, muzzleY);

    // Hand origin
    auto handX = charFrame->ptHand.x;
    auto handY = charFrame->ptHand.y;
    if (m_bFlip) handX = -handX;
    if (m_pHandOrigin) m_pHandOrigin->RelMove(handX, handY);

    // Tail origin
    auto tailX = charFrame->ptTail.x;
    auto tailY = charFrame->ptTail.y;
    if (m_bFlip) tailX = -tailX;
    if (m_pTailOrigin) m_pTailOrigin->RelMove(tailX, tailY);

    return true;
}

// ============================================================================
// TamingMobFrameUpdate — taming mob frame advance
// ============================================================================

void Avatar::TamingMobFrameUpdate()
{
    std::int32_t nDir = 0;
    const auto nRawAction = GetCurrentAction(&nDir, false);
    const auto nCharAction = ConvertCharacterAction(
        static_cast<std::int32_t>(nRawAction));

    // Determine taming mob action
    std::int32_t nTMAction;
    if (m_nTamingMobOneTimeAction > -1)
        nTMAction = m_nTamingMobOneTimeAction;
    else
        nTMAction = m_nTamingMobAction;

    // Decrement TM frame remain by one tick (30ms)
    const bool bTMOneTime = (m_nTamingMobOneTimeAction > -1);
    m_aiAction[bTMOneTime].tCurTMFrameRemain -= 30;

    if (m_aiAction[bTMOneTime].tCurTMFrameRemain > 0
        || m_aiAction[bTMOneTime].bCurFrameStop)
    {
        return;
    }

    const bool bCharOneTime =
        (GetOneTimeAction() > static_cast<CharacterAction>(-1));
    auto* charAi = &m_aiAction[bCharOneTime];
    auto* tmAi = &m_aiAction[bTMOneTime];

    // Rope/ladder: freeze frame if Y position hasn't changed
    const auto ca = static_cast<CharacterAction>(nCharAction);
    const bool bRopeOrLadder =
        ca == CharacterAction::Ladder
        || ca == CharacterAction::Rope
        || ca == CharacterAction::GhostLadder
        || ca == CharacterAction::GhostRope
        || is_battle_pvp_rope_action(nCharAction)
        || ca == CharacterAction::PinkbeanLadder
        || ca == CharacterAction::PinkbeanRope;

    if (bRopeOrLadder
        && m_ptPosPrev.y.Get() == m_ptPos.y.Get())
    {
        tmAi->tCurTMFrameRemain = 0;
        return;
    }

    // Advance TM frame index
    auto itTM = tmAi->aaTamingMobAction.find(nTMAction);
    if (itTM == tmAi->aaTamingMobAction.end() || itTM->second.empty())
        return;

    auto& aTMAction = itTM->second;
    const auto nTMFrameCount = static_cast<std::int32_t>(aTMAction.size());

    ++tmAi->nCurTMFrameIndex;
    if (tmAi->nCurTMFrameIndex >= nTMFrameCount)
    {
        // One-time TM action completed
        if (m_nTamingMobOneTimeAction > -1 && !m_bRepeatOneTimeAction)
        {
            ResetTamingMobOneTimeAction();
            PrepareTamingMobActionLayer(6, 120, 0);
            PrepareJaguarCannonLayer();
            FixCharacterPosition();
            return;
        }
        tmAi->nCurTMFrameIndex = 0;
    }

    // Accumulate TM frame delay
    if (tmAi->nCurTMFrameIndex
        < static_cast<std::int32_t>(tmAi->aTMFrameDelay.size()))
    {
        tmAi->tCurTMFrameRemain +=
            tmAi->aTMFrameDelay[tmAi->nCurTMFrameIndex];
    }

    // Validate character frame data exists
    auto itChar = charAi->aaAction.find(nCharAction);
    if (itChar == charAi->aaAction.end() || itChar->second.empty())
        return;

    // ShiftCanvas on UnderCharacter, OverCharacter, JaguarCannon
    if (m_pLayerUnderCharacter) m_pLayerUnderCharacter->ShiftCanvas(1);
    if (m_pLayerOverCharacter) m_pLayerOverCharacter->ShiftCanvas(1);
    if (m_pLayerJaguarCannon) m_pLayerJaguarCannon->ShiftCanvas(1);

    // Calculate body relative move (TM navel − character navel)
    auto* tmFrame = aTMAction[tmAi->nCurTMFrameIndex].get();
    auto* charFrame = itChar->second[charAi->nCurFrameIndex].get();

    auto dx = tmFrame->ptNavel.x - charFrame->ptNavel.x;
    auto dy = tmFrame->ptNavel.y - charFrame->ptNavel.y;

    if (m_pLayerUnderCharacter && m_pLayerUnderCharacter->get_flip())
        dx = -dx;

    m_ptBodyRelMove.x.Put(dx);
    m_ptBodyRelMove.y.Put(dy);

    // Apply body relative move
    if (m_pBodyOrigin)
        m_pBodyOrigin->RelMove(
            m_ptBodyRelMove.x.Get(),
            m_ptBodyRelMove.y.Get());

    // Update taming mob body rect
    m_rcTamingMobBody = tmFrame->rcBody;

    // Update TM navel origin
    auto navelX = tmFrame->ptNavel.x;
    if (m_bFlip) navelX = -navelX;
    if (m_pTMNavelOrigin)
        m_pTMNavelOrigin->RelMove(navelX, tmFrame->ptNavel.y);

    // Update TM head origin
    auto headX = tmFrame->ptHead.x;
    if (m_bFlip) headX = -headX;
    if (m_pTMHeadOrigin)
        m_pTMHeadOrigin->RelMove(headX, tmFrame->ptHead.y);

    // Update TM muzzle origin
    auto muzzleX = tmFrame->ptMuzzle.x;
    if (m_bFlip) muzzleX = -muzzleX;
    if (m_pTMMuzzleOrigin)
        m_pTMMuzzleOrigin->RelMove(muzzleX, tmFrame->ptMuzzle.y);
}

// ============================================================================
// ActionProcess — post-action transition logic
// ============================================================================

void Avatar::ActionProcess(CharacterAction nAction)
{
    switch (nAction)
    {
    case CharacterAction::CyclonePre:
    case CharacterAction::Cyclone:
        // Continue cyclone loop
        m_nOneTimeAction = CharacterAction::Cyclone;
        return;

    case CharacterAction::DarktornadoHitPre:
        LoadDarkTornado();
        [[fallthrough]];
    case CharacterAction::DarktornadoHit:
        // Continue dark tornado hit
        m_nOneTimeAction = CharacterAction::DarktornadoHit;
        return;

    case CharacterAction::DarktornadoHitAfter:
        // Clear dark tornado layer
        m_pLayerDarkTornado.reset();
        return;

    case CharacterAction::Getoff:
    case CharacterAction::Getoff2:
    case CharacterAction::Getoff3:
    case CharacterAction::TankGetoff2:
    {
        const bool bWasMechanic = (m_nRidingVehicleID == kMechanicTankVehicle);
        m_nRidingVehicleID = 0;
        if (bWasMechanic)
            SetMechanicHUE(0, 1);
        ResetOneTimeAction();
        PrepareActionLayer(6, 120, 0, 0);
        return;
    }

    default:
        return;
    }
}

// ============================================================================
// SetMorphed — set/clear morph state
// ============================================================================

void Avatar::SetMorphed(std::uint32_t dwMorphTemplateID)
{
    if (m_dwMorphTemplateID == dwMorphTemplateID)
        return;

    const bool bWasMechanic = (m_nRidingVehicleID == kMechanicTankVehicle);
    m_dwMorphTemplateID = dwMorphTemplateID;
    m_nRidingVehicleID = 0;

    if (bWasMechanic)
        SetMechanicHUE(0, 1);

    ClearActionLayer(0);
    ResetOneTimeAction();
    PrepareActionLayer(6, 120, 0, 0);

    if (!m_dwMorphTemplateID)
        SetEmotion(0, -1);

    // Alpha fade transition (500ms default, 0 for morph 1002)
    const std::int32_t tDelay = (dwMorphTemplateID == 1002) ? 0 : 500;

    // Schedule alpha fade-in on each rendering layer
    const auto fadeLayer = [tDelay](
        const std::shared_ptr<WzGr2DLayer>& layer)
    {
        if (!layer) return;
        const auto dwColor = layer->get_color();
        layer->put_color(0x00FFFFFF);
        const auto tCur = layer->GetCurrentTime();
        auto* alpha = layer->get_alpha();
        if (alpha)
        {
            const auto nAlpha = static_cast<std::int32_t>(
                (dwColor >> 24) & 0xFF);
            alpha->RelMove(nAlpha, nAlpha, tCur + tDelay);
        }
    };

    fadeLayer(m_pLayerFace);
    fadeLayer(m_pLayerOverFace);
    fadeLayer(m_pLayerUnderFace);
    fadeLayer(m_pLayerShadowPartner);
    fadeLayer(m_pLayerOverCharacter);
    fadeLayer(m_pLayerUnderCharacter);
}

// ============================================================================
// SetLayerColor — apply color to all avatar rendering layers
// ============================================================================

void Avatar::SetLayerColor(std::uint32_t dwColor)
{
    if (m_bHideAction)
    {
        m_nHideActionBeforeColor = static_cast<std::int32_t>(dwColor);
        return;
    }

    if (m_pLayerFace) m_pLayerFace->put_color(dwColor);
    if (m_pLayerOverFace) m_pLayerOverFace->put_color(dwColor);
    if (m_pLayerUnderFace) m_pLayerUnderFace->put_color(dwColor);
    if (m_pLayerDefaultWing) m_pLayerDefaultWing->put_color(dwColor);
    if (m_pLayerKaiserWing) m_pLayerKaiserWing->put_color(dwColor);
    if (m_pLayerKaiserTail) m_pLayerKaiserTail->put_color(dwColor);
}

// ============================================================================
// SetSitEmotion
// ============================================================================

void Avatar::SetSitEmotion(std::int32_t nEmotion)
{
    if (static_cast<std::uint32_t>(nEmotion) > 0x26)
    {
        // Out of range — use default emotion
        const auto nDefault = m_nDefaultEmotion;
        m_bSitEmotion = false;
        m_nSitEmotion = nDefault;
        SetEmotion(nDefault, -1);
    }
    else
    {
        m_bSitEmotion = true;
        m_nSitEmotion = nEmotion;
        SetEmotion(nEmotion, -1);
    }
}

// ============================================================================
// IsDoingEquipedEmotion / SetEquipedEmotion
// ============================================================================

bool Avatar::IsDoingEquipedEmotion() const
{
    return m_nEquipedEmotion >= 0;
}

void Avatar::SetEquipedEmotion(std::int32_t nEmotion)
{
    auto nResult = nEmotion;
    if (static_cast<std::uint32_t>(nEmotion - 1) > 0x25)
        nResult = m_nDefaultEmotion;
    m_nEquipedEmotion = nResult;
    SetEmotion(nResult, -1);
}

// ============================================================================
// DoLevitationAction / StopLevitationAction
// ============================================================================

void Avatar::DoLevitationAction()
{
    const auto tCur = get_update_time();

    // Accumulate flow time from delta
    if (m_tLevitationLastUpdateTime)
        m_tLevitationFlowTime += tCur - m_tLevitationLastUpdateTime;

    // Wrap at 1200ms period
    if (m_tLevitationFlowTime > 1200)
        m_tLevitationFlowTime -= 1200;

    // Reset origin (clear previous relative offset)
    if (m_pOrigin)
        m_pOrigin->RelMove(0, 0);

    // Sinusoidal oscillation: sin(t/1200 * 2π) * 4
    constexpr double kPeriod = 1200.0;
    constexpr double kAmplitude = 4.0;
    const double t = static_cast<double>(m_tLevitationFlowTime) / kPeriod;
    const auto dy = static_cast<std::int32_t>(
        std::sin(t * 2.0 * M_PI) * kAmplitude + 0.5);

    if (m_pOrigin)
        m_pOrigin->RelOffset(0, dy);

    m_tLevitationLastUpdateTime = tCur;
}

void Avatar::StopLevitationAction()
{
    m_tLevitationFlowTime = 0;
    m_tLevitationLastUpdateTime = 0;
}

// ============================================================================
// UpdateAlbatross
// ============================================================================

void Avatar::UpdateAlbatross()
{
    if (!m_pAlbatrossInfo
        || GetOneTimeAction() > static_cast<CharacterAction>(-1))
    {
        return;
    }

    if (m_pAlbatrossInfo->bToRemove)
    {
        // Remove albatross and rebuild layers
        m_pAlbatrossInfo.reset();
        ClearActionLayer(0);
        PrepareActionLayer(6, 120, 0, 0);
        PrepareFaceLayer(-1);
        return;
    }

    if (!m_pAlbatrossInfo->bApplied)
    {
        // First application — rebuild layers with albatross
        m_pAlbatrossInfo->bApplied = true;
        ClearActionLayer(0);
        PrepareActionLayer(6, 120, 0, 0);
        PrepareFaceLayer(-1);
    }
}

// ============================================================================
// UpdateBattlePvP
// ============================================================================

void Avatar::UpdateBattlePvP(std::int32_t nAction)
{
    if (!m_nBattlePvPPAvatar)
        return;

    // BattlePvP action range: BattlepvpManjiWalk(1051)–BattlepvpLeemalnyunDestroy(1151)
    if (nAction >= static_cast<std::int32_t>(CharacterAction::No)
        || static_cast<std::uint32_t>(
               nAction - static_cast<std::int32_t>(CharacterAction::BattlepvpManjiWalk)) > 0x64)
    {
        return;
    }

    auto* ai = GetActionInfo();
    const auto& ad = s_aCharacterActionData[nAction];

    if (is_battle_pvp_dead_action(nAction))
    {
        // On frame 1, load death effect animation
        if (ai->nCurFrameIndex == 1
            && (m_dwBattlePvPInvisibleAction & 1) == 0)
        {
            // TODO: CAnimationDisplayer::LoadLayer/RegisterOneTimeAnimation
            // UOL: "Effect/PvPEff.img/die/PVPA%d_die" with m_nBattlePvPPAvatar
        }

        // At last frame, mark as invisible
        const auto nPieceCount =
            static_cast<std::int32_t>(ad.m_aPieces.size());
        if (ai->nCurFrameIndex == nPieceCount - 1)
            m_dwBattlePvPInvisibleAction |= 1u;
    }
    else
    {
        // Clear dead-invisible flag
        m_dwBattlePvPInvisibleAction &= ~1u;
    }

    // Hide action check (BattlepvpDraklordBat = 1088)
    if (nAction == static_cast<std::int32_t>(
            CharacterAction::BattlepvpDraklordBat))
    {
        const auto nPieceCount =
            static_cast<std::int32_t>(ad.m_aPieces.size());
        const auto idx = ai->nCurFrameIndex;
        if (idx >= 0 && idx < nPieceCount)
        {
            // Piece action 33 = Blink → invisible
            if (ad.m_aPieces[idx].m_nAction
                == static_cast<std::int32_t>(CharacterAction::Blink))
            {
                m_dwBattlePvPInvisibleAction |= 2u;
            }
            else
            {
                m_dwBattlePvPInvisibleAction &= ~2u;
            }
        }
    }
    else
    {
        if (m_dwBattlePvPInvisibleAction & 2)
            m_dwBattlePvPInvisibleAction &= ~2u;
    }
}

// ============================================================================
// ResetActionAniInfo
// ============================================================================

void Avatar::ResetActionAniInfo()
{
    m_pActionAni.reset();
    m_sActionAniUOL.clear();
    m_nActionAniDelayRate = 1000;
    m_nActionAniPlayCount = -1;
}

// ============================================================================
// UpdateFinalization — common finalization extracted from Update
// ============================================================================

void Avatar::UpdateFinalization(CharacterAction nAction, std::int32_t tCur)
{
    // --- Cube origin animation ---
    if (m_pCubeOrigin)
    {
        auto nCubeX = (m_nMoveAction & 1) ? 50 : -50;
        m_pCubeOrigin->RelMove(nCubeX, -50);
    }

    // --- Hide action check ---
    const auto nActionInt = static_cast<std::int32_t>(nAction);
    ActionData* pActionData = nullptr;
    if (nActionInt >= 0
        && nActionInt < static_cast<std::int32_t>(ACTIONDATA_COUNT))
    {
        pActionData = &s_aCharacterActionData[nActionInt];
    }

    const bool bHasOneTime = (GetOneTimeAction() > static_cast<CharacterAction>(-1));
    auto& activeAI = m_aiAction[bHasOneTime ? 1 : 0];
    auto nFrameIdx = activeAI.nCurFrameIndex;

    // ExtendFrame mapping
    if (pActionData)
    {
        auto nOrigCount = static_cast<std::int32_t>(pActionData->m_aPieces.size());
        if (!activeAI.aFrameDelay.empty()
            && nOrigCount > 0
            && nOrigCount < static_cast<std::int32_t>(activeAI.aFrameDelay.size())
            && activeAI.aFrameDelay.size() % nOrigCount == 0)
        {
            nFrameIdx /= activeAI.GetFrameMultipleCountOf(nOrigCount);
        }
    }

    if (pActionData && nFrameIdx >= 0
        && nFrameIdx < static_cast<std::int32_t>(pActionData->m_aPieces.size()))
    {
        auto& piece = pActionData->m_aPieces[nFrameIdx];
        if (static_cast<std::int32_t>(piece.m_nAction) ==
            static_cast<std::int32_t>(CharacterAction::Hide))
        {
            if (!m_bHideAction)
            {
                if (m_pLayerFace)
                    m_nHideActionBeforeColor =
                        static_cast<std::int32_t>(m_pLayerFace->get_color());
                SetLayerColor(0x00FFFFFF);
                m_bHideAction = true;
            }
        }
        else if (m_bHideAction)
        {
            m_bHideAction = false;
            SetLayerColor(static_cast<std::uint32_t>(m_nHideActionBeforeColor));
        }
    }

    // --- Emotion handling ---
    bool bEmotionHandled = false;
    if (pActionData && nFrameIdx >= 0
        && nFrameIdx < static_cast<std::int32_t>(pActionData->m_aPieces.size()))
    {
        auto nEmotion = pActionData->m_aPieces[nFrameIdx].m_nEmotion;
        if (!m_bIgnoreEmotionByAction && nEmotion >= 0
            && nEmotion != m_nEmotion)
        {
            SetEmotion(nEmotion,
                pActionData->m_aPieces[nFrameIdx].m_nFrameDelay);
            bEmotionHandled = true;
        }
    }

    if (!bEmotionHandled)
    {
        // Emotion expiry / blink logic
        if (m_nEmotion != 0)
        {
            if (tCur - m_tEmotionEnd > 0)
            {
                if (m_bSitEmotion)
                    SetSitEmotion(m_nSitEmotion);
                else if (m_bRidingEmotion)
                    SetRidingEmotion(m_nRidingVehicleID,
                        GetCurrentAction(nullptr, false));
                else if (IsGroupEffectEmotion())
                    SetGroupEffectEmotion();
                else if (IsDoingEquipedEmotion())
                    SetEquipedEmotion(m_nEquipedEmotion);
                else
                    SetEmotion(0, -1);

                if (m_bIgnoreEmotionByAction)
                    m_bIgnoreEmotionByAction = false;
            }
        }
        else if (m_bBlinking)
        {
            if (m_pLayerFace && !m_pLayerFace->get_animationState())
            {
                m_pLayerFace->ShiftCanvas(1);
                RegisterNextBlink();
            }
        }
        else if (tCur - m_tNextBlink > 0)
        {
            auto nBlinkCount = (std::rand() % 3) + 1;
            for (int i = 0; i < nBlinkCount; ++i)
            {
                if (m_pLayerFace)
                    m_pLayerFace->Animate(Gr2DAnimationType::Wait);
            }
            m_bBlinking = true;
        }
    }

    // --- Position snapshot ---
    m_ptPosPrev.x = m_ptPos.x.Get();
    m_ptPosPrev.y = m_ptPos.y.Get();

    if (m_pRawOrigin)
    {
        std::int32_t snapX = 0, snapY = 0;
        m_pRawOrigin->GetSnapshot(&snapX, &snapY,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        m_ptPos.x = snapX;
        m_ptPos.y = snapY;
    }

    // --- Levitation ---
    {
        auto nCurAction = GetCurrentAction(nullptr, false);
        bool bIsLevitationAction =
            (nCurAction == CharacterAction::Fly2
                || nCurAction == CharacterAction::PinkbeanFly);

        bool bLevitationExempt = false;
        if (bIsLevitationAction)
        {
            auto morphID = m_dwMorphTemplateID;
            if (morphID == 1000 || morphID == 1001
                || morphID == 1100 || morphID == 1101
                || is_vehicle(m_nRidingVehicleID))
            {
                bLevitationExempt = true;
            }
        }

        if (bIsLevitationAction && !bLevitationExempt)
            DoLevitationAction();
        else if (m_tLevitationFlowTime)
            StopLevitationAction();
    }

    // --- Action animation layer ---
    {
        const bool bHasOTA = (GetOneTimeAction() > static_cast<CharacterAction>(-1));
        auto& ai = m_aiAction[bHasOTA ? 1 : 0];

        if (ai.nCurFrameIndex == 0)
        {
            if (!m_sActionAniUOL.empty() && !m_pActionAni)
            {
                if (m_nActionAniPlayCount != 0)
                {
                    // TODO: Load action animation layer via AnimationDisplayer::LoadLayer
                    // using m_sActionAniUOL, m_pOrigin, m_pLayerOverCharacter
                    // Then Animate with m_nActionAniDelayRate
                    // Decrement m_nActionAniPlayCount if != -1
                }
                else
                {
                    ResetActionAniInfo();
                }
            }
        }
    }

    // --- Forced origin Z ---
    if (m_bForcedOrigin && m_pLayerUnderFace)
    {
        if (m_pLayerUnderFace->get_z() != m_nForcedOriginZ)
            m_pLayerUnderFace->put_z(m_nForcedOriginZ);
    }

    // --- Albatross update ---
    UpdateAlbatross();

    // --- BattlePvP invisible ---
    if (m_dwBattlePvPInvisibleAction)
        SetVisibleMan(false);
}

// ============================================================================
// Update — from decompiled CAvatar::Update @ 0x6006c0
//
// Per-frame tick: advance animation state, update origin vectors,
// handle timers, emotion, blinking, levitation, action animation.
// ============================================================================

void Avatar::Update()
{
    // --- Early exit if avatar is still loading ---
    if (m_bDelayedLoad)
        return;

    const auto tCur = get_update_time();

    // --- FaceOff check ---
    // If faceOff is active and not in game direction mode, clear it
    if (m_bFaceOff)
    {
        // TODO: check CWvsContext::m_bInGameDirectionMode
        m_bFaceOff = false;
        m_nFaceOffItemID = 0;
        PrepareFaceLayer(-1);
    }

    // --- Get current action and validate range ---
    std::int32_t nDir = 0;
    auto nAction = GetCurrentAction(&nDir, false);

    if (nAction < CharacterAction::Walk1 || nAction >= CharacterAction::No)
    {
        if (!m_dwMorphTemplateID || !MorphTemplate::IsKaiserDragon(m_dwMorphTemplateID))
            return;
        nAction = CharacterAction::Walk2;
    }

    // --- Timer: alert remain ---
    if (m_tAlertRemain > 0)
    {
        m_tAlertRemain -= 30;
        if (m_tAlertRemain < 0)
        {
            auto nMA = m_nMoveAction;
            m_tAlertRemain = 0;
            // If standing in alert (nMA & ~1 == 8), switch to walk stand
            if ((nMA & ~1) == 8)
                SetMoveAction((nMA & 1) | 4, 0);
        }
    }

    // --- Timer: repeat one-time action ---
    if (m_tRepeatOneTimeActionEnd && tCur - m_tRepeatOneTimeActionEnd > 0)
    {
        m_bRepeatOneTimeAction = false;
        m_tRepeatOneTimeActionEnd = 0;
        m_bRepeatOneTimeActionMove = false;
    }

    // --- Timer: shield attack blink ---
    if (m_pLayerShieldAttack)
    {
        if (m_tShieldAttack && tCur - m_tShieldAttack > 0)
        {
            ++m_uShieldAttackLayerStateCount;
            // Blink: alternate every 3 ticks between full white and half-transparent
            std::uint32_t dwColor = 0xFFFFFFFF;
            if (m_uShieldAttackLayerStateCount % 6 < 3)
                dwColor = 0x80FFFFFF;
            m_pLayerShieldAttack->put_color(dwColor);
        }
    }

    // =======================================================================
    // Branch: Morph vs Non-Morph
    // =======================================================================

    if (m_dwMorphTemplateID)
    {
        // --- Morph path ---
        // Morph uses a separate animation frame system (aaMorphAction)
        // which is not yet implemented in the action info system.
        // TODO: implement morph frame update from decompiled code @ 0x600860-0x600ae0
        // For now, fall through to the common finalization path.

        // Morph frame remain decrement
        const bool bOneTimeMorph = (GetOneTimeAction() > static_cast<CharacterAction>(-1));
        m_aiAction[bOneTimeMorph ? 1 : 0].tCurFrameRemain -= 30;

        // Cube origin update for morph path
        if (m_pCubeOrigin)
        {
            auto nCubeX = (m_nMoveAction & 1) ? 50 : -50;
            m_pCubeOrigin->RelMove(nCubeX, -50);
        }

        UpdateFinalization(nAction, tCur);
        return;
    }

    // --- Non-morph path ---

    // Day of week check: reload appearance if day changed
    // TODO: check CWvsContext direction mode flags
    {
        // TODO: CDayOfWeek check — skipped for now
    }

    // --- Portable chair fixFrameIdx ---
    std::int32_t nChairFixedFrameIdx = -1;
    {
        auto nChairID = GetPortableChairID();
        if (nChairID)
        {
            // TODO: read fixFrameIdx from CItemInfo for the chair item
            // nChairFixedFrameIdx = itemInfo->GetProperty("fixFrameIdx", -1);
        }
    }

    // --- Riding path ---
    if (IsRidingEx())
    {
        if (CharacterFrameUpdate())
            TamingMobFrameUpdate();
        UpdateFinalization(nAction, tCur);
        return;
    }

    // --- Normal character frame decrement ---
    {
        const bool bOneTime = (GetOneTimeAction() > static_cast<CharacterAction>(-1));
        auto& ai = m_aiAction[bOneTime ? 1 : 0];
        ai.tCurFrameRemain -= 30;

        if (ai.tCurFrameRemain > 0 || GetActionInfo()->bCurFrameStop)
        {
            UpdateFinalization(nAction, tCur);
            return;
        }
    }

    // =======================================================================
    // Frame advance
    // =======================================================================
    {
        auto* pAI = GetActionInfo();
        const auto nActionInt = static_cast<std::int32_t>(nAction);

        auto itChar = pAI->aaAction.find(nActionInt);
        auto itTM = pAI->aaTamingMobAction.find(nActionInt);

        auto* pCharFrames = (itChar != pAI->aaAction.end()) ? &itChar->second : nullptr;
        auto* pTMFrames = (itTM != pAI->aaTamingMobAction.end()) ? &itTM->second : nullptr;

        // Back action stillness check: if position hasn't changed, freeze frame
        if (is_back_action(nActionInt, 0)
            && m_ptPosPrev.y.Get() == m_ptPos.y.Get()
            && m_ptPosPrev.x.Get() == m_ptPos.x.Get())
        {
            pAI->tCurFrameRemain = 0;
            UpdateFinalization(nAction, tCur);
            return;
        }

        // Determine frame count from appropriate frame array
        std::int32_t nFrameCount = 0;
        if (!is_vehicle(m_nRidingVehicleID) || m_bSitAction)
        {
            if (pCharFrames)
                nFrameCount = static_cast<std::int32_t>(pCharFrames->size());
        }
        else
        {
            if (pTMFrames)
                nFrameCount = static_cast<std::int32_t>(pTMFrames->size());
        }

        // Advance or set frame index
        const bool bChairFixed = (nChairFixedFrameIdx >= 0);
        if (nChairFixedFrameIdx < 0)
        {
            ++pAI->nCurFrameIndex;
        }
        else
        {
            if (pAI->nCurFrameIndex == nChairFixedFrameIdx)
            {
                UpdateFinalization(nAction, tCur);
                return;
            }
            pAI->nCurFrameIndex = nChairFixedFrameIdx;
        }

        // BattlePvP update
        if (m_nBattlePvPPAvatar)
            UpdateBattlePvP(nActionInt);

        // Check if frame overflowed
        bool bWrapped = false;
        if (pAI->nCurFrameIndex >= nFrameCount)
        {
            if (pAI->nRepeatFrame)
            {
                pAI->nCurFrameIndex = pAI->nRepeatFrame;
                bWrapped = true;
            }
            else
            {
                // End of action
                if (GetOneTimeAction() > static_cast<CharacterAction>(-1)
                    && !m_bRepeatOneTimeAction && !m_bSitAction)
                {
                    // One-time action finished
                    auto nOTA = GetOneTimeAction();
                    ResetOneTimeAction();
                    OnEndFrameOfAction(static_cast<std::int32_t>(nOTA));
                    ActionProcess(nOTA);
                    PrepareActionLayer(6, 120, 0, 0);

                    // Dark tornado / stun with float: apply special movement
                    if (nOTA == CharacterAction::DarktornadoHitPre
                        || nOTA == CharacterAction::DarktornadoHit)
                    {
                        auto& actionData = s_aCharacterActionData[
                            static_cast<std::int32_t>(CharacterAction::DarktornadoHitAfter)];
                        if (!actionData.m_aPieces.empty())
                        {
                            auto& piece = actionData.m_aPieces[0];
                            m_pOrigin->RelMove(piece.m_ptMove.x, piece.m_ptMove.y);
                        }
                    }
                    else if (nOTA == CharacterAction::StunWithFloat)
                    {
                        // TODO: stru_27ABAF8 = some special action data
                    }

                    UpdateFinalization(nAction, tCur);
                    return;
                }

                // Pinkbean job: reset action layer at end of non-one-time action
                if (is_pinkbean_job(m_avatarLook.nJob)
                    && !m_bRepeatOneTimeAction && !m_bSitAction)
                {
                    PrepareActionLayer(6, 120, 0, 0);
                    UpdateFinalization(nAction, tCur);
                    return;
                }

                pAI->nCurFrameIndex = 0;
            }

            // Release action animation layer on wrap
            m_pActionAni.reset();
        }

        // --- Get action data for current action ---
        ActionData* pActionData = nullptr;
        if (nActionInt >= 0
            && nActionInt < static_cast<std::int32_t>(ACTIONDATA_COUNT))
        {
            pActionData = &s_aCharacterActionData[nActionInt];
        }

        // =================================================================
        // Sub-paths: compute body offset per rendering mode
        // =================================================================
        if (m_bSitAction && is_vehicle(m_nRidingVehicleID))
        {
            // --- Sit on vehicle ---
            if (m_pLayerUnderFace) m_pLayerUnderFace->ShiftCanvas(1);
            if (m_pLayerOverFace) m_pLayerOverFace->ShiftCanvas(1);

            if (pTMFrames && !pTMFrames->empty()
                && pCharFrames && m_nCharacterActionFrame >= 0
                && m_nCharacterActionFrame < static_cast<std::int32_t>(pCharFrames->size()))
            {
                auto& tmFrame = (*pTMFrames)[0];
                auto& charFrame = (*pCharFrames)[m_nCharacterActionFrame];
                auto dx = tmFrame->ptNavel.x - charFrame->ptNavel.x;
                auto dy = tmFrame->ptNavel.y - charFrame->ptNavel.y;

                if (m_pLayerUnderCharacter && m_pLayerUnderCharacter->get_flip())
                    dx = -dx;

                m_ptBodyRelMove.x = dx;
                m_ptBodyRelMove.y = dy;
                m_pBodyOrigin->RelMove(
                    m_ptBodyRelMove.x.Get(), m_ptBodyRelMove.y.Get());

                m_rcTamingMobBody = tmFrame->rcBody;
            }
        }
        else if (is_vehicle(m_nRidingVehicleID))
        {
            // --- Vehicle rider ---
            if (!pTMFrames || pTMFrames->empty())
            {
                UpdateFinalization(nAction, tCur);
                return;
            }

            bool bSpecialRideAction =
                (nActionInt == static_cast<std::int32_t>(CharacterAction::Ride2)
                 || nActionInt == static_cast<std::int32_t>(CharacterAction::Getoff2)
                 || nActionInt == static_cast<std::int32_t>(CharacterAction::Getoff3)
                 || nActionInt == static_cast<std::int32_t>(CharacterAction::TankGetoff2)
                 || nActionInt == static_cast<std::int32_t>(CharacterAction::TankRide2));

            if (bSpecialRideAction)
            {
                if (m_pLayerUnderFace) m_pLayerUnderFace->ShiftCanvas(1);
                if (m_pLayerOverFace) m_pLayerOverFace->ShiftCanvas(1);
                if (m_pLayerShadowPartner) m_pLayerShadowPartner->ShiftCanvas(1);
                ++m_nCharacterActionFrame;
            }

            if (m_pLayerUnderCharacter) m_pLayerUnderCharacter->ShiftCanvas(1);
            if (m_pLayerOverCharacter) m_pLayerOverCharacter->ShiftCanvas(1);
            if (m_pLayerJaguarCannon) m_pLayerJaguarCannon->ShiftCanvas(1);

            if (!pCharFrames || pCharFrames->empty())
            {
                UpdateFinalization(nAction, tCur);
                return;
            }

            auto nTMIdx = pAI->nCurFrameIndex;
            if (nTMIdx >= 0 && nTMIdx < static_cast<std::int32_t>(pTMFrames->size())
                && m_nCharacterActionFrame >= 0
                && m_nCharacterActionFrame < static_cast<std::int32_t>(pCharFrames->size()))
            {
                auto& tmFrame = (*pTMFrames)[nTMIdx];
                auto& charFrame = (*pCharFrames)[m_nCharacterActionFrame];
                auto dx = tmFrame->ptNavel.x - charFrame->ptNavel.x;
                auto dy = tmFrame->ptNavel.y - charFrame->ptNavel.y;

                if (m_pLayerUnderCharacter && m_pLayerUnderCharacter->get_flip())
                    dx = -dx;

                m_ptBodyRelMove.x = dx;
                m_ptBodyRelMove.y = dy;
                m_pBodyOrigin->RelMove(
                    m_ptBodyRelMove.x.Get(), m_ptBodyRelMove.y.Get());

                m_rcTamingMobBody = tmFrame->rcBody;
            }
        }
        else
        {
            // --- Normal character (no vehicle) ---
            if (!pCharFrames || pCharFrames->empty())
            {
                UpdateFinalization(nAction, tCur);
                return;
            }

            std::int32_t nShiftCount = 1;
            if (bWrapped)
                nShiftCount = pAI->nRepeatFrame + 1;

            if (m_pLayerUnderFace) m_pLayerUnderFace->ShiftCanvas(nShiftCount);
            if (m_pLayerOverFace) m_pLayerOverFace->ShiftCanvas(nShiftCount);
            if (m_pLayerShadowPartner) m_pLayerShadowPartner->ShiftCanvas(nShiftCount);

            auto ptBodyRel = GetPortableChairPtBodyRelMove();
            if (GetFieldSeatID() != -1)
            {
                // TODO: get ptBodyRelMove from CField seat data
            }

            auto brX = ptBodyRel.x;
            if (m_pLayerUnderFace && m_pLayerUnderFace->get_flip())
                brX = -brX;

            m_ptBodyRelMove.x = brX;
            m_ptBodyRelMove.y = ptBodyRel.y;
            m_pBodyOrigin->RelMove(
                m_ptBodyRelMove.x.Get(), m_ptBodyRelMove.y.Get());
        }

        // =================================================================
        // Common post-frame-advance logic
        // =================================================================

        // Determine character action frame index
        std::int32_t nCharFrame;
        if (is_vehicle(m_nRidingVehicleID)
            || nActionInt == static_cast<std::int32_t>(CharacterAction::Ride2)
            || nActionInt == static_cast<std::int32_t>(CharacterAction::Getoff2)
            || nActionInt == static_cast<std::int32_t>(CharacterAction::TankRide2)
            || nActionInt == static_cast<std::int32_t>(CharacterAction::TankGetoff2)
            || nActionInt == static_cast<std::int32_t>(CharacterAction::Getoff3))
        {
            nCharFrame = m_nCharacterActionFrame;
        }
        else
        {
            nCharFrame = pAI->nCurFrameIndex;
        }

        // Get character frame entry
        CharacterActionFrameEntry* pCurFrame = nullptr;
        if (pCharFrames && nCharFrame >= 0
            && nCharFrame < static_cast<std::int32_t>(pCharFrames->size()))
        {
            pCurFrame = (*pCharFrames)[nCharFrame].get();
        }

        // Add frame delay to remain timer
        if (!bChairFixed && pAI->nCurFrameIndex >= 0
            && pAI->nCurFrameIndex < static_cast<std::int32_t>(pAI->aFrameDelay.size()))
        {
            pAI->tCurFrameRemain += pAI->aFrameDelay[pAI->nCurFrameIndex];
        }

        // Extended frame: map to original frame index
        if (pActionData)
        {
            auto nOrigCount = static_cast<std::int32_t>(pActionData->m_aPieces.size());
            if (pAI->IsExtendFrame(nOrigCount))
            {
                nCharFrame = pAI->nCurFrameIndex
                    / pAI->GetFrameMultipleCountOf(nOrigCount);
            }
        }

        // Use sit action frame if sitting
        CharacterActionFrameEntry* pFrameForBrow = pCurFrame;
        if (m_bSitAction && pCharFrames && pAI->nCurFrameIndex >= 0
            && pAI->nCurFrameIndex < static_cast<std::int32_t>(pCharFrames->size()))
        {
            pFrameForBrow = (*pCharFrames)[pAI->nCurFrameIndex].get();
        }

        // --- Flip handling ---
        if (m_pLayerUnderFace)
            m_pLayerUnderFace->put_flip(m_bFlip ? 1 : 0);

        // XOR with action piece flip
        if (pActionData && nCharFrame >= 0
            && nCharFrame < static_cast<std::int32_t>(pActionData->m_aPieces.size()))
        {
            auto& piece = pActionData->m_aPieces[nCharFrame];
            auto nCurFlip = m_pLayerUnderFace ? m_pLayerUnderFace->get_flip() : 0;
            if (m_pLayerUnderFace)
                m_pLayerUnderFace->put_flip(piece.m_bFlip ^ nCurFlip);

            // Direction fix
            auto nDirFix = piece.m_nDirectionFix;
            if (nDirFix > 0 && nDirFix - 1 != nDir)
            {
                if (nDirFix == 1)
                {
                    if (m_pLayerUnderFace)
                        m_pLayerUnderFace->put_flip(1);
                }
                else if (nDirFix == 2)
                {
                    if (m_pLayerUnderFace)
                        m_pLayerUnderFace->put_flip(0);
                }
            }
        }

        // Sync flip to other layers
        if (m_pLayerUnderFace)
        {
            auto nFlip = m_pLayerUnderFace->get_flip();
            if (m_pLayerOverFace) m_pLayerOverFace->put_flip(nFlip);
            if (m_pLayerFace) m_pLayerFace->put_flip(nFlip);
        }

        // --- Face origin RelMove ---
        if (pFrameForBrow)
        {
            auto browX = static_cast<std::int32_t>(pFrameForBrow->ptBrow.x);
            auto browY = static_cast<std::int32_t>(pFrameForBrow->ptBrow.y);

            // If no rotation and face is flipped, negate brow X
            bool bShowFace = false;
            if (pActionData && nCharFrame >= 0
                && nCharFrame < static_cast<std::int32_t>(pActionData->m_aPieces.size()))
            {
                bShowFace = (pActionData->m_aPieces[nCharFrame].m_nRotate == 0);
            }
            if (bShowFace && m_pLayerFace && m_pLayerFace->get_flip())
                browX = -browX;

            m_pFaceOrigin->RelMove(browX, browY);
        }

        // --- Face alpha origin from body ---
        if (pActionData && nCharFrame >= 0
            && nCharFrame < static_cast<std::int32_t>(pActionData->m_aPieces.size()))
        {
            auto& piece = pActionData->m_aPieces[nCharFrame];

            if (piece.m_bShowFace && m_pLayerUnderFace)
            {
                // Set face alpha origin to body alpha
                auto* pBodyAlpha = m_pLayerUnderFace->get_alpha();
                if (pBodyAlpha && m_pLayerFace)
                {
                    auto* pFaceAlpha = m_pLayerFace->get_alpha();
                    if (pFaceAlpha)
                        pFaceAlpha->PutOrigin(pBodyAlpha);
                }
            }

            // --- Origin RelMove (0,0) to reset, then RelOffset for movement ---
            if (GetCurrentAction(nullptr, false) != CharacterAction::Fly2
                && GetCurrentAction(nullptr, false) != CharacterAction::PinkbeanFly
                && m_ptForcedMove.x == 0 && m_ptForcedMove.y == 0
                && !m_bForcedOrigin)
            {
                m_pOrigin->RelMove(0, 0);

                // Apply piece movement offset
                auto moveX = piece.m_ptMove.x;
                auto moveY = piece.m_ptMove.y;

                if (piece.m_nRotate == 0 && m_bFlip)
                    moveX = -moveX;

                m_pOrigin->RelOffset(moveX, moveY);
            }

            // Muzzle flip X from raw origin if face flipped and has rotation
            if (m_pLayerFace && m_pLayerFace->get_flip() && piece.m_nRotate)
            {
                std::int32_t nRawX = 0;
                m_pRawOrigin->GetSnapshot(&nRawX, nullptr,
                    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
                m_pOrigin->PutFlipX(nRawX);
            }

            // Set rotation angle on origin
            m_pOrigin->PutRA(static_cast<double>(piece.m_nRotate));

            // Reset face alpha RelMove
            if (m_pLayerFace)
            {
                auto* pFaceAlpha = m_pLayerFace->get_alpha();
                if (pFaceAlpha)
                    pFaceAlpha->RelMove(0, 0);
            }
        }

        // --- Muzzle origin ---
        if (m_pLayerMuzzle && pCurFrame)
        {
            if (m_pLayerMuzzle)
                m_pLayerMuzzle->put_flip(m_bFlip ? 1 : 0);
        }

        if (pCurFrame)
        {
            auto muzzleX = pCurFrame->ptMuzzle.x;
            auto muzzleY = pCurFrame->ptMuzzle.y;
            if (m_bFlip) muzzleX = -muzzleX;
            m_pMuzzleOrigin->RelMove(muzzleX, muzzleY);
        }

        // --- TM origins (vehicle) ---
        if (is_vehicle(m_nRidingVehicleID) && !m_bSitAction && pTMFrames)
        {
            auto nTMIdx = pAI->nCurFrameIndex;
            if (nTMIdx >= 0 && nTMIdx < static_cast<std::int32_t>(pTMFrames->size()))
            {
                auto& tmFrame = (*pTMFrames)[nTMIdx];

                // TM navel origin
                auto navelX = tmFrame->ptNavel.x;
                if (m_bFlip) navelX = -navelX;
                m_pTMNavelOrigin->RelMove(navelX, tmFrame->ptNavel.y);

                // TM head origin
                auto headX = tmFrame->ptHead.x;
                if (m_bFlip) headX = -headX;
                m_pTMHeadOrigin->RelMove(headX, tmFrame->ptHead.y);

                // TM muzzle origin
                auto tmMuzzleX = tmFrame->ptMuzzle.x;
                if (m_bFlip) tmMuzzleX = -tmMuzzleX;
                m_pTMMuzzleOrigin->RelMove(tmMuzzleX, tmFrame->ptMuzzle.y);
            }
        }

        // --- Hand origin ---
        if (pCurFrame)
        {
            auto handX = pCurFrame->ptHand.x;
            auto handY = pCurFrame->ptHand.y;
            if (m_bFlip) handX = -handX;
            m_pHandOrigin->RelMove(handX, handY);
        }

        // --- Tail origin ---
        if (pCurFrame)
        {
            auto tailX = pCurFrame->ptTail.x;
            auto tailY = pCurFrame->ptTail.y;
            if (m_bFlip) tailX = -tailX;
            m_pTailOrigin->RelMove(tailX, tailY);
        }
    }

    UpdateFinalization(nAction, tCur);
}

} // namespace ms
