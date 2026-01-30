#pragma once

#ifdef MS_DEBUG_CANVAS

#include "util/Point.h"
#include "util/Singleton.h"

#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class WzGr2DLayer;

struct CanvasHitInfo
{
    std::string wzPath;
    std::string layerName;
    int zOrder = 0;
    int canvasIndex = 0;
};

/**
 * @brief Debug overlay for inspecting canvas WZ paths
 *
 * Shows a popup with WZ path when clicking on sprites.
 * When multiple sprites overlap, shows a selection list.
 */
class DebugOverlay final : public Singleton<DebugOverlay>
{
    friend class Singleton<DebugOverlay>;

public:
    /**
     * @brief Handle mouse click
     * @return true if event was consumed
     */
    auto OnMouseClick(int screenX, int screenY) -> bool;

    /**
     * @brief Handle key press
     * @return true if event was consumed
     */
    auto OnKeyDown(SDL_Keycode key) -> bool;

    /**
     * @brief Render the debug overlay
     * @param renderer SDL renderer
     */
    void Render(SDL_Renderer* renderer);

    /**
     * @brief Check if overlay is currently active
     */
    [[nodiscard]] auto IsActive() const noexcept -> bool { return m_bActive; }

    /**
     * @brief Register a layer for hit testing
     */
    void RegisterLayer(const std::shared_ptr<WzGr2DLayer>& layer, const std::string& name);

    /**
     * @brief Unregister a layer
     */
    void UnregisterLayer(const std::shared_ptr<WzGr2DLayer>& layer);

    /**
     * @brief Clear all registered layers
     */
    void ClearLayers();

private:
    DebugOverlay() = default;
    ~DebugOverlay() override = default;

    auto FindCanvasesAt(int screenX, int screenY) -> std::vector<CanvasHitInfo>;
    void RenderSelectionList(SDL_Renderer* renderer);
    void RenderInfoPopup(SDL_Renderer* renderer);
    void RenderText(SDL_Renderer* renderer, const std::string& text, int x, int y);
    void Close();

    struct LayerEntry
    {
        std::weak_ptr<WzGr2DLayer> layer;
        std::string name;
    };

    std::vector<LayerEntry> m_layers;
    std::vector<CanvasHitInfo> m_hitList;

    bool m_bActive = false;
    bool m_bShowingList = false;
    int m_nSelectedIndex = -1;
    Point2D m_clickPos{0, 0};
};

} // namespace ms

#endif // MS_DEBUG_CANVAS
