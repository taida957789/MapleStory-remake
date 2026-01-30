#ifdef MS_DEBUG_CANVAS

#include "DebugOverlay.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "wz/WzCanvas.h"

#include <algorithm>

namespace ms
{

auto DebugOverlay::OnMouseClick(int screenX, int screenY) -> bool
{
    // If showing selection list, check if clicking on a list item
    if (m_bShowingList && m_bActive)
    {
        constexpr int itemHeight = 20;
        constexpr int padding = 5;

        int listX = m_clickPos.x + 10;
        int listY = m_clickPos.y + 10;

        for (std::size_t i = 0; i < m_hitList.size(); ++i)
        {
            int itemY = listY + padding + static_cast<int>(i) * itemHeight;
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

    // Find canvases at click position
    m_hitList = FindCanvasesAt(screenX, screenY);

    if (m_hitList.empty())
    {
        return false;
    }

    m_clickPos = {screenX, screenY};
    m_bActive = true;

    if (m_hitList.size() == 1)
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

        // Get layer position in screen coordinates
        int layerScreenX, layerScreenY;
        if (layer->IsScreenSpace())
        {
            layerScreenX = layer->GetLeft();
            layerScreenY = layer->GetTop();
        }
        else
        {
            layerScreenX = layer->GetLeft() - cameraPos.x + screenCenterX;
            layerScreenY = layer->GetTop() - cameraPos.y + screenCenterY;
        }

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

void DebugOverlay::RenderSelectionList(SDL_Renderer* renderer)
{
    constexpr int itemHeight = 20;
    constexpr int padding = 5;
    constexpr int maxWidth = 400;

    int listX = m_clickPos.x + 10;
    int listY = m_clickPos.y + 10;
    int listH = padding * 2 + static_cast<int>(m_hitList.size()) * itemHeight;

    // Clamp to screen
    auto& gr = WzGr2D::GetInstance();
    if (listX + maxWidth > static_cast<int>(gr.GetWidth()))
    {
        listX = m_clickPos.x - maxWidth - 10;
    }
    if (listY + listH > static_cast<int>(gr.GetHeight()))
    {
        listY = static_cast<int>(gr.GetHeight()) - listH - 10;
    }

    // Draw background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect bgRect = {static_cast<float>(listX), static_cast<float>(listY),
                        static_cast<float>(maxWidth), static_cast<float>(listH)};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &bgRect);

    // Draw border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
    SDL_RenderRect(renderer, &bgRect);

    // Draw items
    for (std::size_t i = 0; i < m_hitList.size(); ++i)
    {
        int itemY = listY + padding + static_cast<int>(i) * itemHeight;

        std::string text = "[" + std::to_string(m_hitList[i].zOrder) + "] " + m_hitList[i].wzPath;
        if (text.length() > 50)
        {
            text = text.substr(0, 47) + "...";
        }

        RenderText(renderer, text, listX + padding, itemY);
    }
}

void DebugOverlay::RenderInfoPopup(SDL_Renderer* renderer)
{
    if (m_nSelectedIndex < 0 || m_nSelectedIndex >= static_cast<int>(m_hitList.size()))
    {
        return;
    }

    const auto& hit = m_hitList[static_cast<std::size_t>(m_nSelectedIndex)];

    constexpr int padding = 8;
    int textWidth = static_cast<int>(hit.wzPath.length()) * 8;  // Approximate
    int popupW = textWidth + padding * 2;
    int popupH = 24 + padding * 2;

    int popupX = m_clickPos.x + 10;
    int popupY = m_clickPos.y + 10;

    // Clamp to screen
    auto& gr = WzGr2D::GetInstance();
    if (popupX + popupW > static_cast<int>(gr.GetWidth()))
    {
        popupX = m_clickPos.x - popupW - 10;
    }
    if (popupY + popupH > static_cast<int>(gr.GetHeight()))
    {
        popupY = m_clickPos.y - popupH - 10;
    }

    // Draw background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_FRect bgRect = {static_cast<float>(popupX), static_cast<float>(popupY),
                        static_cast<float>(popupW), static_cast<float>(popupH)};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &bgRect);

    // Draw border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 200);
    SDL_RenderRect(renderer, &bgRect);

    // Draw text
    RenderText(renderer, hit.wzPath, popupX + padding, popupY + padding);
}

void DebugOverlay::RenderText(SDL_Renderer* renderer, const std::string& text, int x, int y)
{
    // Simple text rendering using SDL debug text
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDebugText(renderer, static_cast<float>(x), static_cast<float>(y), text.c_str());
}

void DebugOverlay::Close()
{
    m_bActive = false;
    m_bShowingList = false;
    m_nSelectedIndex = -1;
    m_hitList.clear();
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

    m_layers.push_back({layer, name});
}

void DebugOverlay::UnregisterLayer(const std::shared_ptr<WzGr2DLayer>& layer)
{
    m_layers.erase(
        std::remove_if(m_layers.begin(), m_layers.end(),
                       [&layer](const auto& entry) { return entry.layer.lock() == layer; }),
        m_layers.end());
}

void DebugOverlay::ClearLayers()
{
    m_layers.clear();
    Close();
}

} // namespace ms

#endif // MS_DEBUG_CANVAS
