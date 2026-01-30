#include "Application.h"
#include "Configuration.h"
#include "WvsContext.h"
#include "audio/SoundSystem.h"
#include "graphics/WzGr2D.h"
#include "input/InputSystem.h"
#include "stage/Logo.h"
#include "stage/Stage.h"
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

    // Parse command line arguments for WZ path
    // Usage: --wz-path <path> or -w <path>
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if ((arg == "--wz-path" || arg == "-w") && i + 1 < argc)
        {
            config.SetWzPath(argv[i + 1]);
            ++i;  // Skip next argument
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

    // Create initial stage (CLogo)
    auto logo = std::make_shared<Logo>();
    SetStage(logo);

    m_bIsRunning = true;
    m_tLastUpdate = GetTick();

    return true;
}

void Application::Run()
{
    // Main game loop (mirrors CWvsApp::Run)
    SDL_Event event;

    while (m_bIsRunning && !m_bIsTerminating)
    {
        // Process SDL events
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                m_bIsTerminating = true;
                break;
            }

            // Process input
            InputSystem::GetInstance().ProcessEvent(event);

            // Forward events to current stage
            if (m_pStage)
            {
                switch (event.type)
                {
                case SDL_EVENT_MOUSE_MOTION:
                    m_pStage->OnMouseMove(static_cast<std::int32_t>(event.motion.x),
                                          static_cast<std::int32_t>(event.motion.y));
                    break;

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
#ifdef MS_DEBUG_CANVAS
                    if (DebugOverlay::GetInstance().OnMouseClick(
                            static_cast<int>(event.button.x),
                            static_cast<int>(event.button.y)))
                    {
                        break;
                    }
#endif
                    m_pStage->OnMouseDown(static_cast<std::int32_t>(event.button.x),
                                          static_cast<std::int32_t>(event.button.y),
                                          static_cast<std::int32_t>(event.button.button));
                    break;

                case SDL_EVENT_MOUSE_BUTTON_UP:
                    m_pStage->OnMouseUp(static_cast<std::int32_t>(event.button.x),
                                        static_cast<std::int32_t>(event.button.y),
                                        static_cast<std::int32_t>(event.button.button));
                    break;

                case SDL_EVENT_KEY_DOWN:
                    if (!event.key.repeat)
                    {
#ifdef MS_DEBUG_CANVAS
                        if (DebugOverlay::GetInstance().OnKeyDown(event.key.key))
                        {
                            break;
                        }
#endif
                        m_pStage->OnKeyDown(static_cast<std::int32_t>(event.key.key));
                    }
                    break;

                case SDL_EVENT_KEY_UP:
                    m_pStage->OnKeyUp(static_cast<std::int32_t>(event.key.key));
                    break;

                default:
                    break;
                }
            }
        }

        // Get current time
        const auto tCurTime = GetTick();

        // Update game state
        Update(tCurTime);

        // Render frame
        Render();

        // Frame rate limiting (approximately 60 FPS)
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
    return InputSystem::GetInstance().Initialize();
}

auto Application::InitializeSound() -> bool
{
    return SoundSystem::GetInstance().Initialize();
}

auto Application::InitializeResMan() -> bool
{
    // Get WZ path from configuration (default: "resources/old")
    auto& config = Configuration::GetInstance();
    auto& resMan = WzResMan::GetInstance();

    // Use configured WZ path
    std::string wzPath = config.GetWzPath();

    // Check if configured path exists, fall back to auto-detection if not
    if (!std::filesystem::exists(wzPath))
    {
        LOG_WARN("Configured WZ path '{}' not found, searching...", wzPath);

        // Try common WZ file locations
        if (std::filesystem::exists("./UI.wz"))
        {
            wzPath = ".";
        }
        else if (std::filesystem::exists("../UI.wz"))
        {
            wzPath = "..";
        }
        else if (std::filesystem::exists("./data/UI.wz"))
        {
            wzPath = "./data";
        }
        else if (std::filesystem::exists("./resources/old/UI.wz"))
        {
            wzPath = "./resources/old";
        }
    }

    LOG_INFO("Using WZ path: {}", wzPath);
    resMan.SetBasePath(wzPath);

    return resMan.Initialize();
}

void Application::ProcessInput()
{
    // Process input from InputSystem
    [[maybe_unused]] auto& input = InputSystem::GetInstance();

    // Forward input to current stage
    if (m_pStage)
    {
        // TODO: Implement input forwarding
    }
}

void Application::Update(std::uint64_t tCurTime)
{
    m_tUpdateTime = tCurTime;

    // Calculate delta time
    [[maybe_unused]] const auto deltaTime = tCurTime - m_tLastUpdate;
    m_tLastUpdate = tCurTime;

    // Update current stage
    if (m_pStage)
    {
        m_pStage->Update();
    }
}

void Application::Render()
{
    auto& gr = get_gr();

    // Update time and render frame through WzGr2D
    // This handles clearing, layer rendering, and present
    auto tCur = static_cast<std::int32_t>(GetTick());
    gr.UpdateCurrentTime(tCur);

    // Render current stage (allows stage to update layers)
    if (m_pStage)
    {
        m_pStage->Draw();
    }

    // Render all layers and present
    (void)gr.RenderFrame(tCur);
}

} // namespace ms
