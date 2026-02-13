#pragma once

#include "SpriteSource.h"
#include "util/Point.h"

#include <cstdint>
#include <memory>

namespace ms
{

/**
 * @brief Positioned instance of a sprite in an action frame
 *
 * Based on CSpriteInstance from the original MapleStory client.
 * Inherits ZRefCounted in original (we use shared_ptr instead).
 *
 * Places a SpriteSource at a specific position within a frame,
 * with visibility control and group association.
 */
class SpriteInstance
{
public:
    /// Position within the frame
    Point2D m_pt;

    /// Visibility flag
    bool m_bVisible{true};

    /// Pointer to owning group (opaque, original: void*)
    void* m_pGroup{nullptr};

    /// The sprite source (image + metadata)
    std::shared_ptr<SpriteSource> m_pSource;
};

} // namespace ms
