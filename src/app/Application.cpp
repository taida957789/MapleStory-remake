#include "Application.h"
#include "Configuration.h"
#include "UpdateManager.h"
#include "WvsContext.h"
#include "animation/ActionFrame.h"
#include "animation/ActionMan.h"
#include "audio/SoundMan.h"
#include "graphics/WzGr2D.h"
#include "input/InputSystem.h"
#include "stage/Logo.h"
#include "stage/Stage.h"
#include "text/TextRenderer.h"
#include "ui/WndMan.h"
#include "util/Logger.h"
#include "wz/WzResMan.h"

#ifdef MS_DEBUG_CANVAS
#include "debug/DebugOverlay.h"
#endif

#include <SDL3/SDL.h>
#include <filesystem>

namespace ms
{

Application::Application() = default;

Application::~Application()
{
    Shutdown();
}

auto Application::Initialize(int argc, char* argv[]) -> bool
{
    // Parse command line (mirrors CWvsApp constructor logic)
    for (int i = 1; i < argc; ++i)
    {
        m_sCmdLine += argv[i];
        if (i < argc - 1)
            m_sCmdLine += " ";
    }

    // Check for WebStart or GameLaunching mode
    if (m_sCmdLine.find("WebStart") != std::string::npos)
    {
        m_nGameStartMode = 1;
    }
    else if (m_sCmdLine.find("GameLaunching") != std::string::npos)
    {
        m_nGameStartMode = 2;
    }

    // Initialize subsystems (mirrors CWvsApp::SetUp)

    // Create Configuration singleton
    auto& config = Configuration::GetInstance();

    // Parse command line arguments
    // Usage: --wz-path <path> or -w <path>
    //        --offline - Run in offline mode with sample worlds
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "--wz-path" || arg == "-w") && i + 1 < argc)
        {
            config.SetWzPath(argv[i + 1]);
            ++i;  // Skip next argument
        }
        else if (arg == "--offline")
        {
            config.SetOfflineMode(true);
            LOG_INFO("Offline mode enabled");
        }
    }

    // Create WvsContext singleton
    (void)WvsContext::GetInstance();

    // Initialize graphics (CreateMainWindow + InitializeGr2D)
    if (!InitializeGraphics())
    {
        LOG_ERROR("Failed to initialize graphics");
        return false;
    }

    // Initialize input system
    if (!InitializeInput())
    {
        LOG_ERROR("Failed to initialize input");
        return false;
    }

    // Initialize sound system
    if (!InitializeSound())
    {
        LOG_ERROR("Failed to initialize sound");
        return false;
    }

    // Initialize resource manager
    if (!InitializeResMan())
    {
        LOG_ERROR("Failed to initialize resource manager");
        return false;
    }

    // Initialize text renderer
    if (!TextRenderer::GetInstance().Initialize())
    {
        LOG_WARN("Failed to initialize text renderer - text will not be displayed");
    }

    // Initialize action frame mappers and action manager
    ActionFrame::LoadMappers();
    (void)ActionMan::GetInstance().Initialize();

    // Create WndMan singleton (constructor handles initialization)
    (void)WndMan::GetInstance();

    // Create initial stage (CLogo)
    auto logo = std::make_shared<Logo>();
    SetStage(logo);

    m_bIsRunning = true;
    m_tLastUpdate = GetTick();

    return true;
}

void Application::Run()
{
    // Main game loop (mirrors CWvsApp::Run at 0x1a93e60)
    //
    // Original flow:
    //   1. MsgWaitForMultipleObjects (waits for input/Windows messages)
    //   2. UpdateDevice → drain GetISMessage → ISMsgProc
    //   3. GenerateAutoKeyDown → ISMsgProc
    //   4. PreUpdate / CallUpdate / PostUpdate / RenderFrame
    //
    // SDL3 adaptation:
    //   1. SDL_PollEvent → InputSystem::ProcessEvent (generates ISMSGs)
    //   2. Drain ISMSG queue → ISMsgProc (→ WndMan::ProcessKey/ProcessMouse)
    //   3. GenerateAutoKeyDown → ISMsgProc
    //   4. PreUpdate / CallUpdate / PostUpdate / Render

    SDL_Event event;

    while (m_bIsRunning && !m_bIsTerminating)
    {
        auto& input = InputSystem::GetInstance();

        // 1. Process SDL events → InputSystem generates ISMSGs
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                m_bIsTerminating = true;
                break;
            }
            // Process SDL events → InputSystem generates ISMSGs
            input.ProcessEvent(event);
        }

        // 2. Drain ISMSG queue → dispatch via ISMsgProc
        ISMSG msg{};
        while (input.GetISMessage(&msg))
        {
            ISMsgProc(msg.message, msg.wParam, msg.lParam);
        }

        // 3. Generate auto-repeat key events
        if (input.GenerateAutoKeyDown(&msg))
        {
            ISMsgProc(msg.message, msg.wParam, msg.lParam);
        }

        // --- Per-frame update/render (mirrors CWvsApp::Run idle branch) ---
        auto& gr = get_gr();

        // 4a. Pre-update phase
        UpdateManager::s_PreUpdate();

        // 4b. Fixed-timestep update (uses nextRenderTime as in original)
        const auto tCurTime = static_cast<std::uint64_t>(gr.GetNextRenderTime());
        CallUpdate(tCurTime);

        // 4c. Post-update phase
        UpdateManager::s_PostUpdate();

        // 4d. Render: let stage/UI prepare layers, then WzGr2D renders all
        Render();

        SDL_Delay(1);
    }
}

void Application::Shutdown()
{
    m_bIsRunning = false;

    // Release stage
    if (m_pStage)
    {
        m_pStage->Close();
        m_pStage.reset();
        g_pStage = nullptr;
    }

    // Shutdown graphics engine
    get_gr().Shutdown();
}

void Application::SetStage(std::shared_ptr<Stage> stage, void* param)
{
    // Mirrors set_stage function

    // Close old stage
    if (m_pStage)
    {
        m_pStage->Close();
    }

    // Set new stage
    m_pStage = std::move(stage);
    g_pStage = m_pStage.get();

    // Initialize new stage
    if (m_pStage)
    {
        m_pStage->Init(param);
    }
}

auto Application::GetTick() noexcept -> std::uint64_t
{
    return SDL_GetTicks();
}

auto Application::GetTimeGap() const noexcept -> std::int32_t
{
    return static_cast<std::int32_t>(m_tUpdateTime) - get_gr().GetCurrentTime();
}

auto Application::InitializeGraphics() -> bool
{
    // Initialize WzGr2D graphics engine (mirrors IWzGr2D::Initialize)
    auto& gr = get_gr();

    if (!gr.Initialize(static_cast<std::uint32_t>(m_nWidth),
                       static_cast<std::uint32_t>(m_nHeight)))
    {
        LOG_ERROR("Failed to initialize graphics engine: {}", SDL_GetError());
        return false;
    }

    return true;
}

auto Application::GetWindow() const noexcept -> SDL_Window*
{
    return get_gr().GetWindow();
}

auto Application::GetRenderer() const noexcept -> SDL_Renderer*
{
    return get_gr().GetRenderer();
}

auto Application::InitializeInput() -> bool
{
    InputSystem::GetInstance().Init();
    return true;
}

auto Application::InitializeSound() -> bool
{
    return SoundMan::GetInstance().Initialize();
}

auto Application::InitializeResMan() -> bool
{
    auto& config = Configuration::GetInstance();
    auto& resMan = WzResMan::GetInstance();

    // Set WZ base path from configuration
    resMan.SetBasePath(config.GetWzPath());

    // Initialize WzResMan - this will auto-discover and load all WZ files
    // WZ files use lazy loading, so they're only parsed when actually accessed
    if (!resMan.Initialize())
    {
        LOG_ERROR("Failed to initialize WzResMan from path: {}", config.GetWzPath());
        return false;
    }

    LOG_INFO("WzResMan initialized from: {}", config.GetWzPath());
    return true;
}

void Application::ISMsgProc(std::uint32_t message, std::uint32_t wParam,
                            std::int32_t lParam)
{
    // CWvsApp::ISMsgProc at 0x1a8bc30
    // Routes ISMSG to CWndMan::ProcessKey or CWndMan::ProcessMouse

    constexpr std::uint32_t kWM_KEYDOWN = 0x0100;

    if (message == kWM_KEYDOWN)
    {
        WndMan::GetInstance().ProcessKey(kWM_KEYDOWN, wParam, lParam);
    }
    else if (message > 0x01FF && message <= 0x020A)
    {
        // WM_MOUSEMOVE (0x200) through WM_MOUSEWHEEL (0x20A)
        WndMan::GetInstance().ProcessMouse(message, wParam, lParam);
    }
}

void Application::ProcessInput()
{
    // Legacy path — no longer used.
    // Input is now routed through ISMsgProc in Run().
}

void Application::CallUpdate(std::uint64_t tCurTime)
{
    // Based on CWvsApp::CallUpdate
    // Fixed 30ms timestep loop: accumulate time and run discrete update ticks.

    if (m_bFirstUpdate)
    {
        m_tUpdateTime = tCurTime;
        m_bFirstUpdate = false;
    }

    auto& gr = get_gr();

    while (static_cast<std::int64_t>(tCurTime - m_tUpdateTime) > 0)
    {
        auto stage = m_pStage;

        UpdateManager::s_Update();

        if (stage)
            stage->Update();

        WndMan::s_Update();

        m_tUpdateTime += kUpdateInterval;

        if (static_cast<std::int64_t>(tCurTime - m_tUpdateTime) > 0)
            gr.UpdateCurrentTime(static_cast<std::int32_t>(m_tUpdateTime));
    }

    gr.UpdateCurrentTime(static_cast<std::int32_t>(tCurTime));
}

void Application::Render()
{
    auto& gr = get_gr();

    // Update time and render frame through WzGr2D
    // This handles clearing, layer rendering, and present
    auto tCur = static_cast<std::int32_t>(GetTick());
    gr.UpdateCurrentTime(tCur);

    // Render current stage (allows stage to update layers)
    if (g_pStage)
    {
        g_pStage->Draw();
    }

    // Render all layers and present
    (void)gr.RenderFrame(tCur);
}

} // namespace ms
