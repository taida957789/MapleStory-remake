#pragma once

#include "util/Singleton.h"

#include <cstdint>
#include <memory>
#include <string>

struct SDL_Window;
struct SDL_Renderer;

namespace ms
{

class Stage;
class WvsContext;
class Configuration;
class InputSystem;
class SoundMan;
class WzResMan;
class ClientSocket;
class WzGr2D;

/**
 * @brief Main application class
 *
 * Based on CWvsApp from the original MapleStory client.
 *
 * Responsibilities:
 * - Window management (m_hWnd)
 * - Main game loop (Run)
 * - Subsystem initialization (SetUp)
 * - Input handling
 * - Time management
 */
class Application final : public Singleton<Application>
{
    friend class Singleton<Application>;

public:
    /**
     * @brief Initialize the application
     *
     * Based on CWvsApp::CWvsApp and CWvsApp::SetUp
     *
     * @param argc Command line argument count
     * @param argv Command line arguments
     * @return true on success, false on failure
     */
    [[nodiscard]] auto Initialize(int argc, char* argv[]) -> bool;

    /**
     * @brief Main game loop
     *
     * Based on CWvsApp::Run
     */
    void Run();

    /**
     * @brief Shutdown and cleanup
     *
     * Based on CWvsApp::~CWvsApp
     */
    void Shutdown();

    /**
     * @brief Set the current stage (scene)
     *
     * Based on set_stage global function
     *
     * @param stage The new stage to set
     * @param param Optional parameter passed to stage Init
     */
    void SetStage(std::shared_ptr<Stage> stage, void* param = nullptr);

    /**
     * @brief Get the current stage
     * @return Shared pointer to current stage
     */
    [[nodiscard]] auto GetStage() const noexcept -> std::shared_ptr<Stage>
    {
        return m_pStage;
    }

    // Window/Renderer accessors (delegated to WzGr2D)
    [[nodiscard]] auto GetWindow() const noexcept -> SDL_Window*;
    [[nodiscard]] auto GetRenderer() const noexcept -> SDL_Renderer*;

    // Timing
    [[nodiscard]] auto GetUpdateTime() const noexcept -> std::uint64_t
    {
        return m_tUpdateTime;
    }

    /// CWvsApp::GetTimeGap — returns m_tUpdateTime - g_gr.get_currentTime()
    [[nodiscard]] auto GetTimeGap() const noexcept -> std::int32_t;

    [[nodiscard]] static auto GetTick() noexcept -> std::uint64_t;

    // State
    [[nodiscard]] auto IsTerminating() const noexcept -> bool
    {
        return m_bIsTerminating;
    }

    [[nodiscard]] auto IsRunning() const noexcept -> bool
    {
        return m_bIsRunning;
    }

    // Dimensions
    [[nodiscard]] auto GetWidth() const noexcept -> int
    {
        return m_nWidth;
    }

    [[nodiscard]] auto GetHeight() const noexcept -> int
    {
        return m_nHeight;
    }

private:
    Application();
    ~Application() override;

    // Initialization helpers
    [[nodiscard]] auto InitializeGraphics() -> bool;
    [[nodiscard]] auto InitializeInput() -> bool;
    [[nodiscard]] auto InitializeSound() -> bool;
    [[nodiscard]] auto InitializeResMan() -> bool;

    // Main loop components
    void ProcessInput();
    void CallUpdate(std::uint64_t tCurTime);
    void Render();

    /// CWvsApp::ISMsgProc — routes ISMSG to CWndMan::ProcessKey/ProcessMouse
    void ISMsgProc(std::uint32_t message, std::uint32_t wParam,
                   std::int32_t lParam);

    /// Fixed update interval in ms (original: m_tUpdateTime += 30)
    static constexpr std::uint64_t kUpdateInterval = 30;

private:
    // Screen dimensions (used to initialize WzGr2D)
    int m_nWidth = 1366;
    int m_nHeight = 768;

    // Current stage (scene)
    std::shared_ptr<Stage> m_pStage;

    // Timing
    std::uint64_t m_tUpdateTime = 0;
    std::uint64_t m_tLastUpdate = 0;
    bool m_bFirstUpdate = true;

    // State
    bool m_bIsTerminating = false;
    bool m_bIsRunning = false;

    // Command line
    std::string m_sCmdLine;

    // Game start mode (0 = normal, 1 = webstart, 2 = game launching)
    int m_nGameStartMode = 0;
};

} // namespace ms
