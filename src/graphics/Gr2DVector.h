#pragma once

#include "IWzVector2D.h"

#include <cstdint>
#include <cmath>
#include <memory>
#include <vector>

namespace ms
{

class Gr2DVector;
struct AnimChain;

/**
 * @brief Global time management for Gr2D animations
 */
namespace Gr2DTime
{
    auto GetCurrentTime() -> std::int32_t;
    void SetCurrentTime(std::int32_t t);
}

/**
 * @brief Abstract base class for animation nodes
 *
 * Animation nodes are chained together and evaluated in order of phase.
 * Each node type has a unique type ID that includes phase information.
 */
struct AnimNode
{
    AnimNode* prev = nullptr;
    AnimNode* next = nullptr;

    virtual ~AnimNode() = default;

    [[nodiscard]] virtual auto type() const -> std::uint32_t = 0;

    // Position-based evaluate (phases 0, 1, 2). Returns 1=complete, 0=continue.
    virtual auto evaluatePos(std::int32_t& x, std::int32_t& y,
                            std::int32_t frame, bool commit) -> int
    {
        (void)x; (void)y; (void)frame; (void)commit;
        return 0;
    }

    // Angle-based evaluate (phase 3). Returns 1=complete, 0=continue.
    virtual auto evaluateAngle(double& angle, std::int32_t frame, bool commit) -> int
    {
        (void)angle; (void)frame; (void)commit;
        return 0;
    }

    // For FlyNode — completion target
    virtual auto takeCompletion() -> IWzVector2D* { return nullptr; }

    [[nodiscard]] auto phase() const -> int { return static_cast<int>(type() & 0xFFFF); }
};

/**
 * @brief Phase 1: Linear/bounce easing movement
 * Type: 0x00000001
 */
struct EasingNode : public AnimNode
{
    std::int32_t accum_x = 0, accum_y = 0;
    std::int32_t dx = 0, dy = 0;
    std::int32_t startTime = 0, endTime = 0;
    bool bounce = false;
    bool pingpong = false;
    std::int32_t looseLevel = 0;
    std::int32_t looseTimer = 0;

    [[nodiscard]] auto type() const -> std::uint32_t override { return 0x00000001; }
    auto evaluatePos(std::int32_t& x, std::int32_t& y,
                    std::int32_t frame, bool commit) -> int override;
};

/**
 * @brief Phase 1: Ratio-based position following
 * Type: 0x000A0001
 */
struct RatioNode : public AnimNode
{
    IWzVector2D* target = nullptr;
    std::int32_t base_x = 0, base_y = 0;
    std::int32_t denomX = 1, denomY = 1;
    std::int32_t scaleX = 1, scaleY = 1;

    [[nodiscard]] auto type() const -> std::uint32_t override { return 0x000A0001; }
    auto evaluatePos(std::int32_t& x, std::int32_t& y,
                    std::int32_t frame, bool commit) -> int override;
};

/**
 * @brief Phase 0: Cubic Hermite spline path animation (Fly)
 * Type: 0x00320000
 */
struct FlyNode : public AnimNode
{
    std::vector<FlyKeyframe> keyframes;
    IWzVector2D* completion = nullptr;

    [[nodiscard]] auto type() const -> std::uint32_t override { return 0x00320000; }
    auto evaluatePos(std::int32_t& x, std::int32_t& y,
                    std::int32_t frame, bool commit) -> int override;
    auto takeCompletion() -> IWzVector2D* override;
};

/**
 * @brief Phase 2: Boundary wrapping/clipping
 * Type: 0x00140002
 */
struct WrapClipNode : public AnimNode
{
    IWzVector2D* bounds = nullptr;
    std::int32_t left = 0, top = 0;
    std::int32_t right = 0, bottom = 0;
    bool clampMode = false;

    [[nodiscard]] auto type() const -> std::uint32_t override { return 0x00140002; }
    auto evaluatePos(std::int32_t& x, std::int32_t& y,
                    std::int32_t frame, bool commit) -> int override;

    static auto wrapVal(std::int32_t val, std::int32_t start, std::int32_t size) -> std::int32_t;
    static auto clampVal(std::int32_t val, std::int32_t start, std::int32_t size) -> std::int32_t;
};

/**
 * @brief Phase 3: Rotation animation with easing
 * Type: 0x00280003
 */
struct RotateNode : public AnimNode
{
    double totalAngle = 0.0;
    std::int32_t startTime = 0;
    std::int32_t period = 0;
    std::int32_t easeFrames = 0;

    [[nodiscard]] auto type() const -> std::uint32_t override { return 0x00280003; }
    auto evaluateAngle(double& angle, std::int32_t frame, bool commit) -> int override;
};

/**
 * @brief Animation chain evaluator
 *
 * Maintains a linked list of AnimNodes sorted by phase.
 * Evaluates all nodes using a 9-step pipeline from source.
 */
struct AnimChain
{
    IWzVector2D* parent_ref = nullptr;

    std::int32_t base_x = 0, base_y = 0;
    std::int32_t offset_x = 0, offset_y = 0;

    AnimNode* head = nullptr;
    AnimNode* tail = nullptr;

    double base_angle = 0.0;
    std::int32_t flip_accum = 0;
    bool first_eval_init = false;
    bool first_eval = false;

    bool evaluated = false;
    std::int32_t evaluated_frame = 0;
    std::int32_t flip_result = 0;

    // Cached outputs
    std::int32_t parent_cache_x = 0, parent_cache_y = 0;
    std::int32_t local_cache_x = 0, local_cache_y = 0;
    std::int32_t world_cache_x = 0, world_cache_y = 0;
    double local_angle_cache = 0.0;
    double total_angle_cache = 0.0;
    double parent_angle_cache = 0.0;

    AnimChain() = default;
    AnimChain(std::int32_t x, std::int32_t y) : base_x(x), base_y(y) {}
    ~AnimChain();

    void insertNode(AnimNode* node);
    void removeNode(AnimNode* node);
    void clearNodes();
    void removeNodesByType(std::uint32_t nodeType);
    void evaluate(std::int32_t frame, bool commit);
    void reset(std::int32_t x, std::int32_t y);
};

/**
 * @brief 2D Vector with hierarchical animation support
 *
 * Concrete implementation of IWzVector2D from the original MapleStory client.
 * Supports position, rotation, and complex animation chains.
 */
class Gr2DVector : public IWzVector2D
{
public:
    Gr2DVector() = default;
    explicit Gr2DVector(std::int32_t x, std::int32_t y);
    ~Gr2DVector() override = default;

    // Movable
    Gr2DVector(Gr2DVector&&) noexcept = default;
    auto operator=(Gr2DVector&&) noexcept -> Gr2DVector& = default;

    // Non-copyable
    Gr2DVector(const Gr2DVector&) = delete;
    auto operator=(const Gr2DVector&) -> Gr2DVector& = delete;

    // === IWzShape2D overrides ===
    [[nodiscard]] auto GetX() -> std::int32_t override;
    auto GetY() -> std::int32_t override;
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
                 std::int32_t startTime = 0, std::int32_t endTime = 0,
                 bool bounce = false, bool pingpong = false,
                 bool replace = false) override;
    void RelOffset(std::int32_t dx, std::int32_t dy,
                   std::int32_t startTime = 0, std::int32_t endTime = 0) override;
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

    // === Non-interface methods ===
    void Serialize(const char* data);

    // Direct access
    [[nodiscard]] auto RawX() const -> std::int32_t { return m_x; }
    [[nodiscard]] auto RawY() const -> std::int32_t { return m_y; }
    [[nodiscard]] auto Chain() const -> AnimChain* { return m_chain.get(); }

private:
    auto ensureChain() -> AnimChain*;

    std::int32_t m_x = 0;
    std::int32_t m_y = 0;
    std::unique_ptr<AnimChain> m_chain;
};

} // namespace ms
