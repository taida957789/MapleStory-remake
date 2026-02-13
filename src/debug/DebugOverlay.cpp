#ifdef MS_DEBUG_CANVAS

#include "DebugOverlay.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DCanvas.h"
#include "graphics/WzGr2DLayer.h"
#include "ui/UIElement.h"
#include "wz/WzCanvas.h"

#include <algorithm>
#include <unordered_set>

namespace ms
{

auto DebugOverlay::OnMouseClick(int screenX, int screenY) -> bool
{
    // If showing selection list, check if clicking on a list item
    if (m_bShowingList && m_bActive)
    {
        constexpr int itemHeight = 25;

        int listX = m_clickPos.x + 10;
        int listY = m_clickPos.y + 10;

        for (std::size_t i = 0; i < m_uiHitList.size(); ++i)
        {
            int itemY = listY + 40 + static_cast<int>(i) * itemHeight;
            if (screenX >= listX && screenX <= listX + 400 &&
                screenY >= itemY && screenY <= itemY + itemHeight)
            {
                m_nSelectedIndex = static_cast<int>(i);
                m_bShowingList = false;
                return true;
            }
        }

        // Click outside list - close
        Close();
        return true;
    }

    // If showing info popup, close it
    if (m_bActive && !m_bShowingList)
    {
        Close();
        return true;
    }

    // Find UI elements at click position (prioritize UI elements over raw canvases)
    m_uiHitList = FindUIElementsAt(screenX, screenY);

    if (m_uiHitList.empty())
    {
        return false;
    }

    m_clickPos = {screenX, screenY};
    m_bActive = true;

    if (m_uiHitList.size() == 1)
    {
        m_nSelectedIndex = 0;
        m_bShowingList = false;
    }
    else
    {
        m_nSelectedIndex = -1;
        m_bShowingList = true;
    }

    return true;
}

auto DebugOverlay::OnKeyDown(SDL_Keycode key) -> bool
{
    if (!m_bActive)
    {
        return false;
    }

    if (key == SDLK_ESCAPE)
    {
        Close();
        return true;
    }

    return false;
}

void DebugOverlay::Render(SDL_Renderer* renderer)
{
    if (!m_bActive)
    {
        return;
    }

    if (m_bShowingList)
    {
        RenderSelectionList(renderer);
    }
    else if (m_nSelectedIndex >= 0)
    {
        RenderInfoPopup(renderer);
    }
}

auto DebugOverlay::FindCanvasesAt(int screenX, int screenY) -> std::vector<CanvasHitInfo>
{
    std::vector<CanvasHitInfo> hits;

    auto& gr = WzGr2D::GetInstance();
    auto screenCenterX = static_cast<int>(gr.GetWidth() / 2);
    auto screenCenterY = static_cast<int>(gr.GetHeight() / 2);
    auto cameraPos = gr.GetCameraPosition();

    for (auto& entry : m_layers)
    {
        auto layer = entry.layer.lock();
        if (!layer || !layer->IsVisible())
        {
            continue;
        }

        // Get layer position in screen coordinates (world space with camera offset)
        int layerScreenX = layer->GetLeft() - cameraPos.x + screenCenterX;
        int layerScreenY = layer->GetTop() - cameraPos.y + screenCenterY;

        // Check each canvas in the layer
        auto canvasCount = layer->GetCanvasCount();
        for (std::size_t i = 0; i < canvasCount; ++i)
        {
            auto canvas = layer->GetCanvas(i);
            if (!canvas)
            {
                continue;
            }

            // Calculate canvas bounds
            auto origin = canvas->GetOrigin();
            int canvasX = layerScreenX - origin.x;
            int canvasY = layerScreenY - origin.y;
            int canvasW = canvas->GetWidth();
            int canvasH = canvas->GetHeight();

            // Hit test
            if (screenX >= canvasX && screenX < canvasX + canvasW &&
                screenY >= canvasY && screenY < canvasY + canvasH)
            {
                CanvasHitInfo hit;
                hit.wzPath = canvas->GetWzPath();
                hit.layerName = entry.name;
                hit.zOrder = layer->GetZ();
                hit.canvasIndex = static_cast<int>(i);

                if (!hit.wzPath.empty())
                {
                    hits.push_back(std::move(hit));
                }
            }
        }
    }

    // Sort by Z-order (highest first - topmost layer)
    std::sort(hits.begin(), hits.end(),
              [](const auto& a, const auto& b) { return a.zOrder > b.zOrder; });

    return hits;
}

auto DebugOverlay::LayerHitTest(const std::shared_ptr<WzGr2DLayer>& layer,
                                int screenX, int screenY) -> bool
{
    if (!layer || !layer->IsVisible())
    {
        return false;
    }

    auto& gr = WzGr2D::GetInstance();
    auto screenCenterX = static_cast<int>(gr.GetWidth() / 2);
    auto screenCenterY = static_cast<int>(gr.GetHeight() / 2);
    auto cameraPos = gr.GetCameraPosition();

    // Get layer position in screen coordinates (world space with camera offset)
    int layerScreenX = layer->GetLeft() - cameraPos.x + screenCenterX;
    int layerScreenY = layer->GetTop() - cameraPos.y + screenCenterY;

    // Check each canvas in the layer
    auto canvasCount = layer->GetCanvasCount();
    for (std::size_t i = 0; i < canvasCount; ++i)
    {
        auto canvas = layer->GetCanvas(i);
        if (!canvas)
        {
            continue;
        }

        // Calculate canvas bounds
        auto origin = canvas->GetOrigin();
        int canvasX = layerScreenX - origin.x;
        int canvasY = layerScreenY - origin.y;
        int canvasW = static_cast<int>(canvas->GetWidth());
        int canvasH = static_cast<int>(canvas->GetHeight());

        // Hit test
        if (screenX >= canvasX && screenX < canvasX + canvasW &&
            screenY >= canvasY && screenY < canvasY + canvasH)
        {
            return true;
        }
    }

    return false;
}

auto DebugOverlay::FindUIElementsAt(int screenX, int screenY) -> std::vector<UIElementHitInfo>
{
    std::vector<UIElementHitInfo> hits;
    std::unordered_set<UIElement*> processedElements;

    // 1. Iterate through all registered layers
    for (auto& entry : m_layers)
    {
        auto layer = entry.layer.lock();
        if (!layer || !layer->IsVisible() || !entry.uiElement)
        {
            continue;
        }

        // 2. Check if layer is at click position
        if (!LayerHitTest(layer, screenX, screenY))
        {
            continue;
        }

        // 3. Deduplicate: skip if this UIElement already processed
        if (processedElements.count(entry.uiElement) > 0)
        {
            continue;
        }
        processedElements.insert(entry.uiElement);

        // 4. Collect complete information for this UI element
        UIElementHitInfo info;
        info.element = entry.uiElement;
        info.typeName = entry.uiElement->GetDebugTypeName();
        info.localPos = entry.uiElement->GetPosition();
        info.absolutePos = entry.uiElement->GetAbsolutePosition();
        info.width = entry.uiElement->GetWidth();
        info.height = entry.uiElement->GetHeight();
        info.zOrder = entry.uiElement->GetZ();

        // 5. Collect parent information
        auto parent = entry.uiElement->GetParent();
        info.parentName = parent ? parent->GetDebugTypeName() : "None";

        // 6. Collect child count
        info.childCount = static_cast<int>(entry.uiElement->GetChildren().size());

        // 7. Collect all layer names for this UI element
        for (auto& e : m_layers)
        {
            if (e.uiElement == entry.uiElement)
            {
                info.layerNames.push_back(e.name);
            }
        }

        hits.push_back(std::move(info));
    }

    // 8. Sort by Z-order (descending - highest on top)
    std::sort(hits.begin(), hits.end(),
              [](const auto& a, const auto& b) { return a.zOrder > b.zOrder; });

    return hits;
}

void DebugOverlay::RenderSelectionList(SDL_Renderer* renderer)
{
    constexpr int itemHeight = 25;
    constexpr int padding = 10;
    constexpr int maxWidth = 450;

    int listX = m_clickPos.x + 10;
    int listY = m_clickPos.y + 10;
    int listH = 40 + static_cast<int>(m_uiHitList.size()) * itemHeight + padding;

    // Adjust position to stay on screen
    AdjustPopupPosition(listX, listY, maxWidth, listH);

    // Draw background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect bgRect = {static_cast<float>(listX), static_cast<float>(listY),
                        static_cast<float>(maxWidth), static_cast<float>(listH)};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 220);
    SDL_RenderFillRect(renderer, &bgRect);

    // Draw border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
    SDL_RenderRect(renderer, &bgRect);

    // Draw title
    RenderText(renderer, "UI Elements (click to inspect):", listX + padding, listY + padding);

    // Draw items
    for (std::size_t i = 0; i < m_uiHitList.size(); ++i)
    {
        int itemY = listY + 40 + static_cast<int>(i) * itemHeight;

        std::string text = "[Z:" + std::to_string(m_uiHitList[i].zOrder) + "] " +
                          m_uiHitList[i].typeName + " (" +
                          std::to_string(m_uiHitList[i].localPos.x) + ", " +
                          std::to_string(m_uiHitList[i].localPos.y) + ")";

        if (text.length() > 50)
        {
            text = text.substr(0, 47) + "...";
        }

        RenderText(renderer, text, listX + padding + 10, itemY);
    }
}

void DebugOverlay::RenderInfoPopup(SDL_Renderer* renderer)
{
    if (m_nSelectedIndex < 0 || m_nSelectedIndex >= static_cast<int>(m_uiHitList.size()))
    {
        return;
    }

    // Validate element pointer
    const auto& info = m_uiHitList[static_cast<std::size_t>(m_nSelectedIndex)];
    if (!info.element)
    {
        Close();
        return;
    }

    constexpr int padding = 15;
    constexpr int lineHeight = 20;
    constexpr int popupW = 450;

    // Calculate height based on content
    int baseLines = 7;  // Type, Local, Absolute, Size, Z, Parent, Children
    int layerLines = static_cast<int>(info.layerNames.size());
    int totalLines = baseLines + (layerLines > 0 ? layerLines + 2 : 0) + 2;  // +2 for separator and hint
    int popupH = totalLines * lineHeight + padding * 2 + 30;

    int popupX = m_clickPos.x + 10;
    int popupY = m_clickPos.y + 10;

    // Adjust position to stay on screen
    AdjustPopupPosition(popupX, popupY, popupW, popupH);

    // Draw background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect bgRect = {static_cast<float>(popupX), static_cast<float>(popupY),
                        static_cast<float>(popupW), static_cast<float>(popupH)};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 220);
    SDL_RenderFillRect(renderer, &bgRect);

    // Draw border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
    SDL_RenderRect(renderer, &bgRect);

    // Draw title bar
    SDL_FRect titleRect = {static_cast<float>(popupX), static_cast<float>(popupY),
                           static_cast<float>(popupW), 25.0f};
    SDL_SetRenderDrawColor(renderer, 40, 80, 120, 255);
    SDL_RenderFillRect(renderer, &titleRect);

    int x = popupX + padding;
    int y = popupY + padding;

    // Title
    RenderText(renderer, info.typeName, x, y);
    y += 30;

    // Basic information
    RenderText(renderer, "Local Pos:    (" + std::to_string(info.localPos.x) + ", " +
                         std::to_string(info.localPos.y) + ")", x, y);
    y += lineHeight;

    RenderText(renderer, "Absolute Pos: (" + std::to_string(info.absolutePos.x) + ", " +
                         std::to_string(info.absolutePos.y) + ")", x, y);
    y += lineHeight;

    RenderText(renderer, "Size:         " + std::to_string(info.width) + " x " +
                         std::to_string(info.height), x, y);
    y += lineHeight;

    RenderText(renderer, "Z-order:      " + std::to_string(info.zOrder), x, y);
    y += lineHeight;

    RenderText(renderer, "Parent:       " + info.parentName, x, y);
    y += lineHeight;

    RenderText(renderer, "Children:     " + std::to_string(info.childCount), x, y);
    y += lineHeight + 10;

    // Separator line
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderLine(renderer, static_cast<float>(popupX + 10), static_cast<float>(y),
                   static_cast<float>(popupX + popupW - 10), static_cast<float>(y));
    y += 10;

    // Layer list
    if (!info.layerNames.empty())
    {
        RenderText(renderer, "Layers (" + std::to_string(info.layerNames.size()) + "):", x, y);
        y += lineHeight;

        for (const auto& layerName : info.layerNames)
        {
            RenderText(renderer, "  * " + layerName, x + 10, y);
            y += lineHeight;
        }
        y += 10;
    }

    // Hint text
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    RenderText(renderer, "Click anywhere or press ESC to close", x, y);
}

void DebugOverlay::RenderText(SDL_Renderer* renderer, const std::string& text, int x, int y)
{
    // Simple text rendering using SDL debug text
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, static_cast<float>(x), static_cast<float>(y), text.c_str());
}

void DebugOverlay::AdjustPopupPosition(int& x, int& y, int width, int height)
{
    auto& gr = WzGr2D::GetInstance();
    int screenW = static_cast<int>(gr.GetWidth());
    int screenH = static_cast<int>(gr.GetHeight());

    // Ensure popup stays on screen with 10px padding
    if (x + width > screenW)
    {
        x = screenW - width - 10;
    }
    if (y + height > screenH)
    {
        y = screenH - height - 10;
    }
    if (x < 10)
    {
        x = 10;
    }
    if (y < 10)
    {
        y = 10;
    }
}

void DebugOverlay::Close()
{
    m_bActive = false;
    m_bShowingList = false;
    m_nSelectedIndex = -1;
    m_hitList.clear();
    m_uiHitList.clear();
}

void DebugOverlay::RegisterLayer(const std::shared_ptr<WzGr2DLayer>& layer, const std::string& name)
{
    // Check if already registered
    for (const auto& entry : m_layers)
    {
        if (entry.layer.lock() == layer)
        {
            return;
        }
    }

    m_layers.push_back({layer, name, nullptr});
}

void DebugOverlay::RegisterUIElement(UIElement* element,
                                     const std::shared_ptr<WzGr2DLayer>& layer,
                                     const std::string& layerName)
{
    if (!element || !layer)
    {
        return;
    }

    // Check if this layer is already registered
    for (auto& entry : m_layers)
    {
        if (entry.layer.lock() == layer)
        {
            // Update UI element association
            entry.uiElement = element;
            return;
        }
    }

    // Layer not found, register it with UI element
    m_layers.push_back({layer, layerName, element});
}

void DebugOverlay::UnregisterLayer(const std::shared_ptr<WzGr2DLayer>& layer)
{
    m_layers.erase(
        std::remove_if(m_layers.begin(), m_layers.end(),
                       [&layer](const auto& entry) { return entry.layer.lock() == layer; }),
        m_layers.end());
}

void DebugOverlay::UnregisterUIElement(UIElement* element)
{
    if (!element)
    {
        return;
    }

    // Clear UI element references in all layers
    for (auto& entry : m_layers)
    {
        if (entry.uiElement == element)
        {
            entry.uiElement = nullptr;
        }
    }

    // If currently showing this element's info, close the popup
    if (m_bActive && m_nSelectedIndex >= 0 &&
        m_nSelectedIndex < static_cast<int>(m_uiHitList.size()) &&
        m_uiHitList[static_cast<std::size_t>(m_nSelectedIndex)].element == element)
    {
        Close();
    }
}

void DebugOverlay::ClearLayers()
{
    m_layers.clear();
    Close();
}

} // namespace ms

#endif // MS_DEBUG_CANVAS
