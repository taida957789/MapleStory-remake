#pragma once

#include "WzGr2DTypes.h"
#include "util/Point.h"
#include "wz/WzCanvas.h"
#include <SDL3/SDL.h>
#include <memory>

namespace ms
{

/**
 * @brief Graphics wrapper for WzCanvas, implementing ICanvas
 *
 * Separates WZ parsing logic (WzCanvas) from graphics rendering logic.
 * Contains SDL3-specific rendering data like textures, origin points, and z-ordering.
 * Implements the ICanvas abstract interface for use with WzGr2DLayer's FrameNode system.
 */
class WzGr2DCanvas : public ICanvas
{
public:
    WzGr2DCanvas() = default;
    explicit WzGr2DCanvas(std::shared_ptr<WzCanvas> canvas);
    ~WzGr2DCanvas() override;

    // Non-copyable, movable
    WzGr2DCanvas(const WzGr2DCanvas&) = delete;
    auto operator=(const WzGr2DCanvas&) -> WzGr2DCanvas& = delete;
    WzGr2DCanvas(WzGr2DCanvas&& other) noexcept;
    auto operator=(WzGr2DCanvas&& other) noexcept -> WzGr2DCanvas&;

    // Canvas access
    [[nodiscard]] auto GetCanvas() const noexcept -> const std::shared_ptr<WzCanvas>& { return m_canvas; }
    void SetCanvas(std::shared_ptr<WzCanvas> canvas);

    // Dimensions (forwarded from WzCanvas)
    [[nodiscard]] auto GetWidth() const noexcept -> int;
    [[nodiscard]] auto GetHeight() const noexcept -> int;

    // Position (where the canvas should be rendered in world/screen coordinates)
    [[nodiscard]] auto GetPosition() const noexcept -> Point2D { return m_position; }
    void SetPosition(const Point2D& position) noexcept { m_position = position; }
    void SetPosition(int x, int y) noexcept { m_position = {x, y}; }

    // Origin point (anchor point within the canvas image)
    [[nodiscard]] auto GetOrigin() const noexcept -> Point2D { return m_origin; }
    void SetOrigin(const Point2D& origin) noexcept { m_origin = origin; }

    // Frame delay (from WZ "delay" property, default 100ms)
    [[nodiscard]] auto GetDelay() const noexcept -> int { return m_nDelay; }
    void SetDelay(int delay) noexcept { m_nDelay = delay; }

    // Z value (for layering)
    [[nodiscard]] auto GetZ() const noexcept -> int { return m_nZ; }
    void SetZ(int z) noexcept { m_nZ = z; }

    // SDL texture
    [[nodiscard]] auto GetTexture() const noexcept -> SDL_Texture* { return m_pTexture; }
    void SetTexture(SDL_Texture* texture);

    // Create texture from pixel data
    auto CreateTexture(SDL_Renderer* renderer) -> SDL_Texture*;

    // State checks
    [[nodiscard]] auto HasPixelData() const noexcept -> bool;
    [[nodiscard]] auto HasTexture() const noexcept -> bool { return m_pTexture != nullptr; }

    // === ICanvas implementation ===
    [[nodiscard]] auto getWidth() const -> int override { return GetWidth(); }
    [[nodiscard]] auto getHeight() const -> int override { return GetHeight(); }
    [[nodiscard]] auto getOriginX() const -> int override { return m_origin.x; }
    [[nodiscard]] auto getOriginY() const -> int override { return m_origin.y; }
    [[nodiscard]] auto getTextureHandle() const -> std::intptr_t override
    {
        return reinterpret_cast<std::intptr_t>(m_pTexture);
    }
    [[nodiscard]] auto isReady() const -> bool override
    {
        return HasTexture() || HasPixelData();
    }

#ifdef MS_DEBUG_CANVAS
public:
    [[nodiscard]] auto GetWzPath() const noexcept -> std::string;
#endif

private:
    // WZ canvas data (parsing layer)
    std::shared_ptr<WzCanvas> m_canvas;

    // Graphics rendering data
    Point2D m_position = {0, 0};  // Canvas position in world/screen coordinates
    Point2D m_origin = {0, 0};    // Anchor point within the canvas image
    int m_nDelay = 100;           // Frame delay in ms (from WZ "delay" property)
    int m_nZ = 0;
    SDL_Texture* m_pTexture = nullptr;
};

} // namespace ms
