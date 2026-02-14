#pragma once

#include "WzGr2DTypes.h"
#include "util/Point.h"
#include "Gr2DVector.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

struct SDL_Renderer;
struct SDL_Texture;

namespace ms
{

class WzGr2DCanvas;

/**
 * @brief 2D sprite layer class
 *
 * Based on IWzGr2DLayer interface from the original MapleStory client.
 * GUID: 6dc8c7ce-8e81-4420-b4f6-4b60b7d5fcdf
 *
 * Uses FrameNode doubly-linked list + ROR5 hash table for frame management,
 * 6 Gr2DVector objects for coordinate chains, 3 Gr2DVector color channels,
 * and RenderCommand output from Animate().
 */
class WzGr2DLayer : public IWzVector2D
{
public:
    WzGr2DLayer();
    WzGr2DLayer(std::int32_t left, std::int32_t top,
                std::uint32_t width, std::uint32_t height,
                std::int32_t z);
    ~WzGr2DLayer() override;

    // Non-copyable, movable
    WzGr2DLayer(const WzGr2DLayer&) = delete;
    auto operator=(const WzGr2DLayer&) -> WzGr2DLayer& = delete;
    WzGr2DLayer(WzGr2DLayer&&) noexcept;
    auto operator=(WzGr2DLayer&&) noexcept -> WzGr2DLayer&;

    // === Initialization (source-matching) ===
    void SetVideoMode(std::int32_t screenW, std::int32_t screenH,
                      std::int32_t viewW, std::int32_t viewH);
    void InitAnimation(std::int32_t baseTimestamp);
    void SetAnimOrigin(Gr2DVector* origin);

    // === Frame management (source-matching API) ===
    auto InsertCanvas(ICanvas* canvas, std::int32_t duration,
                      std::int32_t alpha = -1, std::int32_t colorMod = -1,
                      std::int32_t blendSrc = 0, std::int32_t blendDst = 0) -> int;
    void RemoveCanvas(int index);
    void InitCanvasOrder();
    void ShiftCanvas(int index);
    void SetFrameCanvas(int index, ICanvas* canvas);
    [[nodiscard]] auto get_canvasCount() const -> int;
    [[nodiscard]] auto get_canvas() const -> ICanvas*;

    // === Frame management (backward-compatible wrappers) ===
    auto InsertCanvas(std::shared_ptr<WzGr2DCanvas> canvas,
                      std::int32_t delay = 100,
                      std::uint8_t alpha0 = 255,
                      std::uint8_t alpha1 = 255,
                      std::int32_t zoom0 = 1000,
                      std::int32_t zoom1 = 1000) -> std::size_t;
    void RemoveAllCanvases();
    [[nodiscard]] auto GetCanvasCount() const noexcept -> std::size_t;
    [[nodiscard]] auto GetCanvas(std::size_t index) const -> std::shared_ptr<WzGr2DCanvas>;
    [[nodiscard]] auto GetCurrentCanvas() const -> std::shared_ptr<WzGr2DCanvas>;

    // === Animation (source-matching API) ===
    auto Animate(std::uint32_t flags, std::int32_t timeDelta, int targetFrame = -1) -> int;
    [[nodiscard]] auto get_animationState() const -> int;
    [[nodiscard]] auto get_animationTime() const -> int;
    [[nodiscard]] auto getRenderCommands() const -> const std::vector<RenderCommand>&;

    // === Animation (backward-compatible wrappers) ===
    auto Animate(Gr2DAnimationType type,
                 std::int32_t delayRate = 1000,
                 std::int32_t repeat = -1) -> bool;
    void StopAnimation();
    [[nodiscard]] auto IsAnimating() const noexcept -> bool { return m_bAnimating; }
    [[nodiscard]] auto GetCurrentFrame() const noexcept -> std::size_t;
    void SetCurrentFrame(std::size_t frame);
    [[nodiscard]] auto GetAnimationState() const noexcept -> AnimationState;
    [[nodiscard]] auto GetAnimationTime() const noexcept -> std::int32_t { return m_animTimer; }

    // === Dimensions (source-matching) ===
    [[nodiscard]] auto get_width() const -> std::int32_t { return m_width; }
    void put_width(std::int32_t w) { m_width = w; }
    [[nodiscard]] auto get_height() const -> std::int32_t { return m_height; }
    void put_height(std::int32_t h) { m_height = h; }

    // === Dimensions (backward-compatible) ===
    [[nodiscard]] auto GetWidth() const noexcept -> std::uint32_t
    {
        return static_cast<std::uint32_t>(m_width > 0 ? m_width : 0);
    }
    [[nodiscard]] auto GetHeight() const noexcept -> std::uint32_t
    {
        return static_cast<std::uint32_t>(m_height > 0 ? m_height : 0);
    }
    void SetWidth(std::uint32_t width) noexcept { m_width = static_cast<std::int32_t>(width); }
    void SetHeight(std::uint32_t height) noexcept { m_height = static_cast<std::int32_t>(height); }

    // === Position ===
    [[nodiscard]] auto GetLeft() const noexcept -> std::int32_t { return m_nLeft; }
    [[nodiscard]] auto GetTop() const noexcept -> std::int32_t { return m_nTop; }
    void SetPosition(std::int32_t left, std::int32_t top) noexcept;
    [[nodiscard]] auto GetLeftTop() const noexcept -> Point2D { return {m_nLeft, m_nTop}; }
    [[nodiscard]] auto GetRightBottom() const noexcept -> Point2D
    {
        return {m_nLeft + m_width, m_nTop + m_height};
    }

    // === Boundary vectors (source-matching) ===
    [[nodiscard]] auto get_lt() const -> Gr2DVector*;
    [[nodiscard]] auto get_rb() const -> Gr2DVector*;
    void InterlockedOffset(std::int32_t ltX, std::int32_t ltY,
                           std::int32_t rbX, std::int32_t rbY);

    // === Position helpers (operate on positionVec origin) ===
    void MoveOrigin(std::int32_t x, std::int32_t y);
    void OffsetOrigin(std::int32_t dx, std::int32_t dy);
    void ScaleOrigin(std::int32_t sx, std::int32_t divx,
                     std::int32_t sy, std::int32_t divy,
                     std::int32_t cx, std::int32_t cy);

    // === Z-order ===
    [[nodiscard]] auto get_z() const -> std::int32_t { return m_zOrder; }
    void put_z(std::int32_t z) { m_zOrder = z; }
    [[nodiscard]] auto GetZ() const noexcept -> std::int32_t { return m_zOrder; }
    void SetZ(std::int32_t z) noexcept { m_zOrder = z; }

    // === Flip ===
    [[nodiscard]] auto get_flip() const -> std::int32_t { return m_flipMode; }
    void put_flip(std::int32_t mode) { m_flipMode = mode; }
    [[nodiscard]] auto GetFlip() const noexcept -> LayerFlipState
    {
        return static_cast<LayerFlipState>(m_flipMode);
    }
    void SetFlip(LayerFlipState flip) noexcept { m_flipMode = static_cast<std::int32_t>(flip); }
    void SetFlip(std::int32_t flip) noexcept { m_flipMode = flip; }

    // === Color (source-matching: 3 Gr2DVector channels) ===
    void put_color(std::uint32_t argb);
    [[nodiscard]] auto get_color() const -> std::uint32_t;
    [[nodiscard]] auto get_alpha() const -> Gr2DVector*;
    [[nodiscard]] auto get_redTone() const -> Gr2DVector*;
    [[nodiscard]] auto get_greenBlueTone() const -> Gr2DVector*;

    // === Color (backward-compatible) ===
    [[nodiscard]] auto GetColor() const noexcept -> std::uint32_t { return get_color(); }
    void SetColor(std::uint32_t color) noexcept { put_color(color); }
    [[nodiscard]] auto GetAlpha() const noexcept -> std::uint8_t;
    void SetAlpha(std::uint8_t alpha) noexcept;

    // === Visibility ===
    [[nodiscard]] auto get_visible() const -> bool { return m_visible; }
    void put_visible(bool v) { m_visible = v; }
    [[nodiscard]] auto IsVisible() const noexcept -> bool { return m_visible; }
    void SetVisible(bool visible) noexcept { m_visible = visible; }

    // === Overlay (parent layer in render tree) ===
    void put_overlay(const std::shared_ptr<WzGr2DLayer>& parent) { m_pOverlay = parent; }
    [[nodiscard]] auto get_overlay() const -> std::shared_ptr<WzGr2DLayer> { return m_pOverlay; }

    // === Blend ===
    [[nodiscard]] auto get_blend() const -> std::int32_t { return m_blendMode; }
    void put_blend(std::int32_t mode) { m_blendMode = mode; }
    [[nodiscard]] auto GetBlend() const noexcept -> LayerBlendType
    {
        return static_cast<LayerBlendType>(m_blendMode);
    }
    void SetBlend(LayerBlendType blend) noexcept { m_blendMode = static_cast<std::int32_t>(blend); }
    void SetBlend(std::int32_t blend) noexcept { m_blendMode = blend; }

    // === Rotation (SDL-specific, kept from local) ===
    [[nodiscard]] auto GetRotation() const noexcept -> float { return m_fRotation; }
    void SetRotation(float degrees) noexcept { m_fRotation = degrees; }
    void SetRotation(std::int32_t degrees) noexcept { m_fRotation = static_cast<float>(degrees); }

    // === Tiling (local feature) ===
    void SetTiling(std::int32_t cx, std::int32_t cy) noexcept { m_nTileCx = cx; m_nTileCy = cy; }
    [[nodiscard]] auto GetTileCx() const noexcept -> std::int32_t { return m_nTileCx; }
    [[nodiscard]] auto GetTileCy() const noexcept -> std::int32_t { return m_nTileCy; }

    // === Position animation (backward-compatible) ===
    void StartPositionAnimation(std::int32_t offsetX, std::int32_t offsetY,
                                std::int32_t duration, bool loop = true);
    void StopPositionAnimation();
    [[nodiscard]] auto IsPositionAnimating() const noexcept -> bool;

    // === Non-vtable helpers (source-matching) ===
    [[nodiscard]] auto getTag() const -> std::int32_t { return m_tag; }
    void setTag(std::int32_t tag) { m_tag = tag; }
    void setColorKey(std::uint8_t a, std::uint8_t r, std::uint8_t g, std::uint8_t b);
    void setFlags(std::uint32_t mask) { m_flags |= mask; }
    void clearFlags(std::uint32_t mask) { m_flags &= ~mask; }
    void setAnimSpeed(float speed) { m_animSpeed = speed; }

    // === Particle system ===
    auto getEmitter() -> ParticleEmitter*;
    void UpdateParticles(float deltaTime);

    // === Animation origin ===
    [[nodiscard]] auto getAnimOriginVector() const -> Gr2DVector*;

    // === Update and Render (SDL-specific) ===
    void Update(std::int32_t tCur);
    void Render(SDL_Renderer* renderer, std::int32_t offsetX = 0, std::int32_t offsetY = 0);

    // === IWzShape2D / IWzVector2D overrides (delegate to m_positionVec) ===
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

private:
    // === Identification ===
    std::int32_t m_tag = 0;
    std::int32_t m_uniqueId = 0;
    static std::int32_t s_idCounter;

    // === Layer dimensions ===
    std::int32_t m_width = 0;
    std::int32_t m_height = 0;

    // === Vector2D objects (created by SetVideoMode or ensureVectors) ===
    std::unique_ptr<Gr2DVector> m_screenVec;
    std::unique_ptr<Gr2DVector> m_alphaVec;
    std::unique_ptr<Gr2DVector> m_colorRedVec;
    std::unique_ptr<Gr2DVector> m_colorGBVec;
    std::unique_ptr<Gr2DVector> m_positionVec;
    std::unique_ptr<Gr2DVector> m_rbVec;

    // === Animation Vector2D (created by InitAnimation) ===
    std::unique_ptr<Gr2DVector> m_animOriginVec;
    std::unique_ptr<Gr2DVector> m_animIntermediate;

    // === Frame linked list ===
    FrameNode* m_frameHead = nullptr;
    FrameNode* m_frameTail = nullptr;
    FrameNode* m_currentFrame = nullptr;
    std::int32_t m_frameCount = 0;
    std::int32_t m_frameIdCounter = 0;
    std::int32_t m_totalDuration = 0;

    // === Frame hash table (O(1) ID lookup) ===
    static constexpr int HASH_BUCKETS = 31;
    std::array<FrameNode*, HASH_BUCKETS> m_hashTable = {};

    // === Render commands output ===
    std::vector<RenderCommand> m_renderCommands;

    // === Display properties ===
    bool m_visible = true;
    std::int32_t m_zOrder = 0;
    std::int32_t m_flipMode = 0;
    std::int32_t m_blendMode = 0;
    std::uint32_t m_flags = 0;
    bool m_colorKeyEnabled = false;
    std::uint32_t m_colorKey = 0xFFFFFFFF;
    std::int32_t m_lastUpdateFlags = 0;
    std::int32_t m_surfaceMode = 1;
    float m_animSpeed = 1.0F;

    // === SDL-specific (local) ===
    float m_fRotation = 0.0F;

    // === Tiling (local) ===
    std::int32_t m_nTileCx = 0;
    std::int32_t m_nTileCy = 0;

    // === Backward-compatible position state ===
    std::int32_t m_nLeft = 0;
    std::int32_t m_nTop = 0;

    // === Backward-compatible animation state ===
    bool m_bAnimating = false;
    Gr2DAnimationType m_animType = Gr2DAnimationType::None;
    std::int32_t m_nDelayRate = 1000;
    std::int32_t m_nRepeatCount = -1;
    std::int32_t m_nCurrentRepeat = 0;
    std::int32_t m_tLastFrameTime = 0;
    bool m_bReverseDirection = false;

    // === Particle emitter ===
    std::unique_ptr<ParticleEmitter> m_emitter;

    // === Timing ===
    std::int32_t m_baseTimestamp = 0;
    std::int32_t m_animTimer = 0;

    // === Overlay (parent layer reference) ===
    std::shared_ptr<WzGr2DLayer> m_pOverlay;

    // === Ownership for backward-compatible InsertCanvas ===
    std::vector<std::shared_ptr<WzGr2DCanvas>> m_ownedCanvases;

    // === Internal helpers ===
    auto findFrameById(std::int32_t id) const -> FrameNode*;
    auto findFrameByIndex(int index) const -> FrameNode*;
    void insertFrameHash(FrameNode* node);
    void removeFrameHash(FrameNode* node);
    static auto hashFrameId(std::int32_t id) -> std::uint32_t;
    void clearFrames();
    auto computeAlpha(std::int32_t frameAlpha) const -> std::int32_t;
    void ensureVectors();
    void advanceFrame();
};

} // namespace ms
