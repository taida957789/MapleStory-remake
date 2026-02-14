#pragma once

#include "user/avatar/AvatarLook.h"
#include "animation/ActionData.h"
#include "enums/CharacterAction.h"
#include "enums/MoveActionType.h"
#include "util/Point.h"
#include "util/security/SecPoint.h"
#include "util/security/ZtlSecureTear.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ms
{

class CharacterActionFrameEntry;
class Gr2DVector;
class InPacket;
class TamingMobActionFrameEntry;
class WzGr2DCanvas;
class WzGr2DLayer;

/**
 * @brief Character avatar — visual representation and action state
 *
 * Based on CAvatar (__cppobj) from the original MapleStory client.
 * Manages all visual layers, morph/riding state, action mapping,
 * origin vectors, and the full rendering pipeline for a character.
 */
class Avatar
{
public:
    /**
     * @brief Per-action rendering state (frame index, speed, layer info)
     *
     * Based on CAvatar::ACTIONINFO from the original MapleStory client.
     * Holds the current animation state for one action slot,
     * including cached frame data for all loaded actions.
     */
    struct ActionInfo
    {
        // --- Current animation parameters ---
        std::int32_t nActionSpeed{0};
        std::int32_t nWalkSpeed{0};
        std::int32_t nKeyDown{0};
        std::int32_t nChangeWeapon{0};

        // --- Per-action cached frame data ---
        // Key: action code (CharacterAction enum value)
        // Value: array of frame entries for that action
        std::unordered_map<std::int32_t,
            std::vector<std::shared_ptr<CharacterActionFrameEntry>>> aaAction;
        std::unordered_map<std::int32_t,
            std::vector<std::shared_ptr<TamingMobActionFrameEntry>>> aaTamingMobAction;

        // --- Per-action alpha ---
        std::unordered_map<std::int32_t, std::int32_t> aAlpha;

        // --- Current frame playback state ---
        std::vector<std::int32_t> aFrameDelay;
        std::vector<std::int32_t> aTMFrameDelay;
        std::int32_t tTotFrameDelay{0};
        std::int32_t nCurFrameIndex{0};
        std::int32_t nCurTMFrameIndex{0};
        std::int32_t tCurFrameRemain{0};
        std::int32_t tCurTMFrameRemain{0};
        std::int32_t nRepeatFrame{0};

        /// Check if action frame data is already loaded for a given action
        [[nodiscard]] bool HasAction(std::int32_t nAction) const
        {
            auto it = aaAction.find(nAction);
            return it != aaAction.end() && !it->second.empty();
        }

        /// Check if taming mob frame data is already loaded for a given action
        [[nodiscard]] bool HasTamingMobAction(std::int32_t nAction) const
        {
            auto it = aaTamingMobAction.find(nAction);
            return it != aaTamingMobAction.end() && !it->second.empty();
        }

        /// Check if frame data uses extended frames (from CharacterImgEntry)
        [[nodiscard]] bool IsExtendFrame(std::int32_t /*nOrigCount*/) const
        {
            // TODO: implement based on equipped item extendFrame flag
            return false;
        }

        /// Get the frame multiplication factor for extended frames.
        [[nodiscard]] std::int32_t GetFrameMultipleCountOf(std::int32_t nOrigCount) const
        {
            if (nOrigCount <= 0) return 1;
            auto nFrameCount = static_cast<std::int32_t>(aFrameDelay.size());
            if (nFrameCount <= 0 || nFrameCount % nOrigCount != 0) return 1;
            return nFrameCount / nOrigCount;
        }

        /// Whether the current frame is held (paused).
        bool bCurFrameStop{false};
    };

    /**
     * @brief Albatross (Cygnus Knight) effect state
     *
     * Based on CAvatar::AlbatrossInfo from the original MapleStory client.
     * Manages the albatross companion rendering for Wind Archer.
     */
    struct AlbatrossInfo
    {
        std::int32_t nState{0};
        std::int32_t nAlbatrossID{0};
        bool bApplied{false};
        bool bToRemove{false};
        std::int32_t nFaceColor{-1};
    };

    virtual ~Avatar() = default;

    // --- Virtual methods (from CAvatar_vtbl) ---
    [[nodiscard]] virtual auto CanUseBareHand() const -> bool { return false; }
    [[nodiscard]] virtual auto IsEvanJob() const -> bool { return false; }
    virtual void OnAvatarModified() {}
    [[nodiscard]] virtual auto GetFieldSeatID() const -> std::int32_t { return -1; }
    [[nodiscard]] virtual auto GetPortableChairID() const -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto GetPortableChairPtBodyRelMove() const -> Point2D { return {0, 0}; }
    virtual void SetPortableChairPtBodyRelMove(Point2D /*pt*/) {}
    [[nodiscard]] virtual auto IsTagUser() const -> bool { return false; }
    virtual void ReflashDemonAvangerWings() {}
    virtual void ReflashKaiserTransformTail() {}
    virtual void SetMoveAction(std::int32_t nMA, std::int32_t bReload);
    virtual void ResetOneTimeAction() {}
    virtual void PrepareActionLayer(
        std::int32_t nActionSpeed,
        std::int32_t nWalkSpeed,
        std::int32_t bKeyDown,
        std::int32_t nGatherToolID
    );
    virtual void UpdateAdditionalLayer() {}
    virtual void OnEndFrameOfAction(std::int32_t nAction) { (void)nAction; }
    virtual void RemoveGroupEffect() {}
    [[nodiscard]] virtual auto IsGroupEffectEmotion() const -> bool { return false; }
    virtual void SetGroupEffectEmotion() {}
    [[nodiscard]] virtual auto IsMovementAvatar() const -> bool { return false; }
    virtual void SetBattlePvPAvatar(std::int32_t nAvatar) { (void)nAvatar; }
    [[nodiscard]] virtual auto GetRolePlayingCharacterIndex() const -> std::int32_t { return -1; }
    virtual void SetRolePlayingCharacterIndex(std::int32_t nIdx) { (void)nIdx; }

    // --- Initialization ---

    /// Public Init: full avatar setup with appearance, move action, etc.
    void Init(
        const AvatarLook& al,
        std::int32_t nMoveAction,
        std::shared_ptr<Gr2DVector> pOrigin,
        std::shared_ptr<WzGr2DLayer> pOverlay,
        std::int32_t z,
        std::int32_t x, std::int32_t y,
        std::int32_t nScale,
        std::int32_t nDefaultEmotion);

    /// Notify that avatar appearance has changed (triggers layer rebuild).
    void NotifyAvatarModified(bool bResetAction);

    // --- Non-virtual methods ---

    /// Convert raw move action (direction + action encoded) to CharacterAction.
    /// pnDir receives the direction bit (nMA & 1) if non-null.
    [[nodiscard]] auto MoveAction2RawAction(
        std::int32_t nMA,
        std::int32_t* pnDir,
        bool bRandom
    ) const -> CharacterAction;

    /// Convert a MoveActionType to a CharacterAction for mechanic tank mode.
    [[nodiscard]] auto MoveAction2RawActionForMechanic(
        MoveActionType nAction,
        std::int32_t nMechanicMode
    ) const -> CharacterAction;

    /// Convert a MoveActionType to a CharacterAction for monster jobs (13000/13100).
    [[nodiscard]] auto MoveAction2RawActionForMonsterJob(
        MoveActionType nAction,
        bool bRandom
    ) const -> CharacterAction;

    /// Convert a MoveActionType to a CharacterAction for BattlePvP avatars.
    [[nodiscard]] auto MoveAction2RawActionForBattlePvP(
        MoveActionType nAction,
        std::int32_t nBattlePvPAvatar
    ) const -> CharacterAction;

    /// Get the current action (combining move action, forced move, one-time action).
    [[nodiscard]] auto GetCurrentAction(std::int32_t* pnDir, bool bRandom) const
        -> CharacterAction;

    /// Get the one-time action, remapped for morph/mechanic/riding state.
    [[nodiscard]] auto GetOneTimeAction() const -> CharacterAction;

    /// Set visibility on all avatar layers (color alpha = 0xFF or 0x00).
    void SetVisibleMan(bool bVisible);

    /// Check if standing idle (for group effects like party aura).
    [[nodiscard]] auto IsGroupEffectON() const -> bool;

    // --- Morph checks ---
    [[nodiscard]] bool IsMonsterMorphed() const;
    [[nodiscard]] bool IsSuperMan() const;
    [[nodiscard]] bool IsIceKnight() const;
    [[nodiscard]] bool IsKaiserDragon() const;

    // --- Additional methods ---
    void SetResistanceRidingMoveAction(std::int32_t nMA, std::int32_t bReload);
    void PrepareFaceLayer(std::int32_t tDuration);
    void SetEmotion(std::int32_t nEmotion, std::int32_t nDuration);

    /// Register next eye blink timer.
    void RegisterNextBlink();

    /// Per-frame update: advance animation, update origins, handle timers.
    void Update();

    /// Get the active ActionInfo slot (slot 1 if one-time action active, else slot 0).
    [[nodiscard]] auto GetActionInfo() -> ActionInfo*;

    /// Frame advance logic for normal character path. Returns true if frame advanced.
    [[nodiscard]] bool CharacterFrameUpdate();

    /// Frame advance logic for taming mob (riding) path.
    void TamingMobFrameUpdate();

    /// Post-action processing (e.g. combo transitions, special dying).
    void ActionProcess(CharacterAction nAction);

    /// Set/clear morph state.
    void SetMorphed(std::uint32_t dwMorphTemplateID);

    /// Set color on all avatar layers.
    void SetLayerColor(std::uint32_t dwColor);

    /// Set emotion for sit action.
    void SetSitEmotion(std::int32_t nEmotion);

    /// Check if currently performing an equipped emotion.
    [[nodiscard]] bool IsDoingEquipedEmotion() const;

    /// Set equipped emotion.
    void SetEquipedEmotion(std::int32_t nEmotion);

    /// Perform levitation oscillation animation.
    void DoLevitationAction();

    /// Stop levitation oscillation.
    void StopLevitationAction();

    /// Update albatross companion (Wind Archer).
    void UpdateAlbatross();

    /// Update BattlePvP visual state.
    void UpdateBattlePvP(std::int32_t nAction);

    /// Common finalization logic for Update (timers, emotion, levitation, etc.).
    void UpdateFinalization(CharacterAction nAction, std::int32_t tCur);

    /// Reset action animation info.
    void ResetActionAniInfo();

    /// Reset character one-time action (clear slot 1).
    void ResetCharacterOneTimeAction();

    /// Reset taming mob one-time action.
    void ResetTamingMobOneTimeAction();

    /// Check if riding Demon Slayer wing mount.
    [[nodiscard]] bool IsRidingDslayerWing() const;

    /// Clear character action layer for a slot.
    void ClearCharacterActionLayer(std::int32_t nSlot);

    /// Clear taming mob action layer for a slot.
    void ClearTamingMobActionLayer(std::int32_t nSlot);

    /// Remove all canvas from layers at z order.
    void AvatarLayerRemoveCanvas(std::int32_t nZ);

    /// Load dark tornado effect layer.
    void LoadDarkTornado();

    /// Get the origin vector (raw or adjusted).
    [[nodiscard]] auto GetOrigin() -> std::shared_ptr<Gr2DVector>&;

protected:
    /// Protected Init: creates origin/layer hierarchy.
    void Init(
        std::shared_ptr<Gr2DVector> pOrigin,
        std::int32_t x, std::int32_t y,
        std::shared_ptr<WzGr2DLayer> pOverlay,
        std::int32_t z);

public:
    // --- PrepareActionLayer sub-functions ---
    void ClearActionLayer(std::int32_t nSlot);
    void PrepareMorphActionLayer(
        std::int32_t nAction, std::int32_t nDir,
        std::int32_t nActionSpeed, std::int32_t bKeyDown);
    void PrepareCharacterActionLayer(
        std::int32_t nActionSpeed, std::int32_t nWalkSpeed,
        std::int32_t bKeyDown, std::int32_t nGatherToolID);
    void PrepareTamingMobActionLayer(
        std::int32_t nActionSpeed, std::int32_t nWalkSpeed,
        std::int32_t bKeyDown);
    void PrepareJaguarCannonLayer();
    void FixCharacterPosition();
    void SetMechanicHUE(std::int32_t nHUE, std::int32_t bForce);
    void LoadMechanicRocket();
    void ApplyScaleAndOffset(
        std::shared_ptr<WzGr2DLayer>& pDstLayer,
        const std::shared_ptr<WzGr2DCanvas>& pSrcCanvas,
        std::int32_t nFrameIndex);
    void GetModifiedAvatarHairEquip(std::int32_t (&aOut)[32]) const;
    void LoadCustomRidingSet(
        std::int32_t nRidingVehicleID,
        std::vector<std::int32_t>& aCustomRiding);
    [[nodiscard]] bool IsRidingEx() const;
    [[nodiscard]] bool IsRidingWildHunterJaguar() const;
    void SetRidingEmotion(std::int32_t nVehicleID, CharacterAction nAction);

    /// Convert a character action for the current riding state.
    [[nodiscard]] auto ConvertCharacterAction(std::int32_t nAction) const -> std::int32_t;

    /// Set flip state on all avatar rendering layers.
    void AvatarLayerFlip(std::int32_t nFlip);

    // ========================================================================
    // Instance members (from decompiled CAvatar struct)
    // ========================================================================

    // --- Dark Tornado ---
    std::int32_t nDarkTornadoSLV{0};

    // --- Special dying ---
    std::int32_t m_nSpecialDyingAction{0};

    // --- Appearance ---
    AvatarLook m_avatarLook;
    AvatarLook m_avatarLookLast;
    std::array<std::int32_t, 32> m_aAvatarHairEquipForced{};
    std::array<std::int32_t, 32> m_aOnlyAvatarHairEquipForced{};
    std::int32_t m_nAvatarFaceForced{0};
    std::int32_t m_nAvatarSkinForced{-1};
    bool m_bDrawElfEarForced{false};
    bool m_bForcingAppearance{false};
    bool m_bNeedUpdateBodyPartEffect{false};

    // --- Weapon/equipment ---
    std::int32_t m_nWeaponItemID{0};
    std::int32_t m_nSubWeaponItemID{0};
    std::int32_t m_nShieldItemID{0};

    // --- Movement/stand type ---
    std::int32_t m_nWalkType{0};
    std::int32_t m_nStandType{0};
    std::int32_t m_nReplacedStandAction{0};
    std::int32_t m_nPose{0};
    std::int32_t m_nForcedStandAction{-1};
    std::int32_t m_nForcedMoveAction{-1};
    std::int32_t m_nAttackActionType{0};

    // --- Secure weapon attack speed (ZtlSecureTear) ---
    ZtlSecureTear<std::int32_t> m_nWeaponAttackSpeed;

    // --- Weapon afterimage ---
    std::string m_sWeaponAfterimage;

    // --- Change/move action ---
    mutable std::int32_t m_nChangeMoveAction{-1};

    // --- Blinking ---
    bool m_bBlinking{false};
    std::int32_t m_tNextBlink{0};

    // --- Emotion ---
    std::int32_t m_tEmotionEnd{0};
    std::int32_t m_nEmotion{-1};
    bool m_bIgnoreEmotionByAction{false};

    // --- Morph ---
    std::uint32_t m_dwMorphTemplateID{0};
    Rect m_rcMorphBody{};

    // --- Ghost ---
    std::int32_t m_nGhostIndex{0};

    // --- Mechanic ---
    std::int32_t m_nMechanicMode{0};
    std::int32_t m_nPrevMechanicMode{0};
    bool m_bRocketBoosterStart{false};
    bool m_bRocketBoosterLoop{false};

    // --- Visibility ---
    bool m_bForcedInvisible{false};

    // --- Riding ---
    std::int32_t m_nRidingVehicleID{0};
    std::int32_t m_nRidingChairID{0};
    Rect m_rcTamingMobBody{};

    // --- Character action frame ---
    std::int32_t m_nCharacterActionFrame{0};
    SecPoint m_ptBodyRelMove;
    bool m_bTamingMobTired{false};

    // --- Forced move ---
    Point2D m_ptForcedMove{0, 0};

    // --- Shield attack ---
    Rect m_rcShieldAttack{};
    std::int32_t m_tShieldAttack{0};
    std::uint32_t m_uShieldAttackLayerStateCount{0};

    // --- Taming mob action ---
    std::int32_t m_nTamingMobOneTimeAction{-1};
    std::int32_t m_nTamingMobAction{2};

    // --- Delayed load ---
    bool m_bDelayedLoad{false};

    // --- Timers ---
    std::int32_t m_tAlertRemain{0};

    // --- Action state ---
    // Original: (CWvsContext::ms_pInstance->m_Data[2156].m_str != 0) | 6
    // = 6 (Stand << 1) or 7 if CWvsContext condition
    std::int32_t m_nMoveAction{6};
    CharacterAction m_nOneTimeAction{static_cast<CharacterAction>(-1)};
    std::int32_t m_nSaveOneTimeActionForPinkbean{-1};
    std::int32_t m_nDefaultEmotion{0};

    // --- Action info (2 slots) ---
    std::array<ActionInfo, 2> m_aiAction{};

    // --- Origin vectors (IWzVector2D → Gr2DVector) ---
    std::shared_ptr<Gr2DVector> m_pRawOrigin;
    std::shared_ptr<Gr2DVector> m_pFakeOrigin;
    std::shared_ptr<Gr2DVector> m_pOrigin;
    std::shared_ptr<Gr2DVector> m_pFaceOrigin;
    std::shared_ptr<Gr2DVector> m_pBodyOrigin;
    std::shared_ptr<Gr2DVector> m_pMuzzleOrigin;
    std::shared_ptr<Gr2DVector> m_pHandOrigin;
    std::shared_ptr<Gr2DVector> m_pTailOrigin;
    std::shared_ptr<Gr2DVector> m_pTMNavelOrigin;
    std::shared_ptr<Gr2DVector> m_pTMHeadOrigin;
    std::shared_ptr<Gr2DVector> m_pTMMuzzleOrigin;

    // --- Rendering layers (IWzGr2DLayer → WzGr2DLayer) ---
    std::shared_ptr<WzGr2DLayer> m_pLayerFace;
    std::shared_ptr<WzGr2DLayer> m_pLayerOverFace;
    std::shared_ptr<WzGr2DLayer> m_pLayerUnderFace;
    std::shared_ptr<WzGr2DLayer> m_pLayerShadowPartner;
    std::shared_ptr<WzGr2DLayer> m_pLayerOverCharacter;
    std::shared_ptr<WzGr2DLayer> m_pLayerUnderCharacter;
    std::shared_ptr<WzGr2DLayer> m_pLayerOverlay;
    std::shared_ptr<WzGr2DLayer> m_pLayerMuzzle;
    std::shared_ptr<WzGr2DLayer> m_pLayerJaguarCannon;
    std::shared_ptr<WzGr2DLayer> m_pLayerRocketBooster;
    std::shared_ptr<WzGr2DLayer> m_pLayerBarrier;
    std::shared_ptr<WzGr2DLayer> m_pLayerCyclone;
    std::shared_ptr<WzGr2DLayer> m_pLayerAR01;
    std::shared_ptr<WzGr2DLayer> m_pLayerDarkTornado;
    std::shared_ptr<WzGr2DLayer> m_pLayerDefaultWing;
    std::shared_ptr<WzGr2DLayer> m_pLayerShieldAttack;
    std::shared_ptr<WzGr2DLayer> m_pLayerKaiserWing;
    std::shared_ptr<WzGr2DLayer> m_pLayerKaiserTail;
    std::shared_ptr<WzGr2DLayer> m_pLayerTransparent;

    // --- Cube origin ---
    std::shared_ptr<Gr2DVector> m_pCubeOrigin;

    // --- Position (SECPOINT — secure) ---
    SecPoint m_ptPos;
    SecPoint m_ptPosPrev;

    // --- Scale/flip ---
    std::int32_t m_nScale{100};
    bool m_bFlip{false};

    // --- Day of week ---
    std::uint16_t m_wLastDayOfWeek{0};

    // --- Skill action ---
    bool m_bSkillAction{false};

    // --- Custom riding (ZArray<long> → vector) ---
    std::vector<std::int32_t> m_aCustomRiding;

    // --- Levitation ---
    std::int32_t m_tLevitationFlowTime{0};
    std::int32_t m_tLevitationLastUpdateTime{0};

    // --- Repeat one-time action ---
    bool m_bRepeatOneTimeAction{false};
    std::int32_t m_tRepeatOneTimeActionEnd{0};
    bool m_bRepeatOneTimeActionMove{false};

    // --- Action animation layer ---
    std::shared_ptr<WzGr2DLayer> m_pActionAni;
    std::wstring m_sActionAniUOL;
    std::int32_t m_nActionAniDelayRate{1000};
    std::int32_t m_nActionAniPlayCount{-1};

    // --- Weapon change effect ---
    std::int32_t m_nChangeWeaponLook{0};
    bool m_bChangeWeaponEffSwitch{false};

    // --- Hide action ---
    std::int32_t m_nHideActionBeforeColor{0};
    bool m_bHideAction{false};

    // --- Larkness (light/dark state) ---
    std::int32_t m_nLarknessState{0};

    // --- Vanshee / effects ---
    bool m_bVansheeMode{false};
    bool m_bHideEffect{false};

    // --- Face off ---
    bool m_bFaceOff{false};
    std::int32_t m_nFaceOffItemID{0};

    // --- Hue/color ---
    std::int32_t m_nHUE{0};
    std::int32_t m_nKaiserMorphRotateHueExtern{0};
    std::int32_t m_nKaiserMorphRotateHueInnner{0};
    bool m_bKaiserMorphPrimiumBlack{false};

    // --- Albatross (Wind Archer companion) ---
    std::shared_ptr<AlbatrossInfo> m_pAlbatrossInfo;

    // --- Sit action ---
    bool m_bSitAction{false};
    std::int32_t m_nSitAction{-1};
    bool m_bSitEmotion{false};
    std::int32_t m_nSitEmotion{-1};

    // --- Riding emotion ---
    bool m_bRidingEmotion{false};
    std::int32_t m_nRidingEmotion{-1};

    // --- Equipped emotion ---
    std::int32_t m_nEquipedEmotion{-1};

    // --- Dance ---
    std::int32_t m_nDanceState{0};

    // --- Forced origin ---
    bool m_bForcedOrigin{false};
    std::int32_t m_nForcedOriginZ{0};

    // --- Walk delay ---
    std::int32_t m_tWalkDelay{0};

    // --- BattlePvP ---
    std::int32_t m_nBattlePvPPAvatar{0};
    long double m_dForcedAddActionDelayRate{0.0L};
    std::uint32_t m_dwBattlePvPInvisibleAction{0};

    // --- Flying skill ---
    std::int32_t m_nIsNewFlyingSkillID{0};
};

} // namespace ms
