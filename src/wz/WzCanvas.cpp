#include "WzCanvas.h"
#include <utility>

namespace ms
{

WzCanvas::WzCanvas() = default;

WzCanvas::WzCanvas(int width, int height)
    : m_nWidth(width)
    , m_nHeight(height)
{
    if (width > 0 && height > 0)
    {
        m_pixelData.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0);
    }
}

WzCanvas::~WzCanvas() = default;

WzCanvas::WzCanvas(WzCanvas&& other) noexcept
    : m_nWidth(other.m_nWidth)
    , m_nHeight(other.m_nHeight)
    , m_pixelData(std::move(other.m_pixelData))
{
    other.m_nWidth = 0;
    other.m_nHeight = 0;
}

auto WzCanvas::operator=(WzCanvas&& other) noexcept -> WzCanvas&
{
    if (this != &other)
    {
        m_nWidth = other.m_nWidth;
        m_nHeight = other.m_nHeight;
        m_pixelData = std::move(other.m_pixelData);

        other.m_nWidth = 0;
        other.m_nHeight = 0;
    }
    return *this;
}

void WzCanvas::SetPixelData(const std::vector<std::uint8_t>& data)
{
    m_pixelData = data;
}

void WzCanvas::SetPixelData(std::vector<std::uint8_t>&& data)
{
    m_pixelData = std::move(data);
}

} // namespace ms
