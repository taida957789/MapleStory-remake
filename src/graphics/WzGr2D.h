#pragma once

#include "WzGr2DTypes.h"
#include "util/Point.h"
#include "util/Singleton.h"

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;

namespace ms
{

class WzGr2DCanvas;
class WzGr2DLayer;

/**
 * @brief 2D Graphics Engine
 *
 * Based on IWzGr2D interface from the original MapleStory client.
 * GUID: e576ea33-d465-4f08-aab1-e78df73ee6d9
 *
 * Main graphics engine that manages rendering layers and the SDL context.
 * Accessed globally via get_gr() function (matching original client).
 */
class WzGr2D : public Singleton<WzGr2D>
{
    friend class Singleton<WzGr2D>;

public:
    ~WzGr2D();

    /**
     * @brief Initialize the graphics engine
     * @param width Screen width
     * @param height Screen height
     * @param window SDL window handle (or nullptr to create new)
     * @param bpp Bits per pixel (default 32)
     * @param refreshRate Refresh rate in Hz (0 = use display default)
     * @return true if initialization successful
     */
    [[nodiscard]] auto Initialize(std::uint32_t width, std::uint32_t height,
                                   SDL_Window* window = nullptr,
                                   std::int32_t bpp = 32,
                                   std::int32_t refreshRate = 0) -> bool;

    /**
     * @brief Shutdown the graphics engine
     */
    void Shutdown();

    /**
     * @brief Check if engine is initialized
     */
    [[nodiscard]] auto IsInitialized() const noexcept -> bool { return m_bInitialized; }

    // Screen properties
    [[nodiscard]] auto GetWidth() const noexcept -> std::uint32_t { return m_uWidth; }
    [[nodiscard]] auto GetHeight() const noexcept -> std::uint32_t { return m_uHeight; }
    [[nodiscard]] auto GetCenter() const noexcept -> Point2D;
    [[nodiscard]] auto IsFullScreen() const noexcept -> bool { return m_bFullScreen; }
    void SetFullScreen(bool fullscreen);

    // Background color (ARGB)
    [[nodiscard]] auto GetBackColor() const noexcept -> std::uint32_t { return m_dwBackColor; }
    void SetBackColor(std::uint32_t color) noexcept { m_dwBackColor = color; }

    // Timing
    [[nodiscard]] auto GetCurrentTime() const noexcept -> std::int32_t { return m_tCurrent; }
    [[nodiscard]] auto GetNextRenderTime() const noexcept -> std::int32_t;
    [[nodiscard]] auto GetFps100() const noexcept -> std::uint32_t { return m_uFps100; }
    void UpdateCurrentTime(std::int32_t tCur);

    // Layer management
    /**
     * @brief Create a new rendering layer
     * @param left Left position
     * @param top Top position
     * @param width Layer width
     * @param height Layer height
     * @param z Z-order (depth, higher = on top)
     * @param canvas Optional initial canvas
     * @param filter Optional filter flags
     * @return Shared pointer to the created layer
     */
    [[nodiscard]] auto CreateLayer(std::int32_t left, std::int32_t top,
                                    std::uint32_t width, std::uint32_t height,
                                    std::int32_t z,
                                    std::shared_ptr<WzGr2DCanvas> canvas = nullptr,
                                    std::uint32_t filter = 0) -> std::shared_ptr<WzGr2DLayer>;

    /**
     * @brief Remove a layer
     * @param layer Layer to remove
     */
    void RemoveLayer(const std::shared_ptr<WzGr2DLayer>& layer);

    /**
     * @brief Remove all layers
     */
    void RemoveAllLayers();

    /**
     * @brief Get layer count
     */
    [[nodiscard]] auto GetLayerCount() const noexcept -> std::size_t;

    // Rendering
    /**
     * @brief Render a single frame
     * @param tCur Current time in milliseconds
     * @return true if frame was rendered
     */
    [[nodiscard]] auto RenderFrame(std::int32_t tCur) -> bool;

    /**
     * @brief Check display mode support
     * @param width Desired width
     * @param height Desired height
     * @param bpp Desired bits per pixel
     * @return true if mode is supported
     */
    [[nodiscard]] auto CheckMode(std::uint32_t width, std::uint32_t height,
                                  std::uint32_t bpp) const -> bool;

    // SDL access
    [[nodiscard]] auto GetWindow() const noexcept -> SDL_Window* { return m_pWindow; }
    [[nodiscard]] auto GetRenderer() const noexcept -> SDL_Renderer* { return m_pRenderer; }

    // Camera control (for scrolling maps)
    [[nodiscard]] auto GetCameraPosition() const noexcept -> Point2D { return m_cameraPos; }
    void SetCameraPosition(const Point2D& pos) noexcept { m_cameraPos = pos; }
    void SetCameraPosition(std::int32_t x, std::int32_t y) noexcept { m_cameraPos = {x, y}; }

    [[nodiscard]] auto GetCameraRotate() const noexcept -> float { return m_fCameraRotate; }
    void SetCameraRotate(float angle) noexcept { m_fCameraRotate = angle; }

    // Coordinate transformation (screen <-> world)
    [[nodiscard]] auto ScreenToWorld(const Point2D& screenPos) const -> Point2D;
    [[nodiscard]] auto WorldToScreen(const Point2D& worldPos) const -> Point2D;

protected:
    WzGr2D();

private:
    void UpdateFps(std::int32_t tCur);
    void SortLayers();

    // Initialization state
    bool m_bInitialized{false};
    bool m_bOwnWindow{false}; // Did we create the window?

    // Screen properties
    std::uint32_t m_uWidth{800};
    std::uint32_t m_uHeight{600};
    std::int32_t m_nBpp{32};
    bool m_bFullScreen{false};
    std::uint32_t m_dwBackColor{0xFF000000}; // Black background

    // SDL handles
    SDL_Window* m_pWindow{nullptr};
    SDL_Renderer* m_pRenderer{nullptr};

    // Timing
    std::int32_t m_tCurrent{0};
    std::int32_t m_tLastFrame{0};
    std::int32_t m_nTargetFrameTime{16}; // ~60 FPS
    std::uint32_t m_uFps100{6000};       // FPS * 100 for precision
    std::int32_t m_nFrameCount{0};
    std::int32_t m_tFpsUpdateTime{0};

    // Layers (sorted by Z-order)
    std::vector<std::shared_ptr<WzGr2DLayer>> m_layers;
    bool m_bLayersDirty{false};

    // Camera
    Point2D m_cameraPos{0, 0};
    float m_fCameraRotate{0.0F};
};

/**
 * @brief Global accessor for graphics engine (matches original client's get_gr())
 * @return Reference to the WzGr2D singleton
 */
[[nodiscard]] inline auto get_gr() noexcept -> WzGr2D&
{
    return WzGr2D::GetInstance();
}

} // namespace ms
