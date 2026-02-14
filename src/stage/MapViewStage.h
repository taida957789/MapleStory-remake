#pragma once

#include "MapLoadable.h"

#include <cstdint>
#include <string>

namespace ms
{

/**
 * @brief Map viewer stage for visual map verification
 *
 * Loads a map by ID and provides free camera movement for exploration.
 * Intended as a standalone test/debug tool.
 *
 * Controls:
 * - Arrow keys / WASD: move camera
 * - Shift: fast camera mode
 * - F: toggle free camera (unclamped from view range)
 * - R: reload current map
 * - ESC: exit
 */
class MapViewStage final : public MapLoadable
{
public:
    explicit MapViewStage(std::int32_t nMapId);
    ~MapViewStage() override;

    void Init(void* param) override;
    void Update() override;
    void Draw() override;
    void Close() override;

    void OnKeyDown(std::int32_t keyCode) override;

private:
    /// Poll keyboard state and move camera accordingly
    void UpdateCamera();

    /// Update window title with map info, camera pos, FPS
    void UpdateHUD();

    /// Resolve WZ properties for the given map ID
    [[nodiscard]] auto ResolveMapProperties() -> bool;

    /// Reload the current map
    void ReloadMap();

    std::int32_t m_nMapId{};
    bool m_bFreeCamera{true};     ///< Free camera (no view range clipping)
    std::int32_t m_nCameraSpeed{8};   ///< Pixels per tick (normal)
    std::int32_t m_nCameraSpeedFast{24}; ///< Pixels per tick (shift held)
};

} // namespace ms
