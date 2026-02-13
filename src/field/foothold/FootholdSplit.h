#pragma once

#include "util/Point.h"

#include <cstdint>
#include <set>

namespace ms
{

/// Based on FOOTHOLDSPLIT from the original MapleStory client (v1029).
class FootholdSplit
{
public:
    static constexpr std::int32_t SplitX = 0x12C;  // FOOTHOLDSPLITX
    static constexpr std::int32_t SplitY = 0x12C;  // FOOTHOLDSPLITY

    Rect rcSplit;
    std::set<std::uint32_t> setHaveFootHold;
    std::int32_t nSN{};
};

} // namespace ms
