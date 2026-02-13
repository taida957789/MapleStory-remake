#pragma once

#include "wz/WzProperty.h"

#include <cstdint>
#include <memory>

namespace ms
{

/// Matches MOBIMGENTRY from the client (__cppobj : ZRefCounted).
class MobImgEntry
{
public:
    std::shared_ptr<WzProperty> m_pImg;
    std::int32_t m_tLastAccessed{0};
};

} // namespace ms
