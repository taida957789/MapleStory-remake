#pragma once

#include "app/IGObj.h"
#include "graphics/WzGr2DTypes.h"
#include "util/Point.h"
#include "util/Singleton.h"
#include "util/ZXString.h"

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class Gr2DVector;
class WzCanvas;
class WzGr2DLayer;
class WzProperty;

class AnimationDisplayer : public IGObj, public Singleton<AnimationDisplayer>
{
    friend class Singleton<AnimationDisplayer>;

public:
    // ========== Nested Types ==========

    /// CAnimationDisplayer::PREPAREINFO
    class PrepareInfo
    {
    public:
        std::uint32_t dwCharacterID{};
        std::uint32_t dwCharacterIDForFlip{};
        std::shared_ptr<WzGr2DLayer> pFlipLayer;
        std::vector<std::shared_ptr<WzGr2DLayer>> apLayer;
        std::int32_t tReservedRemoveTime{};

        auto Update(std::int32_t tCur) -> bool;
    };

    /// CAnimationDisplayer::TrembleCtx
    class TrembleCtx
    {
    public:
        void Update(std::int32_t tCur);

        double m_dTrembleForce{};
        double m_dTrembleReduction{};
        std::int32_t m_tTrembleStart{};
        std::int32_t m_tTrembleEnd{};
        std::int32_t m_tTrembleTerm{};
        std::int32_t m_tTrembleLastUpdate{};
        Point2D m_ptCenterRel;  ///< Center vector's (rx, ry) saved at tremble start
    };

    /// CAnimationDisplayer::ABSORBITEM
    class AbsorbItem
    {
    public:
        std::uint32_t dwCharacterID{};
        std::uint32_t dwMobID{};
        Point2D pt;
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::int32_t tStarted{};
        std::int32_t nPetIdx{};
        std::int32_t tStartDelay{};
    };

    /// CAnimationDisplayer::ABSORBSOUL
    class AbsorbSoul
    {
    public:
        std::uint32_t dwCharacterID{};
        Point2D pt;
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::int32_t tStarted{};
    };

    /// CAnimationDisplayer::ACCELERATION
    class Acceleration
    {
    public:
        std::int32_t tStart{};
        std::int32_t tEnd{};
        Point2D ptEnd;
        float fMaxDistance{};
        float fAngle{};
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::unique_ptr<Gr2DVector> pOrigin;
        Point2D ptLast;
        std::int32_t tNextUpdateTime{};
        std::int32_t nUpdateTickTime{};
        std::int32_t nUpdateTickCount{};
        float fLapsedDistance{};
        std::int32_t nFirstSection{};
        float fTolerance{};
        std::int32_t nUpdateTotalTickCount{};
        std::int32_t nShiftPerTickCount{};
        std::int32_t nShiftExceptCount{};
    };

    /// CAnimationDisplayer::AIMING_EFFECT
    class AimingEffect
    {
    public:
        std::int32_t nState{};
        std::uint32_t dwID{};
        ZXString<wchar_t> sAimUOL;  // original: ZXString<unsigned short>
        std::int32_t tStartTime{};
        std::int32_t tAimStartTime{};
        std::int32_t tAimEndTime{};
        std::unique_ptr<Gr2DVector> pOrigin;
        Point2D ptStart;
        Point2D ptOffset;
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::int32_t tLastMoveTime{};
        std::int32_t tMoveTerm{};
        std::int32_t nRange{};
        std::int32_t nSpeed{};
        std::int32_t nColor{};
    };

    /// CAnimationDisplayer::ANIMATIONINFO
    class AnimationInfo
    {
    public:
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::shared_ptr<WzGr2DLayer> pFlipOrigin;
        std::int32_t bRun{};
        std::int32_t nDelayRate{};
        Gr2DAnimationType eAniType{Gr2DAnimationType::Stop};
        ZXString<char> strKey;
    };

    /// RelOffsetParam (ZRefCounted with offset/time fields)
    struct RelOffsetParam
    {
        std::int32_t nRelOffsetX{};
        std::int32_t nRelOffsetY{};
        std::int32_t tRelOffsetTime{};
    };

    /// CAnimationDisplayer::ONETIMEINFO
    class OneTimeInfo
    {
    public:
        /// CAnimationDisplayer::ONETIMEINFO::MOVINGINFO
        struct MovingInfo
        {
            bool bLeftDirection{};
            std::int32_t nDiv{};
            std::int32_t nForX{};
            std::int32_t nForY{};
        };

        std::shared_ptr<WzGr2DLayer> pLayer;
        std::shared_ptr<WzGr2DLayer> pFlipOrigin;
        std::uint32_t dwOwner{};
        std::int32_t bWaiting{};
        std::int32_t tDelayBeforeStart{};
        std::int32_t nDelayRate{};
        std::int32_t nPrevScale{-1};
        std::int32_t nComboKillCount{};
        std::int32_t nMovingType{};
        std::int32_t nAnimationType{};
        std::int32_t nCurrentTick{};
        std::int32_t nBaseScale{100};
        std::shared_ptr<RelOffsetParam> pRelOffsetParam;
        ZXString<wchar_t> sSoundUOL;  // original: ZXString<unsigned short>
        MovingInfo movingInfo;

        auto Scale(std::int32_t nScale) -> std::int32_t;
    };

    /// CAnimationDisplayer::BONUSABSORBITEM
    class BonusAbsorbItem
    {
    public:
        std::uint32_t dwCharacterID{};
        std::uint32_t dwMobID{};
        Point2D pt;
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::int32_t tStarted{};
        std::int32_t nPetIdx{};
        std::int32_t tStartDelay{};
        Point2D pt2;
    };

    /// CAnimationDisplayer::CHAINLIGHTNINGINFO
    class ChainLightningInfo
    {
    public:
        std::int32_t tStart{};
        std::int32_t tEnd{};
        Point2D pt1;
        std::int32_t z{};
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::string sBallUOL;  // original: Ztl_bstr_t
        std::int32_t nAngle{};
    };

    /// CAnimationDisplayer::EXPLOSIONINFO
    class ExplosionInfo
    {
    public:
        /// CAnimationDisplayer::EXPLOSIONINFO::EXPLOSIONPOSITIONINFO
        struct ExplosionPositionInfo
        {
            std::int32_t nX{};
            std::int32_t nY{};
            std::int32_t nEffectIndex{};
        };

        std::int32_t nX{};
        std::int32_t nY{};
        std::int32_t nWidth{};
        std::int32_t nCurWidth{};
        std::int32_t nHeight{};
        std::int32_t nCurHeight{};
        std::int32_t tUpdateInterval{};
        std::int32_t nUpdateCount{};
        std::int32_t tUpdateNext{};
        std::int32_t tEnd{};
        std::uint32_t dwMobID{};
        std::int32_t nSkillID{};
        std::vector<std::shared_ptr<WzProperty>> apProperty;
        bool bFirstSelectRandom{};
        bool bFadeIn{};
        bool bMinion{};
        std::int32_t nFadeTime{};
        std::shared_ptr<WzGr2DLayer> pOverLay;
        std::int32_t nZ{};
        std::vector<ExplosionPositionInfo> apPositionInfo;
    };

    /// CAnimationDisplayer::FADEINFO
    class FadeInfo
    {
    public:
        /// CAnimationDisplayer::FADEINFO::FADE_TYPE
        enum class FadeType : std::int32_t
        {
            Normal = 0x0,        // FADETYPE_NORMAL
            OverlapDetail = 0x1, // FADETYPE_OVERLAB_DETAIL
            Overlap = 0x2,       // FADETYPE_OVERLAB
        };

        /// FADEINOUT_HIGHLIGHT_TYPE
        enum class HighlightType : std::int32_t
        {
            None = 0x0,   // FADEINOUT_HIGHLIGHT_TYPENONE
            User = 0x1,   // FADEINOUT_HIGHLIGHT_USER
            Mob = 0x2,    // FADEINOUT_HIGHLIGHT_MOB
            Minion = 0x3, // FADEINOUT_HIGHLIGHT_MINION
        };

        /// FADEINOUT_HIGHLIGHT_INFO
        struct HighlightInfo
        {
            HighlightType m_eType{HighlightType::None};
            std::uint32_t m_dwKey{};
            std::uint32_t m_nParam1{};
            std::uint32_t m_nParam2{};
        };

        std::int32_t nType{};
        std::int32_t bNotAutoStartFadeOut{};
        std::int32_t bStartFadeOut{};
        std::int32_t tStartFadeOut{};
        std::int32_t tFadeOut{};
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::list<HighlightInfo> listHighlightInfo;
    };

    /// CAnimationDisplayer::FALLINGINFO
    class FallingInfo
    {
    public:
        std::int32_t bLeft{};
        Rect rcStart;
        std::int32_t nX{};
        std::int32_t nY{};
        std::int32_t nAlpha{};
        std::int32_t tFall{};
        std::int32_t tUpdateInterval{};
        std::int32_t nUpdateCount{};
        std::int32_t tUpdateNext{};
        std::int32_t tEnd{};
        std::vector<std::shared_ptr<WzProperty>> apProperty;
    };

    /// AnimationState (used by ZRef<AnimationState> in FOLLOWINFO)
    class AnimationState
    {
    public:
        std::int32_t bTerminate{};
        std::int32_t bPause{};
        std::int32_t bFlip{};
    };

    /// CAnimationDisplayer::FOLLOWINFO
    class FollowInfo
    {
    public:
        std::vector<std::shared_ptr<WzProperty>> apProperty;
        std::unique_ptr<Gr2DVector> pOrigin;
        std::shared_ptr<WzGr2DLayer> pParentLayer;
        std::vector<std::unique_ptr<Gr2DVector>> apGenPoint;
        Rect rtStart;
        Point2D szOffset0;  // original: tagSIZE (cx, cy)
        Point2D szOffset1;  // original: tagSIZE (cx, cy)
        std::int32_t z{};
        std::int32_t tDelay{};
        std::int32_t tUpdateInterval{};
        std::int32_t bRelPos{};
        std::int32_t bEmission{};
        std::int32_t nTheta{};
        std::int32_t bNoFlip{};
        std::int32_t nRotateSpeed{};
        std::int32_t tUpdateNext{};
        std::int32_t nCurrentAngle{};
        std::int32_t nCurrentGenPointIndex{};
        std::shared_ptr<AnimationState> pAniState;
    };

    /// CAnimationDisplayer::FOOTHOLDINFO
    class FootholdInfo
    {
    public:
        std::int32_t tStart{};
        std::int32_t tEnd{};
        std::int32_t a0{};
        std::int32_t a1{};
        std::int32_t nKey{};
        std::vector<std::shared_ptr<WzGr2DLayer>> apLayer;
        bool bNoRegisterRepeatAnimation{};
    };

    /// CAnimationDisplayer::FOOTHOLDINFO_FOR_SHADOW_RAIN
    class FootholdInfoForShadowRain
    {
    public:
        std::int32_t tStart{};
        std::int32_t tEnd{};
        std::int32_t nState{};
        std::shared_ptr<WzGr2DLayer> pLayerPre;
        std::shared_ptr<WzGr2DLayer> pLayerLoop;
        std::shared_ptr<WzGr2DLayer> pLayerEnd;
        bool bSoundPlay{};
        std::uint32_t dwSoundCookie{};
        std::int32_t nSkillID{};
    };

    /// CAnimationDisplayer::HOOKING_CHAIN_INFO
    class HookingChainInfo
    {
    public:
        std::int32_t tEnd1{};
        std::int32_t tEnd2{};
        std::int32_t tStartDelay{};
        std::int32_t nChainLength{};
        std::int32_t nStretchSpeed{};
        std::uint32_t dwCharacterID{};
        std::uint32_t dwMobID{};
        Point2D ptUser;
        Point2D ptTarget;
        Point2D ptMobTarget;
        std::int32_t bCatchDone{};
        std::int32_t bLeft{};
        std::int32_t nSkillID{};
        std::string sImageUOL;  // original: Ztl_bstr_t
        std::shared_ptr<WzCanvas> pCanvasChain;
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::shared_ptr<WzGr2DLayer> pHookLayer;
    };

    /// CAnimationDisplayer::HOOKING_CHAIN_INFO_FOR_PVP
    class HookingChainInfoForPvp
    {
    public:
        std::int32_t tEnd1{};
        std::int32_t tEnd2{};
        std::int32_t nChainLength{};
        std::int32_t nStretchSpeed{};
        std::uint32_t dwCharacterID{};
        std::uint32_t dwTargetID{};
        Point2D ptUser;
        Point2D ptTarget;
        std::int32_t bCatchDone{};
        std::int32_t bLeft{};
        std::string sImageUOL;  // original: Ztl_bstr_t
        std::shared_ptr<WzCanvas> pCanvasChain;
        std::shared_ptr<WzCanvas> pCanvasHook;
        std::shared_ptr<WzGr2DLayer> pLayer;
    };

    // ========== Effect Methods ==========

    /// Play a general effect animation (from CAnimationDisplayer::Effect_General).
    void Effect_General(
        const std::string& sUOL,
        std::int32_t nFlip,
        const std::shared_ptr<Gr2DVector>& pOrigin,
        std::int32_t rx, std::int32_t ry,
        const std::shared_ptr<WzGr2DLayer>& pOverlay,
        std::int32_t z,
        std::int32_t nMagLevel);

    // ========== IGObj ==========

    void Update() override;

    /// Per-frame canvas info (from decompiled LAYERCANVASINFOSINGLE)
    struct LayerCanvasInfoSingle
    {
        std::int32_t nDelay{0};
        std::vector<std::pair<Point2D, Point2D>> aptDir;
        bool bView{true};
    };

    /// Output info for a loaded layer (from decompiled LAYERCANVASINFO)
    struct LayerCanvasInfo
    {
        std::int32_t nZ{0};
        std::vector<LayerCanvasInfoSingle> aInfo;
    };

    /// Overload 1: UOL string path - resolves property, delegates to overload 2
    static auto LoadLayer(
        const std::string& layerUOL,
        std::int32_t flip,
        Point2D origin,
        std::int32_t rx, std::int32_t ry,
        std::shared_ptr<WzGr2DLayer> pOverlay,
        std::int32_t z,
        std::int32_t alpha,
        std::int32_t magLevel,
        LayerCanvasInfo* pCanvasInfo,
        std::int32_t nZoom0, std::int32_t nZoom1,
        bool bPostRender
    ) -> std::shared_ptr<WzGr2DLayer>;

    /// Overload 2: Property-based - creates layer, reads properties, loops frames
    static auto LoadLayer(
        const std::shared_ptr<WzProperty>& prop,
        std::int32_t flip,
        Point2D origin,
        std::int32_t rx, std::int32_t ry,
        std::shared_ptr<WzGr2DLayer> pOverlay,
        std::int32_t z,
        std::int32_t alpha,
        std::int32_t magLevel,
        LayerCanvasInfo* pCanvasInfo,
        std::int32_t nZoom0, std::int32_t nZoom1,
        bool bPostRender
    ) -> std::shared_ptr<WzGr2DLayer>;

    /// Overload 1: UOL string path - resolves property, delegates to overload 2
    /// When pLayer is null, delegates to LoadLayer(UOL).
    /// When pLayer exists, resolves UOL to property and delegates.
    static auto InsertLayer(
        std::shared_ptr<WzGr2DLayer>& pLayer,
        const std::string& layerUOL,
        std::int32_t flip,
        Point2D origin,
        std::int32_t rx, std::int32_t ry,
        std::shared_ptr<WzGr2DLayer> pOverlay,
        std::int32_t z,
        std::int32_t alpha,
        std::int32_t magLevel
    ) -> std::shared_ptr<WzGr2DLayer>;

    /// Overload 2: Property-based - inserts frames or creates layer.
    /// When pLayer is null, delegates to LoadLayer.
    /// When pLayer exists, appends numbered frame children from prop.
    static auto InsertLayer(
        std::shared_ptr<WzGr2DLayer>& pLayer,
        const std::shared_ptr<WzProperty>& prop,
        std::int32_t flip,
        Point2D origin,
        std::int32_t rx, std::int32_t ry,
        std::shared_ptr<WzGr2DLayer> pOverlay,
        std::int32_t z,
        std::int32_t alpha,
        std::int32_t magLevel
    ) -> std::shared_ptr<WzGr2DLayer>;

    /// Register a one-time (play-once) animation layer.
    auto RegisterOneTimeAnimation(
        const std::shared_ptr<WzGr2DLayer>& pLayer,
        std::int32_t tDelayBeforeStart,
        const std::shared_ptr<WzGr2DLayer>& pFlipOrigin,
        std::int32_t nDelayRate,
        std::int32_t nMovingType,
        const std::shared_ptr<RelOffsetParam>& pRelOffsetParam,
        const ZXString<wchar_t>& sSoundUOL,
        std::int32_t nComboKillCount,
        std::uint32_t dwOwner
    ) -> OneTimeInfo&;

private:
    void UpdateWeaponHeadEffect(std::int32_t tCur);
    void NonFieldUpdate(std::int32_t tCur);
    void UpdateMoveRandSprayEffect(std::int32_t tCur);
    void UpdateUpDownEffect(std::int32_t tCur);
    void UpdateDelaySetViewEffect();
    void RemovePrepareAnimation(std::uint32_t dwCharacterID);

    /// Insert a single canvas frame into a layer (from decompiled LoadCanvas)
    static void LoadCanvas(
        const std::shared_ptr<WzGr2DLayer>& layer,
        const std::shared_ptr<WzProperty>& frameProp,
        const std::shared_ptr<WzGr2DLayer>& overlay,
        std::int32_t globalZoom0, std::int32_t globalZoom1,
        LayerCanvasInfoSingle* pInfoSingle
    );

    // ========== Members ==========

    TrembleCtx m_tremble;
    std::list<std::shared_ptr<PrepareInfo>> m_lPrepare;
    std::list<OneTimeInfo> m_lOneTime;
    std::map<std::uint32_t, std::shared_ptr<WzGr2DLayer>> m_mBladeMovingEffect;
};

} // namespace ms
