#pragma once

#include "graphics/WzGr2DCanvas.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

/// Matches SHADOWPARTNERACTIONFRAMEENTRY from the client (__cppobj : ZRefCounted).
class ShadowPartnerActionFrameEntry
{
public:
    std::shared_ptr<WzGr2DCanvas> m_pCanvas;
    std::int32_t m_a0{0};
    std::int32_t m_a1{0};
};

/// Matches SHADOWPARTNERACTIONENTRY from the client (__cppobj : ZRefCounted).
class ShadowPartnerActionEntry
{
public:
    std::int32_t m_nSkillID{0};
    std::int32_t m_nAction{0};
    std::int32_t m_bZigZag{0};
    std::vector<std::shared_ptr<ShadowPartnerActionFrameEntry>> m_lpFrame;
    std::int32_t m_tLastAccessed{0};
};

} // namespace ms
