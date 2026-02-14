#include "MapViewStage.h"
#include "app/Application.h"
#include "graphics/WzGr2D.h"
#include "input/InputSystem.h"
#include "util/Logger.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

#include <SDL3/SDL.h>
#include <cstdio>
#include <string>

namespace ms
{

// VK codes (matching InputSystem SDL→VK mapping)
constexpr std::int32_t kVK_SHIFT  = 16;
constexpr std::int32_t kVK_ESCAPE = 27;
constexpr std::int32_t kVK_LEFT   = 37;
constexpr std::int32_t kVK_UP     = 38;
constexpr std::int32_t kVK_RIGHT  = 39;
constexpr std::int32_t kVK_DOWN   = 40;
constexpr std::int32_t kVK_A      = 0x41;
constexpr std::int32_t kVK_D      = 0x44;
constexpr std::int32_t kVK_F      = 0x46;
constexpr std::int32_t kVK_R      = 0x52;
constexpr std::int32_t kVK_S      = 0x53;
constexpr std::int32_t kVK_W      = 0x57;

MapViewStage::MapViewStage(std::int32_t nMapId)
    : m_nMapId(nMapId)
{
}

MapViewStage::~MapViewStage() = default;

void MapViewStage::Init(void* param)
{
    MapLoadable::Init(param);

    LOG_INFO("MapViewStage: Loading map {}", m_nMapId);

    if (!ResolveMapProperties())
    {
        LOG_ERROR("MapViewStage: Failed to resolve WZ properties for map {}", m_nMapId);
        return;
    }

    LoadMap();

    // Center camera at map center (midpoint of view range)
    auto* vrect = GetViewRangeRect();
    if (vrect && (vrect->left != 0 || vrect->right != 0))
    {
        auto cx = (vrect->left + vrect->right) / 2;
        auto cy = (vrect->top + vrect->bottom) / 2;
        get_gr().SetCameraPosition(cx, cy);
        LOG_INFO("MapViewStage: Camera centered at ({}, {}), view range: [{},{} - {},{}]",
                 cx, cy, vrect->left, vrect->top, vrect->right, vrect->bottom);
    }

    LOG_INFO("MapViewStage: Map {} loaded successfully", m_nMapId);
}

void MapViewStage::Update()
{
    MapLoadable::Update();
    UpdateCamera();
    UpdateHUD();
}

void MapViewStage::Draw()
{
    MapLoadable::Draw();
}

void MapViewStage::Close()
{
    MapLoadable::Close();
    LOG_INFO("MapViewStage: Closed");
}

void MapViewStage::OnKeyDown(std::int32_t keyCode)
{
    switch (keyCode)
    {
    case kVK_ESCAPE:
        Application::GetInstance().Shutdown();
        break;

    case kVK_F:
        m_bFreeCamera = !m_bFreeCamera;
        LOG_INFO("MapViewStage: Free camera {}", m_bFreeCamera ? "ON" : "OFF");
        break;

    case kVK_R:
        ReloadMap();
        break;

    default:
        break;
    }
}

void MapViewStage::UpdateCamera()
{
    auto& input = InputSystem::GetInstance();

    // Determine speed (shift = fast)
    auto speed = input.IsKeyPressed(kVK_SHIFT) ? m_nCameraSpeedFast : m_nCameraSpeed;

    std::int32_t dx = 0;
    std::int32_t dy = 0;

    if (input.IsKeyPressed(kVK_LEFT) || input.IsKeyPressed(kVK_A))
        dx -= speed;
    if (input.IsKeyPressed(kVK_RIGHT) || input.IsKeyPressed(kVK_D))
        dx += speed;
    if (input.IsKeyPressed(kVK_UP) || input.IsKeyPressed(kVK_W))
        dy -= speed;
    if (input.IsKeyPressed(kVK_DOWN) || input.IsKeyPressed(kVK_S))
        dy += speed;

    if (dx == 0 && dy == 0)
        return;

    auto& gr = get_gr();
    auto pos = gr.GetCameraPosition();
    pos.x += dx;
    pos.y += dy;

    // Clip to view range unless free camera
    if (!m_bFreeCamera)
    {
        auto* vrect = GetViewRangeRect();
        if (vrect && (vrect->left != 0 || vrect->right != 0))
        {
            if (pos.x < vrect->left) pos.x = vrect->left;
            if (pos.x > vrect->right) pos.x = vrect->right;
            if (pos.y < vrect->top) pos.y = vrect->top;
            if (pos.y > vrect->bottom) pos.y = vrect->bottom;
        }
    }

    gr.SetCameraPosition(pos);
}

void MapViewStage::UpdateHUD()
{
    auto* window = Application::GetInstance().GetWindow();
    if (!window)
        return;

    auto& gr = get_gr();
    auto pos = gr.GetCameraPosition();
    auto fps = gr.GetFps100() / 100;

    char title[256];
    std::snprintf(title, sizeof(title),
                  "MapViewer | Map: %d | Camera: (%d, %d) | FPS: %u | %s",
                  m_nMapId, pos.x, pos.y, fps,
                  m_bFreeCamera ? "FREE" : "CLIPPED");

    SDL_SetWindowTitle(window, title);
}

auto MapViewStage::ResolveMapProperties() -> bool
{
    auto& resMan = WzResMan::GetInstance();

    // Map ID → WZ path:
    //   Map area = mapId / 100000000
    //   Image path: Map/Map{area}/{mapId:09d}.img
    //   Info path:  Map/MapInfo.img/{mapId}
    auto area = m_nMapId / 100000000;

    char imgPath[128];
    std::snprintf(imgPath, sizeof(imgPath), "Map/Map%d/%09d.img", area, m_nMapId);

    char infoPath[128];
    std::snprintf(infoPath, sizeof(infoPath), "Map/MapInfo.img/%d", m_nMapId);

    LOG_INFO("MapViewStage: Resolving {} and {}", imgPath, infoPath);

    // Resolve main map property (contains tiles, objects, backgrounds, etc.)
    m_pPropField = resMan.GetProperty(imgPath);
    if (!m_pPropField)
    {
        LOG_ERROR("MapViewStage: Could not resolve map property: {}", imgPath);
        return false;
    }

    // Resolve map info property (contains VR bounds, BGM, etc.)
    m_pPropFieldInfo = resMan.GetProperty(infoPath);
    if (!m_pPropFieldInfo)
    {
        LOG_WARN("MapViewStage: Could not resolve map info: {} (non-fatal)", infoPath);
    }

    // Check for refBack (some maps reference another map's backgrounds)
    if (m_pPropFieldInfo)
    {
        auto refBackProp = m_pPropFieldInfo->GetChild("refBack");
        if (refBackProp)
        {
            auto refMapId = refBackProp->GetInt(0);
            if (refMapId > 0)
            {
                auto refArea = refMapId / 100000000;
                char refPath[128];
                std::snprintf(refPath, sizeof(refPath), "Map/Map%d/%09d.img", refArea, refMapId);
                m_pPropFieldRefBack = resMan.GetProperty(refPath);
                LOG_INFO("MapViewStage: Using refBack map {} for backgrounds", refMapId);
            }
        }
    }

    return true;
}

void MapViewStage::ReloadMap()
{
    LOG_INFO("MapViewStage: Reloading map {}", m_nMapId);

    // Save camera position
    auto& gr = get_gr();
    auto savedPos = gr.GetCameraPosition();

    // Clear and reload
    ClearAllLayers();
    m_mTaggedLayer.clear();

    if (ResolveMapProperties())
    {
        LoadMap();
    }

    // Restore camera
    gr.SetCameraPosition(savedPos);

    LOG_INFO("MapViewStage: Map reloaded");
}

} // namespace ms
