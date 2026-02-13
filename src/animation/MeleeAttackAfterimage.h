#pragma once

#include "graphics/WzGr2DCanvas.h"
#include "util/security/SecRect.h"

#include <memory>
#include <vector>

namespace ms
{

/// Matches MELEEATTACKAFTERIMAGE from the client (__cppobj : ZRefCounted).
class MeleeAttackAfterimage
{
public:
    std::vector<std::vector<std::shared_ptr<WzGr2DCanvas>>> m_aapCanvas;
    std::vector<SecRect> m_arcRange;
};

} // namespace ms
