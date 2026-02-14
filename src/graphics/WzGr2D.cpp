#include "WzGr2D.h"
#include "WzGr2DLayer.h"
#include "WzGr2DCanvas.h"

#ifdef MS_DEBUG_CANVAS
#include "debug/DebugOverlay.h"
#endif

#include <SDL3/SDL.h>
#include <algorithm>

namespace ms
{

WzGr2D::WzGr2D() = default;

WzGr2D::~WzGr2D()
{
    Shutdown();
}

auto WzGr2D::Initialize(std::uint32_t width, std::uint32_t height,
                         SDL_Window* window,
                         std::int32_t bpp,
                         std::int32_t refreshRate) -> bool
{
    if (m_bInitialized)
    {
        return true;
    }

    m_uWidth = width;
    m_uHeight = height;
    m_nBpp = bpp;

    // Use provided window or create our own
    if (window != nullptr)
    {
        m_pWindow = window;
        m_bOwnWindow = false;
    }
    else
    {
        // Initialize SDL video subsystem if not already done
        if (!SDL_WasInit(SDL_INIT_VIDEO))
        {
            if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
            {
                SDL_Log("Failed to initialize SDL video: %s", SDL_GetError());
                return false;
            }
        }

        // Create window
        Uint32 windowFlags = SDL_WINDOW_RESIZABLE;
        if (m_bFullScreen)
        {
            windowFlags |= SDL_WINDOW_FULLSCREEN;
        }

        m_pWindow = SDL_CreateWindow("MapleStory",
                                      static_cast<int>(width),
                                      static_cast<int>(height),
                                      windowFlags);
        if (!m_pWindow)
        {
            SDL_Log("Failed to create window: %s", SDL_GetError());
            return false;
        }
        m_bOwnWindow = true;
    }

    // Create renderer
    m_pRenderer = SDL_CreateRenderer(m_pWindow, nullptr);
    if (!m_pRenderer)
    {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        if (m_bOwnWindow)
        {
            SDL_DestroyWindow(m_pWindow);
            m_pWindow = nullptr;
        }
        return false;
    }

    // Set VSync based on refresh rate
    if (refreshRate > 0)
    {
        m_nTargetFrameTime = 1000 / refreshRate;
    }
    else
    {
        // Default to ~60 FPS
        m_nTargetFrameTime = 16;
    }

    // Enable VSync
    SDL_SetRenderVSync(m_pRenderer, 1);

    // Set blend mode for alpha blending
    SDL_SetRenderDrawBlendMode(m_pRenderer, SDL_BLENDMODE_BLEND);

    m_bInitialized = true;
    m_tCurrent = static_cast<std::int32_t>(SDL_GetTicks());
    m_tLastFrame = m_tCurrent;
    m_tFpsUpdateTime = m_tCurrent;

    // Initialize tone vectors to full brightness (matches CWvsApp::InitializeGr2D)
    // redTone.put_x(255); greenBlueTone.Move(255, 255);
    m_vecRedTone.PutX(255);
    m_vecGreenBlueTone.Move(255, 255);

    return true;
}

void WzGr2D::Shutdown()
{
    if (!m_bInitialized)
    {
        return;
    }

    // Clear all layers
    RemoveAllLayers();

    // Destroy renderer
    if (m_pRenderer)
    {
        SDL_DestroyRenderer(m_pRenderer);
        m_pRenderer = nullptr;
    }

    // Destroy window if we own it
    if (m_bOwnWindow && m_pWindow)
    {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
        m_bOwnWindow = false;
    }

    m_bInitialized = false;
}

auto WzGr2D::GetCenter() const noexcept -> Point2D
{
    return Point2D{static_cast<std::int32_t>(m_uWidth / 2),
                   static_cast<std::int32_t>(m_uHeight / 2)};
}

void WzGr2D::SetFullScreen(bool fullscreen)
{
    if (m_bFullScreen == fullscreen || !m_pWindow)
    {
        return;
    }

    m_bFullScreen = fullscreen;
    SDL_SetWindowFullscreen(m_pWindow, fullscreen);
}

auto WzGr2D::GetNextRenderTime() const noexcept -> std::int32_t
{
    return m_tLastFrame + m_nTargetFrameTime;
}

void WzGr2D::UpdateCurrentTime(std::int32_t tCur)
{
    m_tCurrent = tCur;
}

auto WzGr2D::CreateLayer(std::int32_t left, std::int32_t top,
                          std::uint32_t width, std::uint32_t height,
                          std::int32_t z,
                          std::shared_ptr<WzGr2DCanvas> canvas,
                          std::uint32_t /*filter*/) -> std::shared_ptr<WzGr2DLayer>
{
    auto layer = std::make_shared<WzGr2DLayer>(left, top, width, height, z);

    // Add initial canvas if provided
    if (canvas)
    {
        layer->InsertCanvas(std::move(canvas));
    }

    // Insert layer maintaining Z-order
    m_layers.push_back(layer);
    m_bLayersDirty = true;

    return layer;
}

void WzGr2D::RemoveLayer(const std::shared_ptr<WzGr2DLayer>& layer)
{
    auto it = std::find(m_layers.begin(), m_layers.end(), layer);
    if (it != m_layers.end())
    {
        m_layers.erase(it);
    }
}

void WzGr2D::RemoveAllLayers()
{
    m_layers.clear();
    m_bLayersDirty = false;
}

auto WzGr2D::GetLayerCount() const noexcept -> std::size_t
{
    return m_layers.size();
}

auto WzGr2D::RenderFrame(std::int32_t tCur) -> bool
{
    if (!m_bInitialized || !m_pRenderer)
    {
        return false;
    }

    // Update time
    m_tCurrent = tCur;

    // Check if it's time to render
    if (tCur < GetNextRenderTime())
    {
        return false;
    }

    // Sort layers if needed
    SortLayers();

    // Clear screen with background color
    auto alpha = static_cast<std::uint8_t>((m_dwBackColor >> 24) & 0xFF);
    auto red = static_cast<std::uint8_t>((m_dwBackColor >> 16) & 0xFF);
    auto green = static_cast<std::uint8_t>((m_dwBackColor >> 8) & 0xFF);
    auto blue = static_cast<std::uint8_t>(m_dwBackColor & 0xFF);

    SDL_SetRenderDrawColor(m_pRenderer, red, green, blue, alpha);
    SDL_RenderClear(m_pRenderer);

    // Update and render all layers
    // MapleStory uses a coordinate system where (0,0) is at screen center
    // Both world-space and screen-space layers use this center-based system
    auto screenCenterX = static_cast<std::int32_t>(m_uWidth / 2);
    auto screenCenterY = static_cast<std::int32_t>(m_uHeight / 2);

    // Evaluate center vector (resolves RelMove/WrapClip chain)
    auto camX = m_vecCenter.GetX();
    auto camY = m_vecCenter.GetY();

    for (auto& layer : m_layers)
    {
        if (layer)
        {
            layer->Update(tCur);

            // All layers use world-space coordinates with camera offset
            layer->Render(m_pRenderer,
                          -camX + screenCenterX,
                          -camY + screenCenterY);
        }
    }

    // Apply screen tone modulation (redTone / greenBlueTone)
    {
        const auto r = static_cast<std::uint8_t>(
            std::clamp(m_vecRedTone.GetX(), 0, 255));
        const auto g = static_cast<std::uint8_t>(
            std::clamp(m_vecGreenBlueTone.GetX(), 0, 255));
        const auto b = static_cast<std::uint8_t>(
            std::clamp(m_vecGreenBlueTone.GetY(), 0, 255));

        if (r != 255 || g != 255 || b != 255)
        {
            // Multiply blend: dstRGB = srcRGB * dstRGB (with srcA=255)
            SDL_SetRenderDrawBlendMode(m_pRenderer, SDL_BLENDMODE_MUL);
            SDL_SetRenderDrawColor(m_pRenderer, r, g, b, 255);

            const SDL_FRect fullScreen{0.0F, 0.0F,
                                       static_cast<float>(m_uWidth),
                                       static_cast<float>(m_uHeight)};
            SDL_RenderFillRect(m_pRenderer, &fullScreen);

            // Restore default blend mode
            SDL_SetRenderDrawBlendMode(m_pRenderer, SDL_BLENDMODE_BLEND);
        }
    }

    // Render debug overlay (always on top)
#ifdef MS_DEBUG_CANVAS
    DebugOverlay::GetInstance().Render(m_pRenderer);
#endif

    // Present
    SDL_RenderPresent(m_pRenderer);

    // Update FPS counter
    UpdateFps(tCur);

    m_tLastFrame = tCur;
    return true;
}

auto WzGr2D::CheckMode(std::uint32_t width, std::uint32_t height,
                        std::uint32_t bpp) const -> bool
{
    // With modern graphics cards and SDL3, most modes are supported
    // We could query display modes here, but for simplicity assume all are valid
    return width >= 640 && height >= 480 && (bpp == 16 || bpp == 24 || bpp == 32);
}

auto WzGr2D::ScreenToWorld(const Point2D& screenPos) -> Point2D
{
    // Convert screen coordinates to world coordinates
    // Screen center = world camera position
    auto screenCenterX = static_cast<std::int32_t>(m_uWidth / 2);
    auto screenCenterY = static_cast<std::int32_t>(m_uHeight / 2);
    auto camX = m_vecCenter.GetX();
    auto camY = m_vecCenter.GetY();
    return Point2D{screenPos.x - screenCenterX + camX,
                   screenPos.y - screenCenterY + camY};
}

auto WzGr2D::WorldToScreen(const Point2D& worldPos) -> Point2D
{
    // Convert world coordinates to screen coordinates
    // World at camera position = screen center
    auto screenCenterX = static_cast<std::int32_t>(m_uWidth / 2);
    auto screenCenterY = static_cast<std::int32_t>(m_uHeight / 2);
    auto camX = m_vecCenter.GetX();
    auto camY = m_vecCenter.GetY();
    return Point2D{worldPos.x - camX + screenCenterX,
                   worldPos.y - camY + screenCenterY};
}

void WzGr2D::UpdateFps(std::int32_t tCur)
{
    ++m_nFrameCount;

    // Update FPS every second
    if (tCur - m_tFpsUpdateTime >= 1000)
    {
        m_uFps100 = static_cast<std::uint32_t>(m_nFrameCount * 100000 /
                                                (tCur - m_tFpsUpdateTime));
        m_nFrameCount = 0;
        m_tFpsUpdateTime = tCur;
    }
}

void WzGr2D::SortLayers()
{
    if (!m_bLayersDirty)
    {
        return;
    }

    // Sort by Z-order (lower Z = rendered first = behind)
    std::stable_sort(m_layers.begin(), m_layers.end(),
                     [](const auto& a, const auto& b) {
                         return a->GetZ() < b->GetZ();
                     });

    m_bLayersDirty = false;
}

} // namespace ms
