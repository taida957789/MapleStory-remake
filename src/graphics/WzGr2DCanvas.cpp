#include "WzGr2DCanvas.h"
#include <utility>

namespace ms
{

WzGr2DCanvas::WzGr2DCanvas(std::shared_ptr<WzCanvas> canvas)
    : m_canvas(std::move(canvas))
{
}

WzGr2DCanvas::~WzGr2DCanvas()
{
    if (m_pTexture)
    {
        SDL_DestroyTexture(m_pTexture);
        m_pTexture = nullptr;
    }
}

WzGr2DCanvas::WzGr2DCanvas(WzGr2DCanvas&& other) noexcept
    : m_canvas(std::move(other.m_canvas))
    , m_position(other.m_position)
    , m_origin(other.m_origin)
    , m_nZ(other.m_nZ)
    , m_pTexture(other.m_pTexture)
{
    other.m_nZ = 0;
    other.m_pTexture = nullptr;
}

auto WzGr2DCanvas::operator=(WzGr2DCanvas&& other) noexcept -> WzGr2DCanvas&
{
    if (this != &other)
    {
        // Clean up existing texture
        if (m_pTexture)
        {
            SDL_DestroyTexture(m_pTexture);
        }

        m_canvas = std::move(other.m_canvas);
        m_position = other.m_position;
        m_origin = other.m_origin;
        m_nZ = other.m_nZ;
        m_pTexture = other.m_pTexture;

        other.m_nZ = 0;
        other.m_pTexture = nullptr;
    }
    return *this;
}

void WzGr2DCanvas::SetCanvas(std::shared_ptr<WzCanvas> canvas)
{
    m_canvas = std::move(canvas);

    // Invalidate texture when canvas changes
    if (m_pTexture)
    {
        SDL_DestroyTexture(m_pTexture);
        m_pTexture = nullptr;
    }
}

auto WzGr2DCanvas::GetWidth() const noexcept -> int
{
    return m_canvas ? m_canvas->GetWidth() : 0;
}

auto WzGr2DCanvas::GetHeight() const noexcept -> int
{
    return m_canvas ? m_canvas->GetHeight() : 0;
}

void WzGr2DCanvas::SetTexture(SDL_Texture* texture)
{
    if (m_pTexture && m_pTexture != texture)
    {
        SDL_DestroyTexture(m_pTexture);
    }
    m_pTexture = texture;
}

auto WzGr2DCanvas::CreateTexture(SDL_Renderer* renderer) -> SDL_Texture*
{
    if (m_pTexture)
        return m_pTexture;

    if (!m_canvas || !m_canvas->HasPixelData())
        return nullptr;

    const int width = m_canvas->GetWidth();
    const int height = m_canvas->GetHeight();

    if (width <= 0 || height <= 0)
        return nullptr;

    // Create texture from pixel data (SDL3 API)
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        width,
        height,
        SDL_PIXELFORMAT_RGBA32,
        const_cast<void*>(static_cast<const void*>(m_canvas->GetPixelData().data())),
        width * 4);

    if (!surface)
        return nullptr;

    m_pTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    return m_pTexture;
}

auto WzGr2DCanvas::HasPixelData() const noexcept -> bool
{
    return m_canvas && m_canvas->HasPixelData();
}

#ifdef MS_DEBUG_CANVAS
auto WzGr2DCanvas::GetWzPath() const noexcept -> std::string
{
    return m_canvas ? m_canvas->GetWzPath() : "";
}
#endif

} // namespace ms
