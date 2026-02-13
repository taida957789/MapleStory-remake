#pragma once

#include "graphics/WzGr2DTypes.h"
#include "wz/WzProperty.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

class NpcActionFrameEntry;

/// Matches NPCIMGENTRY from the client (__cppobj : ZRefCounted).
class NpcImgEntry
{
public:
    std::shared_ptr<WzProperty> m_pImg;
    std::int32_t m_tLastAccessed{0};
};

/// Matches NPCACTIONENTRY from the client (__cppobj : ZRefCounted).
class NpcActionEntry
{
public:
    std::uint32_t m_dwTemplateID{0};
    std::int32_t m_nAction{0};
    std::vector<std::shared_ptr<NpcActionFrameEntry>> m_lpFrame;
    LayerBlendType m_blendType{LayerBlendType::Normal};
    std::int32_t m_tLastAccessed{0};
};

} // namespace ms
