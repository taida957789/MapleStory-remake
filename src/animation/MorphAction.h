#pragma once

#include "graphics/WzGr2DCanvas.h"
#include "util/Point.h"
#include "wz/WzProperty.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

/// Matches MORPHIMGENTRY from the client (__cppobj : ZRefCounted).
class MorphImgEntry
{
public:
    std::shared_ptr<WzProperty> m_pImg;
    std::int32_t m_tLastAccessed{0};
};

/// Matches MORPHACTIONFRAMEENTRY from the client (__cppobj : ZRefCounted).
class MorphActionFrameEntry
{
public:
    std::shared_ptr<WzGr2DCanvas> m_pCanvas;
    Rect m_rcBody;
    Point2D m_ptHead{0, 0};
    std::int32_t m_nDelay{0};
};

/// Matches MORPHACTIONENTRY from the client (__cppobj : ZRefCounted).
class MorphActionEntry
{
public:
    std::uint32_t m_dwTemplateID{0};
    std::int32_t m_nAction{0};
    std::vector<std::shared_ptr<MorphActionFrameEntry>> m_apFE;
    std::int32_t m_tLastAccessed{0};
};

} // namespace ms
