#pragma once

#include "WzGr2DTypes.h"
#include "util/Point.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

struct SDL_Renderer;

namespace ms
{

class WzCanvas;

/**
 * @brief Graphics layer for 2D rendering
 *
 * Based on IWzGr2DLayer interface from the original MapleStory client.
 * GUID: 6dc8c7ce-8e81-4420-b4f6-4b60b7d5fcdf
 *
 * Represents a single layer in the rendering system that can contain
 * multiple canvas frames for animation.
 */
class WzGr2DLayer
{
public:
    using PostRenderCallback = std::function<void(WzGr2DLayer&, std::int32_t)>;

    WzGr2DLayer();
    WzGr2DLayer(std::int32_t left, std::int32_t top,
                std::uint32_t width, std::uint32_t height,
                std::int32_t z);
    ~WzGr2DLayer();

    // Non-copyable, movable
    WzGr2DLayer(const WzGr2DLayer&) = delete;
    auto operator=(const WzGr2DLayer&) -> WzGr2DLayer& = delete;
    WzGr2DLayer(WzGr2DLayer&&) noexcept;
    auto operator=(WzGr2DLayer&&) noexcept -> WzGr2DLayer&;

    // Position and dimensions
    [[nodiscard]] auto GetLeft() const noexcept -> std::int32_t { return m_nLeft; }
    [[nodiscard]] auto GetTop() const noexcept -> std::int32_t { return m_nTop; }
    [[nodiscard]] auto GetWidth() const noexcept -> std::uint32_t { return m_uWidth; }
    [[nodiscard]] auto GetHeight() const noexcept -> std::uint32_t { return m_uHeight; }

    void SetPosition(std::int32_t left, std::int32_t top) noexcept;

    // Z-order (depth)
    [[nodiscard]] auto GetZ() const noexcept -> std::int32_t { return m_nZ; }
    void SetZ(std::int32_t z) noexcept { m_nZ = z; }

    // Flip state
    [[nodiscard]] auto GetFlip() const noexcept -> LayerFlipState { return m_flipState; }
    void SetFlip(LayerFlipState flip) noexcept { m_flipState = flip; }
    void SetFlip(std::int32_t flip) noexcept { m_flipState = static_cast<LayerFlipState>(flip); }

    // Color tint (ARGB)
    [[nodiscard]] auto GetColor() const noexcept -> std::uint32_t { return m_dwColor; }
    void SetColor(std::uint32_t color) noexcept { m_dwColor = color; }

    // Visibility
    [[nodiscard]] auto IsVisible() const noexcept -> bool { return m_bVisible; }
    void SetVisible(bool visible) noexcept { m_bVisible = visible; }

    // Screen space (UI elements that don't move with camera)
    [[nodiscard]] auto IsScreenSpace() const noexcept -> bool { return m_bScreenSpace; }
    void SetScreenSpace(bool screenSpace) noexcept { m_bScreenSpace = screenSpace; }

    // Center-based positioning (position is relative to screen center, matching original MS client)
    // Only applies when IsScreenSpace() is true
    [[nodiscard]] auto IsCenterBased() const noexcept -> bool { return m_bCenterBased; }
    void SetCenterBased(bool centerBased) noexcept { m_bCenterBased = centerBased; }

    // Tiling/parallax parameters
    // cx: horizontal tile distance (0 = no tiling)
    // cy: vertical tile distance (0 = no tiling)
    // rx: horizontal parallax factor (0-100, 0 = no scroll with camera)
    // ry: vertical parallax factor (0-100, 0 = no scroll with camera)
    void SetTiling(std::int32_t cx, std::int32_t cy) noexcept { m_nTileCx = cx; m_nTileCy = cy; }
    void SetParallax(std::int32_t rx, std::int32_t ry) noexcept { m_nParallaxRx = rx; m_nParallaxRy = ry; }
    [[nodiscard]] auto GetTileCx() const noexcept -> std::int32_t { return m_nTileCx; }
    [[nodiscard]] auto GetTileCy() const noexcept -> std::int32_t { return m_nTileCy; }
    [[nodiscard]] auto GetParallaxRx() const noexcept -> std::int32_t { return m_nParallaxRx; }
    [[nodiscard]] auto GetParallaxRy() const noexcept -> std::int32_t { return m_nParallaxRy; }

    // Position animation (for background type 4-7 animated movement)
    // Based on IWzVector2D::RelMove from original client
    /**
     * @brief Start position animation with looping
     * @param offsetX X offset from initial position
     * @param offsetY Y offset from initial position
     * @param duration Animation duration in milliseconds
     * @param loop True to loop animation
     */
    void StartPositionAnimation(std::int32_t offsetX, std::int32_t offsetY,
                                std::int32_t duration, bool loop = true);

    /**
     * @brief Stop position animation
     */
    void StopPositionAnimation();

    /**
     * @brief Check if position animation is active
     */
    [[nodiscard]] auto IsPositionAnimating() const noexcept -> bool { return m_bPositionAnimating; }

    // Canvas frame management
    /**
     * @brief Insert a canvas frame
     * @param canvas The canvas to insert
     * @param delay Frame delay in milliseconds
     * @param alpha0 Start alpha (0-255)
     * @param alpha1 End alpha (0-255)
     * @param zoom0 Start zoom in thousandths (1000 = 100%)
     * @param zoom1 End zoom in thousandths (1000 = 100%)
     * @return Index of inserted canvas
     */
    auto InsertCanvas(std::shared_ptr<WzCanvas> canvas,
                      std::int32_t delay = 100,
                      std::uint8_t alpha0 = 255,
                      std::uint8_t alpha1 = 255,
                      std::int32_t zoom0 = 1000,
                      std::int32_t zoom1 = 1000) -> std::size_t;

    /**
     * @brief Remove a canvas frame by index
     * @param index Index of canvas to remove
     * @return The removed canvas, or nullptr if index invalid
     */
    auto RemoveCanvas(std::size_t index) -> std::shared_ptr<WzCanvas>;

    /**
     * @brief Remove all canvas frames
     */
    void RemoveAllCanvases();

    /**
     * @brief Get canvas count
     */
    [[nodiscard]] auto GetCanvasCount() const noexcept -> std::size_t;

    /**
     * @brief Get canvas at index
     */
    [[nodiscard]] auto GetCanvas(std::size_t index) const -> std::shared_ptr<WzCanvas>;

    /**
     * @brief Get current frame canvas
     */
    [[nodiscard]] auto GetCurrentCanvas() const -> std::shared_ptr<WzCanvas>;

    // Animation control
    /**
     * @brief Start animation
     * @param type Animation type
     * @param delayRate Delay multiplier in thousandths (1000 = normal speed)
     * @param repeat Number of times to repeat (-1 = infinite)
     * @return true if animation started
     */
    auto Animate(Gr2DAnimationType type,
                 std::int32_t delayRate = 1000,
                 std::int32_t repeat = -1) -> bool;

    /**
     * @brief Stop animation
     */
    void StopAnimation();

    /**
     * @brief Check if currently animating
     */
    [[nodiscard]] auto IsAnimating() const noexcept -> bool { return m_bAnimating; }

    /**
     * @brief Get current frame index
     */
    [[nodiscard]] auto GetCurrentFrame() const noexcept -> std::size_t { return m_nCurrentFrame; }

    /**
     * @brief Set current frame index
     */
    void SetCurrentFrame(std::size_t frame);

    // Post-render callback
    void SetPostRenderCallback(PostRenderCallback callback);

    // Update and render
    /**
     * @brief Update animation state
     * @param tCur Current time in milliseconds
     */
    void Update(std::int32_t tCur);

    /**
     * @brief Render the layer
     * @param renderer SDL renderer
     * @param offsetX X offset for rendering
     * @param offsetY Y offset for rendering
     */
    void Render(SDL_Renderer* renderer, std::int32_t offsetX = 0, std::int32_t offsetY = 0);

private:
    struct CanvasEntry
    {
        std::shared_ptr<WzCanvas> canvas;
        CanvasFrameInfo frameInfo;
    };

    void AdvanceFrame();
    void UpdateFrameInterpolation(std::int32_t tCur);

    // Position and dimensions
    std::int32_t m_nLeft{0};
    std::int32_t m_nTop{0};
    std::uint32_t m_uWidth{0};
    std::uint32_t m_uHeight{0};
    std::int32_t m_nZ{0};

    // Display properties
    LayerFlipState m_flipState{LayerFlipState::None};
    std::uint32_t m_dwColor{0xFFFFFFFF}; // ARGB white (fully opaque)
    bool m_bVisible{true};
    bool m_bScreenSpace{false}; // If true, position is in screen coordinates (not affected by camera)
    bool m_bCenterBased{false};  // If true (and screen-space), position is relative to screen center

    // Tiling/parallax properties
    std::int32_t m_nTileCx{0};      // Horizontal tile distance (0 = no tiling)
    std::int32_t m_nTileCy{0};      // Vertical tile distance (0 = no tiling)
    std::int32_t m_nParallaxRx{0};  // Horizontal parallax factor (0-100)
    std::int32_t m_nParallaxRy{0};  // Vertical parallax factor (0-100)

    // Position animation properties (for background type 4-7)
    bool m_bPositionAnimating{false};     // Is position animation active
    std::int32_t m_nAnimOffsetX{0};       // Target X offset from initial position
    std::int32_t m_nAnimOffsetY{0};       // Target Y offset from initial position
    std::int32_t m_nAnimDuration{0};      // Animation duration in milliseconds
    bool m_bAnimLoop{false};              // Loop animation
    std::int32_t m_tAnimStart{0};         // Animation start time
    std::int32_t m_nInitialLeft{0};       // Initial left position (before animation)
    std::int32_t m_nInitialTop{0};        // Initial top position (before animation)

    // Canvas frames
    std::vector<CanvasEntry> m_canvases;
    std::size_t m_nCurrentFrame{0};

    // Animation state (based on original this[91] state machine)
    bool m_bAnimating{false};
    AnimationState m_animState{AnimationState::Idle};
    Gr2DAnimationType m_animType{Gr2DAnimationType::None};
    std::int32_t m_nDelayRate{1000};  // Delay rate in thousandths (1000 = normal speed)
    std::int32_t m_nRepeatCount{-1};
    std::int32_t m_nCurrentRepeat{0};
    std::int32_t m_tLastFrameTime{0};
    bool m_bReverseDirection{false};

    // Current interpolated values (using thousandths for zoom, matching original)
    std::uint8_t m_nCurrentAlpha{255};
    std::int32_t m_nCurrentZoom{1000};  // Zoom in thousandths (1000 = 100%)

    // Callbacks
    PostRenderCallback m_postRenderCallback;
};

} // namespace ms
