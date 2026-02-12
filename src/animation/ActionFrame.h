#pragma once

#include "util/Point.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class SpriteInstance;
class WzProperty;

/**
 * @brief Single frame of a character action animation
 *
 * Based on CActionFrame from the original MapleStory client.
 * Contains a list of sprite instances (layers), attachment point groups,
 * body collision rect, and frame timing.
 */
class ActionFrame
{
public:
    /// Named attachment point (from decompiled CActionFrame::MAPINFO)
    struct MapInfo
    {
        std::string sName;
        Point2D pt;
    };

    ActionFrame();
    virtual ~ActionFrame() = default;

    ActionFrame(const ActionFrame& other) = default;
    auto operator=(const ActionFrame& other) -> ActionFrame& = default;
    ActionFrame(ActionFrame&&) noexcept = default;
    auto operator=(ActionFrame&&) noexcept -> ActionFrame& = default;

    // === Static mapper initialization (from decompiled LoadMappers) ===

    /// Load zmap.img and smap.img mapper properties from Base.wz.
    /// Must be called once before using ActionFrame instances.
    static void LoadMappers();

    // === Static mapper accessors ===
    [[nodiscard]] static auto GetZMapper() noexcept
        -> const std::shared_ptr<WzProperty>& { return s_pZMapper; }
    [[nodiscard]] static auto GetSMapper() noexcept
        -> const std::shared_ptr<WzProperty>& { return s_pSMapper; }
    [[nodiscard]] static auto GetFaceZ() noexcept
        -> std::int32_t { return s_nFaceZ; }
    [[nodiscard]] static auto GetCharacterStartZ() noexcept
        -> std::int32_t { return s_nCharacterStartZ; }
    [[nodiscard]] static auto GetCharacterEndZ() noexcept
        -> std::int32_t { return s_nCharacterEndZ; }

    // === Instance members ===

    /// Sprite instances composing this frame (body, arm, head, etc.)
    std::vector<std::shared_ptr<SpriteInstance>> m_lpSprites;

    /// Minimum bounding rectangle enclosing all sprites
    Rect m_rcMBR;
    bool m_bMBRValid{false};

    /// Body sprite (main character body layer)
    std::shared_ptr<SpriteInstance> m_pSpriteBody;
    bool m_bBody{true};

    /// Exclusive equip VSlot (prevents certain equip slots from rendering)
    std::string m_sExclVSlot;

    /// Groups of attachment point maps (navel, neck, hand, etc.)
    /// Original: ZList<ZRef<ZList<MAPINFO>>> — shared inner lists
    std::vector<std::shared_ptr<std::vector<MapInfo>>> m_lGroups;

    /// Frame display duration in milliseconds
    std::int32_t tDelay{0};

    /// Body collision rectangle
    Rect rcBody;

private:
    // === Static mapper state (from decompiled statics) ===

    /// WZ path to zmap.img (original: s_sZMapImg)
    static const std::string s_sZMapImg;
    /// WZ path to smap.img (original: s_sSMapImg)
    static const std::string s_sSMapImg;

    /// Z-order mapper property (Base/zmap.img)
    static std::shared_ptr<WzProperty> s_pZMapper;
    /// Slot mapper property (Base/smap.img)
    static std::shared_ptr<WzProperty> s_pSMapper;

    /// Z-order of face layer
    static std::int32_t s_nFaceZ;
    /// Z-order of character rendering start
    static std::int32_t s_nCharacterStartZ;
    /// Z-order of character rendering end
    static std::int32_t s_nCharacterEndZ;
};

} // namespace ms
