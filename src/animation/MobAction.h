#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

class MobActionFrameEntry;

/// Matches MOBACTIONENTRY from the client (__cppobj : ZRefCounted).
class MobActionEntry
{
public:
    std::uint32_t m_dwTemplateID{0};
    std::int32_t m_nAction{0};
    std::vector<std::shared_ptr<MobActionFrameEntry>> m_lpFrame;
    std::int32_t m_tLastAccessed{0};
    std::int32_t m_tRepeatFrameTime{0};
    std::uint32_t m_tAverageAccessedTime{0};
    std::uint32_t m_tTotalAccessCount{0};
};

} // namespace ms
