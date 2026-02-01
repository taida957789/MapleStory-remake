#include "WzGr2DLayer.h"
#include "wz/WzCanvas.h"

#ifdef MS_DEBUG_CANVAS
#include "util/Logger.h"
#endif

#include <SDL3/SDL.h>
#include <algorithm>

namespace ms
{

WzGr2DLayer::WzGr2DLayer() = default;

WzGr2DLayer::WzGr2DLayer(std::int32_t left, std::int32_t top,
                         std::uint32_t width, std::uint32_t height,
                         std::int32_t z)
    : m_nLeft(left)
    , m_nTop(top)
    , m_uWidth(width)
    , m_uHeight(height)
    , m_nZ(z)
{
}

WzGr2DLayer::~WzGr2DLayer() = default;

WzGr2DLayer::WzGr2DLayer(WzGr2DLayer&&) noexcept = default;
auto WzGr2DLayer::operator=(WzGr2DLayer&&) noexcept -> WzGr2DLayer& = default;

void WzGr2DLayer::SetPosition(std::int32_t left, std::int32_t top) noexcept
{
    m_nLeft = left;
    m_nTop = top;
}

auto WzGr2DLayer::InsertCanvas(std::shared_ptr<WzCanvas> canvas,
                                std::int32_t delay,
                                std::uint8_t alpha0,
                                std::uint8_t alpha1,
                                std::int32_t zoom0,
                                std::int32_t zoom1) -> std::size_t
{
    CanvasEntry entry;
    entry.canvas = std::move(canvas);
    entry.frameInfo.nDelay = delay;
    entry.frameInfo.nAlpha0 = alpha0;
    entry.frameInfo.nAlpha1 = alpha1;
    entry.frameInfo.nZoom0 = zoom0;
    entry.frameInfo.nZoom1 = zoom1;

    m_canvases.push_back(std::move(entry));
    return m_canvases.size() - 1;
}

auto WzGr2DLayer::RemoveCanvas(std::size_t index) -> std::shared_ptr<WzCanvas>
{
    if (index >= m_canvases.size())
    {
        return nullptr;
    }

    auto canvas = std::move(m_canvases[index].canvas);
    m_canvases.erase(m_canvases.begin() + static_cast<std::ptrdiff_t>(index));

    // Adjust current frame if needed
    if (m_nCurrentFrame >= m_canvases.size() && !m_canvases.empty())
    {
        m_nCurrentFrame = m_canvases.size() - 1;
    }

    return canvas;
}

void WzGr2DLayer::RemoveAllCanvases()
{
    m_canvases.clear();
    m_nCurrentFrame = 0;
    m_bAnimating = false;
}

auto WzGr2DLayer::GetCanvasCount() const noexcept -> std::size_t
{
    return m_canvases.size();
}

auto WzGr2DLayer::GetCanvas(std::size_t index) const -> std::shared_ptr<WzCanvas>
{
    if (index >= m_canvases.size())
    {
        return nullptr;
    }
    return m_canvases[index].canvas;
}

auto WzGr2DLayer::GetCurrentCanvas() const -> std::shared_ptr<WzCanvas>
{
    return GetCanvas(m_nCurrentFrame);
}

auto WzGr2DLayer::Animate(Gr2DAnimationType type,
                           std::int32_t delayRate,
                           std::int32_t repeat) -> bool
{
    if (m_canvases.size() < 2)
    {
        // Nothing to animate with less than 2 frames
        return false;
    }

    m_animType = type;
    m_nDelayRate = delayRate;
    m_nRepeatCount = repeat;
    m_nCurrentRepeat = 0;
    m_bAnimating = true;
    m_animState = AnimationState::Forward;
    m_bReverseDirection = (type == Gr2DAnimationType::Reverse ||
                           type == Gr2DAnimationType::ReverseLoop);

    if (m_bReverseDirection)
    {
        m_nCurrentFrame = m_canvases.size() - 1;
        m_animState = AnimationState::Backward;
    }
    else
    {
        m_nCurrentFrame = 0;
    }

    m_tLastFrameTime = 0; // Will be set on first update

    return true;
}

void WzGr2DLayer::StopAnimation()
{
    m_bAnimating = false;
    m_animType = Gr2DAnimationType::None;
    m_animState = AnimationState::Stopped;
}

void WzGr2DLayer::SetCurrentFrame(std::size_t frame)
{
    if (frame < m_canvases.size())
    {
        m_nCurrentFrame = frame;
    }
}

void WzGr2DLayer::SetPostRenderCallback(PostRenderCallback callback)
{
    m_postRenderCallback = std::move(callback);
}

void WzGr2DLayer::StartPositionAnimation(std::int32_t offsetX, std::int32_t offsetY,
                                          std::int32_t duration, bool loop)
{
    // Store initial position if not already animating
    if (!m_bPositionAnimating)
    {
        m_nInitialLeft = m_nLeft;
        m_nInitialTop = m_nTop;
    }

    m_nAnimOffsetX = offsetX;
    m_nAnimOffsetY = offsetY;
    m_nAnimDuration = duration > 0 ? duration : 1000;
    m_bAnimLoop = loop;
    m_bPositionAnimating = true;
    m_tAnimStart = 0;  // Will be set on first update
}

void WzGr2DLayer::StopPositionAnimation()
{
    if (m_bPositionAnimating)
    {
        // Restore initial position
        m_nLeft = m_nInitialLeft;
        m_nTop = m_nInitialTop;
        m_bPositionAnimating = false;
    }
}

void WzGr2DLayer::Update(std::int32_t tCur)
{
    // Update position animation (for background type 4-7)
    if (m_bPositionAnimating)
    {
        // Initialize animation start time on first update
        if (m_tAnimStart == 0)
        {
            m_tAnimStart = tCur;
        }

        // Calculate elapsed time
        auto elapsed = tCur - m_tAnimStart;

        if (m_bAnimLoop)
        {
            // Loop animation - use modulo to repeat
            elapsed = elapsed % m_nAnimDuration;
        }
        else if (elapsed >= m_nAnimDuration)
        {
            // Non-looping animation finished
            elapsed = m_nAnimDuration;
            m_bPositionAnimating = false;
        }

        // Calculate interpolated position (ping-pong for looping, based on original RelMove behavior)
        // Original: moves from initial → initial+offset → initial (loop)
        float t = static_cast<float>(elapsed) / static_cast<float>(m_nAnimDuration);

        if (m_bAnimLoop)
        {
            // Ping-pong: 0→1→0
            if (t <= 0.5F)
            {
                t = t * 2.0F;  // 0→1 in first half
            }
            else
            {
                t = (1.0F - t) * 2.0F;  // 1→0 in second half
            }
        }

        // Apply interpolated offset
        m_nLeft = m_nInitialLeft + static_cast<std::int32_t>(static_cast<float>(m_nAnimOffsetX) * t);
        m_nTop = m_nInitialTop + static_cast<std::int32_t>(static_cast<float>(m_nAnimOffsetY) * t);
    }

    // Update frame animation
    if (!m_bAnimating || m_canvases.empty())
    {
        return;
    }

    // Initialize time on first update
    if (m_tLastFrameTime == 0)
    {
        m_tLastFrameTime = tCur;
        return;
    }

    // Get current frame delay (using thousandths, matching original)
    // Original: v34 = v62 * v33[3]; v64 += v34 / 0x3E8;
    const auto& currentEntry = m_canvases[m_nCurrentFrame];
    auto delay = (currentEntry.frameInfo.nDelay * m_nDelayRate) / 1000;

    if (delay <= 0)
    {
        delay = 1;
    }

    // Update interpolation values
    UpdateFrameInterpolation(tCur);

    // Check if we should advance to next frame
    if (tCur - m_tLastFrameTime >= delay)
    {
        AdvanceFrame();
        m_tLastFrameTime = tCur;
    }
}

void WzGr2DLayer::AdvanceFrame()
{
    if (m_canvases.empty())
    {
        return;
    }

    switch (m_animType)
    {
    case Gr2DAnimationType::Normal:
        if (m_nCurrentFrame + 1 < m_canvases.size())
        {
            ++m_nCurrentFrame;
        }
        else
        {
            // Animation complete
            m_bAnimating = false;
        }
        break;

    case Gr2DAnimationType::Loop:
        m_nCurrentFrame = (m_nCurrentFrame + 1) % m_canvases.size();
        if (m_nCurrentFrame == 0 && m_nRepeatCount > 0)
        {
            ++m_nCurrentRepeat;
            if (m_nCurrentRepeat >= m_nRepeatCount)
            {
                m_bAnimating = false;
            }
        }
        break;

    case Gr2DAnimationType::PingPong:
        if (m_bReverseDirection)
        {
            if (m_nCurrentFrame > 0)
            {
                --m_nCurrentFrame;
            }
            else
            {
                m_bReverseDirection = false;
                if (m_nRepeatCount > 0)
                {
                    ++m_nCurrentRepeat;
                    if (m_nCurrentRepeat >= m_nRepeatCount)
                    {
                        m_bAnimating = false;
                    }
                }
            }
        }
        else
        {
            if (m_nCurrentFrame + 1 < m_canvases.size())
            {
                ++m_nCurrentFrame;
            }
            else
            {
                m_bReverseDirection = true;
            }
        }
        break;

    case Gr2DAnimationType::Reverse:
        if (m_nCurrentFrame > 0)
        {
            --m_nCurrentFrame;
        }
        else
        {
            m_bAnimating = false;
        }
        break;

    case Gr2DAnimationType::ReverseLoop:
        if (m_nCurrentFrame > 0)
        {
            --m_nCurrentFrame;
        }
        else
        {
            m_nCurrentFrame = m_canvases.size() - 1;
            if (m_nRepeatCount > 0)
            {
                ++m_nCurrentRepeat;
                if (m_nCurrentRepeat >= m_nRepeatCount)
                {
                    m_bAnimating = false;
                }
            }
        }
        break;

    default:
        break;
    }
}

void WzGr2DLayer::UpdateFrameInterpolation(std::int32_t tCur)
{
    if (m_canvases.empty())
    {
        return;
    }

    const auto& currentEntry = m_canvases[m_nCurrentFrame];
    const auto& frameInfo = currentEntry.frameInfo;

    // Calculate delay using thousandths (matching original)
    auto delay = (frameInfo.nDelay * m_nDelayRate) / 1000;
    if (delay <= 0)
    {
        delay = 1;
    }

    // Calculate elapsed time and interpolation factor
    auto elapsed = tCur - m_tLastFrameTime;
    if (elapsed < 0)
    {
        elapsed = 0;
    }
    if (elapsed > delay)
    {
        elapsed = delay;
    }

    // Interpolate alpha (linear interpolation)
    // alpha = alpha0 + (alpha1 - alpha0) * elapsed / delay
    m_nCurrentAlpha = static_cast<std::uint8_t>(
        frameInfo.nAlpha0 +
        ((frameInfo.nAlpha1 - frameInfo.nAlpha0) * elapsed) / delay);

    // Interpolate zoom (using thousandths, matching original)
    // zoom = zoom0 + (zoom1 - zoom0) * elapsed / delay
    m_nCurrentZoom = frameInfo.nZoom0 +
        ((frameInfo.nZoom1 - frameInfo.nZoom0) * elapsed) / delay;
}

void WzGr2DLayer::Render(SDL_Renderer* renderer, std::int32_t offsetX, std::int32_t offsetY)
{
    if (!m_bVisible || m_canvases.empty() || renderer == nullptr)
    {
        return;
    }

    auto canvas = GetCurrentCanvas();
    if (!canvas)
    {
        return;
    }

    // Get or create SDL texture
    auto* texture = canvas->GetTexture();
    if (!texture)
    {
        texture = canvas->CreateTexture(renderer);
        if (!texture)
        {
            return;
        }
    }

    // Get canvas properties
    auto canvasOrigin = canvas->GetOrigin();
    auto canvasWidth = static_cast<float>(canvas->GetWidth());
    auto canvasHeight = static_cast<float>(canvas->GetHeight());

    // Calculate zoom factor from thousandths (matching original)
    // Original: v5 = (float)v8 / 1000.0; (CGr2DLayer_ApplyScaleTransform @ 0x53211160)
    auto zoomFactor = static_cast<float>(m_nCurrentZoom) / 1000.0F;

    // Calculate the zoom center point (matching original)
    // Original formula: v14 = (float)((int)a5 + v7[73]) + 0.5;
    // Where a5 = canvas anchor X, v7[73] = layer offset (we use 0 for simplicity)
    // The zoom center is the canvas origin/anchor point
    auto zoomCenterX = static_cast<float>(canvasOrigin.x) + 0.5F;
    auto zoomCenterY = static_cast<float>(canvasOrigin.y) + 0.5F;

    // Calculate base render position (layer position + camera offset with parallax)
    // MapleStory parallax system:
    // - rx=-100: Layer follows camera 100% (appears at fixed world position)
    // - rx=0: Layer doesn't follow camera (fixed on screen, position is screen-relative)
    // - rx=100: Layer scrolls at full camera speed
    // - Negative values: absolute value is the parallax, layer at world position
    // The offset parameter contains: -cameraPos + screenCenter
    float baseX, baseY;

    // Screen-space layers have special positioning
    if (m_bScreenSpace)
    {
        if (m_bCenterBased)
        {
            // Center-based: position is relative to screen center
            // offsetX/offsetY contain screen center (passed by WzGr2D)
            // So layer at (-400, -300) renders at (screenCenterX - 400, screenCenterY - 300)
            baseX = static_cast<float>(m_nLeft + offsetX);
            baseY = static_cast<float>(m_nTop + offsetY);

// #ifdef MS_DEBUG_CANVAS
//             static int debugCounter = 0;
//             if (debugCounter++ % 60 == 0)  // Log every 60 frames
//             {
//                 LOG_DEBUG("Screen-space (center-based) layer: pos=({},{}), offset=({},{}), base=({},{})",
//                           m_nLeft, m_nTop, offsetX, offsetY, baseX, baseY);
//             }
// #endif
        }
        else
        {
            // Standard screen-space: position is absolute screen coordinates (0,0 = top-left)
            baseX = static_cast<float>(m_nLeft);
            baseY = static_cast<float>(m_nTop);

// #ifdef MS_DEBUG_CANVAS
//             static int debugCounter2 = 0;
//             if (debugCounter2++ % 60 == 0)  // Log every 60 frames
//             {
//                 LOG_DEBUG("Screen-space layer: pos=({},{}), base=({},{}), z={}",
//                           m_nLeft, m_nTop, baseX, baseY, m_nZ);
//             }
// #endif
        }
    }
    // Handle X parallax
    else if (m_nParallaxRx <= 0)
    {
        // Negative or zero parallax: layer position is in world coordinates
        // Apply full camera offset to convert to screen coordinates
        // For rx=-100, use 100% camera follow; for rx=0, also use full offset
        // This makes the layer appear at its world position
        baseX = static_cast<float>(m_nLeft + offsetX);
    }
    else
    {
        // Positive parallax: scale the camera offset
        // rx=100 means full camera follow, rx=50 means half speed, etc.
        auto parallaxOffsetX = (offsetX * m_nParallaxRx) / 100;
        baseX = static_cast<float>(m_nLeft + parallaxOffsetX);
    }

    // Handle Y parallax for non-screen-space layers
    // (screen-space Y is already handled in the screen-space block above)
    if (!m_bScreenSpace)
    {
        if (m_nParallaxRy <= 0)
        {
            baseY = static_cast<float>(m_nTop + offsetY);
        }
        else
        {
            auto parallaxOffsetY = (offsetY * m_nParallaxRy) / 100;
            baseY = static_cast<float>(m_nTop + parallaxOffsetY);
        }
    }
    // For screen-space layers, baseY was already set above

    // Apply zoom transformation around canvas origin (matching original)
    // Original: result = (int)(float)((float)((float)((float)(int)v20 - v14) * v23) + v14);
    // This scales the position relative to the zoom center, then adds back the center
    float renderX, renderY, renderWidth, renderHeight;
    if (m_nCurrentZoom != 1000)
    {
        // Scale the canvas origin offset by the zoom factor
        // The render position is: basePos - scaledOrigin
        auto scaledOriginX = zoomCenterX * zoomFactor;
        auto scaledOriginY = zoomCenterY * zoomFactor;

        renderX = baseX - scaledOriginX;
        renderY = baseY - scaledOriginY;
        renderWidth = canvasWidth * zoomFactor;
        renderHeight = canvasHeight * zoomFactor;
    }
    else
    {
        // No scaling: standard origin offset
        renderX = baseX - static_cast<float>(canvasOrigin.x);
        renderY = baseY - static_cast<float>(canvasOrigin.y);
        renderWidth = canvasWidth;
        renderHeight = canvasHeight;
    }

// #ifdef MS_DEBUG_CANVAS
//     if (m_bScreenSpace)
//     {
//         static int debugCounter3 = 0;
//         if (debugCounter3++ % 60 == 0)  // Log every 60 frames
//         {
//             LOG_DEBUG("Screen-space render: base=({:.1f},{:.1f}), origin=({},{}), final=({:.1f},{:.1f}), z={}",
//                       baseX, baseY, canvasOrigin.x, canvasOrigin.y, renderX, renderY, m_nZ);
//         }
//     }
// #endif

    // Apply color modulation
    auto alpha = static_cast<std::uint8_t>((m_dwColor >> 24) & 0xFF);
    auto red = static_cast<std::uint8_t>((m_dwColor >> 16) & 0xFF);
    auto green = static_cast<std::uint8_t>((m_dwColor >> 8) & 0xFF);
    auto blue = static_cast<std::uint8_t>(m_dwColor & 0xFF);

    // Combine layer alpha with frame interpolated alpha
    alpha = static_cast<std::uint8_t>(
        (static_cast<int>(alpha) * static_cast<int>(m_nCurrentAlpha)) / 255);

    SDL_SetTextureColorMod(texture, red, green, blue);
    SDL_SetTextureAlphaMod(texture, alpha);

    // Handle flip
    SDL_FlipMode flipMode = SDL_FLIP_NONE;
    if (m_flipState == LayerFlipState::Horizontal ||
        m_flipState == LayerFlipState::Both)
    {
        flipMode = static_cast<SDL_FlipMode>(flipMode | SDL_FLIP_HORIZONTAL);
    }
    if (m_flipState == LayerFlipState::Vertical ||
        m_flipState == LayerFlipState::Both)
    {
        flipMode = static_cast<SDL_FlipMode>(flipMode | SDL_FLIP_VERTICAL);
    }

    // Get viewport size for tiling calculation
    int viewportW = 0;
    int viewportH = 0;
    SDL_GetRenderOutputSize(renderer, &viewportW, &viewportH);

    // Calculate tile distances (use canvas dimensions if cx/cy is 0)
    auto tileCx = m_nTileCx > 0 ? static_cast<float>(m_nTileCx) : renderWidth;
    auto tileCy = m_nTileCy > 0 ? static_cast<float>(m_nTileCy) : renderHeight;

    // Determine tiling range
    int tilesX = 1;
    int tilesY = 1;
    float startTileX = renderX;
    float startTileY = renderY;

    if (m_nTileCx > 0)
    {
        // Calculate how many tiles needed to cover viewport horizontally
        // Start from a position that ensures we cover the left edge
        while (startTileX > 0)
        {
            startTileX -= tileCx;
        }
        // Count tiles needed to cover from startTileX to viewport right edge
        tilesX = static_cast<int>((static_cast<float>(viewportW) - startTileX) / tileCx) + 2;
    }

    if (m_nTileCy > 0)
    {
        // Calculate how many tiles needed to cover viewport vertically
        while (startTileY > 0)
        {
            startTileY -= tileCy;
        }
        tilesY = static_cast<int>((static_cast<float>(viewportH) - startTileY) / tileCy) + 2;
    }

    // Render tiles
    for (int ty = 0; ty < tilesY; ++ty)
    {
        for (int tx = 0; tx < tilesX; ++tx)
        {
            float tileX = startTileX + static_cast<float>(tx) * tileCx;
            float tileY = startTileY + static_cast<float>(ty) * tileCy;

            // Skip tiles outside viewport
            if (tileX + renderWidth < 0 || tileX > static_cast<float>(viewportW) ||
                tileY + renderHeight < 0 || tileY > static_cast<float>(viewportH))
            {
                continue;
            }

            SDL_FRect dstRect{
                tileX,
                tileY,
                renderWidth,
                renderHeight
            };

            if (flipMode != SDL_FLIP_NONE || m_nCurrentZoom != 1000)
            {
                SDL_RenderTextureRotated(renderer, texture, nullptr, &dstRect,
                                          0.0, nullptr, flipMode);
            }
            else
            {
                SDL_RenderTexture(renderer, texture, nullptr, &dstRect);
            }
        }
    }

    // Post-render callback
    if (m_postRenderCallback)
    {
        m_postRenderCallback(*this, m_tLastFrameTime);
    }
}

} // namespace ms
