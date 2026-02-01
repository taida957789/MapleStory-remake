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
class UIElement;

struct CanvasHitInfo
{
    std::string wzPath;
    std::string layerName;
    int zOrder = 0;
    int canvasIndex = 0;
};

struct UIElementHitInfo
{
    UIElement* element = nullptr;        // UI element pointer
    std::string typeName;                // Type name (e.g., "UIButton")
    Point2D localPos{0, 0};              // Local position (relative to parent)
    Point2D absolutePos{0, 0};           // Absolute position (screen coords)
    int width = 0;                       // Width
    int height = 0;                      // Height
    int zOrder = 0;                      // Z-order
    std::string parentName;              // Parent element type name
    int childCount = 0;                  // Number of children
    std::vector<std::string> layerNames; // All layers of this element
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
     * @brief Register a UI element with its layer
     * @param element UI element pointer
     * @param layer Associated layer
     * @param layerName Name for this layer
     */
    void RegisterUIElement(UIElement* element,
                          const std::shared_ptr<WzGr2DLayer>& layer,
                          const std::string& layerName);

    /**
     * @brief Unregister a layer
     */
    void UnregisterLayer(const std::shared_ptr<WzGr2DLayer>& layer);

    /**
     * @brief Unregister a UI element (called from UIElement destructor)
     * @param element UI element to unregister
     */
    void UnregisterUIElement(UIElement* element);

    /**
     * @brief Clear all registered layers
     */
    void ClearLayers();

private:
    DebugOverlay() = default;
    ~DebugOverlay() override = default;

    auto FindCanvasesAt(int screenX, int screenY) -> std::vector<CanvasHitInfo>;
    auto FindUIElementsAt(int screenX, int screenY) -> std::vector<UIElementHitInfo>;
    auto LayerHitTest(const std::shared_ptr<WzGr2DLayer>& layer, int screenX, int screenY) -> bool;
    void RenderSelectionList(SDL_Renderer* renderer);
    void RenderInfoPopup(SDL_Renderer* renderer);
    void RenderText(SDL_Renderer* renderer, const std::string& text, int x, int y);
    void AdjustPopupPosition(int& x, int& y, int width, int height);
    void Close();

    struct LayerEntry
    {
        std::weak_ptr<WzGr2DLayer> layer;
        std::string name;
        UIElement* uiElement = nullptr;  // Associated UI element
    };

    std::vector<LayerEntry> m_layers;
    std::vector<CanvasHitInfo> m_hitList;         // Legacy canvas hit list
    std::vector<UIElementHitInfo> m_uiHitList;    // UI element hit list

    bool m_bActive = false;
    bool m_bShowingList = false;
    int m_nSelectedIndex = -1;
    Point2D m_clickPos{0, 0};
};

} // namespace ms

#endif // MS_DEBUG_CANVAS
