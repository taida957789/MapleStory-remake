#pragma once

#include "util/Point.h"

#include <cstdint>
#include <memory>
#include <string>

namespace ms
{

class WzGr2DCanvas;

/**
 * @brief Sprite source data (image + equip slot metadata)
 *
 * Based on CSpriteSource from the original MapleStory client.
 * Inherits ZRefCounted in original (we use shared_ptr instead).
 *
 * Holds the actual canvas/image data plus equipment slot info:
 * - VSlot (Visual Slot) determines where it renders on the character
 * - ISlot (Item Slot) determines the equipment type
 */
class SpriteSource
{
public:
    /// Base visual slot identifier
    std::string m_sBaseVSlot;

    /// Item slot string (e.g., "Wp", "Cp", "Si")
    std::string m_sISlot;

    /// Visual slot string (e.g., "Bd", "ArmOverHair")
    std::string m_sVSlot;

    /// Item slot number
    std::int32_t m_nISlot{0};

    /// Sprite dimensions (original: tagSIZE with cx/cy)
    std::int32_t m_cx{0};
    std::int32_t m_cy{0};

    /// Z-order for layering
    std::int32_t m_z{0};

    /// Center/origin point within the sprite
    Point2D m_ptCenter;

    /// Canvas containing the sprite image (original: IWzCanvas COM ptr)
    std::shared_ptr<WzGr2DCanvas> m_pSprite;
};

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
