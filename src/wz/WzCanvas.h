#pragma once

#include "util/Point.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
#include <vector>

namespace ms
{

/**
 * @brief WZ Canvas (image data)
 *
 * Based on IWzCanvas interface from the original MapleStory client.
 * Represents image data from WZ files.
 * Supports various compression formats used in WZ files.
 */
class WzCanvas
{
public:
    WzCanvas();
    WzCanvas(int width, int height);
    ~WzCanvas();

    // Non-copyable, movable
    WzCanvas(const WzCanvas&) = delete;
    auto operator=(const WzCanvas&) -> WzCanvas& = delete;
    WzCanvas(WzCanvas&& other) noexcept;
    auto operator=(WzCanvas&& other) noexcept -> WzCanvas&;

    // Dimensions
    [[nodiscard]] auto GetWidth() const noexcept -> int { return m_nWidth; }
    [[nodiscard]] auto GetHeight() const noexcept -> int { return m_nHeight; }

    // Origin point (for sprites)
    [[nodiscard]] auto GetOrigin() const noexcept -> Point2D { return m_origin; }
    void SetOrigin(const Point2D& origin) noexcept { m_origin = origin; }

    // Z value (for layering)
    [[nodiscard]] auto GetZ() const noexcept -> int { return m_nZ; }
    void SetZ(int z) noexcept { m_nZ = z; }

    // SDL texture
    [[nodiscard]] auto GetTexture() const noexcept -> SDL_Texture* { return m_pTexture; }
    void SetTexture(SDL_Texture* texture);

    // Raw pixel data
    [[nodiscard]] auto GetPixelData() const noexcept -> const std::vector<std::uint8_t>&
    {
        return m_pixelData;
    }
    void SetPixelData(const std::vector<std::uint8_t>& data);
    void SetPixelData(std::vector<std::uint8_t>&& data);

    // Create texture from pixel data
    auto CreateTexture(SDL_Renderer* renderer) -> SDL_Texture*;

    // State checks
    [[nodiscard]] auto HasPixelData() const noexcept -> bool
    {
        return !m_pixelData.empty();
    }

    [[nodiscard]] auto HasTexture() const noexcept -> bool
    {
        return m_pTexture != nullptr;
    }

#ifdef MS_DEBUG_CANVAS
public:
    [[nodiscard]] auto GetWzPath() const noexcept -> const std::string& { return m_strWzPath; }
    void SetWzPath(const std::string& path) { m_strWzPath = path; }

private:
    std::string m_strWzPath;
#endif

private:
    int m_nWidth = 0;
    int m_nHeight = 0;
    Point2D m_origin;
    int m_nZ = 0;

    // Raw RGBA pixel data
    std::vector<std::uint8_t> m_pixelData;

    // SDL texture (cached)
    SDL_Texture* m_pTexture = nullptr;
};

} // namespace ms
