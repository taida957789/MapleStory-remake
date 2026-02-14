#pragma once

#include "Stage.h"
#include "graphics/WzGr2DTypes.h"
#include "util/Point.h"

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class WzCanvas;
class WzGr2DLayer;
class WzProperty;
class WvsPhysicalSpace2D;

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
    struct ObjectState
    {
        std::int32_t nRepeat;
        std::string bsSfx;
        std::shared_ptr<WzGr2DLayer> pLayer;
    };
    struct ChangingObject
    {
        std::int32_t nState;
        std::uint32_t dwSN;
        std::vector<ObjectState> aState;
    };

    struct DelayInvisibleLayer
    {
        std::int32_t tDelayTime;
        std::int32_t tStartTime;
        std::int32_t nManual;
        std::int32_t bVisible;
        std::int32_t bSmooth;
        std::string sTag;
    };

    struct Obstacle
    {
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::int32_t bFlip;
        std::int32_t nDamage;
        std::int32_t nMobDamage;
        std::int32_t nDirection;
        std::int32_t nMobSkillID;
        std::int32_t nSLV;
        std::string sName;
        std::uint32_t dwTargetField;
    };
    struct ObstacleInfo
    {
        Rect rcObs;
        Point2D vecForce;
        std::int32_t bLinearCheck;
        const Obstacle* pObstacle;
    };

    struct ReflectionInfo
    {
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::shared_ptr<WzCanvas> pOriginalCanvas;
        std::shared_ptr<WzCanvas> pAvatarCanvas;
        std::shared_ptr<WzCanvas> pRemoverCanvas;
        Rect rcArea;
        std::int32_t nAlpha;
        std::int32_t bLastFrameUpdated;
    };

    struct VisibleByQuest
    {
        std::shared_ptr<WzGr2DLayer> pLayer;
        std::map<std::int32_t, std::vector<std::pair<std::int32_t, std::string>>> mCond;
        std::vector<std::pair<std::int32_t, std::int32_t>> aCond;
    };

    struct CameraMoveInfo
    {
        std::int32_t bOn{};
        std::int32_t bClipInViewRange{};
        std::int32_t tStart{};
        std::int32_t tEnd{};
        Point2D ptVelocity_First{};
        Point2D ptAccelation{};
        Point2D ptVelocity_AdjustRate{};
        Point2D ptAccelation_AdjustRate{};
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
    void DisableEffectObject(const std::string& sName, bool bCheckPreWord);
    static void AnimateObjLayer(const std::shared_ptr<WzGr2DLayer>& pLayer, std::int32_t nRepeat);

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
    [[nodiscard]] auto GetViewRangeRect() const -> const Rect*;

    // Properties
    [[nodiscard]] auto GetMagLevelObj() const noexcept -> std::int32_t { return m_nMagLevel_Obj; }
    [[nodiscard]] auto GetMagLevelBack() const noexcept -> std::int32_t { return m_nMagLevel_Back; }
    [[nodiscard]] auto IsQuarterViewMap() const noexcept -> bool { return m_bQuarterView; }
    [[nodiscard]] auto GetMinScaleForZoomOut() const noexcept -> std::int32_t { return m_nMinZoomOutScale; }

    void SetSysTrembleOpt(bool enable) noexcept { m_bSysOptTremble = enable; }

    // --- Properties / Getters (IDA stubs) ---

    /// @brief 0xbd39b0 - Check if BGM volume only mode
    [[nodiscard]] auto IsBGMVolumeOnly() const noexcept -> bool { return m_bBGMVolumeOnly; }

    /// @brief 0x1a9a7c0 - Check if jukebox is currently playing
    [[nodiscard]] auto IsJukeBoxPlaying() const noexcept -> bool { return m_bJukeBoxPlaying != 0; }

    /// @brief 0xbdf9f0 - Check if object is a fade object
    [[nodiscard]] auto IsFadeObject(const std::string& sGroupName, std::int32_t nShowType) const -> bool;

    /// @brief 0xbd9ab0 - Get collide obstacle rectangle
    [[nodiscard]] auto GetCollideObstacleRect(const Point2D& pt, Point2D* pvecForce) const -> const ObstacleInfo*;

    /// @brief 0xbdceb0 - Get NPC rect event type
    [[nodiscard]] auto GetNpcRectEventType(const std::string& sName) const -> std::int32_t;

    /// @brief 0xbf5d00 - Get current object layer by name
    [[nodiscard]] auto GetCurrentObject(const std::string& sName) const -> std::shared_ptr<WzGr2DLayer>;

    /// @brief 0xbf6460 - Get object serial number
    [[nodiscard]] auto GetObjectSN(const std::string& sName) const -> std::uint32_t;

    /// @brief 0xbf6490 - Get object state index
    [[nodiscard]] auto GetObjectState(const std::string& sName) const -> std::int32_t;

    /// @brief 0xbf64c0 - Get object bounding rectangle
    [[nodiscard]] auto GetObjectRect(const std::string& sName) const -> Rect;

    /// @brief 0x1aa2fe0 - Check if any transient layer exists
    [[nodiscard]] auto TransientLayer_Exist() const -> bool;

    // --- Layer / Visual (IDA stubs) ---

    /// @brief 0xbdd070 - Set gray background filter
    void SetGrayBackGround(bool bGray);

    /// @brief 0xbdd3c0 - Set background color (RGB channels + delay)
    void SetBackGroundColor(std::int32_t nR, std::int32_t nG, std::int32_t nB, std::int32_t tDelay);

    /// @brief 0xbdfab0 - Set background color by tag
    void SetBackGroundColorByTag(const std::string& sTag, std::int32_t nR, std::int32_t nG, std::int32_t nB, std::int32_t tDelay);

    /// @brief 0xbdfc50 - Set object visibility
    void SetObjectVisible(const std::string& sName, bool bVisible);

    /// @brief 0xbdfd20 - Set object movement
    void SetObjectMove(const std::string& sName, std::int32_t nX, std::int32_t nY, std::int32_t tDuration);

    /// @brief 0xbe8570 - Create object layer at position from property path
    void SetObjectCreateLayer(const std::string& sKeyName, const std::string& sPath,
                              std::uint32_t nX, std::uint32_t nY);

    /// @brief 0xbf5840 - Set object state
    void SetObjectState(const std::string& sName, std::int32_t nState);

    /// @brief 0xbf8700 - Enqueue delayed layer visibility change
    void SetLayerInvisible(const std::string& sTag, std::int32_t tDelay,
                           std::int32_t bVisible, std::int32_t nManual, std::int32_t bSmooth);

    /// @brief 0xbf8e00 - Set visibility for a list of layers
    void SetLayerListVisible(const std::shared_ptr<std::list<std::shared_ptr<WzGr2DLayer>>>& plLayer,
                             std::int32_t bVisible, bool bSmooth, std::int32_t nManual,
                             const std::string& sTag);

    /// @brief 0xbfb180 - Set layer list visible by tag (quest-aware)
    void SetLayerListVisibleByTag(const std::string& sTag,
                                  const std::shared_ptr<std::list<std::shared_ptr<WzGr2DLayer>>>& plObjs);

    /// @brief 0xbfb440 - Set map tagged object visible
    void SetMapTagedObjectVisible(const std::string& sTag, std::int32_t bVisible,
                                  std::int32_t bSmooth, std::int32_t tDuration);

    /// @brief 0xbffcc0 - Set field magnification level (reads from config)
    void SetFieldMagLevel();

    /// @brief 0xbd9850 - Clear all transient layers
    void TransientLayer_Clear();

    /// @brief 0xbe3da0 - Create transient weather layer
    void TransientLayer_Weather(std::int32_t nItemID, const std::string& sMsg);

    // --- BGM / Sound (IDA stubs) ---

    /// @brief 0xbee210 - Play next music track
    void PlayNextMusic();

    /// @brief 0xbee8d0 - Play sound with muted BGM
    void PlaySoundWithMuteBgm(const std::string& sName, bool bExcl, bool bDown,
                              std::uint32_t uVolume128);

    /// @brief 0xbe8430 - Set camera move info (string overload)
    void SetCameraMoveInfo(const std::string& sInfo);

    // --- Foothold (IDA stubs) ---

    /// @brief 0xbf54a0 - Move foothold
    void FootHoldMove(std::int32_t nSN, std::int32_t nX, std::int32_t nY);

    /// @brief 0xbf56d0 - Change foothold state
    void FootHoldStateChange(std::int32_t nSN, std::int32_t nState);

    // --- Rendering (IDA stubs) ---

    /// @brief 0xbde930 - Render avatar reflection
    void RenderAvatar();

    /// @brief 0xbdf2b0 - Process reflection effect
    void ProcessReflection();

    // --- Fade (IDA stubs) ---

    /// @brief 0xbef2e0 - Set fade data (layer overload)
    void SetFadeData(const std::shared_ptr<WzGr2DLayer>& pLayer, std::int32_t nAlpha, std::int32_t tDuration);

    /// @brief 0xbef560 - Set fade data (index overload)
    void SetFadeData(std::int32_t nIndex, std::int32_t nAlpha, std::int32_t tDuration);

    // --- Event Handlers (IDA stubs) ---

    /// @brief 0xbd5140 - Leave direction mode
    void OnLeaveDirectionMode();

    /// @brief 0xbe14e0 - Set back effect
    void OnSetBackEffect(const std::string& sName, std::int32_t nEffect);

    /// @brief 0xbeb210 - Set spine back effect
    void OnSetSpineBackEffect(const std::string& sName);

    /// @brief 0xbeb430 - Set spine object effect
    void OnSetSpineObjectEffect(const std::string& sName);

    /// @brief 0xbeb5f0 - Remove spine rect event
    void OnRemoveSpineRectEvent(const std::string& sName);

    /// @brief 0xbeb730 - Remove camera control zone
    void OnRemoveCameraCtrlZone(const std::string& sName);

    /// @brief 0xbec280 - Set map object animation (packet handler)
    void OnSetMapObjectAnimation(const std::string& sName, std::int32_t nAniType);

    /// @brief 0xbec320 - Set map tagged object animation (packet handler)
    void OnSetMapTaggedObjectAnimation(const std::string& sTag, std::int32_t nAniType);

    /// @brief 0xbec3e0 - Set map object visible (packet handler)
    void OnSetMapObjectVisible(const std::string& sName, bool bVisible);

    /// @brief 0xbec4a0 - Set map object move (packet handler)
    void OnSetMapObjectMove(const std::string& sName, std::int32_t nX, std::int32_t nY, std::int32_t tDuration);

    /// @brief 0xbec570 - Set map object create layer (packet handler)
    void OnSetMapObjectCreateLayer(const std::string& sKeyName, const std::string& sPath,
                                   std::uint32_t nX, std::uint32_t nY);

    /// @brief 0xbfc160 - Clear back effect
    void OnClearBackEffect();

    /// @brief 0xbfc170 - Set map tagged object visible (packet handler, decodes loop)
    void OnSetMapTagedObjectVisible(const std::string& sTag, std::int32_t bVisible,
                                    std::int32_t tDuration, std::int32_t tDelay);

    /// @brief 0xbfc270 - Set map tagged object smooth visible (packet handler, decodes loop)
    void OnSetMapTaggedObjectSmoothVisible(const std::string& sTag, std::int32_t bVisible,
                                           std::int32_t tDuration, std::int32_t tDelay);

    /// @brief 0xbfc370 - Event: screen resolution changed (virtual)
    virtual void OnEventChangeScreenResolution();

    /// @brief 0xbfc460 - Handle incoming packet (virtual)
    virtual void OnPacket(std::int32_t nType, const void* pData);

    /// @brief 0xbf8a60 - Create spine rect event
    void OnCreateSpineRectEvent(const std::string& sName);

    /// @brief 0xbf8bf0 - Create camera control zone
    void OnCreateCameraCtrlZone(const std::string& sName);

    /// @brief 0xbee530 - Spine rect event: add back event
    void OnSpineRE_AddBackEvent(const std::string& sName);

    /// @brief 0xbee700 - Spine rect event: add object event
    void OnSpineRE_AddObjectEvent(const std::string& sName);

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
    void PlayFootStepSound();

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

    // --- Make / Create (IDA stubs) ---

    /// @brief 0xbe07c0 - Create object skeleton layer
    void MakeObjSkeleton(std::int32_t nPageIdx,
                         const std::shared_ptr<WzProperty>& pPiece,
                         bool bLoad);

    /// @brief 0xbed9b0 - Create object layer from property
    void MakeObjLayer(std::int32_t nPageIdx,
                      const std::shared_ptr<WzProperty>& pPiece,
                      std::shared_ptr<WzGr2DLayer>& pOutLayer);

    /// @brief 0xbe2990 - Create vector animate for layer
    void MakeVectorAnimate(const std::shared_ptr<WzGr2DLayer>& pLayer,
                           const std::shared_ptr<WzProperty>& pProp);

    /// @brief 0xbea8c0 - Create obstacles from WZ property
    void MakeObstacles();

    /// @brief 0xbeff40 - Create grid layout
    void MakeGrid(std::int32_t nPageIdx,
                  const std::shared_ptr<WzProperty>& pPiece,
                  bool bLoad);

    /// @brief 0xbf1640 - Create grid skeleton layout
    void MakeGridSkeleton(std::int32_t nPageIdx,
                          const std::shared_ptr<WzProperty>& pPiece,
                          bool bLoad);

    // --- Restore / Load / Reload (IDA stubs) ---

    /// @brief 0xbdcb60 - Restore back effect layers
    void RestoreBackEffect();

    /// @brief 0xbeb070 - Restore BGM from map info
    void RestoreBGM(bool bForceRestart = false);

    /// @brief 0xbfc000 - Reload background layers
    void ReloadBack();

    /// @brief 0xbf83d0 - Load BGM sub info from property
    void LoadBgmSubInfo(const std::shared_ptr<WzProperty>& pProp);

    /// @brief 0xbfadc0 - Load BGM sub-track
    void LoadBgmSub();

    /// @brief 0xbef690 - Insert back layer by tag (iterates tag list, adds to m_mTagedBack)
    void InsertbackLayerByTag(const std::vector<std::string>& tags,
                              const std::shared_ptr<WzGr2DLayer>& pLayer);

    /// @brief 0xbef8b0 - Insert back skeleton by tag
    void InsertbackSkeletonByTag(const std::vector<std::string>& tags,
                                 const std::shared_ptr<WzGr2DLayer>& pLayer);

    // --- Update (IDA stubs) ---

    /// @brief 0xbe7410 - Update obstacle info from current state
    void UpdateObstacleInfo();

    /// @brief 0xbfc3f0 - Update tag layer references
    void UpdateTagLayer();

    /// @brief 0xbfba10 - Update layer invisibility (delay invisible layers)
    void UpdateLayerInvisible();

protected:
    // --- BGM ---
    std::int32_t m_nJukeBoxItemID{};
    std::int32_t m_tNextMusic{};
    std::int32_t m_bJukeBoxPlaying{};
    std::uint32_t m_unWeatherSoundCookie{};
    std::u16string m_sChangedBgmUOL;
    std::u16string m_sFieldCustomBgmUOL;

    // --- WZ Properties ---
    std::shared_ptr<WzProperty> m_pPropFieldInfo;
    std::shared_ptr<WzProperty> m_pPropField;
    std::shared_ptr<WzProperty> m_pPropFieldRefBack;

    // --- Physical space ---
    std::shared_ptr<WvsPhysicalSpace2D> m_pSpace2D;

    // --- Layer lists ---
    std::list<std::shared_ptr<WzGr2DLayer>> m_lpLayerGen;
    std::list<std::shared_ptr<WzGr2DLayer>> m_lpLayerObj;
    std::map<std::string, std::shared_ptr<WzGr2DLayer>> m_mpLayerObj;
    std::list<std::shared_ptr<WzGr2DLayer>> m_lpLayerTransient;

    // --- Obstacle / Reflection / Quest visibility ---
    std::list<std::shared_ptr<Obstacle>> m_lpObstacle;
    std::list<std::shared_ptr<ReflectionInfo>> m_lpRefInfo;
    std::list<VisibleByQuest> m_lVisibleByQuest;

    // --- Named/tagged objects ---
    std::map<std::string, ChangingObject> m_mNamedObj;
    std::map<std::string, std::shared_ptr<WzGr2DLayer>> m_mTaggedLayer;
    std::map<std::string, std::shared_ptr<std::list<std::shared_ptr<WzGr2DLayer>>>> m_mTagedObj;
    std::map<std::string, std::shared_ptr<std::list<std::shared_ptr<WzGr2DLayer>>>> m_mTagedBack;

    // --- Background layers --- (original: ZMap<long, ZRef<ZList<IWzGr2DLayer>>, long>)
    std::map<std::int32_t, std::shared_ptr<std::list<std::shared_ptr<WzGr2DLayer>>>> m_mlLayerBack;
    std::list<std::int32_t> m_lBackEffect;

    // --- Awesomium ---
    struct AwesomiumInfo
    {
        std::shared_ptr<WzGr2DLayer> pAwesomiumBackgrnd;
        std::shared_ptr<WzCanvas> pAwesomiumCanvas;
        std::string sURL;
        std::uint32_t dwIndex;
        std::int32_t nWebWidth;
        std::int32_t nWebHeight;
        std::int32_t nWebX;
        std::int32_t nWebY;
    };
    std::list<AwesomiumInfo> m_lAwesomiumInfo;

    // --- Letterbox ---
    std::list<std::shared_ptr<WzGr2DLayer>> m_lpLayerLetterBox;

    // --- Mag levels ---
    std::int32_t m_nMagLevel_Obj{};
    std::int32_t m_nMagLevel_Back{};
    std::int32_t m_nMagLevel_SkillEffect{};

    // --- View range ---
    Rect m_rcViewRange;
    std::int32_t m_nMinZoomOutScale{};
    std::int32_t m_bSysOptTremble{};
    std::int32_t m_bMagLevelModifying{};

    // --- Obstacle info ---
    std::vector<ObstacleInfo> m_aObstacleInfo;

    // --- Weather ---
    std::int32_t m_nWeatherFadeInTime{};
    std::int32_t m_tForceFadeOutTime{};

    // --- Scale ---
    std::int32_t m_nScaleField{};

    // --- Map properties ---
    std::int32_t m_bNeedZoomOutMap{};
    bool m_bNoFollowCharacter{};
    bool m_bStandAlone{};
    bool m_bPartyStandAlone{};
    bool m_bQuarterView{};

    // --- BGM restore ---
    std::int32_t m_tRestoreBgmVolume{};
    std::int32_t m_nRestoreBgmVolume{};
    bool m_bBGMVolumeOnly{};

    // --- Camera ---
    CameraMoveInfo m_cameraMoveInfo;

    // --- Rect event / zone data ---
    std::map<std::string, std::shared_ptr<void>> m_mpRectEventData;    // RectEventData
    std::map<std::string, std::shared_ptr<void>> m_mpFadeData;         // FadeData
    std::map<std::string, std::shared_ptr<void>> m_mpBgmZoneData;      // BgmZoneData
    std::map<std::string, std::shared_ptr<void>> m_mpAmbienceZoneData; // AmbienceZoneData
    std::map<std::string, std::shared_ptr<void>> m_mpFootstepZoneData; // FootStepZoneData
    std::map<std::string, std::shared_ptr<void>> m_mpEffectZoneData;   // EffectZoneData
    std::map<std::string, std::shared_ptr<void>> m_mpScriptRunZoneData;// ScriptRunZoneData
    std::map<std::string, std::shared_ptr<void>> m_mpSpineEventZoneData;// SpineEventZone
    std::map<std::string, std::shared_ptr<void>> m_mpCameraCtrlZoneData;// CameraCtrlZone

    // --- Sub BGM ---
    std::map<std::int32_t, std::string> m_mSubBgm;

    // --- Footstep sound ---
    std::u16string m_wsFootStepSound;
    std::int32_t m_nFootStepSoundCount{};
    std::shared_ptr<WzProperty> m_pFootStepSoundProp;

    // --- Color flow ---
    std::string m_sColorFlowName;

    // --- Delay invisible ---
    std::vector<DelayInvisibleLayer> m_aDelayInvisibleLayer;

};

} // namespace ms
