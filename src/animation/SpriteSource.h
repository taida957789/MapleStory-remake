#pragma once

#include "util/Point.h"

#include <cstdint>
#include <memory>
#include <string>

namespace ms
{

class WzGr2DCanvas;
class WzProperty;

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
    /// Initialize from slot strings, canvas, and sprite property.
    /// (from decompiled CSpriteSource::Init)
    void Init(
        const std::string& sISlot,
        const std::string& sVSlot,
        const std::shared_ptr<WzGr2DCanvas>& pRawSprite,
        std::int32_t nJob,
        const std::shared_ptr<WzProperty>& pSpriteProp
    );

    /// Resolve z-order from zmap given a base VSlot.
    /// Also sets outVSlot to the resolved visual slot.
    /// (from decompiled CSpriteSource::QueryZ @ 0xfbd4d0)
    static auto QueryZ(
        const std::shared_ptr<WzProperty>& pProp,
        const std::string& sBaseVSlot,
        std::string& outVSlot,
        const std::string& sModifiedZ
    ) -> std::int32_t;

    /// Simplified QueryZ for face-look loading: reads "z" from a canvas
    /// property node and resolves to numeric z-index via ZMapper.
    /// (no slot params — common case used by LoadFaceLook)
    static auto QueryZ(const std::shared_ptr<WzProperty>& pCanvasProp)
        -> std::int32_t;

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

} // namespace ms
