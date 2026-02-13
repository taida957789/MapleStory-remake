#pragma once

#include "graphics/WzGr2DCanvas.h"
#include "util/security/SecRect.h"
#include "wz/WzProperty.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

/// Matches EMPLOYEEIMGENTRY from the client (__cppobj : ZRefCounted).
class EmployeeImgEntry
{
public:
    std::shared_ptr<WzProperty> m_pImg;
    std::int32_t m_tLastAccessed{0};
};

/// Matches EMPLOYEEACTIONFRAMEENTRY from the client (__cppobj : ZRefCounted).
class EmployeeActionFrameEntry
{
public:
    std::shared_ptr<WzGr2DCanvas> m_pCanvas;
    SecRect m_rcBody;
    std::int32_t m_nDelay{0};
};

/// Matches EMPLOYEEACTIONENTRY from the client (__cppobj : ZRefCounted).
class EmployeeActionEntry
{
public:
    std::uint32_t m_dwTemplateID{0};
    std::int32_t m_nAction{0};
    std::int32_t m_bZigZag{0};
    std::vector<std::shared_ptr<EmployeeActionFrameEntry>> m_lpFrame;
    std::int32_t m_tLastAccessed{0};
};

} // namespace ms
