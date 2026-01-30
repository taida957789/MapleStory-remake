#include "MapLoadable.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "util/Logger.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

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

    auto& gr = get_gr();

    // Reset camera to world origin (0,0)
    // The renderer adds screen center offset, so camera at (0,0) shows world (0,0) at screen center
    gr.SetCameraPosition(0, 0);

    // Clear any existing layers
    ClearAllLayers();

    LOG_INFO("MapLoadable initialized");
}

void MapLoadable::Update()
{
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
    Stage::Close();

    // Clear all layers
    ClearAllLayers();

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
    if (IsSameChangeBGM(bgmPath))
    {
        return;
    }

    m_sChangedBgmUOL = bgmPath;

    // TODO: Implement actual BGM change via SoundSystem
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
        // TODO: Restore BGM volume via SoundSystem
        m_tRestoreBgmVolume = 0;
    }
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
        return false;
    }

    // Try to get canvas directly
    auto canvas = prop->GetCanvas();
    if (!canvas)
    {
        // Try to get canvas from first child (some properties wrap canvas in "0" child)
        auto firstChild = prop->GetChild("0");
        if (firstChild)
        {
            canvas = firstChild->GetCanvas();
        }
    }

    if (!canvas)
    {
        return false;
    }

    // Get origin if available
    auto originProp = prop->GetChild("origin");
    if (originProp)
    {
        auto vec = originProp->GetVector();
        canvas->SetOrigin({vec.x, vec.y});
    }

    // Insert canvas into layer (static, no animation)
    layer->InsertCanvas(canvas, 0, 255, 255);

    LOG_DEBUG("LoadStaticLayer: loaded canvas {}x{}", canvas->GetWidth(), canvas->GetHeight());

    return true;
}

void MapLoadable::UpdateObjectLayers()
{
    // Object layers are updated by WzGr2D during RenderFrame
    // This method can be used for custom update logic
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

    // Get the "back" property (string pool 0x9F5 = 2549)
    auto pBack = pPropField->GetChild("back");
    if (!pBack)
    {
        LOG_WARN("RestoreBack: No 'back' property found");
        return;
    }

    // Get number of background pieces
    auto count = pBack->GetChildCount();
    LOG_INFO("RestoreBack: Found {} background pieces", count);

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

    // Movement type (2566)
    auto moveTypeProp = pPiece->GetChild("moveType");
    // auto moveType = moveTypeProp ? moveTypeProp->GetInt(0) : 0;

    // View culling: Only load layers within a reasonable Y range from the initial view
    // The login screen has steps at 600 pixel intervals
    // For step 0, the camera Y is around -308, so we load layers roughly from +300 to -1500
    // This reduces memory usage significantly by not loading distant backgrounds
    constexpr std::int32_t VIEW_LOAD_DISTANCE = 2400;  // Load layers within +/- 2400 pixels of Y=0

    if (y < -VIEW_LOAD_DISTANCE || y > VIEW_LOAD_DISTANCE)
    {
        // Skip layers that are too far from the initial view
        // They can be loaded later if the user scrolls to those steps
        LOG_DEBUG("MakeBack[{}]: Culled (y={} outside +{})", nPageIdx, y, VIEW_LOAD_DISTANCE);
        return;
    }

    LOG_DEBUG("MakeBack[{}]: bS={}, no={}, ani={}, pos=({},{}), rx={}, ry={}, front={}, alpha={}",
              nPageIdx, bS, no, ani, x, y, rx, ry, front, static_cast<int>(alpha));

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
        if (spriteProp && spriteProp->HasChildren())
        {
            LOG_DEBUG("MakeBack[{}]: Found sprite at {}", nPageIdx, path);
            break;
        }
        spriteProp.reset();
    }

    if (!spriteProp)
    {
        LOG_ERROR("MakeBack[{}]: Failed to load {}", nPageIdx, subPath);
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

    // Store parallax info for later use (TODO: implement parallax scrolling)
    // layer->SetParallax(rx, ry, cx, cy);

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

} // namespace ms
