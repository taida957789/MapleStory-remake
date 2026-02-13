#pragma once

#include "Stage.h"
#include "graphics/WzGr2DTypes.h"
#include "util/Point.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class WzGr2DLayer;
class WzProperty;

/**
 * @brief Background layer type
 *
 * Defines how a background layer tiles and moves.
 * Based on the original MapleStory background system.
 */
enum class BackgroundType : std::int32_t
{
    Normal = 0,      // No tiling, parallax with rx/ry
    HTiled = 1,      // Horizontal tiling, parallax with rx/ry
    VTiled = 2,      // Vertical tiling, parallax with rx/ry
    Tiled = 3,       // Both H+V tiling, parallax with rx/ry
    HMoveA = 4,      // Animated H movement (rx), then H-tiling
    VMoveA = 5,      // Animated V movement (ry), then V-tiling
    HMoveB = 6,      // Animated H movement (rx), then both tiling
    VMoveB = 7,      // Animated V movement (ry), then both tiling
};

/**
 * @brief Loadable map/scene base class
 *
 * Based on CMapLoadable from the original MapleStory client.
 *
 * CMapLoadable is an intermediate class between CStage and specific stages
 * like CLogin, CField, etc. It provides:
 * - Layer management for objects and backgrounds
 * - BGM/sound handling
 * - Camera control
 * - Object animation
 *
 * Inheritance: CStage -> CMapLoadable -> CLogin/CField/etc.
 */
class MapLoadable : public Stage
{
public:
    /**
     * @brief Camera movement info
     */
    struct CameraMoveInfo
    {
        bool bOn{false};
        bool bClipInViewRange{true};
        std::int32_t tStart{0};
        std::int32_t tEnd{0};
        Point2D ptVelocityFirst{0, 0};
        Point2D ptAcceleration{0, 0};
        Point2D ptVelocityAdjustRate{0, 0};
        Point2D ptAccelerationAdjustRate{0, 0};
    };

    MapLoadable();
    ~MapLoadable() override;

    // Stage interface
    void Init(void* param) override;
    void Update() override;
    void Draw() override;
    void Close() override;

    /**
     * @brief Load map data from WZ properties
     *
     * Based on CMapLoadable::LoadMap at 0xbfff50.
     * This orchestrates loading of:
     * - Physical space (foothold, ladderRope)
     * - Map info properties (quarterView, colorFlow, scale)
     * - Tiles, objects, backgrounds
     * - Life (NPCs, mobs), reactors
     * - Weather, letterbox
     */
    virtual void LoadMap();

    // Layer management
    /**
     * @brief Create a general layer
     * @param z Z-order
     * @return Created layer
     */
    [[nodiscard]] auto CreateLayer(std::int32_t z) -> std::shared_ptr<WzGr2DLayer>;

    /**
     * @brief Create an object layer with a name
     * @param name Layer name for lookup
     * @param z Z-order
     * @return Created layer
     */
    [[nodiscard]] auto CreateObjectLayer(const std::string& name, std::int32_t z)
        -> std::shared_ptr<WzGr2DLayer>;

    /**
     * @brief Get object layer by name
     */
    [[nodiscard]] auto GetObjectLayer(const std::string& name) -> std::shared_ptr<WzGr2DLayer>;

    /**
     * @brief Set animation on a named object layer
     */
    void SetObjectAnimation(const std::string& name, Gr2DAnimationType type);

    /**
     * @brief Set animation on a tagged object layer
     */
    void SetTaggedObjectAnimation(const std::string& tag, Gr2DAnimationType type);

    // BGM handling
    /**
     * @brief Change background music
     */
    void ChangeBGM(const std::string& bgmPath);

    /**
     * @brief Check if BGM path is same as current
     */
    [[nodiscard]] auto IsSameChangeBGM(const std::string& bgmPath) const -> bool;

    /**
     * @brief Prepare next BGM (for transitions)
     */
    void PrepareNextBGM();

    /**
     * @brief Restore muted BGM
     */
    void RestoreMutedBGM();

    /**
     * @brief Play BGM from map info property
     *
     * Based on CMapLoadable::PlayBGMFromMapInfo (0xbeaef8).
     * Reads "bgm" property from m_pPropFieldInfo and plays it.
     * Uses "Sound/" prefix (StringPool 2580).
     */
    void PlayBGMFromMapInfo();

    // Camera control
    /**
     * @brief Set camera movement info
     */
    void SetCameraMoveInfo(std::int32_t tStart, const Point2D& velocity,
                           const Point2D& acceleration,
                           const Point2D& velocityAdjust,
                           const Point2D& accelAdjust,
                           std::int32_t duration, bool clipInViewRange);

    /**
     * @brief Clear camera movement
     */
    void ClearCameraMove();

    /**
     * @brief Get view range rectangle
     */
    [[nodiscard]] auto GetViewRangeRect() const -> Rect;

    // Properties
    [[nodiscard]] auto GetMagLevelObj() const noexcept -> std::int32_t { return m_nMagLevelObj; }
    [[nodiscard]] auto GetMagLevelBack() const noexcept -> std::int32_t { return m_nMagLevelBack; }
    [[nodiscard]] auto IsQuarterViewMap() const noexcept -> bool { return m_bQuarterView; }
    [[nodiscard]] auto GetMinScaleForZoomOut() const noexcept -> std::int32_t { return m_nMinZoomOutScale; }

    void SetSysTrembleOpt(bool enable) noexcept { m_bSysTrembleOpt = enable; }

protected:
    /**
     * @brief Load objects from WZ property
     */
    void LoadObjects(const std::shared_ptr<WzProperty>& prop, std::int32_t baseZ);

    /**
     * @brief Load animated frames from WZ property into a layer
     *
     * This reads numbered children (0, 1, 2, ...) from the property and
     * loads their canvas frames into the layer.
     *
     * @param layer Target layer to add canvases to
     * @param prop WZ property containing animated frames
     * @return Number of frames loaded
     */
    auto LoadAnimatedLayer(const std::shared_ptr<WzGr2DLayer>& layer,
                           const std::shared_ptr<WzProperty>& prop) -> std::size_t;

    /**
     * @brief Load a single canvas from WZ property into a layer
     *
     * @param layer Target layer to add canvas to
     * @param prop WZ property containing the canvas
     * @return true if canvas was loaded successfully
     */
    auto LoadStaticLayer(const std::shared_ptr<WzGr2DLayer>& layer,
                         const std::shared_ptr<WzProperty>& prop) -> bool;

    /**
     * @brief Update all object layers
     */
    void UpdateObjectLayers();

    /**
     * @brief Update camera movement effect
     *
     * Based on CMapLoadable::UpdateCameraMoveEffect at 0xbda610.
     * Handles animated camera movement with velocity and acceleration.
     */
    void UpdateCameraMoveEffect();

    /**
     * @brief Clip camera position to view range bounds
     */
    void ClipCameraToViewRange();

    /**
     * @brief Clear all layers
     */
    void ClearAllLayers();

    /**
     * @brief Restore background layers from WZ property
     *
     * This is based on CMapLoadable::RestoreBack from the original client.
     * It reads the "back" property from m_pPropFieldRefBack or m_pPropField
     * and creates layers for each background piece.
     *
     * @param bLoad Whether to load the resources (true) or just prepare (false)
     */
    void RestoreBack(bool bLoad = true);

    /**
     * @brief Create a single background layer from a back piece property
     *
     * Based on CMapLoadable::MakeBack. Reads properties like bS, no, ani,
     * x, y, rx, ry, cx, cy, etc. and creates the appropriate layer.
     *
     * @param nPageIdx Page/layer index for z-ordering
     * @param pPiece The WZ property for this background piece
     * @param bLoad Whether to load resources
     */
    void MakeBack(std::int32_t nPageIdx, const std::shared_ptr<WzProperty>& pPiece, bool bLoad);

    /**
     * @brief Update the background tag layer references after loading
     */
    void UpdateBackTagLayer();

    /**
     * @brief Clear all background layers
     */
    void ClearBackLayers();

    /**
     * @brief Restore view range from map info
     *
     * Based on CMapLoadable::RestoreViewRange at 0xbe6ff0.
     * Calculates the view range rectangle from the field info properties
     * (VRLeft, VRTop, VRRight, VRBottom) with fallback to physical space MBR.
     * Also calculates m_nMinZoomOutScale for zoom limits.
     */
    void RestoreViewRange();

    /**
     * @brief Restore tile layers from WZ property
     *
     * Based on CMapLoadable::RestoreTile at 0xbea1d0.
     * Iterates through 8 pages and creates tile layers from tile properties.
     */
    void RestoreTile();

    /**
     * @brief Restore object layers from WZ property
     *
     * Based on CMapLoadable::RestoreObj at 0xbff8e0.
     * @param bLoad Whether to load the resources
     */
    void RestoreObj(bool bLoad);

    /**
     * @brief Restore weather effects
     *
     * Based on CMapLoadable::RestoreWeather at 0xbeada0.
     */
    void RestoreWeather();

    /**
     * @brief Restore letterbox (cinema bars)
     *
     * Based on CMapLoadable::RestoreLetterBox at 0xbeb920.
     */
    void RestoreLetterBox();

    /**
     * @brief Load life (NPCs and mobs) from WZ property
     *
     * Part of CMapLoadable::LoadMap at 0xbfff50.
     */
    void LoadLife();

    /**
     * @brief Load reactors from WZ property
     *
     * Part of CMapLoadable::LoadMap at 0xbfff50.
     */
    void LoadReactors();

    /**
     * @brief Load rect event data
     *
     * Part of CMapLoadable::LoadMap at 0xbfff50.
     */
    void LoadRectEventData();

    /**
     * @brief Set footstep sound
     */
    void SetFootStepSound(const std::string& sound);

    /**
     * @brief Create a single tile layer
     *
     * Based on CMapLoadable::MakeTile at 0xbe21b0.
     * @param nPageIdx Page/layer index for z-ordering
     * @param pTileSet The tile set WZ property
     * @param pPiece The specific tile piece property
     */
    void MakeTile(std::int32_t nPageIdx,
                  const std::shared_ptr<WzProperty>& pTileSet,
                  const std::shared_ptr<WzProperty>& pPiece);

    /**
     * @brief Create a single object layer
     *
     * Based on CMapLoadable::MakeObj at 0xbfc950.
     * Reads properties: oS (object set), l0, l1, l2 (layers), x, y, z, f (flip),
     * rx, ry (parallax), flow, name, tags.
     *
     * @param nPageIdx Page/layer index for z-ordering
     * @param pPiece The object piece WZ property
     * @param bLoad Whether to load the resources
     */
    void MakeObj(std::int32_t nPageIdx,
                 const std::shared_ptr<WzProperty>& pPiece,
                 bool bLoad);

    /**
     * @brief Update object tag layer references after loading
     */
    void UpdateObjectTagLayer();

    /**
     * @brief Create cloud weather effects
     *
     * Based on CMapLoadable::MakeCloud at 0xbe3230.
     * Creates random cloud layers that move across the map.
     */
    void MakeCloud();

    /**
     * @brief Add a letterbox (black bar) layer
     *
     * Based on CMapLoadable::AddLetterBox at 0xbe7ec0.
     * Creates a black rectangle at the specified position.
     *
     * @param w Width of the letterbox
     * @param h Height of the letterbox
     * @param l Left position (relative to screen center)
     * @param t Top position (relative to screen center)
     */
    void AddLetterBox(std::int32_t w, std::int32_t h, std::int32_t l, std::int32_t t);

    // Color flow name
    std::string m_sColorFlowName;

    // Background layers
    std::vector<std::shared_ptr<WzGr2DLayer>> m_lpLayerBack; // Background layers

protected:
    // BGM
    std::string m_sChangedBgmUOL;
    std::string m_sFieldCustomBgmUOL;
    std::int32_t m_nJukeBoxItemID{0};
    bool m_bJukeBoxPlaying{false};
    std::uint32_t m_unWeatherSoundCookie{0};

    // Properties from WZ
    std::shared_ptr<WzProperty> m_pPropFieldInfo;
    std::shared_ptr<WzProperty> m_pPropField;
    std::shared_ptr<WzProperty> m_pPropFieldRefBack;

    // Layer lists
    std::vector<std::shared_ptr<WzGr2DLayer>> m_lpLayerGen;       // General layers
    std::vector<std::shared_ptr<WzGr2DLayer>> m_lpLayerObj;       // Object layers
    std::vector<std::shared_ptr<WzGr2DLayer>> m_lpLayerTransient; // Transient layers
    std::vector<std::shared_ptr<WzGr2DLayer>> m_lpLayerLetterBox; // Letterbox layers

    // Named layer maps
    std::map<std::string, std::shared_ptr<WzGr2DLayer>> m_mpLayerObj;
    std::map<std::string, std::shared_ptr<WzGr2DLayer>> m_mTaggedLayer;

    // Mag levels (zoom/scale)
    std::int32_t m_nMagLevelObj{0};
    std::int32_t m_nMagLevelBack{0};
    std::int32_t m_nMagLevelSkillEffect{0};
    std::int32_t m_nMinZoomOutScale{1000};
    bool m_bMagLevelModifying{false};

    // Scale
    std::int32_t m_nScaleField{1000};

    // Weather
    std::int32_t m_nWeatherFadeInTime{0};
    std::int32_t m_tForceFadeOutTime{0};

    // Map properties
    bool m_bNeedZoomOutMap{false};
    bool m_bNoFollowCharacter{false};
    bool m_bStandAlone{false};
    bool m_bPartyStandAlone{false};
    bool m_bQuarterView{false};
    bool m_bBGMVolumeOnly{false};
    bool m_bSysTrembleOpt{false};

    // BGM restore
    std::int32_t m_tRestoreBgmVolume{0};
    std::int32_t m_nRestoreBgmVolume{0};

    // Camera
    CameraMoveInfo m_cameraMoveInfo;

    // View range
    Rect m_viewRangeRect;
};

} // namespace ms
