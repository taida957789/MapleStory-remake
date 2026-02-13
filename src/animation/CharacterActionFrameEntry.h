#pragma once

#include "util/Point.h"

#include <cstdint>
#include <memory>

namespace ms
{

class WzGr2DCanvas;

/// Matches CHARACTERACTIONFRAMEENTRY from the decompiled client.
/// Holds the pre-rendered canvas layers and attachment point origins
/// for a single frame of a character action.
struct CharacterActionFrameEntry
{
    /// Canvas for the under-face layer (body, arms, etc.)
    std::shared_ptr<WzGr2DCanvas> pCanvasUnderFace;

    /// Canvas for the over-face layer (hair front, cap, etc.)
    std::shared_ptr<WzGr2DCanvas> pCanvasOverFace;

    /// Frame display duration in milliseconds
    std::int32_t tDelay{150};

    /// Body collision rectangle
    Rect rcBody{};

    /// Attachment point origins (extracted from ActionFrame groups)
    Point2D ptBrow{0, 0};
    Point2D ptNavel{0, 0};
    Point2D ptHead{0, 0};
    Point2D ptMuzzle{0, 0};
    Point2D ptHand{0, 0};
    Point2D ptTail{0, 0};
};

} // namespace ms
