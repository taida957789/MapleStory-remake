#pragma once

#include "graphics/WzGr2DCanvas.h"
#include "util/security/SecRect.h"
#include "wz/WzProperty.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

/// Matches SKILLPETIMGENTRY from the client (__cppobj : ZRefCounted).
class SkillPetImgEntry
{
public:
    std::shared_ptr<WzProperty> m_pImg;
    std::int32_t m_tLastAccessed{0};
};

/// Matches SKILLPETACTIONFRAMEENTRY from the client (__cppobj : ZRefCounted).
class SkillPetActionFrameEntry
{
public:
    std::shared_ptr<WzGr2DCanvas> m_pCanvas;
    SecRect m_rcBody;
    std::int32_t m_nDelay{0};
};

/// Matches SKILLPETACTIONENTRY from the client (__cppobj : ZRefCounted).
class SkillPetActionEntry
{
public:
    std::uint32_t m_dwTemplateID{0};
    std::u16string m_strEffect;
    std::int32_t m_nAction{0};
    std::int32_t m_bZigZag{0};
    std::vector<std::shared_ptr<SkillPetActionFrameEntry>> m_lpFrame;
    std::int32_t m_tLastAccessed{0};
};

} // namespace ms
