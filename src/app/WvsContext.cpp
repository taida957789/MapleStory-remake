#include "WvsContext.h"
#include "Application.h"
#include "util/Logger.h"

#include <SDL3/SDL.h>

namespace ms
{

WvsContext::WvsContext() = default;

WvsContext::~WvsContext() = default;

void WvsContext::SetScreenResolution(int width, int height, [[maybe_unused]] int flags)
{
    auto* window = Application::GetInstance().GetWindow();
    if (window)
    {
        SDL_SetWindowSize(window, width, height);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
}

void WvsContext::OnEnterGame()
{
    // Called when entering game field
    // Initialize character data, stats, etc.
    LOG_INFO("Entering game");
}

void WvsContext::OnLeaveGame()
{
    // Called when leaving game field
    // Cleanup character data
    LOG_INFO("Leaving game");
}

void WvsContext::OnGameStageChanged()
{
    // Called when game stage changes
    LOG_DEBUG("Game stage changed");
}

void WvsContext::SetCookieString(const char* cookie)
{
    if (cookie)
    {
        m_sCookieString = cookie;
    }
    else
    {
        m_sCookieString.clear();
    }
}

void WvsContext::ResetWorldInfoOnWorldSelect()
{
    // Based on CWvsContext::ResetWorldInfoOnWorldSelect
    // Called when returning to world selection
    // Resets world/channel selection state

    LOG_DEBUG("Resetting world info on world select");

    // Clear relogin cookie since user is selecting a new world
    m_sReloginCookie.clear();

    // Reset world/channel IDs to invalid state
    // (will be set again when user selects a world)
    // Note: We don't reset m_nWorldID/m_nChannelID here as they may be needed
    // for UI display purposes
}

} // namespace ms
