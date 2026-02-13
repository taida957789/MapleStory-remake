#pragma once

#include "AbsPosEx.h"
#include "IWzVector2D.h"
#include "RelPosEx.h"
#include "field/CAttrField.h"
#include "life/AttrShoe.h"
#include "life/movement/MovePath.h"
#include "physics/b2Stub.h"
#include "util/Point.h"
#include "util/security/ZtlSecureTear.h"

#include <cstdint>
#include <memory>
#include <queue>

namespace ms
{

class IVecCtrlOwner;
class LadderOrRope;
class StaticFoothold;

/**
 * @brief Velocity controller for physics-based movement
 *
 * Based on CVecCtrl (__cppobj : ZRefCounted, IWzVector2D) from the original
 * MapleStory client. Implements IWzVector2D with physics state: velocity,
 * gravity, friction, and foothold interaction.
 * Used by IVecCtrlOwner (Life subclasses) for field movement.
 */
class VecCtrl : public IWzVector2D
{
public:
    VecCtrl();
    ~VecCtrl() override = default;

    // Non-copyable, movable
    VecCtrl(const VecCtrl&) = delete;
    auto operator=(const VecCtrl&) -> VecCtrl& = delete;
    VecCtrl(VecCtrl&&) noexcept = default;
    auto operator=(VecCtrl&&) noexcept -> VecCtrl& = default;

    // === Inner types ===

    /// CVecCtrl::IMPACTNEXT — post-impact velocity override
    struct ImpactNext
    {
        std::int32_t bValid{};
        std::int32_t nReason{};
        long double vx{};
        long double vy{};
    };

    /// CVecCtrl::unnamed_type_m_falldownNext — pending fall-down state
    struct FalldownNext
    {
        std::int32_t bValid{};
        const StaticFoothold* pfhFallStart{};
    };

    /// CVecCtrl::unnamed_type_m_DragdownNext — pending drag-down state
    struct DragdownNext
    {
        std::int32_t bValid{};
        const StaticFoothold* pfhDragStart{};
        std::int32_t nType{};
    };

    /// CVecCtrl::STATICNEWFLYINGROUTE — flying route waypoint
    struct StaticNewFlyingRoute
    {
        std::int32_t nDir{};
        std::uint32_t nTimeCount{};
    };

    // === IWzShape2D overrides ===
    [[nodiscard]] auto GetX() -> std::int32_t override;
    [[nodiscard]] auto GetY() -> std::int32_t override;
    void PutX(std::int32_t x) override;
    void PutY(std::int32_t y) override;
    void Move(std::int32_t x, std::int32_t y) override;
    void Offset(std::int32_t dx, std::int32_t dy) override;
    void Scale(std::int32_t sx, std::int32_t divx,
               std::int32_t sy, std::int32_t divy,
               std::int32_t cx, std::int32_t cy) override;
    void Init(std::int32_t x, std::int32_t y) override;

    // === IWzVector2D overrides ===
    [[nodiscard]] auto GetCurrentTime() -> std::int32_t override;
    void PutCurrentTime(std::int32_t t) override;
    [[nodiscard]] auto GetOrigin() const -> IWzVector2D* override;
    void PutOrigin(IWzVector2D* parent) override;
    [[nodiscard]] auto GetRX() -> std::int32_t override;
    void PutRX(std::int32_t x) override;
    [[nodiscard]] auto GetRY() -> std::int32_t override;
    void PutRY(std::int32_t y) override;
    [[nodiscard]] auto GetA() -> double override;
    [[nodiscard]] auto GetRA() -> double override;
    void PutRA(double a) override;
    [[nodiscard]] auto GetFlipX() -> bool override;
    void PutFlipX(std::int32_t f) override;
    void GetSnapshot(std::int32_t* x, std::int32_t* y,
                     std::int32_t* rx, std::int32_t* ry,
                     std::int32_t* ox, std::int32_t* oy,
                     double* a, double* ra,
                     std::int32_t time = -1) override;
    void RelMove(std::int32_t x, std::int32_t y,
                 std::int32_t startTime, std::int32_t endTime,
                 bool bounce = false, bool pingpong = false,
                 bool replace = false) override;
    void RelOffset(std::int32_t dx, std::int32_t dy,
                   std::int32_t startTime, std::int32_t endTime) override;
    void Ratio(IWzVector2D* target,
               std::int32_t denomX, std::int32_t denomY,
               std::int32_t scaleX, std::int32_t scaleY) override;
    void WrapClip(IWzVector2D* bounds,
                  std::int32_t x, std::int32_t y,
                  std::int32_t w, std::int32_t h,
                  bool clampMode) override;
    void Rotate(double angle, std::int32_t period,
                std::int32_t easeFrames = 0) override;
    [[nodiscard]] auto GetLooseLevel() -> std::int32_t override;
    void PutLooseLevel(std::int32_t level) override;
    void Fly(const std::vector<FlyKeyframe>& keyframes,
             IWzVector2D* completionTarget = nullptr) override;

    // === CVecCtrl virtuals ===
    void SetApToApl() { m_apl = m_ap; }

    // === Owner ===
    [[nodiscard]] auto GetOwner() const -> IVecCtrlOwner* { return m_pOwner; }
    void SetOwner(IVecCtrlOwner* pOwner) { m_pOwner = pOwner; }

    // === New flying route ===
    std::queue<StaticNewFlyingRoute> m_qStaticNewFlyingRoute;
    bool m_bUserNewFlyingSkill{};
    bool m_bMobFlyingControl{};
    bool m_bIgnoreInertiaFlying{};
    bool m_bOnlyMaxSpeedFlying{};
    float m_fNewFlyingV{};
    float m_fNewFlyingMaxV{};
    float m_fNewFlyingAcceleration{};
    std::int32_t m_nLastNewFlyingDir{};
    std::int32_t m_nNewFlyingSkillID{};
    b2Vec2 m_vecNewFlyingInertia;
    b2Vec2 m_vecNewFlyingBefore;

    // === Navi flying ===
    bool m_bUserNaviFlyingSkill{};
    float m_fNaviFlyingV{};
    long double m_dNaviDestinationX{};
    long double m_dNaviDestinationY{};

    // === Mob flying roaming ===
    bool m_bMobFlyingRoaming{};
    long double m_dRoamingCenterX{};
    long double m_dRoamingCenterY{};
    float m_fRoamingAngle{};
    float m_fRoamingVx{};

    // === Mob flying to target ===
    bool m_bMobFlyingToTarget{};
    Point2D m_ptMobFlyingTarget;
    b2Vec2 m_vecFlyingDir;

    // === Core state ===
    std::int32_t m_bActive{};
    IWzVector2D* m_pVecAlternate{};

    // === Position ===
    AbsPosEx m_ap;
    AbsPosEx m_apl;
    AbsPosEx m_apOffset;
    RelPosEx m_rp;
    RelPosEx m_rpLast;

    // === Foothold ===
    ZtlSecureTear<const StaticFoothold*> m_pfh;
    const StaticFoothold* m_pfhLast{};
    const StaticFoothold* m_pfhFallStart{};
    const StaticFoothold* m_pfhLandingNext{};
    const StaticFoothold* m_pfhOldLandingNext{};

    // === Movement modifiers ===
    bool m_bSlowDown{};
    bool m_bFastDown{};

    // === Ladder/Rope ===
    ZtlSecureTear<LadderOrRope*> m_pLadderOrRope;

    // === Box2D foothold ===
    ZtlSecureTear<b2Body*> m_pB2Foothold;

    // === Layer / mass / bounds ===
    std::int32_t m_lPage{};
    std::int32_t m_lZMass{};
    Rect m_rcBound;

    // === Movement action ===
    std::int32_t m_nMoveAction{};
    std::int32_t m_bAttachedObjectChanged{};
    std::int32_t m_bBeginUpdateActivePassed{};
    std::int32_t m_bSetLayerZNext{};

    // === Input ===
    std::int32_t m_nInputX{};
    std::int32_t m_nInputY{};
    std::int32_t m_bJumpNext{};
    std::int32_t m_bTryJumpedInFly{};

    // === Falldown / Dragdown ===
    FalldownNext m_falldownNext;
    DragdownNext m_DragdownNext;

    // === Wings ===
    std::int32_t m_bWingsNext{};
    std::int32_t m_bWingsNow{};
    std::int32_t m_bWingsNowOnJump{};
    std::int32_t m_bWingsPrev{};
    std::int32_t m_nWingsSpeed{};
    std::int32_t m_nWingsSpeedX{};
    std::int32_t m_tWingsEnd{};
    std::int32_t m_bWingsFixSpeed{};

    // === Climbing ===
    std::int32_t m_bClimbing{};

    // === Foothold force ===
    long double m_dFootholdForce{};
    long double m_dFootholdForceX{};

    // === Pogo stick ===
    float m_fPogoStickSpeedMulti{};

    // === Impact ===
    ImpactNext m_impactNext;
    std::int32_t m_nImpactFlyReason{};

    // === Attributes ===
    std::shared_ptr<CAttrField> m_pAttrField;
    AttrShoe m_CurAttrShoe;

    // === Move path ===
    MovePath m_path;

    // === Misc ===
    std::int32_t m_nSlideCount{};
    long double m_dShortDrag{};
    std::int32_t m_bEscortMob{};

    // === Secure foothold SN ===
    ZtlSecureTear<std::uint32_t> m_dwIntegratedFootholdSNCRC;
    ZtlSecureTear<std::uint32_t> m_dwFootholdSN;

    // === Box2D foothold flag ===
    bool m_bOnB2FootHold{};

    // === User-local foothold ===
    const StaticFoothold* m_pfhUserLocal{};

    // === Forced stop ===
    bool m_bForcedStop{};

    // === Air hit ===
    std::int32_t m_tAirHitElapse{};
    std::int32_t m_nAirHitVy{};
    std::int32_t m_tAirHitElapseExtra{};

private:
    IVecCtrlOwner* m_pOwner{};
};

} // namespace ms
