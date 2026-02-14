#pragma once

#include "util/security/ZtlSecureTear.h"

#include <cstdint>

namespace ms
{

/**
 * @brief Secondary (buff/debuff) stats for a character
 *
 * Based on CSecondaryStat from the original MapleStory client.
 * Tracks temporary stat modifiers applied by buffs, debuffs,
 * and special states.
 */
class SecondaryStat
{
public:
    ZTL_SECURE_MEMBER(std::int32_t, nAttract)
    ZTL_SECURE_MEMBER(std::int32_t, rAttract)
};

} // namespace ms
