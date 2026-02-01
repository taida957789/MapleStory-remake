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

WzCanvas::~WzCanvas()
{
    if (m_pTexture)
    {
        SDL_DestroyTexture(m_pTexture);
        m_pTexture = nullptr;
    }
}

WzCanvas::WzCanvas(WzCanvas&& other) noexcept
    : m_nWidth(other.m_nWidth)
    , m_nHeight(other.m_nHeight)
    , m_origin(other.m_origin)
    , m_nZ(other.m_nZ)
    , m_pixelData(std::move(other.m_pixelData))
    , m_pTexture(other.m_pTexture)
{
    other.m_nWidth = 0;
    other.m_nHeight = 0;
    other.m_nZ = 0;
    other.m_pTexture = nullptr;
}

auto WzCanvas::operator=(WzCanvas&& other) noexcept -> WzCanvas&
{
    if (this != &other)
    {
        // Clean up existing texture
        if (m_pTexture)
        {
            SDL_DestroyTexture(m_pTexture);
        }

        m_nWidth = other.m_nWidth;
        m_nHeight = other.m_nHeight;
        m_origin = other.m_origin;
        m_nZ = other.m_nZ;
        m_pixelData = std::move(other.m_pixelData);
        m_pTexture = other.m_pTexture;

        other.m_nWidth = 0;
        other.m_nHeight = 0;
        other.m_nZ = 0;
        other.m_pTexture = nullptr;
    }
    return *this;
}

void WzCanvas::SetTexture(SDL_Texture* texture)
{
    if (m_pTexture && m_pTexture != texture)
    {
        SDL_DestroyTexture(m_pTexture);
    }
    m_pTexture = texture;
}

void WzCanvas::SetPixelData(const std::vector<std::uint8_t>& data)
{
    m_pixelData = data;

    // Invalidate texture when pixel data changes
    if (m_pTexture)
    {
        SDL_DestroyTexture(m_pTexture);
        m_pTexture = nullptr;
    }
}

void WzCanvas::SetPixelData(std::vector<std::uint8_t>&& data)
{
    m_pixelData = std::move(data);

    // Invalidate texture when pixel data changes
    if (m_pTexture)
    {
        SDL_DestroyTexture(m_pTexture);
        m_pTexture = nullptr;
    }
}

auto WzCanvas::CreateTexture(SDL_Renderer* renderer) -> SDL_Texture*
{
    if (m_pTexture)
        return m_pTexture;

    if (m_pixelData.empty() || m_nWidth <= 0 || m_nHeight <= 0)
        return nullptr;

    // Create texture from pixel data (SDL3 API)
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        m_nWidth,
        m_nHeight,
        SDL_PIXELFORMAT_RGBA32,
        m_pixelData.data(),
        m_nWidth * 4);

    if (!surface)
        return nullptr;

    m_pTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    return m_pTexture;
}

} // namespace ms
