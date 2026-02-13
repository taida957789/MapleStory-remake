#pragma once

#include "life/Life.h"
#include "user/avatar/Avatar.h"
#include "models/GW_CharacterStat.h"

#include <cstdint>

namespace ms
{

/**
 * @brief Character user — combines field life and avatar visual state
 *
 * Based on CUser (__cppobj : CLife, CAvatar) from the original MapleStory client.
 * Multiple inheritance: Life provides field positioning, Avatar provides rendering.
 */
class User : public Life, public Avatar
{
public:
    ~User() override = default;

    // --- Character stat ---
    GW_CharacterStat m_characterStat;
};

} // namespace ms
