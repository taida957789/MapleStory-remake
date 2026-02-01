#include "MapLoadable.h"
#include "audio/SoundSystem.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "util/Logger.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

#include <algorithm>

#ifdef MS_DEBUG_CANVAS
#include "debug/DebugOverlay.h"
#endif

namespace ms
{

MapLoadable::MapLoadable() = default;

MapLoadable::~MapLoadable()
{
    ClearAllLayers();
}

void MapLoadable::Init(void* param)
{
    Stage::Init(param);

    // Based on CMapLoadable::Init at 0xbe1f70

    auto& gr = get_gr();

    // Reset camera to world origin (0,0)
    // The renderer adds screen center offset, so camera at (0,0) shows world (0,0) at screen center
    gr.SetCameraPosition(0, 0);

    // Clear any existing layers
    ClearAllLayers();

    // Initialize default values (matching constructor defaults from decompiled code)
    m_nMagLevelObj = 0;
    m_nMagLevelBack = 0;
    m_nMagLevelSkillEffect = 0;
    m_nMinZoomOutScale = 1000;
    m_bMagLevelModifying = false;
    m_nScaleField = 1000;
    m_bNeedZoomOutMap = false;
    m_bNoFollowCharacter = false;
    m_bStandAlone = false;
    m_bPartyStandAlone = false;
    m_bQuarterView = false;
    m_bBGMVolumeOnly = false;

    // Initialize camera move info
    ClearCameraMove();
    m_cameraMoveInfo.bClipInViewRange = true;

    LOG_INFO("MapLoadable initialized");
}

void MapLoadable::Update()
{
    static int nCounter = 0;

    // Based on CMapLoadable::Update at 0xbfbb00
    // Every 4 frames, clean up transient layers with 0 alpha
    if (!(++nCounter % 4))
    {
        auto& gr = get_gr();
        auto it = m_lpLayerTransient.begin();
        while (it != m_lpLayerTransient.end())
        {
            auto& layer = *it;
            if (layer)
            {
                // Extract alpha from ARGB color (high byte)
                auto alpha = static_cast<std::uint8_t>((layer->GetColor() >> 24) & 0xFF);
                if (alpha == 0)
                {
                    gr.RemoveLayer(layer);
                    it = m_lpLayerTransient.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    // Handle BGM volume restore
    if (m_nRestoreBgmVolume != 0)
    {
        auto& gr = get_gr();
        auto now = gr.GetCurrentTime();
        if (static_cast<std::int32_t>(now - m_tRestoreBgmVolume) < 0)
        {
            SoundSystem::GetInstance().SetBGMVolume(
                static_cast<std::uint32_t>(m_nRestoreBgmVolume), 500);
            m_tRestoreBgmVolume = 0;
            m_nRestoreBgmVolume = 0;
        }
    }

    // Update camera movement effect
    UpdateCameraMoveEffect();

    // Update all object layers
    UpdateObjectLayers();
}

void MapLoadable::UpdateCameraMoveEffect()
{
    // Based on CMapLoadable::UpdateCameraMoveEffect at 0xbda610
    // This handles animated camera movement with velocity and acceleration

    if (!m_cameraMoveInfo.bOn)
    {
        return;
    }

    auto& gr = get_gr();
    auto tCur = gr.GetCurrentTime();

    // Check if camera movement has ended
    if (tCur > m_cameraMoveInfo.tEnd)
    {
        // Movement complete - turn off
        ClearCameraMove();
        return;
    }

    // Calculate elapsed time since start
    auto tElapsed = tCur - m_cameraMoveInfo.tStart;
    if (tElapsed <= 0)
    {
        return;  // Not started yet
    }

    // Calculate movement based on velocity and acceleration
    // Time in 100ms units for calculation (matching original)
    double t = tElapsed / 100.0;

    // Displacement = v0 * t + 0.5 * a * t^2
    double dx = m_cameraMoveInfo.ptVelocityFirst.x * t +
                0.5 * m_cameraMoveInfo.ptAcceleration.x * t * t;
    double dy = m_cameraMoveInfo.ptVelocityFirst.y * t +
                0.5 * m_cameraMoveInfo.ptAcceleration.y * t * t;

    // Apply relative movement to camera
    auto currentPos = gr.GetCameraPosition();
    gr.SetCameraPosition(currentPos.x + static_cast<std::int32_t>(dx),
                         currentPos.y + static_cast<std::int32_t>(dy));

    // Apply velocity adjust rate (damping/acceleration factor)
    if (m_cameraMoveInfo.ptVelocityAdjustRate.x != 0)
    {
        m_cameraMoveInfo.ptVelocityFirst.x =
            static_cast<std::int32_t>(m_cameraMoveInfo.ptVelocityAdjustRate.x *
                                       m_cameraMoveInfo.ptVelocityFirst.x / 100.0);
    }
    if (m_cameraMoveInfo.ptVelocityAdjustRate.y != 0)
    {
        m_cameraMoveInfo.ptVelocityFirst.y =
            static_cast<std::int32_t>(m_cameraMoveInfo.ptVelocityAdjustRate.y *
                                       m_cameraMoveInfo.ptVelocityFirst.y / 100.0);
    }

    // Apply acceleration adjust rate
    if (m_cameraMoveInfo.ptAccelerationAdjustRate.x != 0)
    {
        m_cameraMoveInfo.ptAcceleration.x =
            static_cast<std::int32_t>(m_cameraMoveInfo.ptAccelerationAdjustRate.x *
                                       m_cameraMoveInfo.ptAcceleration.x / 100.0);
    }
    if (m_cameraMoveInfo.ptAccelerationAdjustRate.y != 0)
    {
        m_cameraMoveInfo.ptAcceleration.y =
            static_cast<std::int32_t>(m_cameraMoveInfo.ptAccelerationAdjustRate.y *
                                       m_cameraMoveInfo.ptAcceleration.y / 100.0);
    }

    // Clip camera to view range if enabled
    if (m_cameraMoveInfo.bClipInViewRange)
    {
        ClipCameraToViewRange();
    }
}

void MapLoadable::ClipCameraToViewRange()
{
    // Clip camera position to stay within the view range bounds
    auto& gr = get_gr();
    auto pos = gr.GetCameraPosition();
    auto screenWidth = static_cast<std::int32_t>(gr.GetWidth());
    auto screenHeight = static_cast<std::int32_t>(gr.GetHeight());

    // Calculate the center position that would keep the view within bounds
    auto viewLeft = pos.x;
    auto viewTop = pos.y;
    auto viewRight = pos.x + screenWidth;
    auto viewBottom = pos.y + screenHeight;

    // Clamp to view range
    if (viewLeft < m_viewRangeRect.left)
    {
        pos.x = m_viewRangeRect.left;
    }
    if (viewRight > m_viewRangeRect.right)
    {
        pos.x = m_viewRangeRect.right - screenWidth;
    }
    if (viewTop < m_viewRangeRect.top)
    {
        pos.y = m_viewRangeRect.top;
    }
    if (viewBottom > m_viewRangeRect.bottom)
    {
        pos.y = m_viewRangeRect.bottom - screenHeight;
    }

    gr.SetCameraPosition(pos);
}

void MapLoadable::Draw()
{
    // Layers are rendered by WzGr2D::RenderFrame
    // This method can be used for additional custom rendering
}

void MapLoadable::Close()
{
    // Based on CMapLoadable::Close at 0xbdc910

    // Fade out with force fade out time
    auto& gr = get_gr();

    if (m_tForceFadeOutTime > 0)
    {
        // FadeOut would trigger fade animation, for now just log
        LOG_DEBUG("MapLoadable::Close - FadeOut with time {}", m_tForceFadeOutTime);
    }

    // Reset screen scale to 1000 (100%)
    // gr.SetScreenScale(1000);  // TODO: implement screen scale

    // Release field properties
    m_pPropField.reset();
    m_pPropFieldRefBack.reset();

    // Clear all layers (includes background, general, object, transient, letterbox)
    ClearAllLayers();

    // Clear tagged layer maps
    m_mTaggedLayer.clear();

    // Call base class close
    Stage::Close();

    LOG_INFO("MapLoadable closed");
}

auto MapLoadable::CreateLayer(std::int32_t z) -> std::shared_ptr<WzGr2DLayer>
{
    auto& gr = get_gr();
    auto layer = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), z);

    if (layer)
    {
        m_lpLayerGen.push_back(layer);
#ifdef MS_DEBUG_CANVAS
        DebugOverlay::GetInstance().RegisterLayer(layer, "gen_" + std::to_string(m_lpLayerGen.size()));
#endif
    }

    return layer;
}

auto MapLoadable::CreateObjectLayer(const std::string& name, std::int32_t z)
    -> std::shared_ptr<WzGr2DLayer>
{
    auto& gr = get_gr();
    auto layer = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), z);

    if (layer)
    {
        m_lpLayerObj.push_back(layer);
        m_mpLayerObj[name] = layer;
#ifdef MS_DEBUG_CANVAS
        DebugOverlay::GetInstance().RegisterLayer(layer, name);
#endif
    }

    return layer;
}

auto MapLoadable::GetObjectLayer(const std::string& name) -> std::shared_ptr<WzGr2DLayer>
{
    auto it = m_mpLayerObj.find(name);
    if (it != m_mpLayerObj.end())
    {
        return it->second;
    }
    return nullptr;
}

void MapLoadable::SetObjectAnimation(const std::string& name, Gr2DAnimationType type)
{
    auto layer = GetObjectLayer(name);
    if (layer)
    {
        if (type == Gr2DAnimationType::None || type == Gr2DAnimationType::Loop)
        {
            // Reset to first frame before starting animation
            layer->SetCurrentFrame(0);
        }
        layer->Animate(type);
    }
}

void MapLoadable::SetTaggedObjectAnimation(const std::string& tag, Gr2DAnimationType type)
{
    auto it = m_mTaggedLayer.find(tag);
    if (it != m_mTaggedLayer.end() && it->second)
    {
        if (type == Gr2DAnimationType::None || type == Gr2DAnimationType::Loop)
        {
            it->second->SetCurrentFrame(0);
        }
        it->second->Animate(type);
    }
}

void MapLoadable::ChangeBGM(const std::string& bgmPath)
{
    // Based on CMapLoadable::PlayBGMFromMapInfo (0xbeaef8)
    // The bgmPath should be the full path like "Sound/Bgm0.img/NightMarket"

    if (IsSameChangeBGM(bgmPath))
    {
        return;
    }

    m_sChangedBgmUOL = bgmPath;

    if (bgmPath.empty())
    {
        // Stop BGM if path is empty
        SoundSystem::GetInstance().StopBGM(0);
        LOG_DEBUG("ChangeBGM: stopped");
        return;
    }

    // Play BGM with looping (nLoop=1), volume 0x258 (600 -> scaled to 128)
    // Based on CSoundMan::PlayBGM call at 0xbeafc2
    SoundSystem::GetInstance().PlayBGM(bgmPath, 1, 128, 128, 0, 0);
    LOG_DEBUG("ChangeBGM: {}", bgmPath);
}

auto MapLoadable::IsSameChangeBGM(const std::string& bgmPath) const -> bool
{
    return m_sChangedBgmUOL == bgmPath;
}

void MapLoadable::PrepareNextBGM()
{
    // TODO: Implement BGM preparation for smooth transitions
}

void MapLoadable::RestoreMutedBGM()
{
    if (m_tRestoreBgmVolume != 0)
    {
        // Restore BGM volume via SoundSystem
        SoundSystem::GetInstance().SetBGMVolume(
            static_cast<std::uint32_t>(m_nRestoreBgmVolume), 0);
        m_tRestoreBgmVolume = 0;
    }
}

void MapLoadable::PlayBGMFromMapInfo()
{
    // Based on CMapLoadable::PlayBGMFromMapInfo (0xbeaef8)
    // Get "bgm" property from m_pPropFieldInfo (StringPool 0xA13 = 2579)

    if (!m_pPropFieldInfo)
    {
        LOG_DEBUG("PlayBGMFromMapInfo: No field info property");
        return;
    }

    // First check if there's a custom BGM UOL
    if (!m_sFieldCustomBgmUOL.empty())
    {
        // Use custom BGM path: "Sound/" + customBgmUOL
        std::string fullPath = "Sound/" + m_sFieldCustomBgmUOL;
        ChangeBGM(fullPath);
        return;
    }

    // Get "bgm" property from map info
    auto bgmProp = m_pPropFieldInfo->GetChild("bgm");
    if (!bgmProp)
    {
        LOG_DEBUG("PlayBGMFromMapInfo: No 'bgm' property in field info");
        return;
    }

    // Get the BGM path string
    std::string bgmValue = bgmProp->GetString("");
    if (bgmValue.empty())
    {
        LOG_DEBUG("PlayBGMFromMapInfo: Empty bgm value");
        return;
    }

    // Build full path: "Sound/" + bgmValue (StringPool 2580 = "Sound/")
    std::string fullPath = "Sound/" + bgmValue;
    ChangeBGM(fullPath);
}

void MapLoadable::SetCameraMoveInfo(std::int32_t tStart, const Point2D& velocity,
                                     const Point2D& acceleration,
                                     const Point2D& velocityAdjust,
                                     const Point2D& accelAdjust,
                                     std::int32_t duration, bool clipInViewRange)
{
    m_cameraMoveInfo.bOn = true;
    m_cameraMoveInfo.tStart = tStart;
    m_cameraMoveInfo.tEnd = tStart + duration;
    m_cameraMoveInfo.ptVelocityFirst = velocity;
    m_cameraMoveInfo.ptAcceleration = acceleration;
    m_cameraMoveInfo.ptVelocityAdjustRate = velocityAdjust;
    m_cameraMoveInfo.ptAccelerationAdjustRate = accelAdjust;
    m_cameraMoveInfo.bClipInViewRange = clipInViewRange;
}

void MapLoadable::ClearCameraMove()
{
    m_cameraMoveInfo.bOn = false;
}

auto MapLoadable::GetViewRangeRect() const -> Rect
{
    return m_viewRangeRect;
}

void MapLoadable::LoadObjects(const std::shared_ptr<WzProperty>& prop, std::int32_t baseZ)
{
    if (!prop)
    {
        return;
    }

    // TODO: Implement object loading from WZ property
    // This would iterate through the property's children and create
    // layers for each object with appropriate canvases
}

auto MapLoadable::LoadAnimatedLayer(const std::shared_ptr<WzGr2DLayer>& layer,
                                     const std::shared_ptr<WzProperty>& prop) -> std::size_t
{
    if (!layer || !prop)
    {
        return 0;
    }

    std::size_t frameCount = 0;

    // Iterate through numbered children (0, 1, 2, ...)
    for (std::size_t i = 0;; ++i)
    {
        auto frameProp = prop->GetChild(std::to_string(i));
        if (!frameProp)
        {
            break; // No more frames
        }

        // Get canvas from frame property
        auto canvas = frameProp->GetCanvas();
        if (!canvas)
        {
            // Frame might be the canvas itself
            canvas = prop->GetChild(std::to_string(i))->GetCanvas();
            if (!canvas)
            {
                continue; // Skip frames without canvas
            }
        }

        // Get frame delay (default 100ms)
        std::int32_t delay = 100;
        auto delayProp = frameProp->GetChild("delay");
        if (delayProp)
        {
            delay = delayProp->GetInt(100);
        }

        // Get alpha values (a0 and a1 for interpolation)
        std::uint8_t alpha0 = 255;
        std::uint8_t alpha1 = 255;
        auto a0Prop = frameProp->GetChild("a0");
        auto a1Prop = frameProp->GetChild("a1");
        if (a0Prop)
        {
            alpha0 = static_cast<std::uint8_t>(a0Prop->GetInt(255));
        }
        if (a1Prop)
        {
            alpha1 = static_cast<std::uint8_t>(a1Prop->GetInt(255));
        }

        // Get origin from canvas or property
        auto originProp = frameProp->GetChild("origin");
        if (originProp)
        {
            auto vec = originProp->GetVector();
            canvas->SetOrigin({vec.x, vec.y});
        }

        // Insert canvas into layer
        layer->InsertCanvas(canvas, delay, alpha0, alpha1);
        ++frameCount;
    }

    if (frameCount > 0)
    {
        LOG_DEBUG("LoadAnimatedLayer: loaded {} frames", frameCount);
    }

    return frameCount;
}

auto MapLoadable::LoadStaticLayer(const std::shared_ptr<WzGr2DLayer>& layer,
                                   const std::shared_ptr<WzProperty>& prop) -> bool
{
    if (!layer || !prop)
    {
        LOG_DEBUG("LoadStaticLayer: layer or prop is null");
        return false;
    }

    LOG_DEBUG("LoadStaticLayer: Loading from property '{}'", prop->GetName());

    // Try to get canvas directly
    auto canvas = prop->GetCanvas();
    if (!canvas)
    {
        LOG_DEBUG("LoadStaticLayer: No direct canvas, trying child '0'");
        // Try to get canvas from first child (some properties wrap canvas in "0" child)
        auto firstChild = prop->GetChild("0");
        if (firstChild)
        {
            canvas = firstChild->GetCanvas();
            if (canvas)
            {
                LOG_DEBUG("LoadStaticLayer: Found canvas in child '0'");
            }
        }
    }
    else
    {
        LOG_DEBUG("LoadStaticLayer: Got direct canvas");
    }

    if (!canvas)
    {
        LOG_WARN("LoadStaticLayer: No canvas found in property '{}'", prop->GetName());
        return false;
    }

    // Get origin if available
    auto originProp = prop->GetChild("origin");
    if (originProp)
    {
        auto vec = originProp->GetVector();
        canvas->SetOrigin({vec.x, vec.y});
        LOG_DEBUG("LoadStaticLayer: origin=({}, {})", vec.x, vec.y);
    }
    else
    {
        LOG_DEBUG("LoadStaticLayer: no origin property, using (0, 0)");
    }

    // Insert canvas into layer (static, no animation)
    layer->InsertCanvas(canvas, 0, 255, 255);

    LOG_DEBUG("LoadStaticLayer: loaded canvas {}x{}, origin=({}, {})",
              canvas->GetWidth(), canvas->GetHeight(),
              canvas->GetOrigin().x, canvas->GetOrigin().y);

    return true;
}

void MapLoadable::UpdateObjectLayers()
{
    // Object layers are updated by WzGr2D during RenderFrame
    // This method can be used for custom update logic
}

void MapLoadable::RestoreViewRange()
{
    // Based on CMapLoadable::RestoreViewRange at 0xbe6ff0

    auto& gr = get_gr();

    // Get current screen scale (default 1000 = 100%)
    std::int32_t nScale = 1000;
    // TODO: gr.GetScreenScale(&nScale);

    // Calculate half screen dimensions
    auto screenWidth = static_cast<std::int32_t>(gr.GetWidth());
    auto screenHeight = static_cast<std::int32_t>(gr.GetHeight());
    auto nHalfWidth = screenWidth / 2;
    auto nHalfHeight = screenHeight / 2;

    // Scale by current zoom level
    float fScale = nScale * 0.001f;
    auto nScaledHalfWidth = static_cast<std::int32_t>(nHalfWidth / fScale);
    auto nScaledHalfHeight = static_cast<std::int32_t>(nHalfHeight / fScale);

    // Default view range based on screen size if no field info
    if (!m_pPropFieldInfo)
    {
        // Use screen-based defaults
        m_viewRangeRect.left = -nScaledHalfWidth;
        m_viewRangeRect.top = -nScaledHalfHeight;
        m_viewRangeRect.right = nScaledHalfWidth;
        m_viewRangeRect.bottom = nScaledHalfHeight;
        m_nMinZoomOutScale = 1000;
        LOG_DEBUG("RestoreViewRange: Using default view range (no field info)");
        return;
    }

    // Read view range from field info properties
    // Default values based on physical space MBR + margins
    constexpr std::int32_t LEFT_MARGIN = 20;
    constexpr std::int32_t TOP_MARGIN = 60;
    constexpr std::int32_t RIGHT_MARGIN = 20;
    constexpr std::int32_t BOTTOM_MARGIN = 190;

    // Default MBR (minimum bounding rectangle) - typically from physical space
    constexpr std::int32_t DEFAULT_LEFT = -500 - LEFT_MARGIN;
    constexpr std::int32_t DEFAULT_TOP = -500 - TOP_MARGIN;
    constexpr std::int32_t DEFAULT_RIGHT = 500 + RIGHT_MARGIN;
    constexpr std::int32_t DEFAULT_BOTTOM = 500 + BOTTOM_MARGIN;

    auto vrLeftProp = m_pPropFieldInfo->GetChild("VRLeft");
    auto vrTopProp = m_pPropFieldInfo->GetChild("VRTop");
    auto vrRightProp = m_pPropFieldInfo->GetChild("VRRight");
    auto vrBottomProp = m_pPropFieldInfo->GetChild("VRBottom");

    m_viewRangeRect.left = vrLeftProp ? vrLeftProp->GetInt(DEFAULT_LEFT) : DEFAULT_LEFT;
    m_viewRangeRect.top = vrTopProp ? vrTopProp->GetInt(DEFAULT_TOP) : DEFAULT_TOP;
    m_viewRangeRect.right = vrRightProp ? vrRightProp->GetInt(DEFAULT_RIGHT) : DEFAULT_RIGHT;
    m_viewRangeRect.bottom = vrBottomProp ? vrBottomProp->GetInt(DEFAULT_BOTTOM) : DEFAULT_BOTTOM;

    // Calculate min zoom out scale
    auto nMarginX = (m_viewRangeRect.left + m_viewRangeRect.right) / 2 - m_viewRangeRect.left;
    auto nMarginY = (m_viewRangeRect.top + m_viewRangeRect.bottom) / 2 - m_viewRangeRect.top;

    if (nMarginX > 0 && nMarginY > 0)
    {
        float scaleX = static_cast<float>(nHalfWidth) * 1000.0f / nMarginX;
        float scaleY = static_cast<float>(nHalfHeight) * 1000.0f / nMarginY;

        m_nMinZoomOutScale = static_cast<std::int32_t>(std::max(scaleX, scaleY));

        if (m_nMinZoomOutScale < 1)
        {
            m_nMinZoomOutScale = 1;
        }
        else if (m_nMinZoomOutScale > 1000)
        {
            m_nMinZoomOutScale = 1000;
        }
    }
    else
    {
        m_nMinZoomOutScale = 1000;
    }

    // Adjust view range by scaled half dimensions
    m_viewRangeRect.left += nScaledHalfWidth;
    m_viewRangeRect.right -= nScaledHalfWidth;
    m_viewRangeRect.top += nScaledHalfHeight;
    m_viewRangeRect.bottom -= nScaledHalfHeight;

    // Clamp if dimensions become negative (map smaller than screen)
    if (m_viewRangeRect.right - m_viewRangeRect.left <= 0)
    {
        auto mid = (m_viewRangeRect.right + m_viewRangeRect.left) / 2;
        m_viewRangeRect.left = mid;
        m_viewRangeRect.right = mid;
    }
    if (m_viewRangeRect.bottom - m_viewRangeRect.top <= 0)
    {
        auto mid = (m_viewRangeRect.bottom + m_viewRangeRect.top) / 2;
        m_viewRangeRect.top = mid;
        m_viewRangeRect.bottom = mid;
    }

    LOG_DEBUG("RestoreViewRange: ({},{}) - ({},{}), minScale={}",
              m_viewRangeRect.left, m_viewRangeRect.top,
              m_viewRangeRect.right, m_viewRangeRect.bottom,
              m_nMinZoomOutScale);
}

void MapLoadable::RestoreBack(bool bLoad)
{
    // Based on CMapLoadable::RestoreBack at 0xbfbcb0
    // Get the "back" property from m_pPropFieldRefBack or m_pPropField

    std::shared_ptr<WzProperty> pPropField;
    if (m_pPropFieldRefBack)
    {
        pPropField = m_pPropFieldRefBack;
    }
    else
    {
        pPropField = m_pPropField;
    }

    if (!pPropField)
    {
        LOG_WARN("RestoreBack: No field property set");
        return;
    }

    LOG_INFO("RestoreBack: Using field property '{}'", pPropField->GetName());

    // Get the "back" property (string pool 0x9F5 = 2549)
    auto pBack = pPropField->GetChild("back");
    if (!pBack)
    {
        LOG_WARN("RestoreBack: No 'back' property found in '{}'", pPropField->GetName());
        // Debug: List available children
        LOG_DEBUG("RestoreBack: Available children in field property:");
        for (const auto& [name, child] : pPropField->GetChildren())
        {
            LOG_DEBUG("  - {}", name);
        }
        return;
    }

    // Get number of background pieces
    auto count = pBack->GetChildCount();
    LOG_INFO("RestoreBack: Found {} background pieces in 'back'", count);

    if (count == 0)
    {
        LOG_WARN("RestoreBack: 'back' property has no children");
        return;
    }

    // Iterate through numbered children (0, 1, 2, ...)
    for (std::size_t i = 0; i < count; ++i)
    {
        auto pPiece = pBack->GetChild(std::to_string(i));
        if (!pPiece)
        {
            continue;
        }

        // Check screenMode filter (string pool 0x9F0 = 2544)
        auto screenModeProp = pPiece->GetChild("screenMode");
        if (screenModeProp)
        {
            auto screenMode = screenModeProp->GetInt(0);
            if (screenMode != 0)
            {
                auto& gr = get_gr();
                auto screenW = gr.GetWidth();
                auto screenH = gr.GetHeight();

                // Filter by resolution
                bool skip = false;
                if ((screenMode & 1) == 0 && screenW == 800 && screenH == 600)
                {
                    skip = true;
                }
                if ((screenMode & 2) == 0 && screenW == 1024 && screenH == 768)
                {
                    skip = true;
                }
                if ((screenMode & 4) == 0 && screenW == 1366 && screenH == 768)
                {
                    skip = true;
                }

                if (skip)
                {
                    LOG_DEBUG("RestoreBack: Skipping piece {} due to screenMode filter", i);
                    continue;
                }
            }
        }

        // Create the background layer
        MakeBack(static_cast<std::int32_t>(i), pPiece, bLoad);
    }

    // Update tag layer references
    UpdateBackTagLayer();
}

void MapLoadable::MakeBack(std::int32_t nPageIdx, const std::shared_ptr<WzProperty>& pPiece, bool bLoad)
{
    // Based on CMapLoadable::MakeBack at 0xbf2af0
    // This creates a background layer from a back piece property

    if (!pPiece)
    {
        return;
    }

    // Read background properties
    // bS (2550) - background set name (e.g., "login")
    auto bsProp = pPiece->GetChild("bS");
    std::string bS = bsProp ? bsProp->GetString("") : "";

    // no - frame/sprite number
    auto noProp = pPiece->GetChild("no");
    auto no = noProp ? noProp->GetInt(0) : 0;

    // ani (2558) - is animated
    auto aniProp = pPiece->GetChild("ani");
    auto ani = aniProp ? (aniProp->GetInt(0) != 0) : false;

    // Position properties
    auto xProp = pPiece->GetChild("x");
    auto yProp = pPiece->GetChild("y");
    auto x = xProp ? xProp->GetInt(0) : 0;
    auto y = yProp ? yProp->GetInt(0) : 0;

    // Parallax/scroll properties
    auto rxProp = pPiece->GetChild("rx");
    auto ryProp = pPiece->GetChild("ry");
    auto rx = rxProp ? rxProp->GetInt(0) : 0;
    auto ry = ryProp ? ryProp->GetInt(0) : 0;

    auto cxProp = pPiece->GetChild("cx");
    auto cyProp = pPiece->GetChild("cy");
    auto cx = cxProp ? cxProp->GetInt(0) : 0;
    auto cy = cyProp ? cyProp->GetInt(0) : 0;

    // Alpha (2559)
    auto aProp = pPiece->GetChild("a");
    auto alpha = aProp ? static_cast<std::uint8_t>(aProp->GetInt(255)) : static_cast<std::uint8_t>(255);

    // front (2561) - is front layer (rendered above objects)
    auto frontProp = pPiece->GetChild("front");
    auto front = frontProp ? (frontProp->GetInt(0) != 0) : false;

    // Flip
    auto fProp = pPiece->GetChild("f");
    auto flip = fProp ? (fProp->GetInt(0) != 0) : false;

    // type (StringPool 0x2B55) - Background type, controls positioning/tiling behavior:
    // 0: Normal - position relative to map coordinates
    // 1: Horizontal tiling only
    // 2: Vertical tiling only
    // 3: Both H+V tiling
    // 4: Horizontal stretch to screen width
    // 5: Vertical stretch to screen height
    // 6: Stretch to fill entire screen
    auto typeProp = pPiece->GetChild("type");
    auto type = typeProp ? typeProp->GetInt(0) : 0;

    // Movement type (2566)
    auto moveTypeProp = pPiece->GetChild("moveType");
    // auto moveType = moveTypeProp ? moveTypeProp->GetInt(0) : 0;

    // View culling: Only load layers within a reasonable Y range from the initial view
    // The login screen has 5+ steps at 600 pixel intervals (up to ~3000 pixels)
    // Load all login backgrounds by using a large range
    // Race-specific character creation can add more steps beyond step 4
    constexpr std::int32_t VIEW_LOAD_DISTANCE = 6000;  // Load layers within +/- 6000 pixels of Y=0

    if (y < -VIEW_LOAD_DISTANCE || y > VIEW_LOAD_DISTANCE)
    {
        // Skip layers that are too far from the initial view
        // They can be loaded later if the user scrolls to those steps
        LOG_DEBUG("MakeBack[{}]: Culled (y={} outside +{})", nPageIdx, y, VIEW_LOAD_DISTANCE);
        return;
    }

    LOG_DEBUG("MakeBack[{}]: bS={}, no={}, ani={}, pos=({},{}), rx={}, ry={}, front={}, alpha={}, type={}",
              nPageIdx, bS, no, ani, x, y, rx, ry, front, static_cast<int>(alpha), type);

    if (bS.empty())
    {
        LOG_WARN("MakeBack[{}]: Empty bS, skipping", nPageIdx);
        return;
    }

    if (!bLoad)
    {
        return;
    }

    // Construct the path to the background image
    // Path: Map/Back/{bS}.img/{ani ? "ani" : "back"}/{no}
    // Note: Background images can be in Map.wz, Map1.wz, Map2.wz, etc.
    std::string subPath = bS + ".img/" + (ani ? "ani" : "back") + "/" + std::to_string(no);

    auto& resMan = WzResMan::GetInstance();

    // Try different Map WZ files in order
    std::shared_ptr<WzProperty> spriteProp;
    const char* mapPrefixes[] = {"Map1/Back/", "Map2/Back/", "Map/Back/"};

    for (const auto* prefix : mapPrefixes)
    {
        std::string path = std::string(prefix) + subPath;
        spriteProp = resMan.GetProperty(path);
        // Check if property exists and is valid (has children OR is a canvas/value node)
        if (spriteProp && (spriteProp->HasChildren() || spriteProp->GetCanvas() != nullptr))
        {
            LOG_DEBUG("MakeBack[{}]: Found sprite at {}", nPageIdx, path);
            break;
        }
        spriteProp.reset();
    }

    if (!spriteProp)
    {
        LOG_ERROR("MakeBack[{}]: Failed to load {} (tried Map1, Map2, Map)", nPageIdx, subPath);
        return;
    }

    // Determine z-order based on front flag and page index
    // Front layers are rendered above objects (high z), back layers are behind (low z)
    std::int32_t z = front ? (1000 + nPageIdx) : nPageIdx;

    // Create the layer
    auto& gr = get_gr();
    auto layer = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), z);

    if (!layer)
    {
        LOG_ERROR("MakeBack[{}]: Failed to create layer", nPageIdx);
        return;
    }

    // Load frames into the layer
    std::size_t frameCount = 0;

    if (ani)
    {
        // Animated background - load multiple frames
        frameCount = LoadAnimatedLayer(layer, spriteProp);
        if (frameCount > 1)
        {
            layer->Animate(Gr2DAnimationType::Loop);
        }
    }
    else
    {
        // Static background - load single frame
        if (LoadStaticLayer(layer, spriteProp))
        {
            frameCount = 1;
        }
    }

    if (frameCount == 0)
    {
        LOG_WARN("MakeBack[{}]: No frames loaded from {}", nPageIdx, subPath);
        gr.RemoveLayer(layer);
        return;
    }

    // Set position
    layer->SetPosition(x, y);

    // Set alpha
    std::uint32_t color = (static_cast<std::uint32_t>(alpha) << 24) | 0x00FFFFFF;
    layer->SetColor(color);

    // Set flip if needed
    if (flip)
    {
        layer->SetFlip(true);
    }

    // Handle background type-specific tiling and movement (based on IDA decompilation)
    // Type meanings:
    // 0: NORMAL - no tiling, parallax with rx/ry
    // 1: HTILED - horizontal tiling, parallax with rx/ry
    // 2: VTILED - vertical tiling, parallax with rx/ry
    // 3: TILED - both H+V tiling, parallax with rx/ry
    // 4: HMOVEA - animated H movement (rx), then H-tiling
    // 5: VMOVEA - animated V movement (ry), then V-tiling
    // 6: HMOVEB - animated H movement (rx), then both tiling
    // 7: VMOVEB - animated V movement (ry), then both tiling

    // Check if this is an animated movement type
    bool isAnimatedType = (type >= 4 && type <= 7);
    std::int32_t moveRel = 0;      // Movement parameter (rx or ry)
    std::int32_t moveOffsetX = 0;  // Animated X offset
    std::int32_t moveOffsetY = 0;  // Animated Y offset

    // Determine tiling and movement based on type
    auto effectiveType = type;
    bool hTile = false;
    bool vTile = false;

    switch (type)
    {
    case 0: // NORMAL - no tiling
        break;
    case 1: // HTILED - horizontal tiling only
        hTile = true;
        break;
    case 2: // VTILED - vertical tiling only
        vTile = true;
        break;
    case 3: // TILED - both H+V tiling
        hTile = true;
        vTile = true;
        break;
    case 4: // HMOVEA - animated H movement (rx), then H-tiling
        moveRel = rx;
        moveOffsetX = rx > 0 ? -rx : -rx;  // Movement direction based on sign
        effectiveType = 1;
        hTile = true;
        break;
    case 5: // VMOVEA - animated V movement (ry), then V-tiling
        moveRel = ry;
        moveOffsetY = ry > 0 ? -ry : -ry;  // Movement direction based on sign
        effectiveType = 2;
        vTile = true;
        break;
    case 6: // HMOVEB - animated H movement (rx), then both tiling
        moveRel = rx;
        moveOffsetX = rx > 0 ? -rx : -rx;
        effectiveType = 3;
        hTile = true;
        vTile = true;
        break;
    case 7: // VMOVEB - animated V movement (ry), then both tiling
        moveRel = ry;
        moveOffsetY = ry > 0 ? -ry : -ry;
        effectiveType = 3;
        hTile = true;
        vTile = true;
        break;
    default:
        LOG_WARN("MakeBack[{}]: Unknown type {}", nPageIdx, type);
        break;
    }

    // Apply tiling
    if (hTile || vTile)
    {
        // Get canvas dimensions for default tiling size
        // When cx/cy is 0, use the canvas width/height as the tile size
        auto canvas = layer->GetCurrentCanvas();
        auto canvasWidth = canvas ? static_cast<std::int32_t>(canvas->GetWidth()) : 0;
        auto canvasHeight = canvas ? static_cast<std::int32_t>(canvas->GetHeight()) : 0;

        auto tileCx = hTile ? (cx > 0 ? cx : canvasWidth) : 0;
        auto tileCy = vTile ? (cy > 0 ? cy : canvasHeight) : 0;
        layer->SetTiling(tileCx, tileCy);
        LOG_DEBUG("MakeBack[{}]: Type {} (effective {}) tiling cx={}, cy={} (canvas {}x{})",
                  nPageIdx, type, effectiveType, tileCx, tileCy, canvasWidth, canvasHeight);
    }

    // For animated types (4-7), set up position animation
    if (isAnimatedType && moveRel != 0)
    {
        // Calculate animation duration based on movement speed
        // Original: tOffset += 20000 / abs(nRel)
        auto absRel = moveRel > 0 ? moveRel : -moveRel;
        if (absRel == 0) absRel = 1;
        auto duration = 20000 / absRel;

        layer->StartPositionAnimation(moveOffsetX, moveOffsetY, duration, true);
        LOG_DEBUG("MakeBack[{}]: Type {} animated movement offset=({}, {}), duration={}ms",
                  nPageIdx, type, moveOffsetX, moveOffsetY, duration);

        // For types 4-7, also set parallax on the animated axis
        // Original: Ratio(center, 100, 100, rx+100, 0) for type 4,6
        //           Ratio(center, 100, 100, 0, ry+100) for type 5,7
        std::int32_t parallaxRx = 0;
        std::int32_t parallaxRy = 0;

        if (type == 4 || type == 6)
        {
            // Horizontal movement types - set horizontal parallax
            parallaxRx = rx + 100;
        }
        else if (type == 5 || type == 7)
        {
            // Vertical movement types - set vertical parallax
            parallaxRy = ry + 100;
        }

        layer->SetParallax(parallaxRx, parallaxRy);
        LOG_DEBUG("MakeBack[{}]: Type {} animated parallax rx={}, ry={}", nPageIdx, type, parallaxRx, parallaxRy);
    }
    else if (rx != 0 || ry != 0)
    {
        // Apply parallax for types 0-3
        // Original: Ratio(center, 100, 100, rx, ry)
        layer->SetParallax(rx, ry);
        LOG_DEBUG("MakeBack[{}]: Type {} parallax rx={}, ry={}", nPageIdx, type, rx, ry);
    }

    // Add to background layer list
    m_lpLayerBack.push_back(layer);
#ifdef MS_DEBUG_CANVAS
    DebugOverlay::GetInstance().RegisterLayer(layer, "back_" + std::to_string(nPageIdx));
#endif

    LOG_INFO("MakeBack[{}]: Created layer with {} frames at z={}", nPageIdx, frameCount, z);
}

void MapLoadable::UpdateBackTagLayer()
{
    // Update tag layer references after loading backgrounds
    // This is used for named background lookup
}

void MapLoadable::ClearBackLayers()
{
    auto& gr = get_gr();

    for (auto& layer : m_lpLayerBack)
    {
        if (layer)
        {
            gr.RemoveLayer(layer);
        }
    }
    m_lpLayerBack.clear();
}

void MapLoadable::ClearAllLayers()
{
#ifdef MS_DEBUG_CANVAS
    DebugOverlay::GetInstance().ClearLayers();
#endif

    auto& gr = get_gr();

    // Remove all background layers
    ClearBackLayers();

    // Remove all general layers
    for (auto& layer : m_lpLayerGen)
    {
        if (layer)
        {
            gr.RemoveLayer(layer);
        }
    }
    m_lpLayerGen.clear();

    // Remove all object layers
    for (auto& layer : m_lpLayerObj)
    {
        if (layer)
        {
            gr.RemoveLayer(layer);
        }
    }
    m_lpLayerObj.clear();

    // Remove all transient layers
    for (auto& layer : m_lpLayerTransient)
    {
        if (layer)
        {
            gr.RemoveLayer(layer);
        }
    }
    m_lpLayerTransient.clear();

    // Remove letterbox layers
    for (auto& layer : m_lpLayerLetterBox)
    {
        if (layer)
        {
            gr.RemoveLayer(layer);
        }
    }
    m_lpLayerLetterBox.clear();

    // Clear maps
    m_mpLayerObj.clear();
    m_mTaggedLayer.clear();
}

// ========== LoadMap (0xbfff50) ==========

void MapLoadable::LoadMap()
{
    // Based on CMapLoadable::LoadMap at 0xbfff50
    // This orchestrates loading of all map elements from WZ properties

    if (!m_pPropField)
    {
        LOG_WARN("LoadMap: m_pPropField is null");
        return;
    }

    LOG_DEBUG("LoadMap: Starting map load");

    // Get "foothold" property (StringPool 0x9D0)
    auto pPropFoothold = m_pPropField->GetChild("foothold");

    // Get "ladderRope" property (StringPool 0x9D1)
    auto pLadderRope = m_pPropField->GetChild("ladderRope");

    // TODO: Load physical space with foothold and ladder info
    // m_pSpace2D->Load(pPropFoothold, pLadderRope, m_pPropFieldInfo);

    // Read map info properties
    if (m_pPropFieldInfo)
    {
        // Quarter view flag (StringPool 0x2B35)
        auto quarterViewProp = m_pPropFieldInfo->GetChild("quarterView");
        m_bQuarterView = quarterViewProp ? (quarterViewProp->GetInt(0) != 0) : false;

        // Color flow name
        auto colorFlowProp = m_pPropFieldInfo->GetChild("colorFlow");
        m_sColorFlowName = colorFlowProp ? colorFlowProp->GetString("") : "";

        // Enter scale (overrides regular scale if set)
        auto enterScaleProp = m_pPropFieldInfo->GetChild("enterScale");
        std::int32_t enterScale = enterScaleProp ? enterScaleProp->GetInt(1000) : 1000;

        if (enterScale != 1000)
        {
            m_nScaleField = enterScale;
            // TODO: Implement screen scale
            // auto& gr = get_gr();
            // gr.SetScreenScale(enterScale);
        }
        else
        {
            // Use regular scale property
            auto scaleProp = m_pPropFieldInfo->GetChild("scale");
            m_nScaleField = scaleProp ? scaleProp->GetInt(1000) : 1000;
        }

        // Zoom out map flag
        auto zoomOutProp = m_pPropFieldInfo->GetChild("zoomOutField");
        m_bNeedZoomOutMap = zoomOutProp ? (zoomOutProp->GetInt(0) != 0) : false;

        LOG_DEBUG("LoadMap: quarterView={}, colorFlow={}, scale={}, needZoomOut={}",
                  m_bQuarterView, m_sColorFlowName, m_nScaleField, m_bNeedZoomOutMap);
    }

    // Restore map elements in order
    RestoreTile();
    RestoreViewRange();
    RestoreObj(true);
    RestoreBack(true);
    RestoreWeather();
    RestoreLetterBox();

    // Load life (NPCs and mobs)
    LoadLife();

    // Load reactors
    LoadReactors();

    // Load rect event data
    LoadRectEventData();

    // Set default footstep sound
    SetFootStepSound("");

    LOG_DEBUG("LoadMap: Map load complete");
}

void MapLoadable::RestoreTile()
{
    // Based on CMapLoadable::RestoreTile at 0xbea1d0
    // Iterates through 8 pages and creates tile layers from tile properties

    if (!m_pPropField)
    {
        return;
    }

    auto& resMan = WzResMan::GetInstance();

    // Iterate through 8 pages
    for (int nPageIdx = 0; nPageIdx < 8; ++nPageIdx)
    {
        // Get page property (e.g., "0", "1", ...)
        auto pPage = m_pPropField->GetChild(std::to_string(nPageIdx));
        if (!pPage)
        {
            continue;
        }

        // Get info sub-property
        auto pInfo = pPage->GetChild("info");

        // Get tile sub-property
        auto pTile = pPage->GetChild("tile");
        if (!pTile || !pTile->HasChildren())
        {
            continue;
        }

        // Get tile set name from info
        std::string tileSetName;
        if (pInfo)
        {
            auto tsProp = pInfo->GetChild("tS");
            tileSetName = tsProp ? tsProp->GetString("") : "";
        }

        if (tileSetName.empty())
        {
            continue;
        }

        // Build UOL path: "Map/Tile/" + tileSetName
        std::string sUOL = "Map/Tile/" + tileSetName + ".img";

        // Get tile set property
        auto pTileSet = resMan.GetProperty(sUOL);
        if (!pTileSet)
        {
            LOG_DEBUG("RestoreTile: Tile set not found: {}", sUOL);
            continue;
        }

        // Create tile for each entry
        int tileCount = 0;
        for (const auto& [childName, pPiece] : pTile->GetChildren())
        {
            if (pPiece)
            {
                MakeTile(nPageIdx, pTileSet, pPiece);
                ++tileCount;
            }
        }

        LOG_DEBUG("RestoreTile: Loaded {} tiles for page {} with tileset {}",
                  tileCount, nPageIdx, tileSetName);
    }
}

void MapLoadable::RestoreObj(bool bLoad)
{
    // Based on CMapLoadable::RestoreObj at 0xbff8e0
    // Restore object layers from WZ property

    if (!m_pPropField)
    {
        return;
    }

    int totalObjCount = 0;

    // Iterate through 8 pages
    for (int nPageIdx = 0; nPageIdx < 8; ++nPageIdx)
    {
        auto pPage = m_pPropField->GetChild(std::to_string(nPageIdx));
        if (!pPage)
        {
            continue;
        }

        // Get obj sub-property (StringPool 2527)
        auto pObj = pPage->GetChild("obj");
        if (!pObj || !pObj->HasChildren())
        {
            continue;
        }

        // Check "reactor" property (StringPool 0x125B) - skip if reactor flag is set
        for (const auto& [childName, pPiece] : pObj->GetChildren())
        {
            if (!pPiece)
            {
                continue;
            }

            // Check reactor flag
            auto reactorProp = pPiece->GetChild("reactor");
            if (reactorProp && reactorProp->GetInt(0) != 0)
            {
                continue;  // Skip reactor objects
            }

            MakeObj(nPageIdx, pPiece, bLoad);
            ++totalObjCount;
        }
    }

    // Update tag layer references
    UpdateObjectTagLayer();

    LOG_DEBUG("RestoreObj: Loaded {} objects (bLoad={})", totalObjCount, bLoad);
}

void MapLoadable::RestoreWeather()
{
    // Based on CMapLoadable::RestoreWeather at 0xbeada0
    // Reads "cloud" property (StringPool 0xA0B = 2571) from field info
    // If set, calls MakeCloud() to create cloud weather effects

    if (!m_pPropFieldInfo)
    {
        return;
    }

    // Read "cloud" property
    auto cloudProp = m_pPropFieldInfo->GetChild("cloud");
    auto cloudValue = cloudProp ? cloudProp->GetInt(0) : 0;

    if (cloudValue != 0)
    {
        MakeCloud();
    }

    LOG_DEBUG("RestoreWeather: cloud={}", cloudValue);
}

void MapLoadable::RestoreLetterBox()
{
    // Based on CMapLoadable::RestoreLetterBox at 0xbeb920
    // Creates letterbox (black cinema bars) for widescreen displays
    // Only needed for non-800x600 resolutions

    auto& gr = get_gr();
    auto screenWidth = static_cast<std::int32_t>(gr.GetWidth());
    auto screenHeight = static_cast<std::int32_t>(gr.GetHeight());

    // Skip letterbox for 800x600 (original MapleStory resolution)
    if (screenWidth == 800 && screenHeight == 600)
    {
        LOG_DEBUG("RestoreLetterBox: Skipped for 800x600");
        return;
    }

    if (!m_pPropFieldInfo)
    {
        return;
    }

    // Read letterbox properties from field info
    // StringPool 0x9F1 = "sideL" or similar (side letterbox width)
    // StringPool 0x9F2 = "top" (top letterbox height)
    // StringPool 0x9F3 = "bottom" (bottom letterbox height)
    auto sideLProp = m_pPropFieldInfo->GetChild("sideL");
    auto topProp = m_pPropFieldInfo->GetChild("top");
    auto bottomProp = m_pPropFieldInfo->GetChild("bottom");

    auto nSide = sideLProp ? sideLProp->GetInt(0) : 0;
    auto nTop = topProp ? topProp->GetInt(0) : 0;
    auto nBottom = bottomProp ? bottomProp->GetInt(0) : 0;

    // Special adjustment for 1366x768 resolution
    if (screenWidth == 1366 && screenHeight == 768)
    {
        nSide += 171;  // Widen side letterbox for this resolution
    }

    auto halfWidth = static_cast<std::int32_t>(screenWidth) / 2;
    auto halfHeight = static_cast<std::int32_t>(screenHeight) / 2;

    // Add top letterbox
    if (nTop > 0)
    {
        AddLetterBox(screenWidth, nTop, -halfWidth, -halfHeight);
    }

    // Add bottom letterbox (with 65 pixel adjustment from original)
    if (nBottom > 0)
    {
        auto bottomHeight = nBottom + 65;
        auto bottomY = halfHeight - bottomHeight;
        AddLetterBox(screenWidth, bottomHeight, -halfWidth, bottomY);
    }

    // Add side letterboxes (left and right)
    if (nSide > 0)
    {
        // Left side
        AddLetterBox(nSide, screenHeight, -halfWidth, -halfHeight);
        // Right side
        AddLetterBox(nSide, screenHeight, halfWidth - nSide, -halfHeight);
    }

    LOG_DEBUG("RestoreLetterBox: side={}, top={}, bottom={}", nSide, nTop, nBottom);
}

void MapLoadable::LoadLife()
{
    // Part of CMapLoadable::LoadMap at 0xbfff50
    // Load life (NPCs and mobs)

    if (!m_pPropField)
    {
        return;
    }

    // Get "life" property (StringPool 0xC43)
    auto pLife = m_pPropField->GetChild("life");
    if (!pLife)
    {
        return;
    }

    int entityCount = 0;

    for (const auto& [childName, pEntity] : pLife->GetChildren())
    {
        if (!pEntity)
        {
            continue;
        }

        auto typeProp = pEntity->GetChild("type");
        auto idProp = pEntity->GetChild("id");
        auto groupNameProp = pEntity->GetChild("groupName");

        std::string sType = typeProp ? typeProp->GetString("") : "";
        std::int32_t dwID = idProp ? idProp->GetInt(0) : 0;
        std::string sGroupName = groupNameProp ? groupNameProp->GetString("") : "";

        if (sType == "n")
        {
            // NPC
            // TODO: CNpcTemplate::GetNpcTemplate(dwID);
            LOG_DEBUG("LoadLife: NPC id={} group={}", dwID, sGroupName);
        }
        else if (sType == "m")
        {
            // Mob
            // TODO: CMobTemplate::GetMobTemplate(dwID);
            LOG_DEBUG("LoadLife: Mob id={}", dwID);
        }

        ++entityCount;
    }

    LOG_DEBUG("LoadLife: Loaded {} life entities", entityCount);
}

void MapLoadable::LoadReactors()
{
    // Part of CMapLoadable::LoadMap at 0xbfff50
    // Load reactors

    if (!m_pPropField)
    {
        return;
    }

    // Get "reactor" property (StringPool 0x125B)
    auto pReactor = m_pPropField->GetChild("reactor");
    if (!pReactor)
    {
        return;
    }

    int reactorCount = 0;

    for (const auto& [childName, pR] : pReactor->GetChildren())
    {
        if (!pR)
        {
            continue;
        }

        auto idProp = pR->GetChild("id");
        std::int32_t dwID = idProp ? idProp->GetInt(0) : 0;

        // TODO: CReactorTemplate::GetReactorTemplate(dwID);
        LOG_DEBUG("LoadReactors: Reactor id={}", dwID);

        ++reactorCount;
    }

    LOG_DEBUG("LoadReactors: Loaded {} reactors", reactorCount);
}

void MapLoadable::LoadRectEventData()
{
    // Part of CMapLoadable::LoadMap at 0xbfff50
    // Load rect event data for zones (fade, bgm, ambience, etc.)

    // TODO: Implement rect event data loading

    LOG_DEBUG("LoadRectEventData: Rect event data loaded (stub)");
}

void MapLoadable::SetFootStepSound(const std::string& sound)
{
    // Set footstep sound
    // TODO: Implement footstep sound loading

    (void)sound;
    LOG_DEBUG("SetFootStepSound: Footstep sound set (stub)");
}

void MapLoadable::MakeCloud()
{
    // Based on CMapLoadable::MakeCloud at 0xbe3230
    // Creates random cloud layers that drift across the map

    auto& resMan = WzResMan::GetInstance();
    auto& gr = get_gr();

    // Load cloud property from "Map/Obj/cloud.img" (StringPool 0xA0C)
    auto pCloud = resMan.GetProperty("Map/Obj/cloud.img");
    if (!pCloud || !pCloud->HasChildren())
    {
        LOG_WARN("MakeCloud: Map/Obj/cloud.img not found");
        return;
    }

    // Get cloud count (children of cloud.img are individual cloud images)
    auto cloudCount = pCloud->GetChildCount();
    if (cloudCount == 0)
    {
        return;
    }

    // Get map bounds (from view range or default)
    auto lLeft = m_viewRangeRect.left - 200;
    auto lTop = m_viewRangeRect.top - 200;
    auto lWidth = m_viewRangeRect.Width() + 400;
    auto lHeight = m_viewRangeRect.Height() + 400;

    // Calculate cloud count based on map size (larger maps = more clouds)
    auto mapArea = static_cast<float>(lWidth * lHeight);
    auto density = mapArea / 9000000.0f;
    if (density > 1.0f)
    {
        density = 1.0f;
    }

    // Random cloud count: 10 + rand(20) + density * 30
    auto nCloudCount = 10 + (std::rand() % 20) + static_cast<int>(density * 30.0f);

    LOG_DEBUG("MakeCloud: Creating {} clouds for map area {}x{}", nCloudCount, lWidth, lHeight);

    for (int i = 0; i < nCloudCount; ++i)
    {
        // Select random cloud image
        auto cloudIdx = std::rand() % cloudCount;
        auto cloudChild = pCloud->GetChild(std::to_string(cloudIdx));
        if (!cloudChild)
        {
            continue;
        }

        auto canvas = cloudChild->GetCanvas();
        if (!canvas)
        {
            continue;
        }

        // Random position within map bounds
        auto x = lLeft + (std::rand() % lWidth);
        auto y = lTop + (std::rand() % lHeight);

        // Random speed (50-150) affects alpha and velocity
        auto speed = 50 + (std::rand() % 100);
        auto alpha = 140 + (std::rand() % 80);  // 140-220

        // Create cloud layer at z=-1073343224 (very far back)
        auto layer = gr.CreateLayer(x, y,
                                    static_cast<std::uint32_t>(canvas->GetWidth()),
                                    static_cast<std::uint32_t>(canvas->GetHeight()),
                                    -1073343224);
        if (!layer)
        {
            continue;
        }

        layer->InsertCanvas(canvas, 0, 255, 255);
        layer->SetColor(0x00FFFFFF | (static_cast<std::uint32_t>(alpha) << 24));

        // TODO: Set up cloud movement animation
        // Original uses IWzVector2D with WrapClip for wrapping movement

        m_lpLayerGen.push_back(layer);
    }

    LOG_DEBUG("MakeCloud: Created {} cloud layers", nCloudCount);
}

void MapLoadable::AddLetterBox(std::int32_t w, std::int32_t h, std::int32_t l, std::int32_t t)
{
    // Based on CMapLoadable::AddLetterBox at 0xbe7ec0
    // Creates a black rectangle (letterbox bar) at the specified position

    auto& gr = get_gr();

    if (w <= 0 || h <= 0)
    {
        return;
    }

    // Create a solid black canvas for the letterbox
    auto canvas = std::make_shared<WzCanvas>(w, h);
    if (!canvas->HasPixelData())
    {
        LOG_WARN("AddLetterBox: Failed to create canvas");
        return;
    }

    // Fill with solid black (BGRA format: 0xFF000000)
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4);
    for (std::size_t i = 0; i < pixels.size(); i += 4)
    {
        pixels[i + 0] = 0;    // B
        pixels[i + 1] = 0;    // G
        pixels[i + 2] = 0;    // R
        pixels[i + 3] = 255;  // A
    }
    canvas->SetPixelData(pixels);

    // Create layer at z=-1073343174 (letterbox z-order, in front of most things)
    auto layer = gr.CreateLayer(0, 0,
                                static_cast<std::uint32_t>(w),
                                static_cast<std::uint32_t>(h),
                                -1073343174,
                                canvas);
    if (!layer)
    {
        LOG_WARN("AddLetterBox: Failed to create layer");
        return;
    }

    // Position relative to screen center
    layer->SetPosition(l, t);
    layer->SetScreenSpace(true);
    layer->SetCenterBased(true);
    layer->SetColor(0xFFFFFFFF);

    m_lpLayerLetterBox.push_back(layer);

    LOG_DEBUG("AddLetterBox: Created {}x{} at ({}, {})", w, h, l, t);
}

void MapLoadable::MakeTile(std::int32_t nPageIdx,
                           const std::shared_ptr<WzProperty>& pTileSet,
                           const std::shared_ptr<WzProperty>& pPiece)
{
    // Based on CMapLoadable::MakeTile at 0xbe21b0
    // Creates a tile layer from a tile piece property

    if (!pPiece || !pTileSet)
    {
        return;
    }

    // Get "no" property (StringPool 2525) - tile number
    auto noProp = pPiece->GetChild("no");
    auto no = noProp ? noProp->GetInt(0) : 0;

    // Get "u" property (StringPool 0x9D6) - tile type/variant (e.g., "bsc", "edD")
    auto uProp = pPiece->GetChild("u");
    std::string u = uProp ? uProp->GetString("") : "";

    if (u.empty())
    {
        LOG_DEBUG("MakeTile[{}]: Empty 'u' property, skipping", nPageIdx);
        return;
    }

    // Build property path: "{u}/{no}" (e.g., "bsc/0", "edD/1")
    // Format string is from StringPool 0x9D5
    std::string propPath = u + "/" + std::to_string(no);

    // Get canvas from tile set
    auto tileProp = pTileSet->GetChild(propPath);
    if (!tileProp)
    {
        LOG_DEBUG("MakeTile[{}]: Tile property not found: {}", nPageIdx, propPath);
        return;
    }

    auto canvas = tileProp->GetCanvas();
    if (!canvas)
    {
        LOG_DEBUG("MakeTile[{}]: No canvas in tile: {}", nPageIdx, propPath);
        return;
    }

    // Get x position (StringPool 1682)
    auto xProp = pPiece->GetChild("x");
    auto x = xProp ? xProp->GetInt(0) : 0;

    // Get y position (StringPool 1683)
    auto yProp = pPiece->GetChild("y");
    auto y = yProp ? yProp->GetInt(0) : 0;

    // Get zMass (StringPool 2526 = "zM")
    auto zMProp = pPiece->GetChild("zM");
    auto zMass = zMProp ? zMProp->GetInt(0) : 0;

    // Get z value from canvas property (StringPool 1684)
    // The z property is stored in the tile's canvas property
    std::int32_t z = 0;
    auto canvasProp = tileProp->GetChild("z");
    if (canvasProp)
    {
        z = canvasProp->GetInt(0);
    }

    // Create layer with canvas at position (x, y)
    auto& gr = get_gr();
    auto layer = gr.CreateLayer(x, y, canvas->GetWidth(), canvas->GetHeight(), 0);

    if (!layer)
    {
        LOG_ERROR("MakeTile[{}]: Failed to create layer", nPageIdx);
        return;
    }

    // Insert the canvas into the layer
    layer->InsertCanvas(canvas, 0, 255, 255);

    // Calculate z-order: z + 10 * (3000 * nPageIdx - zMass) - 0x3FFFB1EA
    // 0x3FFFB1EA = 1073692138 (this is a large negative offset to place tiles behind objects)
    std::int32_t zOrder = z + 10 * (3000 * nPageIdx - zMass) - 0x3FFFB1EA;
    layer->SetZ(zOrder);

    // Set color to white (0xFFFFFFFF - full alpha, white color)
    layer->SetColor(0xFFFFFFFF);

    // Add layer to general layer list
    m_lpLayerGen.push_back(layer);

#ifdef MS_DEBUG_CANVAS
    DebugOverlay::GetInstance().RegisterLayer(layer,
        "tile_" + std::to_string(nPageIdx) + "_" + propPath);
#endif

    LOG_DEBUG("MakeTile[{}]: Created tile {} at ({}, {}), z={}, zOrder={}",
              nPageIdx, propPath, x, y, z, zOrder);
}

void MapLoadable::MakeObj(std::int32_t nPageIdx,
                          const std::shared_ptr<WzProperty>& pPiece,
                          bool bLoad)
{
    // Based on CMapLoadable::MakeObj at 0xbfc950
    // Creates an object layer from an object piece property

    if (!pPiece || !bLoad)
    {
        return;
    }

    // Read object properties
    // oS - Object Set name (StringPool 2560)
    auto oSProp = pPiece->GetChild("oS");
    std::string oS = oSProp ? oSProp->GetString("") : "";

    // l0, l1, l2 - Layer hierarchy (StringPool 2561, 2562, 2563)
    auto l0Prop = pPiece->GetChild("l0");
    auto l1Prop = pPiece->GetChild("l1");
    auto l2Prop = pPiece->GetChild("l2");
    std::string l0 = l0Prop ? l0Prop->GetString("") : "";
    std::string l1 = l1Prop ? l1Prop->GetString("") : "";
    std::string l2 = l2Prop ? l2Prop->GetString("") : "";

    // Position (StringPool 1682, 1683)
    auto xProp = pPiece->GetChild("x");
    auto yProp = pPiece->GetChild("y");
    auto x = xProp ? xProp->GetInt(0) : 0;
    auto y = yProp ? yProp->GetInt(0) : 0;

    // z value (StringPool 1684)
    auto zProp = pPiece->GetChild("z");
    auto z = zProp ? zProp->GetInt(0) : 0;

    // Flip flag (StringPool 2557)
    auto fProp = pPiece->GetChild("f");
    auto f = fProp ? (fProp->GetInt(0) != 0) : false;

    // Parallax (StringPool 2564, 2565)
    auto rxProp = pPiece->GetChild("rx");
    auto ryProp = pPiece->GetChild("ry");
    auto rx = rxProp ? rxProp->GetInt(0) : 0;
    auto ry = ryProp ? ryProp->GetInt(0) : 0;

    // Flow type (StringPool 2566) - controls movement animation
    auto flowProp = pPiece->GetChild("flow");
    auto flow = flowProp ? flowProp->GetInt(0) : 0;

    // Name for lookup (StringPool 0x9D3)
    auto nameProp = pPiece->GetChild("name");
    std::string name = nameProp ? nameProp->GetString("") : "";

    // Tags for grouped lookup (StringPool 0x1AD2)
    auto tagsProp = pPiece->GetChild("tags");
    std::string tags = tagsProp ? tagsProp->GetString("") : "";

    if (oS.empty())
    {
        LOG_DEBUG("MakeObj[{}]: Empty oS, skipping", nPageIdx);
        return;
    }

    // Build path: "Map/Obj/{oS}.img/{l0}/{l1}/{l2}"
    std::string path = "Map/Obj/" + oS + ".img";
    if (!l0.empty())
    {
        path += "/" + l0;
    }
    if (!l1.empty())
    {
        path += "/" + l1;
    }
    if (!l2.empty())
    {
        path += "/" + l2;
    }

    auto& resMan = WzResMan::GetInstance();
    auto objProp = resMan.GetProperty(path);

    if (!objProp)
    {
        LOG_DEBUG("MakeObj[{}]: Object not found: {}", nPageIdx, path);
        return;
    }

    // Calculate z-order: 30000 * nPageIdx + z - 0x3FFF4C30
    // 0x3FFF4C30 = 1073692720 - places objects in front of tiles but respects page order
    std::int32_t zOrder = 30000 * nPageIdx + z - 0x3FFF4C30;

    // For quarter view maps, use y-based z-ordering
    if (m_bQuarterView)
    {
        zOrder = 10 * y - 0x3FFE2CB0;  // 0x3FFE2CB0 = 1073619120
    }

    // Create the layer
    auto& gr = get_gr();
    auto layer = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), zOrder);

    if (!layer)
    {
        LOG_ERROR("MakeObj[{}]: Failed to create layer", nPageIdx);
        return;
    }

    // Load animated frames into the layer
    std::size_t frameCount = LoadAnimatedLayer(layer, objProp);
    if (frameCount == 0)
    {
        // Try loading as static
        if (!LoadStaticLayer(layer, objProp))
        {
            LOG_WARN("MakeObj[{}]: No frames loaded from {}", nPageIdx, path);
            gr.RemoveLayer(layer);
            return;
        }
        frameCount = 1;
    }

    // Set position
    layer->SetPosition(x, y);

    // Set flip
    if (f)
    {
        layer->SetFlip(true);
    }

    // Set parallax
    if (rx != 0 || ry != 0)
    {
        layer->SetParallax(rx, ry);
    }

    // Set color to white (full opacity)
    layer->SetColor(0xFFFFFFFF);

    // Start animation if more than one frame
    if (frameCount > 1)
    {
        layer->Animate(Gr2DAnimationType::Loop);
    }

    // Add to object layer list
    m_lpLayerObj.push_back(layer);

    // Add to named layer map if name is provided
    if (!name.empty())
    {
        m_mpLayerObj[name] = layer;
    }

    // Add to tagged layer map if tags are provided
    if (!tags.empty())
    {
        m_mTaggedLayer[tags] = layer;
    }

#ifdef MS_DEBUG_CANVAS
    std::string debugName = "obj_" + std::to_string(nPageIdx) + "_" + oS;
    if (!l0.empty()) debugName += "_" + l0;
    if (!l1.empty()) debugName += "_" + l1;
    if (!l2.empty()) debugName += "_" + l2;
    DebugOverlay::GetInstance().RegisterLayer(layer, debugName);
#endif

    LOG_DEBUG("MakeObj[{}]: Created object {} at ({}, {}), z={}, frames={}",
              nPageIdx, path, x, y, zOrder, frameCount);
}

void MapLoadable::UpdateObjectTagLayer()
{
    // Update tag layer references after loading objects
    // This is called after RestoreObj to finalize tag mappings

    LOG_DEBUG("UpdateObjectTagLayer: {} named layers, {} tagged layers",
              m_mpLayerObj.size(), m_mTaggedLayer.size());
}

} // namespace ms
