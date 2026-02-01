#include "Configuration.h"

#include <algorithm>
#include <iostream>

namespace ms
{

Configuration::Configuration()
{
    LoadConfig();
}

Configuration::~Configuration()
{
    SaveConfig();
}

void Configuration::SetScreenResolution(int width, int height) noexcept
{
    m_nScreenWidth = width;
    m_nScreenHeight = height;
}

void Configuration::SetFullScreen(bool fullscreen) noexcept
{
    m_bFullScreen = fullscreen;
}

void Configuration::SetBGMVolume(int volume) noexcept
{
    m_nBGMVolume = std::clamp(volume, 0, 100);
}

void Configuration::SetSFXVolume(int volume) noexcept
{
    m_nSFXVolume = std::clamp(volume, 0, 100);
}

void Configuration::ApplySysOpt([[maybe_unused]] int option, [[maybe_unused]] int value)
{
    // Apply various system options
    // In original: handles different option types
}

void Configuration::SaveConfig()
{
    // Save configuration to file
    // TODO: Implement config file saving
}

void Configuration::LoadConfig()
{
    // Load configuration from file
    // TODO: Implement config file loading
}

void Configuration::SaveUIPos()
{
    // Save UI window positions
    // TODO: Implement UI position saving
}

} // namespace ms
