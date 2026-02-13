#pragma once

#include <cstdint>

namespace ms
{

/**
 * @brief Ladder or rope data for climbing
 *
 * Based on CLadderOrRope from the original MapleStory client.
 * Stores a vertical climbing segment (ladder/rope) in the field.
 */
struct LadderOrRope
{
    std::uint32_t dwSN{};
    std::int32_t bLadder{};
    std::int32_t bUpperFoothold{};
    std::int32_t x{};
    std::int32_t y1{};
    std::int32_t y2{};
    std::int32_t nPage{};
    std::int32_t bOff{};
};

} // namespace ms
