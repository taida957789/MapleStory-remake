#pragma once

#include "user/stats/SecondaryStat.h"
#include "util/Singleton.h"
#include <cstdint>
#include <string>

namespace ms
{

/**
 * @brief Game context - holds global game state
 *
 * Based on CWvsContext from the original MapleStory client.
 *
 * Contains:
 * - Character data
 * - Basic stats
 * - Secondary stats
 * - World/Channel info
 * - Various game state
 */
class WvsContext final : public Singleton<WvsContext>
{
    friend class Singleton<WvsContext>;

public:
    // Character info
    [[nodiscard]] auto GetCharacterID() const noexcept -> std::uint32_t { return m_dwCharacterID; }
    [[nodiscard]] auto GetCharacterLevel() const noexcept -> int { return m_nCharacterLevel; }
    [[nodiscard]] auto GetJobCode() const noexcept -> int { return m_nJobCode; }

    void SetCharacterID(std::uint32_t id) noexcept { m_dwCharacterID = id; }
    void SetCharacterLevel(int level) noexcept { m_nCharacterLevel = level; }
    void SetJobCode(int job) noexcept { m_nJobCode = job; }

    // World/Channel
    [[nodiscard]] auto GetWorldID() const noexcept -> int { return m_nWorldID; }
    [[nodiscard]] auto GetChannelID() const noexcept -> int { return m_nChannelID; }

    void SetWorldID(int world) noexcept { m_nWorldID = world; }
    void SetChannelID(int channel) noexcept { m_nChannelID = channel; }

    /**
     * @brief Screen resolution
     *
     * Based on CWvsContext::SetScreenResolution
     */
    void SetScreenResolution(int width, int height, int flags);

    /**
     * @brief Game state callbacks
     *
     * Based on CWvsContext::OnEnterGame
     */
    void OnEnterGame();

    /**
     * @brief Based on CWvsContext::OnLeaveGame
     */
    void OnLeaveGame();

    /**
     * @brief Based on CWvsContext::OnGameStageChanged
     */
    void OnGameStageChanged();

    // Cookie string (for web login)
    void SetCookieString(const char* cookie);
    [[nodiscard]] auto GetCookieString() const noexcept -> const std::string& { return m_sCookieString; }

    // Relogin cookie (for quick relogin to previous world/channel)
    void SetReloginCookie(const std::string& cookie) { m_sReloginCookie = cookie; }
    [[nodiscard]] auto GetReloginCookie() const noexcept -> const std::string& { return m_sReloginCookie; }
    void ClearReloginCookie() { m_sReloginCookie.clear(); }

    // Login base step (0=normal login, 1=web login)
    void SetLoginBaseStep(std::int32_t step) noexcept { m_nLoginBaseStep = step; }
    [[nodiscard]] auto GetLoginBaseStep() const noexcept -> std::int32_t { return m_nLoginBaseStep; }

    // Reset world info (called on world select)
    void ResetWorldInfoOnWorldSelect();

    // Standalone mode
    [[nodiscard]] auto GetStandAloneMode() const noexcept -> bool { return m_bStandAloneMode; }
    void SetStandAloneMode(bool mode) noexcept { m_bStandAloneMode = mode; }

    // Skip fade out
    [[nodiscard]] auto GetSkipFadeOut() const noexcept -> bool { return m_bSkipFadeOut; }
    void SetSkipFadeOut(bool skip) noexcept { m_bSkipFadeOut = skip; }

    // White fade mode
    [[nodiscard]] auto GetWhiteFadeInOut() const noexcept -> bool { return m_bWhiteFadeInOut; }
    void SetWhiteFadeInOut(bool white) noexcept { m_bWhiteFadeInOut = white; }

    // Secondary stats (buffs/debuffs)
    [[nodiscard]] auto GetSecondaryStat() noexcept -> SecondaryStat& { return m_secondaryStat; }
    [[nodiscard]] auto GetSecondaryStat() const noexcept -> const SecondaryStat& { return m_secondaryStat; }

private:
    WvsContext();
    ~WvsContext() override;

    // Character data
    std::uint32_t m_dwCharacterID{};
    int m_nCharacterLevel{};
    int m_nJobCode{};

    // World/Channel
    int m_nWorldID{};
    int m_nChannelID{};
    int m_nStarPlanetWorldID{};

    // Cookie for web login
    std::string m_sCookieString;

    // Relogin cookie (for quick relogin to previous world/channel)
    std::string m_sReloginCookie;

    // Login base step (0=normal, 1=web login)
    std::int32_t m_nLoginBaseStep{0};

    // Standalone mode (offline)
    bool m_bStandAloneMode{false};

    // Skip fade out on stage change
    bool m_bSkipFadeOut{false};

    // White fade in/out mode
    bool m_bWhiteFadeInOut{false};

    // Secondary stats (buffs/debuffs)
    SecondaryStat m_secondaryStat;
};

} // namespace ms
