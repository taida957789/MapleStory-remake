#pragma once

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

    // Raw pixel data
    [[nodiscard]] auto GetPixelData() const noexcept -> const std::vector<std::uint8_t>&
    {
        return m_pixelData;
    }
    void SetPixelData(const std::vector<std::uint8_t>& data);
    void SetPixelData(std::vector<std::uint8_t>&& data);

    // State checks
    [[nodiscard]] auto HasPixelData() const noexcept -> bool
    {
        return !m_pixelData.empty();
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

    // Raw RGBA pixel data
    std::vector<std::uint8_t> m_pixelData;
};

} // namespace ms
