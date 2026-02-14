#include "Logo.h"
#include "Loading.h"
#include "app/Application.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "util/Logger.h"
#include "graphics/WzGr2DCanvas.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

namespace ms
{

// Timing constants from decompiled code
constexpr std::uint64_t kClickToVideoDelay = 1500;  // 0x5DC - delay after click before video
constexpr std::uint64_t kMinSkipTimeMs = 5000;      // 0x1388 - minimum time before skip allowed
constexpr std::int32_t kMessageFadeInMs = 2500;     // Fade in duration
constexpr std::int32_t kMessageFadeOutMs = 2500;    // Fade out duration
constexpr std::uint64_t kDoneWaitMs = 1000;         // Wait after logo animation ends

Logo::Logo() = default;

Logo::~Logo() = default;

void Logo::Init(void* param)
{
    Stage::Init(param);

    // Based on CLogo::Init @ 0xbc7120
    InitWZLogo();

    // Initialize timing
    m_dwTickInitial = 0;
    m_bTickInitial = false;
    m_dwClick = 0;
    m_dwDoneTick = 0;
    m_bVideoMode = false;
    m_videoState = VideoState::Unavailable;
    m_bLogoSoundPlayed = false;
    m_logoPhase = LogoPhase::Message;

    LOG_INFO("Logo stage initialized");
}

void Logo::InitWZLogo()
{
    // Based on CLogo::InitWZLogo @ 0xbc5b20
    auto& resMan = WzResMan::GetInstance();
    auto& gr = get_gr();

    auto screenW = static_cast<std::int32_t>(gr.GetWidth());
    auto screenH = static_cast<std::int32_t>(gr.GetHeight());

    auto layerX = -screenW / 2;
    auto layerY = -screenH / 2;

    // Load WZ properties
    m_pLogoProp = resMan.GetProperty("UI/Logo.img/Wizet");
    m_pGradeProp = resMan.GetProperty("UI/Logo.img/Grade");
    m_pMessageProp = resMan.GetProperty("UI/Logo.img/Message");

    // === Background layer (full screen black, z=0) ===
    auto canvasBg = std::make_shared<WzCanvas>(screenW, screenH);
    {
        std::vector<std::uint8_t> blackPixels(
            static_cast<std::size_t>(screenW) * static_cast<std::size_t>(screenH) * 4, 0);
        for (std::size_t i = 3; i < blackPixels.size(); i += 4)
        {
            blackPixels[i] = 255;
        }
        canvasBg->SetPixelData(std::move(blackPixels));
    }

    m_pLayerBackground = gr.CreateLayer(0, 0, screenW, screenH, 0);
    if (m_pLayerBackground)
    {
        m_pLayerBackground->InsertCanvas(std::make_shared<WzGr2DCanvas>(canvasBg));
        m_pLayerBackground->SetColor(0xFFFFFFFF);
        m_pLayerBackground->SetVisible(true);
    }

    // === Message layer (z=1) ===
    // Message uses Gr2D alpha interpolation: fade in (0→255), then fade out (255→0)
    m_pLayerMessage = gr.CreateLayer(layerX, layerY, gr.GetWidth(), gr.GetHeight(), 1);
    if (m_pLayerMessage && m_pMessageProp)
    {
        auto child = m_pMessageProp->GetChild("0");
        if (child)
        {
            auto wzCanvas = child->GetCanvas();
            if (wzCanvas)
            {
                auto canvas = std::make_shared<WzGr2DCanvas>(wzCanvas, child);

                // Insert same canvas twice with alpha interpolation
                // Frame 0: fade in (alpha 0 → 255 over kMessageFadeInMs)
                m_pLayerMessage->InsertCanvas(canvas, kMessageFadeInMs, 0, 255);
                // Frame 1: fade out (alpha 255 → 0 over kMessageFadeOutMs)
                m_pLayerMessage->InsertCanvas(canvas, kMessageFadeOutMs, 255, 0);

                // Start animation (play once, no repeat)
                m_pLayerMessage->Animate(Gr2DAnimationType::First, 1000, 0);
                m_pLayerMessage->SetVisible(true);

                LOG_DEBUG("Message layer: {}x{}, origin ({},{})",
                         canvas->GetWidth(), canvas->GetHeight(),
                         canvas->GetOrigin().x, canvas->GetOrigin().y);
            }
        }
    }
    if (!m_pLayerMessage || m_pLayerMessage->GetCanvasCount() == 0)
    {
        LOG_DEBUG("No message frames - will skip to logo phase");
        // No message, skip directly to logo phase
        if (m_pLayerMessage) m_pLayerMessage->SetVisible(false);
        m_logoPhase = LogoPhase::Logo;
    }

    // === Logo layer (z=2, initially hidden) ===
    // Logo uses Gr2D frame animation with WZ delay values per frame
    m_pLayerLogo = gr.CreateLayer(layerX, layerY, gr.GetWidth(), gr.GetHeight(), 2);
    if (m_pLayerLogo && m_pLogoProp)
    {
        m_pLayerLogo->SetVisible(false);

        int i = 0;
        while (auto child = m_pLogoProp->GetChild(std::to_string(i)))
        {
            ++i;

            auto wzCanvas = child->GetCanvas();
            if (!wzCanvas) continue;

            auto canvas = std::make_shared<WzGr2DCanvas>(wzCanvas, child);

            // Read delay from WZ property
            auto delayProp = child->GetChild("delay");
            int delay = delayProp ? delayProp->GetInt(100) : 100;

            m_pLayerLogo->InsertCanvas(canvas, delay, 255, 255);
        }

        LOG_INFO("Logo layer: {} frames loaded", m_pLayerLogo->GetCanvasCount());
    }

    // Flush cached objects after loading
    resMan.FlushCachedObjects(0);
}

void Logo::Update()
{
    // Based on CLogo::Update @ 0xbc7a90
    if (m_bVideoMode)
    {
        UpdateVideo();
    }
    else
    {
        UpdateLogo();
    }
}

void Logo::UpdateLogo()
{
    // Layer animation is driven by WzGr2D::RenderFrame calling layer->Update(tCur)
    // Here we just monitor phase transitions

    auto tNow = Application::GetTick();

    if (!m_bTickInitial)
    {
        m_bTickInitial = true;
        m_dwTickInitial = tNow;
    }

    // If clicked, wait 1500ms before switching to video mode
    if (m_dwClick != 0)
    {
        if (tNow - m_dwClick > kClickToVideoDelay)
        {
            m_bTickInitial = false;
            m_bVideoMode = true;
        }
        return;
    }

    switch (m_logoPhase)
    {
    case LogoPhase::Message:
        // Wait for message fade animation to complete
        if (m_pLayerMessage && !m_pLayerMessage->IsAnimating())
        {
            // Message animation done -> switch to logo phase
            m_pLayerMessage->SetVisible(false);

            if (m_pLayerLogo && m_pLayerLogo->GetCanvasCount() > 0)
            {
                m_pLayerLogo->SetVisible(true);
                m_pLayerLogo->Animate(Gr2DAnimationType::First, 1000, 0);
                m_logoPhase = LogoPhase::Logo;
                LOG_DEBUG("Switched to logo phase ({} frames)", m_pLayerLogo->GetCanvasCount());
            }
            else
            {
                // No logo frames, go directly to done
                m_logoPhase = LogoPhase::Done;
                m_dwDoneTick = tNow;
            }
        }
        break;

    case LogoPhase::Logo:
        // Play logo sound on first update of logo phase
        if (!m_bLogoSoundPlayed)
        {
            m_bLogoSoundPlayed = true;
            LOG_DEBUG("Logo sound would play here");
        }

        // Wait for logo animation to complete
        if (m_pLayerLogo && !m_pLayerLogo->IsAnimating())
        {
            m_logoPhase = LogoPhase::Done;
            m_dwDoneTick = tNow;
            LOG_DEBUG("Logo animation complete");
        }
        break;

    case LogoPhase::Done:
        // Wait a bit then switch to video/loading mode
        if (tNow - m_dwDoneTick >= kDoneWaitMs)
        {
            m_bTickInitial = false;
            m_bVideoMode = true;
        }
        break;
    }
}

void Logo::UpdateVideo()
{
    // Based on CLogo::UpdateVideo @ 0xbc5950
    if (m_videoState == VideoState::Unavailable)
    {
        GoToLoading();
        return;
    }

    if (m_videoState == VideoState::FadeOut || m_videoState == VideoState::End)
    {
        LogoEnd();
        return;
    }

    LogoEnd();
}

void Logo::Draw()
{
    // Rendering is handled by WzGr2D::RenderFrame through the layer system
}

void Logo::Close()
{
    // Based on CLogo::Close @ 0xbc7170
    Stage::Close();

    auto& gr = get_gr();

    if (m_pLayerBackground)
    {
        gr.RemoveLayer(m_pLayerBackground);
        m_pLayerBackground.reset();
    }

    if (m_pLayerMessage)
    {
        gr.RemoveLayer(m_pLayerMessage);
        m_pLayerMessage.reset();
    }

    if (m_pLayerLogo)
    {
        gr.RemoveLayer(m_pLayerLogo);
        m_pLayerLogo.reset();
    }

    m_pLogoProp.reset();
    m_pGradeProp.reset();
    m_pMessageProp.reset();

    LOG_DEBUG("Logo stage closed");
}

void Logo::LogoEnd()
{
    GoToLoading();
}

void Logo::GoToLoading()
{
    auto loading = std::make_shared<Loading>();
    Application::GetInstance().SetStage(loading);
}

auto Logo::CanSkip() const -> bool
{
    // Based on CLogo::CanSkip @ 0xbc55f0
    if (!m_bTickInitial)
    {
        return false;
    }

    auto elapsed = Application::GetTick() - m_dwTickInitial;

    if (m_bVideoMode)
    {
        return elapsed >= kMinSkipTimeMs;
    }
    else
    {
        // Can skip after message phase (logo phase started)
        return m_logoPhase != LogoPhase::Message;
    }
}

void Logo::ForcedEnd()
{
    // Based on CLogo::ForcedEnd @ 0xbc78d0
    if (m_bVideoMode)
    {
        m_videoState = VideoState::FadeOut;
    }
    else if (m_dwClick == 0)
    {
        m_dwClick = Application::GetTick();

        // Show last frame and stop animation
        if (m_pLayerLogo)
        {
            m_pLayerLogo->StopAnimation();
            auto frameCount = m_pLayerLogo->GetCanvasCount();
            if (frameCount > 0)
            {
                m_pLayerLogo->SetCurrentFrame(frameCount - 1);
            }
            m_pLayerLogo->SetVisible(true);
        }
        if (m_pLayerMessage) m_pLayerMessage->SetVisible(false);
    }
}

void Logo::OnKeyDown(std::int32_t keyCode)
{
    if (!CanSkip())
    {
        return;
    }

    if (keyCode == 13 || keyCode == 27 || keyCode == 32)
    {
        ForcedEnd();
    }
}

void Logo::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button)
{
    (void)x;
    (void)y;

    if (!CanSkip())
    {
        return;
    }

    if (button == 1)
    {
        ForcedEnd();
    }
}

} // namespace ms
