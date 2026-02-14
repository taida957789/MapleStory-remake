#include "MapLoadable.h"
#include "audio/SoundMan.h"
#include "physics/WvsPhysicalSpace2D.h"
#include "util/Rand32.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "util/Logger.h"
#include "graphics/WzGr2DCanvas.h"
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

    // Reset camera to world origin — clears any active animation chain (tremble, etc.)
    gr.ResetCameraPosition(0, 0);

    // Clear any existing layers
    ClearAllLayers();

    // Initialize default values (matching constructor defaults from decompiled code)
    m_nMagLevel_Obj = 0;
    m_nMagLevel_Back = 0;
    m_nMagLevel_SkillEffect = 0;
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
            SoundMan::GetInstance().SetBGMVolume(
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
    double dx = m_cameraMoveInfo.ptVelocity_First.x * t +
                0.5 * m_cameraMoveInfo.ptAccelation.x * t * t;
    double dy = m_cameraMoveInfo.ptVelocity_First.y * t +
                0.5 * m_cameraMoveInfo.ptAccelation.y * t * t;

    // Apply relative movement to camera
    auto currentPos = gr.GetCameraPosition();
    gr.SetCameraPosition(currentPos.x + static_cast<std::int32_t>(dx),
                         currentPos.y + static_cast<std::int32_t>(dy));

    // Apply velocity adjust rate (damping/acceleration factor)
    if (m_cameraMoveInfo.ptVelocity_AdjustRate.x != 0)
    {
        m_cameraMoveInfo.ptVelocity_First.x =
            static_cast<std::int32_t>(m_cameraMoveInfo.ptVelocity_AdjustRate.x *
                                       m_cameraMoveInfo.ptVelocity_First.x / 100.0);
    }
    if (m_cameraMoveInfo.ptVelocity_AdjustRate.y != 0)
    {
        m_cameraMoveInfo.ptVelocity_First.y =
            static_cast<std::int32_t>(m_cameraMoveInfo.ptVelocity_AdjustRate.y *
                                       m_cameraMoveInfo.ptVelocity_First.y / 100.0);
    }

    // Apply acceleration adjust rate
    if (m_cameraMoveInfo.ptAccelation_AdjustRate.x != 0)
    {
        m_cameraMoveInfo.ptAccelation.x =
            static_cast<std::int32_t>(m_cameraMoveInfo.ptAccelation_AdjustRate.x *
                                       m_cameraMoveInfo.ptAccelation.x / 100.0);
    }
    if (m_cameraMoveInfo.ptAccelation_AdjustRate.y != 0)
    {
        m_cameraMoveInfo.ptAccelation.y =
            static_cast<std::int32_t>(m_cameraMoveInfo.ptAccelation_AdjustRate.y *
                                       m_cameraMoveInfo.ptAccelation.y / 100.0);
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
    if (viewLeft < m_rcViewRange.left)
        pos.x = m_rcViewRange.left;
    if (viewRight > m_rcViewRange.right)
        pos.x = m_rcViewRange.right - screenWidth;
    if (viewTop < m_rcViewRange.top)
        pos.y = m_rcViewRange.top;
    if (viewBottom > m_rcViewRange.bottom)
        pos.y = m_rcViewRange.bottom - screenHeight;

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
        if (type == Gr2DAnimationType::None || type == Gr2DAnimationType::Repeat)
        {
            // Reset to first frame before starting animation
            layer->SetCurrentFrame(0);
        }
        layer->Animate(type);
    }
}

void MapLoadable::AnimateObjLayer(const std::shared_ptr<WzGr2DLayer>& pLayer,
                                   std::int32_t nRepeat)
{
    // Based on CMapLoadable::AnimateObjLayer
    if (!pLayer)
        return;

    if (nRepeat >= 0)
    {
        // GA_REPEAT with specified repeat count
        pLayer->Animate(Gr2DAnimationType::Repeat, 1000, nRepeat);
    }
    else if (nRepeat == -1)
    {
        // GA_STOP — stop animation
        pLayer->Animate(Gr2DAnimationType::Stop);
    }
    else if (nRepeat == -2)
    {
        // GA_FIRST only if not currently animating
        if (pLayer->get_animationState() == 0)
            pLayer->Animate(Gr2DAnimationType::First);
    }
}

void MapLoadable::DisableEffectObject(const std::string& sName, bool bCheckPreWord)
{
    // Based on CMapLoadable::DisableEffectObject
    // Collect names of objects to disable
    std::vector<std::string> asDisableObj;

    if (bCheckPreWord)
    {
        // Find all named objects whose key contains sName as prefix/substring
        for (auto& [key, obj] : m_mNamedObj)
        {
            if (sName.empty() || key.find(sName) != std::string::npos)
                asDisableObj.push_back(key);
        }
    }
    else
    {
        asDisableObj.push_back(sName);
    }

    // Disable each matched object by setting its alpha to 0
    for (auto& name : asDisableObj)
    {
        auto it = m_mNamedObj.find(name);
        if (it == m_mNamedObj.end())
            continue;

        auto& obj = it->second;
        if (obj.nState < 0 || static_cast<std::size_t>(obj.nState) >= obj.aState.size())
            continue;

        auto& layer = obj.aState[obj.nState].pLayer;
        if (!layer)
            continue;

        auto* pAlpha = layer->get_alpha();
        if (pAlpha)
            pAlpha->Move(0, 0);
    }
}

void MapLoadable::SetTaggedObjectAnimation(const std::string& tag, Gr2DAnimationType type)
{
    auto it = m_mTaggedLayer.find(tag);
    if (it != m_mTaggedLayer.end() && it->second)
    {
        if (type == Gr2DAnimationType::None || type == Gr2DAnimationType::Repeat)
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

    m_sChangedBgmUOL = std::u16string(bgmPath.begin(), bgmPath.end());

    if (bgmPath.empty())
    {
        // Stop BGM if path is empty
        SoundMan::GetInstance().StopBGM(0);
        LOG_DEBUG("ChangeBGM: stopped");
        return;
    }

    // Play BGM with looping (nLoop=1), volume 0x258 (600 -> scaled to 128)
    // Based on CSoundMan::PlayBGM call at 0xbeafc2
    SoundMan::GetInstance().PlayBGM(bgmPath, 1, 128, 128, 0, 0);
    LOG_DEBUG("ChangeBGM: {}", bgmPath);
}

auto MapLoadable::IsSameChangeBGM(const std::string& bgmPath) const -> bool
{
    auto w = std::u16string(bgmPath.begin(), bgmPath.end());
    return m_sChangedBgmUOL == w;
}

void MapLoadable::PrepareNextBGM()
{
    // TODO: Implement BGM preparation for smooth transitions
}

void MapLoadable::RestoreMutedBGM()
{
    if (m_tRestoreBgmVolume != 0)
    {
        // Restore BGM volume via SoundMan
        SoundMan::GetInstance().SetBGMVolume(
            static_cast<std::uint32_t>(m_nRestoreBgmVolume), 0);
        m_tRestoreBgmVolume = 0;
    }
}

void MapLoadable::PlayBGMFromMapInfo()
{
    // Based on CMapLoadable::PlayBGMFromMapInfo (0xbeaef8)
    if (!m_pPropFieldInfo)
        return;

    // Get "bgm" property from field info (StringPool 0xA13)
    auto bgmProp = m_pPropFieldInfo->GetChild("bgm");
    std::string bgmValue = bgmProp ? bgmProp->GetString("") : "";

    // If bgm property is empty and no custom BGM, bail out
    std::string sCustom(m_sFieldCustomBgmUOL.begin(), m_sFieldCustomBgmUOL.end());
    if (bgmValue.empty() && sCustom.empty())
        return;

    // Build UOL: "Sound/" (StringPool 2580) + bgm path
    std::string sUOL = "Sound/";
    if (!sCustom.empty())
        sUOL += sCustom;
    else
        sUOL += bgmValue;

    // PlayBGM(sUOL, nLoop=1, nStartVolume=600, 0, 0, 0)
    SoundMan::GetInstance().PlayBGM(sUOL, 1, 600, 0, 0, 0);

    // Clear m_sChangedBgmUOL
    m_sChangedBgmUOL.clear();
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
    m_cameraMoveInfo.ptVelocity_First = velocity;
    m_cameraMoveInfo.ptAccelation = acceleration;
    m_cameraMoveInfo.ptVelocity_AdjustRate = velocityAdjust;
    m_cameraMoveInfo.ptAccelation_AdjustRate = accelAdjust;
    m_cameraMoveInfo.bClipInViewRange = clipInViewRange;
}

void MapLoadable::ClearCameraMove()
{
    m_cameraMoveInfo.bOn = false;
}

auto MapLoadable::GetViewRangeRect() const -> const Rect*
{
    return &m_rcViewRange;
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
        auto wzCanvas = frameProp->GetCanvas();
        auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas, frameProp) : nullptr;
        if (!canvas)
        {
            // Frame might be the canvas itself
            auto frameWzCanvas = prop->GetChild(std::to_string(i))->GetCanvas();
            canvas = frameWzCanvas ? std::make_shared<WzGr2DCanvas>(frameWzCanvas, frameProp) : nullptr;
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
    auto wzCanvas = prop->GetCanvas();
    auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas, prop) : nullptr;
    if (!canvas)
    {
        LOG_DEBUG("LoadStaticLayer: No direct canvas, trying child '0'");
        // Try to get canvas from first child (some properties wrap canvas in "0" child)
        auto firstChild = prop->GetChild("0");
        if (firstChild)
        {
            auto childWzCanvas = firstChild->GetCanvas();
            canvas = childWzCanvas ? std::make_shared<WzGr2DCanvas>(childWzCanvas, firstChild) : nullptr;
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
        m_rcViewRange.left = -nScaledHalfWidth;
        m_rcViewRange.top = -nScaledHalfHeight;
        m_rcViewRange.right = nScaledHalfWidth;
        m_rcViewRange.bottom = nScaledHalfHeight;
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

    m_rcViewRange.left = vrLeftProp ? vrLeftProp->GetInt(DEFAULT_LEFT) : DEFAULT_LEFT;
    m_rcViewRange.top = vrTopProp ? vrTopProp->GetInt(DEFAULT_TOP) : DEFAULT_TOP;
    m_rcViewRange.right = vrRightProp ? vrRightProp->GetInt(DEFAULT_RIGHT) : DEFAULT_RIGHT;
    m_rcViewRange.bottom = vrBottomProp ? vrBottomProp->GetInt(DEFAULT_BOTTOM) : DEFAULT_BOTTOM;

    // Calculate min zoom out scale
    auto nMarginX = (m_rcViewRange.left + m_rcViewRange.right) / 2 - m_rcViewRange.left;
    auto nMarginY = (m_rcViewRange.top + m_rcViewRange.bottom) / 2 - m_rcViewRange.top;

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
    m_rcViewRange.left += nScaledHalfWidth;
    m_rcViewRange.right -= nScaledHalfWidth;
    m_rcViewRange.top += nScaledHalfHeight;
    m_rcViewRange.bottom -= nScaledHalfHeight;

    // Clamp if dimensions become negative (map smaller than screen)
    if (m_rcViewRange.right - m_rcViewRange.left <= 0)
    {
        auto mid = (m_rcViewRange.right + m_rcViewRange.left) / 2;
        m_rcViewRange.left = mid;
        m_rcViewRange.right = mid;
    }
    if (m_rcViewRange.bottom - m_rcViewRange.top <= 0)
    {
        auto mid = (m_rcViewRange.bottom + m_rcViewRange.top) / 2;
        m_rcViewRange.top = mid;
        m_rcViewRange.bottom = mid;
    }

    LOG_DEBUG("RestoreViewRange: ({},{}) - ({},{}), minScale={}",
              m_rcViewRange.left, m_rcViewRange.top,
              m_rcViewRange.right, m_rcViewRange.bottom,
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
    // 0: Normal - no tiling, position with rx/ry parallax
    // 1: Horizontal tiling
    // 2: Vertical tiling
    // 3: Both H+V tiling
    // 4: Animated H movement, converts to type 1 (H-tile)
    // 5: Animated V movement, converts to type 2 (V-tile)
    // 6: Animated H movement, converts to type 3 (both tile)
    // 7: Animated V movement, converts to type 3 (both tile)
    auto typeProp = pPiece->GetChild("type");
    auto type = typeProp ? typeProp->GetInt(0) : 0;

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

    // Z-order from 0xbf3447. All constants anchored at Z_BASE = 0x40000000:
    //   Front: 0x40000000 - 271200 = 0x3FFBDCA0 (in front of tiles/objects)
    //   Back:  0x40000000 + 128000 = 0x4001F400 (behind tiles)
    std::int32_t z = 1000 * nPageIdx - (front ? 0x3FFBDCA0 : 0x4001F400);

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
            layer->Animate(Gr2DAnimationType::Repeat);
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

    // MakeVectorAnimate — called before alpha/flip (0xbf3c42 for static, 0xbf4380 for ani)
    MakeVectorAnimate(layer, spriteProp);

    // Alpha: original sets color to white then animates alpha from 'a' to 255 (fade-in)
    // put_color(-1) = 0xFFFFFFFF, then Getalpha().RelMove(alpha, 255)
    layer->SetColor(0xFFFFFFFF);
    if (alpha != 255)
    {
        auto* alphaVec = layer->get_alpha();
        if (alphaVec)
        {
            alphaVec->RelMove(alpha, 255);
        }
    }

    // Set flip if needed
    if (flip)
    {
        layer->SetFlip(true);
    }

    // Blend mode: read from first canvas's property (StringPool 2533 = "blend")
    // If blend == 1, enable additive blending
    {
        auto blendProp = spriteProp->GetChild("blend");
        if (!blendProp && spriteProp->HasChildren())
        {
            // For animated sprites, check first frame (frame "0")
            auto frame0 = spriteProp->GetChild("0");
            if (frame0)
                blendProp = frame0->GetChild("blend");
        }
        if (blendProp && blendProp->GetInt(0) == 1)
        {
            layer->put_blend(1);
        }
    }

    // Handle background type positioning and tiling (following original at 0xbf2af0)
    // Types 4-7 are animated variants that convert to 1-3 after setting up Ratio
    if (type >= 4)
    {
        // Animated types: the moving axis has no parallax (0),
        // the OTHER axis gets parallax (r_ + 100).
        // Type 4/6 = horizontal movement → Y-axis parallax: Ratio(0, ry+100)
        // Type 5/7 = vertical movement   → X-axis parallax: Ratio(rx+100, 0)
        std::int32_t ratioRx = 0;
        std::int32_t ratioRy = 0;

        if (type == 4 || type == 6)
        {
            ratioRy = ry + 100;
        }
        else // type == 5 || type == 7
        {
            ratioRx = rx + 100;
        }

        layer->Ratio(gr.GetCenterVec(), 100, 100, ratioRx, ratioRy);
        LOG_DEBUG("MakeBack[{}]: Animated type {} Ratio rx={}, ry={}", nPageIdx, type, ratioRx, ratioRy);

        // Convert animated type to base tiling type: 4→1, 5→2, 6→3, 7→3
        if (type == 4)
            type = 1;
        else if (type == 5)
            type = 2;
        else
            type = 3; // 6 or 7
    }
    else
    {
        // Types 0-3: RelMove for position, then Ratio for parallax
        layer->RelMove(x, y);
        layer->Ratio(gr.GetCenterVec(), 100, 100, rx, ry);
        LOG_DEBUG("MakeBack[{}]: Type {} RelMove({}, {}), Ratio rx={}, ry={}", nPageIdx, type, x, y, rx, ry);
    }

    // Read additional properties for MakeGrid / InsertbackLayerByTag
    auto backTagsProp = pPiece->GetChild("backTags");
    std::vector<std::string> backTags;
    if (backTagsProp)
    {
        auto tagStr = backTagsProp->GetString("");
        if (!tagStr.empty())
            backTags.push_back(tagStr);
    }

    auto groupNameProp = pPiece->GetChild("groupName");
    std::string sGroupName = groupNameProp ? groupNameProp->GetString("") : "";

    auto sideTypeProp = pPiece->GetChild("sideType");
    auto nSideType = sideTypeProp ? sideTypeProp->GetInt(0) : 0;

    // Ensure layer list exists for this page
    auto& layerList = m_mlLayerBack[nPageIdx];
    if (!layerList)
        layerList = std::make_shared<std::list<std::shared_ptr<WzGr2DLayer>>>();

    // Type != 0: delegate to MakeGrid for tiling setup (0xbeff40)
    // Type == 0: insert directly, no tiling
    if (type != 0)
    {
        MakeGrid(layer, type, cx, cy, alpha, ani ? 1 : 0, false,
                 layerList, backTags, sGroupName, nSideType);
    }
    else
    {
        if (!backTags.empty())
        {
            InsertbackLayerByTag(backTags, layer);
        }
        layerList->push_back(layer);
    }

#ifdef MS_DEBUG_CANVAS
    DebugOverlay::GetInstance().RegisterLayer(layer, "back_" + std::to_string(nPageIdx));
#endif

    LOG_INFO("MakeBack[{}]: Created layer with {} frames at z={}, type={}", nPageIdx, frameCount, z, type);
}

void MapLoadable::UpdateBackTagLayer()
{
    // Update tag layer references after loading backgrounds
    // This is used for named background lookup
}

void MapLoadable::ClearBackLayers()
{
    auto& gr = get_gr();

    for (auto& [key, pList] : m_mlLayerBack)
    {
        if (!pList)
            continue;
        for (auto& layer : *pList)
        {
            if (layer)
                gr.RemoveLayer(layer);
        }
    }
    m_mlLayerBack.clear();
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

    // Load physical space with foothold and ladder info
    auto& space2D = WvsPhysicalSpace2D::GetInstance();
    space2D.Load(pPropFoothold, pLadderRope, m_pPropFieldInfo);
    // Store non-owning pointer for later queries
    m_pSpace2D = std::shared_ptr<WvsPhysicalSpace2D>(
        &space2D, [](WvsPhysicalSpace2D*) {});

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

void MapLoadable::PlayFootStepSound()
{
    if (m_nFootStepSoundCount <= 0)
        return;

    auto nRand = static_cast<std::uint32_t>(detail::get_rand().Random());
    auto nIndex = nRand % static_cast<std::uint32_t>(m_nFootStepSoundCount);

    // Original: ZXString<unsigned short>::Format(L"%s/%d", m_wsFootStepSound, index)
    std::string sPath(m_wsFootStepSound.begin(), m_wsFootStepSound.end());
    sPath += "/" + std::to_string(nIndex);

    SoundMan::GetInstance().PlaySE(sPath, 100);
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
    auto lLeft = m_rcViewRange.left - 200;
    auto lTop = m_rcViewRange.top - 200;
    auto lWidth = m_rcViewRange.Width() + 400;
    auto lHeight = m_rcViewRange.Height() + 400;

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

        auto wzCanvas = cloudChild->GetCanvas();
        auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas) : nullptr;
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
                                canvas ? std::make_shared<WzGr2DCanvas>(canvas) : nullptr);
    if (!layer)
    {
        LOG_WARN("AddLetterBox: Failed to create layer");
        return;
    }

    // Position relative to screen center
    layer->SetPosition(l, t);
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

    // Tile set hierarchy: pTileSet/{u}/{no} (e.g., "bsc" -> "0")
    // GetChild only does flat lookup, so resolve in two steps.
    auto uChild = pTileSet->GetChild(u);
    if (!uChild)
    {
        LOG_DEBUG("MakeTile[{}]: Tile type not found in tileset: {}", nPageIdx, u);
        return;
    }

    std::string noStr = std::to_string(no);
    auto tileProp = uChild->GetChild(noStr);
    if (!tileProp)
    {
        LOG_DEBUG("MakeTile[{}]: Tile number not found: {}/{}", nPageIdx, u, noStr);
        return;
    }

    std::string propPath = u + "/" + noStr;  // For debug logging only

    auto wzCanvas = tileProp->GetCanvas();
    auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas, tileProp) : nullptr;
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

    // Z-order from 0xbe2841: 0x40000000 - 19990 = 0x3FFFB1EA
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

    // Calculate z-order (from MakeObjLayer at 0xbeda65)
    // Z_BASE (0x40000000) - 2000 places objects slightly behind tiles at same page
    std::int32_t zOrder = 30000 * nPageIdx + z - 0x3FFFF830;

    // For quarter view maps, use y-based z-ordering (from 0xbeda86)
    if (m_bQuarterView)
    {
        zOrder = 10 * y - 0x3FFCCBB0;  // 0x40000000 - 210000
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

    // Set parallax via Ratio (same as MakeBack)
    if (rx != 0 || ry != 0)
    {
        layer->Ratio(gr.GetCenterVec(), 100, 100, rx, ry);
    }

    // Set color to white (full opacity)
    layer->SetColor(0xFFFFFFFF);

    // Start animation if more than one frame
    if (frameCount > 1)
    {
        layer->Animate(Gr2DAnimationType::Repeat);
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

// ========================================================================
// Properties / Getters
// ========================================================================

auto MapLoadable::IsFadeObject(const std::string& sGroupName, std::int32_t nShowType) const -> bool
{
    // 0xbdf9f0 — CMapLoadable::IsFadeObject
    // Check if the named rect event exists and has fade data with matching state
    auto itRect = m_mpRectEventData.find(sGroupName);
    if (itRect == m_mpRectEventData.end() || !itRect->second)
        return false;

    auto itFade = m_mpFadeData.find(sGroupName);
    if (itFade == m_mpFadeData.end() || !itFade->second)
        return false;

    // In IDA: return p->nState == nShowType
    // We don't have the full RectEventData struct yet, so just return true if fade data exists
    (void)nShowType;
    return true;
}

auto MapLoadable::GetCollideObstacleRect(const Point2D& pt, Point2D* pvecForce) const
    -> const ObstacleInfo*
{
    // 0xbd9ab0 — CMapLoadable::GetCollideObstacleRect
    // Iterate m_aObstacleInfo, find first obstacle whose rect contains pt
    for (auto& info : m_aObstacleInfo)
    {
        if (pt.x >= info.rcObs.left && pt.x <= info.rcObs.right &&
            pt.y >= info.rcObs.top && pt.y <= info.rcObs.bottom)
        {
            if (pvecForce)
                *pvecForce = info.vecForce;
            return &info;
        }
    }
    return nullptr;
}

auto MapLoadable::GetNpcRectEventType(const std::string& sName) const -> std::int32_t
{
    // 0xbdceb0 — CMapLoadable::GetNpcRectEventType
    // Iterate m_mpFadeData looking for NPC name match, return event type
    (void)sName;
    return 0;
}

auto MapLoadable::GetCurrentObject(const std::string& sName) const -> std::shared_ptr<WzGr2DLayer>
{
    // 0xbf5d00 — CMapLoadable::GetCurrentObject
    auto it = m_mNamedObj.find(sName);
    if (it == m_mNamedObj.end())
        return nullptr;

    auto& obj = it->second;
    if (obj.nState < 0 || static_cast<std::size_t>(obj.nState) >= obj.aState.size())
        return nullptr;

    return obj.aState[obj.nState].pLayer;
}

auto MapLoadable::GetObjectSN(const std::string& sName) const -> std::uint32_t
{
    // 0xbf6460 — CMapLoadable::GetObjectSN
    auto it = m_mNamedObj.find(sName);
    if (it == m_mNamedObj.end())
        return 0;
    return it->second.dwSN;
}

auto MapLoadable::GetObjectState(const std::string& sName) const -> std::int32_t
{
    // 0xbf6490 — CMapLoadable::GetObjectState
    auto it = m_mNamedObj.find(sName);
    if (it == m_mNamedObj.end())
        return -1;
    return it->second.nState;
}

auto MapLoadable::GetObjectRect(const std::string& sName) const -> Rect
{
    // 0xbf64c0 — CMapLoadable::GetObjectRect
    // Get bounding rect from object's current state layer
    auto it = m_mNamedObj.find(sName);
    if (it == m_mNamedObj.end())
        return {};

    auto& obj = it->second;
    if (obj.nState < 0 || static_cast<std::size_t>(obj.nState) >= obj.aState.size())
        return {};

    auto& layer = obj.aState[obj.nState].pLayer;
    if (!layer)
        return {};

    auto lt = layer->GetLeftTop();
    auto rb = layer->GetRightBottom();
    return {lt.x, lt.y, rb.x, rb.y};
}

auto MapLoadable::TransientLayer_Exist() const -> bool
{
    // 0x1aa2fe0 — CMapLoadable::TransientLayer_Exist
    // IDA: return this->m_lpLayerTransient._m_uCount != 0
    return !m_lpLayerTransient.empty();
}

// ========================================================================
// Layer / Visual
// ========================================================================

void MapLoadable::SetGrayBackGround(bool bGray)
{
    // 0xbdd070 — CMapLoadable::SetGrayBackGround
    // Iterate all background layer lists and set gray filter on each layer
    for (auto& [key, pList] : m_mlLayerBack)
    {
        if (!pList) continue;
        for (auto& layer : *pList)
        {
            if (!layer) continue;
            if (bGray)
                layer->setFlags(8); // gray flag
            else
                layer->clearFlags(8);
        }
    }
}

void MapLoadable::SetBackGroundColor(std::int32_t nR, std::int32_t nG, std::int32_t nB,
                                      std::int32_t tDelay)
{
    // 0xbdd3c0 — CMapLoadable::SetBackGroundColor
    // Apply pixel shader color change to all background layers
    // In IDA: iterates m_mlLayerBack, calls FieldObjectLayerPixelShader::PushPixelShader
    // for each layer. We store the color for later application.
    (void)nR;
    (void)nG;
    (void)nB;
    (void)tDelay;
    // TODO: Requires FieldObjectLayerPixelShader infrastructure
}

void MapLoadable::SetBackGroundColorByTag(const std::string& sTag, std::int32_t nR,
                                           std::int32_t nG, std::int32_t nB, std::int32_t tDelay)
{
    // 0xbdfab0 — CMapLoadable::SetBackGroundColorByTag
    // Apply pixel shader color change only to tagged background layers
    auto it = m_mTagedBack.find(sTag);
    if (it == m_mTagedBack.end() || !it->second || it->second->empty())
        return;

    (void)nR;
    (void)nG;
    (void)nB;
    (void)tDelay;
    // TODO: Requires FieldObjectLayerPixelShader infrastructure
}

void MapLoadable::SetObjectVisible(const std::string& sName, bool bVisible)
{
    // 0xbdfc50 — CMapLoadable::SetObjectVisible
    // Look up named layer in m_mpLayerObj and set its visibility
    auto it = m_mpLayerObj.find(sName);
    if (it == m_mpLayerObj.end() || !it->second)
        return;

    it->second->SetVisible(bVisible);
}

void MapLoadable::SetObjectMove(const std::string& sName, std::int32_t nX,
                                 std::int32_t nY, std::int32_t tTime)
{
    // 0xbdfd20 — CMapLoadable::SetObjectMove
    // Look up named layer, get current position, then RelMove to target offset
    auto it = m_mpLayerObj.find(sName);
    if (it == m_mpLayerObj.end() || !it->second)
        return;

    auto& layer = it->second;
    auto curTime = layer->GetCurrentTime();
    auto curX = layer->GetX();
    auto curY = layer->GetY();

    // RelMove: animate from current position to (nX + curX, nY + curY) over tTime
    layer->RelMove(nX + curX, nY + curY, curTime, curTime + tTime);
}

void MapLoadable::SetObjectCreateLayer(const std::string& sKeyName, const std::string& sPath,
                                        std::uint32_t nX, std::uint32_t nY)
{
    // 0xbe8570 — CMapLoadable::SetObjectCreateLayer
    // Creates a new object layer from the given property path and adds to transient list
    if (sKeyName.empty() || sPath.empty())
        return;

    // Resolve the property from the path
    auto& rm = WzResMan::GetInstance();
    auto pProp = rm.GetProperty(sPath);
    if (!pProp) return;

    // Create layer and load animation frames
    auto layer = CreateLayer(0);
    if (!layer) return;

    auto frameCount = LoadAnimatedLayer(layer, pProp);
    if (frameCount == 0)
    {
        LoadStaticLayer(layer, pProp);
    }

    layer->SetPosition(static_cast<std::int32_t>(nX), static_cast<std::int32_t>(nY));

    if (frameCount > 1)
        layer->Animate(Gr2DAnimationType::Repeat);

    m_lpLayerTransient.push_back(layer);
}

void MapLoadable::SetObjectState(const std::string& sName, std::int32_t nState)
{
    // 0xbf5840 — CMapLoadable::SetObjectState
    // Switch named object state: fade old state alpha to 0, set new state alpha to 255
    auto it = m_mNamedObj.find(sName);
    if (it == m_mNamedObj.end())
        return;

    auto& obj = it->second;

    if (nState != -1)
    {
        if (nState < 0 || static_cast<std::size_t>(nState) >= obj.aState.size())
            return;

        // Fade out old state layer: set alpha to 0 immediately
        if (obj.nState >= 0 && static_cast<std::size_t>(obj.nState) < obj.aState.size())
        {
            auto& oldLayer = obj.aState[obj.nState].pLayer;
            if (oldLayer)
            {
                auto* alphaVec = oldLayer->get_alpha();
                if (alphaVec) alphaVec->Move(0, 0);
            }
        }

        obj.nState = nState;
    }

    // Set new state alpha to 255 and animate
    if (obj.nState >= 0 && static_cast<std::size_t>(obj.nState) < obj.aState.size())
    {
        auto& state = obj.aState[obj.nState];
        if (state.pLayer)
        {
            auto* alphaVec = state.pLayer->get_alpha();
            if (alphaVec) alphaVec->Move(255, 0);

            // Play sound effect if set
            if (!state.bsSfx.empty())
            {
                SoundMan::GetInstance().PlayFieldSound(state.bsSfx, 100);
            }

            // Animate the layer
            AnimateObjLayer(state.pLayer, state.nRepeat);
        }
    }
}

void MapLoadable::SetLayerInvisible(const std::string& sTag, std::int32_t tDelay,
                                     std::int32_t bVisible, std::int32_t nManual,
                                     std::int32_t bSmooth)
{
    // 0xbf8700 — CMapLoadable::SetLayerInvisible
    // Enqueue a delayed visibility change for a tagged layer
    DelayInvisibleLayer entry;
    entry.sTag = sTag;
    entry.tDelayTime = tDelay;
    entry.tStartTime = static_cast<std::int32_t>(get_gr().GetCurrentTime());
    entry.bVisible = bVisible;
    entry.nManual = nManual;
    entry.bSmooth = bSmooth;

    m_aDelayInvisibleLayer.push_back(std::move(entry));
}

void MapLoadable::SetLayerListVisible(
    const std::shared_ptr<std::list<std::shared_ptr<WzGr2DLayer>>>& plLayer,
    std::int32_t bVisible, bool bSmooth, std::int32_t nManual,
    const std::string& sTag)
{
    // 0xbf8e00 — CMapLoadable::SetLayerListVisible
    // Iterate layer list, set visibility with optional smooth alpha animation
    if (!plLayer) return;

    auto tSmooth = bSmooth ? 2000 : 0;

    for (auto& layer : *plLayer)
    {
        if (!layer) continue;

        if (bVisible)
        {
            layer->SetVisible(true);

            // Animate alpha to 255 (fully visible)
            auto curTime = layer->GetCurrentTime();
            auto* alphaVec = layer->get_alpha();
            if (alphaVec)
                alphaVec->RelMove(255, 255, curTime, curTime + tSmooth);
        }
        else
        {
            // Animate alpha to 0 (invisible)
            auto curTime = layer->GetCurrentTime();
            auto* alphaVec = layer->get_alpha();
            if (alphaVec)
                alphaVec->RelMove(0, 0, curTime, curTime + tSmooth);

            // If nManual == 2, enqueue delayed full hide
            if (nManual == 2 && tSmooth > 0)
            {
                SetLayerInvisible(sTag, tSmooth, 0, 2, bSmooth ? 1 : 0);
            }
            else if (tSmooth == 0)
            {
                layer->SetVisible(false);
            }
        }
    }
}

void MapLoadable::SetLayerListVisibleByTag(
    const std::string& sTag,
    const std::shared_ptr<std::list<std::shared_ptr<WzGr2DLayer>>>& plObjs)
{
    // 0xbfb180 — CMapLoadable::SetLayerListVisibleByTag
    // Quest-aware visibility: checks quest state for the tag and calls SetLayerListVisible
    // For now, just make layers visible (quest system not yet implemented)
    SetLayerListVisible(plObjs, 1, false, 0, sTag);
}

void MapLoadable::SetMapTagedObjectVisible(const std::string& sTag, std::int32_t bVisible,
                                            std::int32_t bSmooth, std::int32_t tDuration)
{
    // 0xbfb440 — CMapLoadable::SetMapTagedObjectVisible
    // Find tagged object list and set visibility on all layers in it
    (void)tDuration;

    // Check tagged objects (m_mTagedObj)
    auto itObj = m_mTagedObj.find(sTag);
    if (itObj != m_mTagedObj.end() && itObj->second)
    {
        SetLayerListVisible(itObj->second, bVisible, bSmooth != 0, 0, sTag);
    }

    // Check tagged back (m_mTagedBack)
    auto itBack = m_mTagedBack.find(sTag);
    if (itBack != m_mTagedBack.end() && itBack->second)
    {
        SetLayerListVisible(itBack->second, bVisible, bSmooth != 0, 0, sTag);
    }
}

void MapLoadable::SetFieldMagLevel()
{
    // 0xbffcc0 — CMapLoadable::SetFieldMagLevel
    // Read mag level from config, if changed flush cache and re-restore
    if (!m_pPropField) return;

    // TODO: Read nMagLevel_Obj and nMagLevel_Back from CConfig when available
    // For now, keep current values
    std::int32_t nMagLevel_Obj = m_nMagLevel_Obj;
    std::int32_t nMagLevel_Back = m_nMagLevel_Back;

    bool bCacheFlushed = false;

    if (m_nMagLevel_Obj != nMagLevel_Obj)
    {
        m_nMagLevel_Obj = nMagLevel_Obj;
        m_lpLayerObj.clear();
        bCacheFlushed = true;
        m_bMagLevelModifying = true;
        RestoreObj(false);
        m_bMagLevelModifying = false;
    }

    if (m_nMagLevel_Back != nMagLevel_Back)
    {
        m_nMagLevel_Back = nMagLevel_Back;
        m_mlLayerBack.clear();

        auto& gr = get_gr();
        gr.ResetCameraPosition(0, 0);

        RestoreBack(false);
        RestoreBackEffect();
    }
}

void MapLoadable::TransientLayer_Clear()
{
    // 0xbd9850 — CMapLoadable::TransientLayer_Clear
    // Fade out transient layers with random timing, then conditionally remove all
    if (m_lpLayerTransient.empty())
        return;

    auto& gr = get_gr();

    for (auto& layer : m_lpLayerTransient)
    {
        if (!layer) continue;

        // Get current time and add random offset for staggered fade
        auto curTime = layer->GetCurrentTime();
        auto fadeEndTime = curTime + 1000 + (detail::get_rand().Random() % 1000);

        // Animate alpha from current to 0
        auto* alphaVec = layer->get_alpha();
        if (alphaVec)
            alphaVec->RelMove(0, 255, curTime, fadeEndTime);
    }

    // If weather fade-in is still active, remove all immediately
    if (m_nWeatherFadeInTime > static_cast<std::int32_t>(gr.GetCurrentTime()))
    {
        for (auto& layer : m_lpLayerTransient)
        {
            if (layer) gr.RemoveLayer(layer);
        }
        m_lpLayerTransient.clear();
    }
}

void MapLoadable::TransientLayer_Weather(std::int32_t nItemID, const std::string& sMsg)
{
    // 0xbe3da0 — CMapLoadable::TransientLayer_Weather
    // TODO: Complex — creates weather effect from item property, adds transient layers
    (void)nItemID;
    (void)sMsg;
}

// ========================================================================
// BGM / Sound
// ========================================================================

void MapLoadable::PlayNextMusic()
{
    // 0xbee210 — CMapLoadable::PlayNextMusic
    // Stop current BGM, then either restore from map or play jukebox item
    auto& sm = SoundMan::GetInstance();
    sm.StopBGM();

    if (m_nJukeBoxItemID == -1)
    {
        // Restore normal BGM
        RestoreBGM(true);
        m_bJukeBoxPlaying = 0;
        m_nJukeBoxItemID = 0;
    }
    else if (m_nJukeBoxItemID != 0)
    {
        // TODO: Look up jukebox item BGM path from CItemInfo and play it
        // For now, just mark as playing and clear
        m_bJukeBoxPlaying = 1;
        m_nJukeBoxItemID = 0;
    }
}

void MapLoadable::PlaySoundWithMuteBgm(const std::string& sName, bool bExcl,
                                        bool bDown, std::uint32_t uVolume128)
{
    // 0xbee8d0 — CMapLoadable::PlaySoundWithMuteBgm
    // Play a sound effect while temporarily reducing BGM volume
    if (sName.empty()) return;

    auto& sm = SoundMan::GetInstance();

    if (!m_bBGMVolumeOnly)
    {
        // Calculate muted volume
        std::uint32_t mutedVol = 0;
        if (bDown)
            mutedVol = 60 * m_nRestoreBgmVolume / 100;

        sm.SetBGMVolume(mutedVol, 0);

        // Schedule BGM volume restore
        auto& gr = get_gr();
        m_tRestoreBgmVolume = static_cast<std::int32_t>(gr.GetCurrentTime()) + 500;
    }

    // Play the sound
    sm.PlayFieldSound(sName, uVolume128);
    (void)bExcl;
}

void MapLoadable::SetCameraMoveInfo(const std::string& sMoveType)
{
    // 0xbe8430 — CMapLoadable::SetCameraMoveInfo (string overload)
    // Look up camera move info by name from CCameraMoveMan and apply
    if (sMoveType.empty()) return;

    // TODO: Requires CCameraMoveMan singleton
    // When available, look up RawCameraMoveInfo by sMoveType and apply to m_cameraMoveInfo
    (void)sMoveType;
}

// ========================================================================
// IDA Stubs — Foothold
// ========================================================================

void MapLoadable::FootHoldMove(std::int32_t nSN, std::int32_t nX, std::int32_t nY)
{
    // 0xbf54a0 — CMapLoadable::FootHoldMove
    // TODO: stub — move foothold position
    (void)nSN;
    (void)nX;
    (void)nY;
}

void MapLoadable::FootHoldStateChange(std::int32_t nSN, std::int32_t nState)
{
    // 0xbf56d0 — CMapLoadable::FootHoldStateChange
    // TODO: stub — change foothold state (enable/disable)
    (void)nSN;
    (void)nState;
}

// ========================================================================
// IDA Stubs — Rendering
// ========================================================================

void MapLoadable::RenderAvatar()
{
    // 0xbde930 — CMapLoadable::RenderAvatar
    // TODO: stub — render avatar into reflection canvas
}

void MapLoadable::ProcessReflection()
{
    // 0xbdf2b0 — CMapLoadable::ProcessReflection
    // TODO: stub — process reflection info list
}

// ========================================================================
// IDA Stubs — Fade
// ========================================================================

void MapLoadable::SetFadeData(const std::shared_ptr<WzGr2DLayer>& pLayer,
                               std::int32_t nAlpha, std::int32_t tDuration)
{
    // 0xbef2e0 — CMapLoadable::SetFadeData (layer overload)
    // TODO: stub — set fade alpha on layer
    (void)pLayer;
    (void)nAlpha;
    (void)tDuration;
}

void MapLoadable::SetFadeData(std::int32_t nIndex, std::int32_t nAlpha, std::int32_t tDuration)
{
    // 0xbef560 — CMapLoadable::SetFadeData (index overload)
    // TODO: stub
    (void)nIndex;
    (void)nAlpha;
    (void)tDuration;
}

// ========================================================================
// IDA Stubs — Event Handlers
// ========================================================================

void MapLoadable::OnLeaveDirectionMode()
{
    // 0xbd5140 — CMapLoadable::OnLeaveDirectionMode
    // If jukebox is idle, set it to -1 to trigger restore on next music call
    if (m_nJukeBoxItemID == 0)
        m_nJukeBoxItemID = -1;

    // Mute BGM with fade out (1500ms)
    SoundMan::GetInstance().SetBGMVolume(0, 1500);

    // Schedule next music in 2500ms
    m_tNextMusic = static_cast<std::int32_t>(get_gr().GetCurrentTime()) + 2500;
}

void MapLoadable::OnSetBackEffect(const std::string& sName, std::int32_t nEffect)
{
    // 0xbe14e0 — CMapLoadable::OnSetBackEffect
    // nEffect == 0: add back effect (fade alpha from current to 255)
    // nEffect == 1: remove back effect (fade alpha from current to 0)
    // IDA shows BackEffect struct with nPageID and tDuration decoded from packet
    // Since we take already-decoded parameters, interpret sName as a page identifier

    // TODO: Full implementation requires BackEffect packet decoding
    // For now, the effect name and type are noted for future implementation
    (void)sName;
    (void)nEffect;
}

void MapLoadable::OnSetSpineBackEffect(const std::string& sName)
{
    // 0xbeb210 — CMapLoadable::OnSetSpineBackEffect
    // TODO: stub
    (void)sName;
}

void MapLoadable::OnSetSpineObjectEffect(const std::string& sName)
{
    // 0xbeb430 — CMapLoadable::OnSetSpineObjectEffect
    // TODO: stub
    (void)sName;
}

void MapLoadable::OnRemoveSpineRectEvent(const std::string& sName)
{
    // 0xbeb5f0 — CMapLoadable::OnRemoveSpineRectEvent
    // Remove spine event zone and associated rect event data
    m_mpSpineEventZoneData.erase(sName);
    m_mpRectEventData.erase(sName);
}

void MapLoadable::OnRemoveCameraCtrlZone(const std::string& sName)
{
    // 0xbeb730 — CMapLoadable::OnRemoveCameraCtrlZone
    // Remove camera control zone and associated rect event data
    m_mpCameraCtrlZoneData.erase(sName);
    m_mpRectEventData.erase(sName);
}

void MapLoadable::OnSetMapObjectAnimation(const std::string& sName, std::int32_t nAniType)
{
    // 0xbec280 — CMapLoadable::OnSetMapObjectAnimation
    SetObjectAnimation(sName, static_cast<Gr2DAnimationType>(nAniType));
}

void MapLoadable::OnSetMapTaggedObjectAnimation(const std::string& sTag, std::int32_t nAniType)
{
    // 0xbec320 — CMapLoadable::OnSetMapTaggedObjectAnimation
    SetTaggedObjectAnimation(sTag, static_cast<Gr2DAnimationType>(nAniType));
}

void MapLoadable::OnSetMapObjectVisible(const std::string& sName, bool bVisible)
{
    // 0xbec3e0 — CMapLoadable::OnSetMapObjectVisible
    SetObjectVisible(sName, bVisible);
}

void MapLoadable::OnSetMapObjectMove(const std::string& sName, std::int32_t nX,
                                      std::int32_t nY, std::int32_t tDuration)
{
    // 0xbec4a0 — CMapLoadable::OnSetMapObjectMove
    SetObjectMove(sName, nX, nY, tDuration);
}

void MapLoadable::OnSetMapObjectCreateLayer(const std::string& sKeyName,
                                              const std::string& sPath,
                                              std::uint32_t nX, std::uint32_t nY)
{
    // 0xbec570 — CMapLoadable::OnSetMapObjectCreateLayer
    if (sKeyName.empty()) return;
    SetObjectCreateLayer(sKeyName, sPath, nX, nY);
}

void MapLoadable::OnClearBackEffect()
{
    // 0xbfc160 — CMapLoadable::OnClearBackEffect
    // IDA shows this is a thunk that just calls ReloadBack
    ReloadBack();
}

void MapLoadable::OnSetMapTagedObjectVisible(const std::string& sTag,
                                               std::int32_t bVisible,
                                               std::int32_t tDuration,
                                               std::int32_t tDelay)
{
    // 0xbfc170 — CMapLoadable::OnSetMapTagedObjectVisible
    // IDA: if tDelay != 0, enqueue delayed visibility change; else immediate
    if (tDelay != 0)
    {
        SetLayerInvisible(sTag, tDelay, bVisible, tDuration, 0);
    }
    else
    {
        SetMapTagedObjectVisible(sTag, bVisible, 0, tDuration);
    }
}

void MapLoadable::OnSetMapTaggedObjectSmoothVisible(const std::string& sTag,
                                                     std::int32_t bVisible,
                                                     std::int32_t tDuration,
                                                     std::int32_t tDelay)
{
    // 0xbfc270 — CMapLoadable::OnSetMapTaggedObjectSmoothVisible
    // Same as OnSetMapTagedObjectVisible but with bSmooth=1
    if (tDelay != 0)
    {
        SetLayerInvisible(sTag, tDelay, bVisible, tDuration, 1);
    }
    else
    {
        SetMapTagedObjectVisible(sTag, bVisible, 1, tDuration);
    }
}

void MapLoadable::OnEventChangeScreenResolution()
{
    // 0xbfc370 — CMapLoadable::OnEventChangeScreenResolution
    if (!m_pPropField) return;

    // IDA: calls g_gr->raw_SetFrameSkip() — skip for now, not in our WzGr2D API

    RestoreViewRange();
    ReloadBack();
    RestoreBackEffect();

    // Clear and recreate letterbox layers
    m_lpLayerLetterBox.clear();
    RestoreLetterBox();

    SetGrayBackGround(false); // TODO: read from CWvsContext::m_bBackGrayScale when available
}

void MapLoadable::OnPacket(std::int32_t nType, const void* pData)
{
    // 0xbfc460 — CMapLoadable::OnPacket
    // TODO: stub — dispatch packet by type
    (void)nType;
    (void)pData;
}

void MapLoadable::OnCreateSpineRectEvent(const std::string& sName)
{
    // 0xbf8a60 — CMapLoadable::OnCreateSpineRectEvent
    // TODO: stub
    (void)sName;
}

void MapLoadable::OnCreateCameraCtrlZone(const std::string& sName)
{
    // 0xbf8bf0 — CMapLoadable::OnCreateCameraCtrlZone
    // TODO: stub
    (void)sName;
}

void MapLoadable::OnSpineRE_AddBackEvent(const std::string& sName)
{
    // 0xbee530 — CMapLoadable::OnSpineRE_AddBackEvent
    // TODO: stub
    (void)sName;
}

void MapLoadable::OnSpineRE_AddObjectEvent(const std::string& sName)
{
    // 0xbee700 — CMapLoadable::OnSpineRE_AddObjectEvent
    // TODO: stub
    (void)sName;
}

// ========================================================================
// IDA Stubs — Make / Create (protected)
// ========================================================================

void MapLoadable::MakeObjSkeleton(std::int32_t nPageIdx,
                                   const std::shared_ptr<WzProperty>& pPiece,
                                   bool bLoad)
{
    // 0xbe07c0 — CMapLoadable::MakeObjSkeleton
    // TODO: stub — create skeleton-animated object layer
    (void)nPageIdx;
    (void)pPiece;
    (void)bLoad;
}

void MapLoadable::MakeObjLayer(std::int32_t nPageIdx,
                                const std::shared_ptr<WzProperty>& pPiece,
                                std::shared_ptr<WzGr2DLayer>& pOutLayer)
{
    // 0xbed9b0 — CMapLoadable::MakeObjLayer
    // TODO: stub — create object layer, output layer pointer
    (void)nPageIdx;
    (void)pPiece;
    pOutLayer = nullptr;
}

void MapLoadable::MakeVectorAnimate(const std::shared_ptr<WzGr2DLayer>& pLayer,
                                     const std::shared_ptr<WzProperty>& pProp)
{
    // 0xbe2990 — CMapLoadable::MakeVectorAnimate
    // TODO: stub — set up vector animation on layer from property
    (void)pLayer;
    (void)pProp;
}

void MapLoadable::MakeObstacles()
{
    // 0xbea8c0 — CMapLoadable::MakeObstacles
    // TODO: stub — create obstacle objects from m_pPropField
}

void MapLoadable::MakeGrid(const std::shared_ptr<WzGr2DLayer>& pLayer,
                            std::int32_t type,
                            std::int32_t cx,
                            std::int32_t cy,
                            std::int32_t alpha,
                            std::int32_t nAnimate,
                            bool bObj,
                            std::shared_ptr<std::list<std::shared_ptr<WzGr2DLayer>>>& pList,
                            const std::vector<std::string>& aTagList,
                            const std::string& sGroupName,
                            std::int32_t nSideType)
{
    // 0xbeff40 — CMapLoadable::MakeGrid
    // Original creates N*M cloned layers in a grid, each with copied frames, positioned at
    // grid offsets, with per-clone Ratio for parallax wrapping.
    // Our engine uses render-time tiling via SetTiling which achieves the same visual result.
    (void)nSideType;

    if (!pLayer)
        return;

    // --- Step 1: Get canvas dimensions from layer (0xbeffbf-0xbf004e) ---
    auto canvas = pLayer->GetCurrentCanvas();
    auto canvasW = canvas ? static_cast<std::int32_t>(canvas->GetWidth()) : 0;
    auto canvasH = canvas ? static_cast<std::int32_t>(canvas->GetHeight()) : 0;

    // --- Step 2: Compute tile dimensions (0xbf0063-0xbf01a4) ---
    // tileW: cx if provided, else MBR width (bObj) or canvas width
    std::int32_t tileW = cx;
    if (tileW == 0)
    {
        if (bObj && m_pSpace2D)
        {
            auto& mbr = m_pSpace2D->GetMBR();
            tileW = mbr.right - mbr.left;
        }
        else
        {
            tileW = canvasW;
        }
    }

    // tileH: cy if provided, else MBR height (bObj) or canvas height
    std::int32_t tileH = cy;
    if (tileH == 0)
    {
        if (bObj && m_pSpace2D)
        {
            auto& mbr = m_pSpace2D->GetMBR();
            tileH = mbr.bottom - mbr.top;
        }
        else
        {
            tileH = canvasH;
        }
    }

    // --- Step 3: Get screen dimensions and apply zoom (0xbf01b0-0xbf0227) ---
    auto& gr = get_gr();
    auto screenW = static_cast<std::int32_t>(gr.GetWidth());
    auto screenH = static_cast<std::int32_t>(gr.GetHeight());

    // Original reads mag level and scales screen dims: scale = 1000.0 / magLevel
    // magLevel=1000 means 100% zoom (no scaling), <1000 = zoomed out (more area visible)
    auto nMagLevel = bObj ? m_nMagLevel_Obj : m_nMagLevel_Back;
    if (nMagLevel != 0 && nMagLevel != 1000)
    {
        auto scale = 1000.0f / static_cast<float>(nMagLevel);
        screenW = static_cast<std::int32_t>(static_cast<float>(screenW) * scale);
        screenH = static_cast<std::int32_t>(static_cast<float>(screenH) * scale);
    }

    // --- Step 4: Compute grid coverage (0xbf0229-0xbf028d) ---
    // totalW = tileW * ceil((screenW + 2*tileW) / tileW) if horizontal tiling
    std::int32_t totalW = 0;
    if (type & 1)
    {
        auto tw = tileW > 0 ? tileW : 1;
        totalW = tileW * ((screenW + 2 * tileW - 2) / tw);
    }

    std::int32_t totalH = 0;
    if (type & 2)
    {
        auto th = tileH > 0 ? tileH : 1;
        totalH = tileH * ((screenH + 2 * tileH - 2) / th);
    }

    // --- Step 5: Apply tiling via engine (replaces original's clone loop 0xbf02b9-0xbf1525) ---
    // Original creates tileW*totalW × tileH*totalH grid of cloned layers.
    // Each clone copies frames, sets RelMove offset, Ratio for parallax, alpha, flip, blend.
    // Our engine achieves the same visual result with render-time tiling.
    auto effectiveTileW = (type & 1) ? tileW : 0;
    auto effectiveTileH = (type & 2) ? tileH : 0;
    pLayer->SetTiling(effectiveTileW, effectiveTileH);

    // --- Step 6: Alpha (original applies per-clone at 0xbf10a9-0xbf1247) ---
    // Original: Getalpha().RelMove(alpha, 255) on each clone
    // In our engine, alpha is already set in MakeBack via SetColor before calling MakeGrid.
    (void)alpha;

    // --- Step 7: Animation (0xbf1308-0xbf13ca or 0xbf12b9-0xbf12c3) ---
    // Original: if sGroupName: AnimateObjLayer(clone, nAnimate)
    //           else if nAnimate: clone.Animate(Repeat)
    if (!sGroupName.empty())
    {
        AnimateObjLayer(pLayer, nAnimate);
    }
    else if (nAnimate)
    {
        pLayer->Animate(Gr2DAnimationType::Repeat);
    }

    // --- Step 8: Tag insertion (0xbf1482-0xbf14a4) ---
    // Original: InsertbackLayerByTag(pTagList, clone) for each clone
    if (!aTagList.empty())
    {
        InsertbackLayerByTag(aTagList, pLayer);
    }

    // --- Step 9: Add to output list ---
    if (pList)
    {
        pList->push_back(pLayer);
    }

    LOG_DEBUG("MakeGrid: type={}, tile={}x{} (canvas={}x{}, cx={}, cy={}, grid={}x{}, bObj={})",
              type, effectiveTileW, effectiveTileH, canvasW, canvasH, cx, cy,
              totalW, totalH, bObj);
}

void MapLoadable::MakeGridSkeleton(std::int32_t nPageIdx,
                                    const std::shared_ptr<WzProperty>& pPiece,
                                    bool bLoad)
{
    // 0xbf1640 — CMapLoadable::MakeGridSkeleton
    // TODO: stub — create grid-tiled skeleton object
    (void)nPageIdx;
    (void)pPiece;
    (void)bLoad;
}

// ========================================================================
// IDA Stubs — Restore / Load / Reload (protected)
// ========================================================================

void MapLoadable::RestoreBackEffect()
{
    // 0xbdcb60 — CMapLoadable::RestoreBackEffect
    // Iterate back effect page IDs, look up their layer lists, and animate alpha to 255
    for (auto nPageID : m_lBackEffect)
    {
        auto it = m_mlLayerBack.find(nPageID);
        if (it == m_mlLayerBack.end() || !it->second) continue;

        for (auto& layer : *it->second)
        {
            if (!layer) continue;

            auto* alphaVec = layer->get_alpha();
            if (!alphaVec) continue;

            // Get current alpha value, then animate from current to 255 (instant)
            alphaVec->Move(255, 0);
        }
    }
}

void MapLoadable::RestoreBGM(bool bForceRestart)
{
    // 0xbeb070 — CMapLoadable::RestoreBGM
    // If no changed BGM, play from map info
    if (m_sChangedBgmUOL.empty())
    {
        PlayBGMFromMapInfo();
        return;
    }

    // Convert u16string to narrow string for SoundMan API
    std::string sPath(m_sChangedBgmUOL.begin(), m_sChangedBgmUOL.end());

    // Play the changed BGM
    auto& sm = SoundMan::GetInstance();
    // IDA: PlayBGM(path, repeat=true, vol=1000, vol=1000, bForceRestart, 0)
    // Map to our API: (sPath, nLoop=-1, startVol=128, endVol=128, fadeInTime, fadeOutTime)
    sm.PlayBGM(sPath, -1, 128, 128, bForceRestart ? 0 : 1000, 0);
}

void MapLoadable::ReloadBack()
{
    // 0xbfc000 — CMapLoadable::ReloadBack
    // Clear all back layers
    m_mlLayerBack.clear();

    // Reset camera center position
    auto& gr = get_gr();
    gr.ResetCameraPosition(0, 0);

    // Restore back layers without loading resources (they are cached)
    RestoreBack(false);
}

void MapLoadable::LoadBgmSubInfo(const std::shared_ptr<WzProperty>& pProp)
{
    // 0xbf83d0 — CMapLoadable::LoadBgmSubInfo
    // TODO: stub — parse sub-BGM info from property
    (void)pProp;
}

void MapLoadable::LoadBgmSub()
{
    // 0xbfadc0 — CMapLoadable::LoadBgmSub
    // TODO: stub — load sub-BGM tracks from m_mSubBgm
}

void MapLoadable::InsertbackLayerByTag(const std::vector<std::string>& tags,
                                        const std::shared_ptr<WzGr2DLayer>& pLayer)
{
    // 0xbef690 — CMapLoadable::InsertbackLayerByTag
    // Iterate tag list, for each tag get or create a list in m_mTagedBack and add the layer
    if (!pLayer) return;

    for (const auto& sTag : tags)
    {
        if (sTag.empty()) continue;

        auto& pList = m_mTagedBack[sTag];
        if (!pList)
            pList = std::make_shared<std::list<std::shared_ptr<WzGr2DLayer>>>();

        pList->push_back(pLayer);
    }
}

void MapLoadable::InsertbackSkeletonByTag(const std::vector<std::string>& tags,
                                           const std::shared_ptr<WzGr2DLayer>& pLayer)
{
    // 0xbef8b0 — CMapLoadable::InsertbackSkeletonByTag
    // Same pattern as InsertbackLayerByTag for skeleton layers
    if (!pLayer) return;

    for (const auto& sTag : tags)
    {
        if (sTag.empty()) continue;

        auto& pList = m_mTagedBack[sTag];
        if (!pList)
            pList = std::make_shared<std::list<std::shared_ptr<WzGr2DLayer>>>();

        pList->push_back(pLayer);
    }
}

// ========================================================================
// IDA Stubs — Update (protected)
// ========================================================================

void MapLoadable::UpdateObstacleInfo()
{
    // 0xbe7410 — CMapLoadable::UpdateObstacleInfo
    // TODO: stub — rebuild m_aObstacleInfo from m_lpObstacle
    m_aObstacleInfo.clear();
}

void MapLoadable::UpdateTagLayer()
{
    // 0xbfc3f0 — CMapLoadable::UpdateTagLayer
    // TODO: stub — update both object and back tag layers
    UpdateObjectTagLayer();
    UpdateBackTagLayer();
}

void MapLoadable::UpdateLayerInvisible()
{
    // 0xbfba10 — CMapLoadable::UpdateLayerInvisible
    // Iterate delayed invisible entries, process those whose delay has elapsed
    auto tCur = static_cast<std::int32_t>(get_gr().GetCurrentTime());

    auto it = m_aDelayInvisibleLayer.begin();
    while (it != m_aDelayInvisibleLayer.end())
    {
        // Check if delay time has elapsed: tCur >= tStartTime + tDelayTime
        if ((tCur - it->tStartTime) >= it->tDelayTime)
        {
            // Apply the visibility change
            SetMapTagedObjectVisible(it->sTag, it->bVisible, it->bSmooth != 0, it->nManual);

            // Remove this entry
            it = m_aDelayInvisibleLayer.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace ms
