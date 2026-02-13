#pragma once

#include "util/Point.h"

#include <cstdint>
#include <memory>

namespace ms
{

class WzGr2DCanvas;

/// Matches TAMINGMOBACTIONFRAMEENTRY from the decompiled client.
/// Holds the pre-rendered canvas layers and attachment point origins
/// for a single frame of a taming mob (riding) action.
struct TamingMobActionFrameEntry
{
    /// Canvas for the under-character layer (mount body behind character)
    std::shared_ptr<WzGr2DCanvas> pCanvasUnderCharacter;

    /// Canvas for the over-character layer (mount body in front of character)
    std::shared_ptr<WzGr2DCanvas> pCanvasOverCharacter;

    /// Frame display duration in milliseconds
    std::int32_t tDelay{150};

    /// Attachment point origins
    Point2D ptNavel{0, 0};
    Point2D ptHead{0, 0};
    Point2D ptMuzzle{0, 0};
};

} // namespace ms
