#pragma once

#include "util/Singleton.h"
#include <cstdint>
#include <string>

namespace ms
{

/**
 * @brief Configuration manager
 *
 * Based on CConfig from the original MapleStory client.
 *
 * Handles:
 * - Screen resolution settings
 * - Audio settings
 * - UI position storage
 * - Key bindings
 * - Various game options
 */
class Configuration final : public Singleton<Configuration>
{
    friend class Singleton<Configuration>;

public:
    // Screen settings
    [[nodiscard]] auto GetScreenWidth() const noexcept -> int { return m_nScreenWidth; }
    [[nodiscard]] auto GetScreenHeight() const noexcept -> int { return m_nScreenHeight; }
    [[nodiscard]] auto IsFullScreen() const noexcept -> bool { return m_bFullScreen; }

    void SetScreenResolution(int width, int height) noexcept;
    void SetFullScreen(bool fullscreen) noexcept;

    // Audio settings
    [[nodiscard]] auto GetBGMVolume() const noexcept -> int { return m_nBGMVolume; }
    [[nodiscard]] auto GetSFXVolume() const noexcept -> int { return m_nSFXVolume; }

    void SetBGMVolume(int volume) noexcept;
    void SetSFXVolume(int volume) noexcept;

    /**
     * @brief Apply system options
     *
     * Based on CConfig::ApplySysOpt
     */
    void ApplySysOpt(int option, int value);

    // Save/Load
    void SaveConfig();
    void LoadConfig();

    /**
     * @brief Save UI positions
     *
     * Based on CConfig::SaveUIPos
     */
    void SaveUIPos();

    // Graphics settings
    [[nodiscard]] auto IsShaderEnabled() const noexcept -> bool { return m_bEnabledShader; }
    [[nodiscard]] auto IsDX9Enabled() const noexcept -> bool { return m_bEnabledDX9; }

    // Path settings
    [[nodiscard]] auto GetExecPath() const noexcept -> const std::string& { return m_sExecPath; }

    [[nodiscard]] auto GetWzPath() const noexcept -> const std::string& { return m_sWzPath; }
    void SetWzPath(const std::string& path) { m_sWzPath = path; }

    // Offline mode
    [[nodiscard]] auto IsOfflineMode() const noexcept -> bool { return m_bOfflineMode; }
    void SetOfflineMode(bool offline) noexcept { m_bOfflineMode = offline; }

private:
    Configuration();
    ~Configuration() override;

    // Screen settings
    int m_nScreenWidth{800};
    int m_nScreenHeight{600};
    bool m_bFullScreen{false};

    // Audio settings
    int m_nBGMVolume{100};
    int m_nSFXVolume{100};

    // Graphics settings
    bool m_bEnabledShader{true};
    bool m_bEnabledDX9{false};

    // Path settings
    std::string m_sExecPath;
    std::string m_sWzPath{"resources/old"};  // Default WZ directory

    // Offline mode
    bool m_bOfflineMode{false};
};

} // namespace ms
