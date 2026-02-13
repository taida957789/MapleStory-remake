#pragma once

#include "user/avatar/AvatarLook.h"
#include "models/GW_CharacterStat.h"

#include <memory>

namespace ms
{

/**
 * @brief Combined character data for display (stats + appearance)
 *
 * Based on AvatarData from the original MapleStory client.
 * Used by the character selection screen and other UI that needs
 * both stat info and visual appearance data together.
 */
struct AvatarData
{
    GW_CharacterStat characterStat;
    AvatarLook avatarLook;

    /// Secondary avatar look for Zero class (Beta form)
    std::shared_ptr<AvatarLook> pZeroSubAvatarLook;
};

} // namespace ms
