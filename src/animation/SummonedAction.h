#pragma once

#include "graphics/WzGr2DCanvas.h"
#include "util/security/SecRect.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

/// Matches SUMMONEDACTIONFRAMEENTRY from the client (__cppobj : ZRefCounted).
class SummonedActionFrameEntry
{
public:
    std::shared_ptr<WzGr2DCanvas> m_pCanvas;
    SecRect m_rcBody;
    std::int32_t m_nDelay{0};
    std::int32_t m_a0{0};
    std::int32_t m_a1{0};
};

/// Matches SUMMONEDACTIONENTRY from the client (__cppobj : ZRefCounted).
class SummonedActionEntry
{
public:
    std::int32_t m_nSkillID{0};
    std::int32_t m_nSLV{0};
    std::int32_t m_nAction{0};
    std::int32_t m_bZigZag{0};
    std::vector<std::shared_ptr<SummonedActionFrameEntry>> m_lpFrame;
    std::int32_t m_tLastAccessed{0};
};

} // namespace ms
