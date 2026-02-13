#pragma once

#include "graphics/WzGr2DCanvas.h"

#include <cstdint>
#include <list>
#include <memory>

namespace ms
{

/// Matches FACELOOKCODES from the client.
struct FaceLookCodes
{
    std::int32_t m_nFace{0};
    std::int32_t m_nEmotion{0};
    std::int32_t m_nAcc{0};

    auto operator<(const FaceLookCodes& o) const -> bool
    {
        if (m_nFace != o.m_nFace) return m_nFace < o.m_nFace;
        if (m_nEmotion != o.m_nEmotion) return m_nEmotion < o.m_nEmotion;
        return m_nAcc < o.m_nAcc;
    }

    auto operator==(const FaceLookCodes& o) const -> bool
    {
        return m_nFace == o.m_nFace
            && m_nEmotion == o.m_nEmotion
            && m_nAcc == o.m_nAcc;
    }
};

/// Matches FACELOOKENTRY from the client (__cppobj : ZRefCounted).
class FaceLookEntry
{
public:
    std::list<std::shared_ptr<WzGr2DCanvas>> m_lpEmotion;
    std::int32_t m_nDuration{0};
    std::int32_t m_tLastAccessed{0};
};

} // namespace ms
