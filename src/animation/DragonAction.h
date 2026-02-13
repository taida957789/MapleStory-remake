#pragma once

#include "SummonedAction.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

/// Matches DRAGONACTIONENTRY from the client (__cppobj : ZRefCounted).
class DragonActionEntry
{
public:
    std::int32_t m_nAction{0};
    std::int32_t m_bZigZag{0};
    std::vector<std::shared_ptr<SummonedActionFrameEntry>> m_lpFrame;
    std::int32_t m_tLastAccessed{0};
};

} // namespace ms
