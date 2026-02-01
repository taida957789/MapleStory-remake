#include "Logo.h"
#include "Login.h"
#include "app/Application.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "util/Logger.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

namespace ms
{

// Timing constants from decompiled code
constexpr std::uint64_t kClickToVideoDelay = 1500;  // 0x5DC - delay after click before video
constexpr std::uint64_t kMinSkipTimeMs = 5000;      // 0x1388 - minimum time before skip allowed
constexpr std::int32_t kMessageFrames = 50;         // Frames 0-49 for message display
constexpr std::int32_t kFinalFrame = 76;            // Frame shown when skipping

Logo::Logo() = default;

Logo::~Logo() = default;

void Logo::Init(void* param)
{
    Stage::Init(param);

    // Based on CLogo::Init @ 0xbc7120
    // Calls InitWZLogo() then records timing
    InitWZLogo();

    // Initialize timing
    m_dwTickInitial = 0;
    m_bTickInitial = false;
    m_dwClick = 0;
    m_bVideoMode = false;
    m_videoState = VideoState::Unavailable;
    m_bLogoSoundPlayed = false;

    LOG_INFO("Logo stage initialized (logoCount={}, gradeCount={})", m_nLogoCount, m_nGradeCount);
}

void Logo::InitWZLogo()
{
    // Based on CLogo::InitWZLogo @ 0xbc5b20
    // Loads resources from UI/Logo.img sub-properties

    auto& resMan = WzResMan::GetInstance();
    auto& gr = get_gr();

    // Load logo property: UI/Logo.img/Logo (from decompiled StringPool 0x958)
    m_pLogoProp = resMan.GetProperty("UI/Logo.img/Logo");
    if (m_pLogoProp && m_pLogoProp->HasChildren())
    {
        // m_nLogoCount = logo frames count + 50 (for message phase)
        m_nLogoCount = static_cast<std::int32_t>(m_pLogoProp->GetChildCount()) + kMessageFrames;
        LOG_DEBUG("Loaded UI/Logo.img/Logo with {} frames", m_pLogoProp->GetChildCount());
    }
    else
    {
        m_nLogoCount = kMessageFrames;  // Just message frames if no logo
        LOG_WARN("UI/Logo.img/Logo not found");
    }

    // Load grade property: UI/Logo.img/Grade
    m_pGradeProp = resMan.GetProperty("UI/Logo.img/Grade");
    if (m_pGradeProp && m_pGradeProp->HasChildren())
    {
        m_nGradeCount = static_cast<std::int32_t>(m_pGradeProp->GetChildCount());
        LOG_DEBUG("Loaded UI/Logo.img/Grade with {} items", m_nGradeCount);
    }
    else
    {
        m_nGradeCount = 0;
        LOG_DEBUG("UI/Logo.img/Grade not found or empty");
    }

    // Load message property: UI/Logo.img/Message
    m_pMessageProp = resMan.GetProperty("UI/Logo.img/Message");
    if (m_pMessageProp)
    {
        LOG_DEBUG("Loaded UI/Logo.img/Message");
    }
    else
    {
        LOG_DEBUG("UI/Logo.img/Message not found");
    }

    // Load frames from properties
    m_logoFrames = LoadLogoFrames(m_pLogoProp);
    m_gradeFrames = LoadLogoFrames(m_pGradeProp);
    m_messageFrames = LoadLogoFrames(m_pMessageProp);

    // Create background layer (800x600 black canvas at z=0)
    // Original: IWzGr2D::CreateLayer with black 800x600 canvas, alpha=255
    auto canvasBg = std::make_shared<WzCanvas>(800, 600);
    {
        std::vector<std::uint8_t> blackPixels(800 * 600 * 4, 0);
        // Set alpha to 255 for all pixels (RGBA format)
        for (std::size_t i = 3; i < blackPixels.size(); i += 4)
        {
            blackPixels[i] = 255;
        }
        canvasBg->SetPixelData(std::move(blackPixels));
    }

    m_pLayerBackground = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 0);
    if (m_pLayerBackground)
    {
        m_pLayerBackground->InsertCanvas(canvasBg);
        m_pLayerBackground->SetColor(0xFFFFFFFF);  // Full opacity
        m_pLayerBackground->SetVisible(true);
        m_pLayerBackground->SetScreenSpace(true);
    }

    // Create main layer (800x600 transparent canvas at z=0)
    // Used for drawing logo/message frames
    auto canvasMain = std::make_shared<WzCanvas>(800, 600);
    {
        // Start with transparent black
        std::vector<std::uint8_t> clearPixels(800 * 600 * 4, 0);
        canvasMain->SetPixelData(std::move(clearPixels));
    }

    m_pLayerMain = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 1);
    if (m_pLayerMain)
    {
        m_pLayerMain->InsertCanvas(canvasMain);
        m_pLayerMain->SetColor(0xFFFFFFFF);  // Full opacity
        m_pLayerMain->SetVisible(true);
        m_pLayerMain->SetScreenSpace(true);
    }

    // Initialize loading screen
    InitLoading();

    // Flush cached objects after loading
    resMan.FlushCachedObjects(0);
}

auto Logo::LoadLogoFrames(const std::shared_ptr<WzProperty>& prop)
    -> std::vector<std::shared_ptr<WzCanvas>>
{
    std::vector<std::shared_ptr<WzCanvas>> frames;

    if (!prop)
    {
        return frames;
    }

    // Try numbered children (0, 1, 2, etc.) - these are animation frames
    for (int i = 0; i < 100; ++i)
    {
        auto child = prop->GetChild(std::to_string(i));
        if (!child)
        {
            if (i == 0)
            {
                // No numbered children, try other structure
                break;
            }
            continue;
        }

        // Canvas might be directly on the numbered child
        auto canvas = child->GetCanvas();
        if (canvas)
        {
            frames.push_back(canvas);
            continue;
        }

        // Check if canvas is a sub-property
        for (const auto& [subName, subChild] : child->GetChildren())
        {
            canvas = subChild->GetCanvas();
            if (canvas)
            {
                frames.push_back(canvas);
                break;
            }
        }
    }

    // If no numbered frames found, check all children for canvases
    if (frames.empty())
    {
        for (const auto& [name, child] : prop->GetChildren())
        {
            auto canvas = child->GetCanvas();
            if (canvas)
            {
                frames.push_back(canvas);
                continue;
            }

            for (const auto& [subName, subChild] : child->GetChildren())
            {
                canvas = subChild->GetCanvas();
                if (canvas)
                {
                    frames.push_back(canvas);
                    break;
                }
            }
        }
    }

    return frames;
}

void Logo::InitLoading()
{
    auto& resMan = WzResMan::GetInstance();
    auto loadingProp = resMan.GetProperty("UI/Logo.img/Loading");

    if (!loadingProp)
    {
        LOG_WARN("UI/Logo.img/Loading not found - loading screen disabled");
        return;
    }

    LOG_DEBUG("Initializing loading screen resources");

    // 1. Load random background
    auto randomBgProp = loadingProp->GetChild("randomBackgrd");
    if (randomBgProp && randomBgProp->HasChildren())
    {
        auto bgCount = static_cast<std::int32_t>(randomBgProp->GetChildCount());
        auto randomIndex = rand() % bgCount;
        auto bgChild = randomBgProp->GetChild(std::to_string(randomIndex));
        if (bgChild)
        {
            m_pLoadingBgCanvas = bgChild->GetCanvas();
            LOG_DEBUG("Selected random background: {}/{}", randomIndex, bgCount);
        }
    }
    else
    {
        LOG_DEBUG("No random backgrounds found in Loading/randomBackgrd");
    }

    // 2. Load repeat animations
    auto repeatProp = loadingProp->GetChild("repeat");
    if (repeatProp && repeatProp->HasChildren())
    {
        for (int n = 0; n < 100; ++n)  // Max 100 repeat animations
        {
            auto repeatN = repeatProp->GetChild(std::to_string(n));
            if (!repeatN) break;

            auto frames = LoadLogoFrames(repeatN);
            if (!frames.empty())
            {
                m_repeatAnims.push_back(frames);
            }
        }
        LOG_DEBUG("Loaded {} repeat animations", m_repeatAnims.size());
    }
    else
    {
        LOG_DEBUG("No repeat animations found in Loading/repeat");
    }

    // 3. Load step progress indicators
    auto stepProp = loadingProp->GetChild("step");
    if (stepProp && stepProp->HasChildren())
    {
        m_stepFrames = LoadLogoFrames(stepProp);
        m_nLoadingStepCount = static_cast<std::int32_t>(m_stepFrames.size());
        LOG_DEBUG("Loaded {} loading steps", m_nLoadingStepCount);
    }
    else
    {
        LOG_DEBUG("No step indicators found in Loading/step");
    }

    // 4. Create loading layers (initially hidden)
    auto& gr = get_gr();

    // Background layer (z=10, above logo layers)
    m_pLayerLoadingBg = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 10);
    if (m_pLayerLoadingBg)
    {
        m_pLayerLoadingBg->SetVisible(false);
        m_pLayerLoadingBg->SetScreenSpace(true);
        LOG_DEBUG("Created loading background layer");
    }

    // Animation layer (z=11)
    m_pLayerLoadingAnim = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 11);
    if (m_pLayerLoadingAnim)
    {
        m_pLayerLoadingAnim->SetVisible(false);
        m_pLayerLoadingAnim->SetScreenSpace(true);
        LOG_DEBUG("Created loading animation layer");
    }

    // Progress step layer (z=12)
    m_pLayerLoadingStep = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 12);
    if (m_pLayerLoadingStep)
    {
        m_pLayerLoadingStep->SetVisible(false);
        m_pLayerLoadingStep->SetScreenSpace(true);
        LOG_DEBUG("Created loading progress layer");
    }

    LOG_INFO("Loading screen initialized (bg={}, anims={}, steps={})",
             m_pLoadingBgCanvas ? "yes" : "no",
             m_repeatAnims.size(),
             m_nLoadingStepCount);
}

void Logo::StartLoadingMode()
{
    LOG_INFO("Starting loading mode");

    m_bLoadingMode = true;
    m_nLoadingStep = 0;
    m_loadingAlpha = 255;
    m_nCurrentRepeat = 0;
    m_nCurrentRepeatFrame = 0;

    // Hide logo layers
    if (m_pLayerBackground) m_pLayerBackground->SetVisible(false);
    if (m_pLayerMain) m_pLayerMain->SetVisible(false);

    // Setup and show background layer
    if (m_pLayerLoadingBg && m_pLoadingBgCanvas)
    {
        m_pLayerLoadingBg->RemoveAllCanvases();
        m_pLayerLoadingBg->InsertCanvas(m_pLoadingBgCanvas);

        // Center with origin offset
        auto origin = m_pLoadingBgCanvas->GetOrigin();
        auto& gr = get_gr();
        auto screenWidth = static_cast<std::int32_t>(gr.GetWidth());
        auto screenHeight = static_cast<std::int32_t>(gr.GetHeight());
        auto layerX = (screenWidth - m_pLoadingBgCanvas->GetWidth()) / 2 + origin.x;
        auto layerY = (screenHeight - m_pLoadingBgCanvas->GetHeight()) / 2 + origin.y;
        m_pLayerLoadingBg->SetPosition(layerX, layerY);
        m_pLayerLoadingBg->SetColor(0xFFFFFFFF);
        m_pLayerLoadingBg->SetVisible(true);
    }

    // Setup and show animation layer with first repeat
    if (m_pLayerLoadingAnim && !m_repeatAnims.empty())
    {
        m_pLayerLoadingAnim->RemoveAllCanvases();

        // Load first repeat animation frames
        auto& firstRepeat = m_repeatAnims[0];
        for (auto& frameCanvas : firstRepeat)
        {
            // Use default delay of 100ms (will be read from WZ in future refinement)
            m_pLayerLoadingAnim->InsertCanvas(frameCanvas, 100, 255, 255);
        }

        // Center first frame for positioning
        if (!firstRepeat.empty())
        {
            auto origin = firstRepeat[0]->GetOrigin();
            auto& gr = get_gr();
            auto screenWidth = static_cast<std::int32_t>(gr.GetWidth());
            auto screenHeight = static_cast<std::int32_t>(gr.GetHeight());
            auto layerX = (screenWidth - firstRepeat[0]->GetWidth()) / 2 + origin.x;
            auto layerY = (screenHeight - firstRepeat[0]->GetHeight()) / 2 + origin.y;
            m_pLayerLoadingAnim->SetPosition(layerX, layerY);
        }

        m_pLayerLoadingAnim->SetColor(0xFFFFFFFF);
        m_pLayerLoadingAnim->SetVisible(true);
    }

    // Setup and show progress layer
    if (m_pLayerLoadingStep)
    {
        m_pLayerLoadingStep->SetVisible(true);
        SetLoadingProgress(0);  // Show first step
    }
}

void Logo::SetLoadingProgress(std::int32_t step)
{
    if (step == m_nLoadingStep)
    {
        return;  // No change
    }

    m_nLoadingStep = step;
    LOG_DEBUG("Loading progress: step {}/{}", step, m_nLoadingStepCount - 1);

    if (!m_pLayerLoadingStep || step >= m_nLoadingStepCount)
    {
        return;
    }

    // Update step indicator
    m_pLayerLoadingStep->RemoveAllCanvases();

    if (step < static_cast<std::int32_t>(m_stepFrames.size()))
    {
        auto& stepCanvas = m_stepFrames[static_cast<std::size_t>(step)];
        m_pLayerLoadingStep->InsertCanvas(stepCanvas);

        // Center with origin offset
        auto origin = stepCanvas->GetOrigin();
        auto& gr = get_gr();
        auto screenWidth = static_cast<std::int32_t>(gr.GetWidth());
        auto screenHeight = static_cast<std::int32_t>(gr.GetHeight());
        auto layerX = (screenWidth - stepCanvas->GetWidth()) / 2 + origin.x;
        auto layerY = (screenHeight - stepCanvas->GetHeight()) / 2 + origin.y;
        m_pLayerLoadingStep->SetPosition(layerX, layerY);
    }

    // If final step reached, start fade out
    if (step >= m_nLoadingStepCount - 1)
    {
        LOG_INFO("Loading complete - starting fade out");
        FadeOutLoading();
    }
}

void Logo::FadeOutLoading()
{
    // Set alpha to trigger fade out (will be handled in UpdateLoading)
    if (m_loadingAlpha == 255)
    {
        m_loadingAlpha = 254;  // Start fade out
        LOG_DEBUG("Starting loading screen fade out");
    }
}

void Logo::UpdateLoading()
{
    // Handle fade out effect
    if (m_loadingAlpha < 255)
    {
        // Gradually reduce alpha (approximately 5 per frame at 60fps = ~3 second fade)
        auto alphaInt = static_cast<std::int32_t>(m_loadingAlpha);
        alphaInt = std::max(0, alphaInt - 5);
        m_loadingAlpha = static_cast<std::uint8_t>(alphaInt);

        // Apply alpha to all loading layers
        auto color = (static_cast<std::uint32_t>(m_loadingAlpha) << 24) | 0x00FFFFFF;
        if (m_pLayerLoadingBg) m_pLayerLoadingBg->SetColor(color);
        if (m_pLayerLoadingAnim) m_pLayerLoadingAnim->SetColor(color);
        if (m_pLayerLoadingStep) m_pLayerLoadingStep->SetColor(color);

        // When fully faded out, transition to Login
        if (m_loadingAlpha == 0)
        {
            LOG_INFO("Fade out complete - transitioning to Login");
            GoToLogin();
        }

        return;
    }

    // Repeat animation cycling (future enhancement)
    // For now, layer animation system handles frame playback automatically
    // Could add repeat switching logic here if needed
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
    // Based on CLogo::UpdateLogo @ 0xbc7a10

    auto tNow = Application::GetTick();

    // Initialize tick on first update
    if (!m_bTickInitial)
    {
        m_bTickInitial = true;
        m_dwTickInitial = tNow;
    }

    auto elapsed = tNow - m_dwTickInitial;

    // If clicked, wait 1500ms (0x5DC) before switching to video mode
    if (m_dwClick != 0)
    {
        if (tNow - m_dwClick > kClickToVideoDelay)
        {
            m_bTickInitial = false;
            m_bVideoMode = true;
        }
        return;
    }

    // Check if main layer exists
    if (!m_pLayerMain)
    {
        return;
    }

    // Calculate frame from elapsed time
    // Original formula: (elapsed_ms * 2748779070) >> 38 ≈ elapsed_ms / 100
    // This gives approximately 10 frames per second
    auto frame = static_cast<std::int32_t>(elapsed / 100);

    if (frame >= m_nLogoCount)
    {
        // All logo frames shown, draw final frame
        DrawWZLogo(-1);

        // Wait additional 10 frames (~1 second) then switch to video
        if (frame >= m_nLogoCount + 10)
        {
            m_bTickInitial = false;
            m_bVideoMode = true;
        }
    }
    else
    {
        DrawWZLogo(frame);
    }
}

void Logo::UpdateVideo()
{
    // Based on CLogo::UpdateVideo @ 0xbc5950

    // Video unavailable - end logo immediately
    if (m_videoState == VideoState::Unavailable)
    {
        LogoEnd();
        return;
    }

    // Video ending - fade out and end
    if (m_videoState == VideoState::FadeOut || m_videoState == VideoState::End)
    {
        // FadeOut(0);
        LogoEnd();
        return;
    }

    // In a full implementation, this would check video playback status
    // and handle video state transitions
    // For now, just end the logo since we don't have video support
    LogoEnd();
}

void Logo::DrawWZLogo(std::int32_t nFrame)
{
    // Based on CLogo::DrawWZLogo @ 0xbc71a0

    if (!m_pLayerMain)
    {
        return;
    }

    // Clear main layer to black
    // In original: canvas->FillRect(0, 0, 800, 600, -1)
    // For now, we'll just update the layer visibility

    if (nFrame < 0)
    {
        // Just clear, no content
        return;
    }

    std::int32_t gradeAlpha = 255;

    if (nFrame < kMessageFrames)
    {
        // Phase 1: Message display (frames 0-49)
        // Fade in for frames 0-24, fade out for frames 25-49

        float alpha;
        if (nFrame >= 25)
        {
            alpha = static_cast<float>(kMessageFrames - nFrame);  // Fade out
        }
        else
        {
            alpha = static_cast<float>(nFrame);  // Fade in
        }
        alpha = alpha / 25.0F * 255.0F;

        // Draw message canvas with calculated alpha
        if (!m_messageFrames.empty())
        {
            auto alphaInt = static_cast<std::uint8_t>(alpha);
            std::uint32_t color = (static_cast<std::uint32_t>(alphaInt) << 24) | 0x00FFFFFF;
            m_pLayerMain->SetColor(color);

            // Update layer with message canvas
            m_pLayerMain->RemoveAllCanvases();
            m_pLayerMain->InsertCanvas(m_messageFrames[0]);

            // Center the canvas
            auto& firstFrame = m_messageFrames[0];
            auto origin = firstFrame->GetOrigin();
            auto& gr = get_gr();
            int screenWidth = static_cast<int>(gr.GetWidth());
            int screenHeight = static_cast<int>(gr.GetHeight());
            int layerX = (screenWidth - firstFrame->GetWidth()) / 2 + origin.x;
            int layerY = (screenHeight - firstFrame->GetHeight()) / 2 + origin.y;
            m_pLayerMain->SetPosition(layerX, layerY);
        }
    }
    else
    {
        // Phase 2: Logo display (frames 50+)
        std::int32_t logoFrame = nFrame - kMessageFrames;

        // Play logo sound on first logo frame
        if (!m_bLogoSoundPlayed)
        {
            // StringPool 0x83E = logo sound path
            // CSoundMan::PlayBGM(soundPath, 0, 0, 0, 0, 0);
            m_bLogoSoundPlayed = true;
            LOG_DEBUG("Logo sound would play here");
        }

        // Fade out effect after frame 36 of logo
        if (logoFrame >= 36 && !m_logoFrames.empty())
        {
            auto fadeProgress = logoFrame - 36;
            auto fadeTotal = static_cast<std::int32_t>(m_logoFrames.size()) - 36;
            if (fadeTotal > 0)
            {
                gradeAlpha = 255 - (fadeProgress * 255 / fadeTotal);
                if (gradeAlpha < 0) gradeAlpha = 0;
            }
        }

        // Draw logo canvas for current frame
        if (!m_logoFrames.empty() && logoFrame < static_cast<std::int32_t>(m_logoFrames.size()))
        {
            m_pLayerMain->SetColor(0xFFFFFFFF);  // Full opacity for logo
            m_pLayerMain->RemoveAllCanvases();
            m_pLayerMain->InsertCanvas(m_logoFrames[static_cast<std::size_t>(logoFrame)]);

            // Center the canvas
            auto& currentFrame = m_logoFrames[static_cast<std::size_t>(logoFrame)];
            auto origin = currentFrame->GetOrigin();
            auto& gr = get_gr();
            int screenWidth = static_cast<int>(gr.GetWidth());
            int screenHeight = static_cast<int>(gr.GetHeight());
            int layerX = (screenWidth - currentFrame->GetWidth()) / 2 + origin.x;
            int layerY = (screenHeight - currentFrame->GetHeight()) / 2 + origin.y;
            m_pLayerMain->SetPosition(layerX, layerY);
        }
    }

    // Draw grade overlays with calculated alpha
    // In original, these are drawn on top of the main content
    // For now, grade overlays are handled separately if needed
    (void)gradeAlpha;  // TODO: Apply grade overlays
}

void Logo::Draw()
{
    // With WzGr2D layer system, actual rendering is done by WzGr2D::RenderFrame
    // Layers are managed by WzGr2D and rendered automatically
}

void Logo::Close()
{
    // Based on CLogo::Close @ 0xbc7170
    // Stop BGM with 1000ms fade out
    // CSoundMan::PlayBGM(0, 0, 0, 1000, 0, 0);

    Stage::Close();

    // Cleanup logo layers
    auto& gr = get_gr();

    if (m_pLayerBackground)
    {
        gr.RemoveLayer(m_pLayerBackground);
        m_pLayerBackground.reset();
    }

    if (m_pLayerMain)
    {
        gr.RemoveLayer(m_pLayerMain);
        m_pLayerMain.reset();
    }

    // Clear frame references
    m_logoFrames.clear();
    m_gradeFrames.clear();
    m_messageFrames.clear();

    // Clear property references
    m_pLogoProp.reset();
    m_pGradeProp.reset();
    m_pMessageProp.reset();

    LOG_DEBUG("Logo stage closed");
}

void Logo::LogoEnd()
{
    // Based on CLogo::LogoEnd @ 0xbc5890
    // Stop video, set program state, transition to login
    GoToLogin();
}

void Logo::GoToLogin()
{
    // Transition to login stage
    auto login = std::make_shared<Login>();
    Application::GetInstance().SetStage(login);
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
        // Video mode: 5000ms minimum
        return elapsed >= kMinSkipTimeMs;
    }
    else
    {
        // Logo mode: frame >= 50 (elapsed >= 5000ms at 10fps)
        auto frame = elapsed / 100;
        return frame >= kMessageFrames;
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
        DrawWZLogo(kFinalFrame);
    }
}

void Logo::OnKeyDown(std::int32_t keyCode)
{
    // Based on CLogo::OnKey @ 0xbc7900
    // Skip keys: Enter (13), Escape (27), Space (32)
    // SDL keycodes: SDLK_RETURN=13, SDLK_ESCAPE=27, SDLK_SPACE=32

    if (!CanSkip())
    {
        return;
    }

    // Check for skip keys
    if (keyCode == 13 || keyCode == 27 || keyCode == 32)  // Enter, Escape, Space
    {
        ForcedEnd();
    }
}

void Logo::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button)
{
    // Based on CLogo::OnMouseButton @ 0xbc7990
    // Skip on left mouse button up (button == 1)

    (void)x;
    (void)y;

    if (!CanSkip())
    {
        return;
    }

    if (button == 1)  // Left mouse button
    {
        ForcedEnd();
    }
}

} // namespace ms
