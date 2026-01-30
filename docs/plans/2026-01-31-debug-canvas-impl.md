# Debug Canvas Overlay Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a debug overlay that shows WZ path when clicking sprites.

**Architecture:** Compile-time flag `MS_DEBUG_CANVAS` guards all debug code. DebugOverlay singleton handles hit testing, selection list, and popup rendering. WzCanvas stores its source path only when flag is enabled.

**Tech Stack:** C++17, SDL3, existing Singleton pattern

---

### Task 1: Add CMake compile flag

**Files:**
- Modify: `CMakeLists.txt:148` (after BUILD_TESTS option)

**Step 1: Add the MS_DEBUG_CANVAS option**

Add after line 148 (after `option(BUILD_TESTS ...)`):

```cmake
# Debug canvas overlay
option(MS_DEBUG_CANVAS "Enable debug canvas overlay for development" OFF)
if(MS_DEBUG_CANVAS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MS_DEBUG_CANVAS)
endif()
```

**Step 2: Verify cmake configure works**

Run: `cd build && cmake -DMS_DEBUG_CANVAS=ON ..`
Expected: Configure completes without error

**Step 3: Commit**

```bash
git add CMakeLists.txt
git commit -m "build: add MS_DEBUG_CANVAS compile flag"
```

---

### Task 2: Add WZ path storage to WzCanvas

**Files:**
- Modify: `src/wz/WzCanvas.h:69-79` (before private members)

**Step 1: Add conditional WZ path field and accessors**

Insert before line 69 (`int m_nWidth = 0;`), inside the class:

```cpp
#ifdef MS_DEBUG_CANVAS
public:
    [[nodiscard]] auto GetWzPath() const noexcept -> const std::string& { return m_strWzPath; }
    void SetWzPath(const std::string& path) { m_strWzPath = path; }

private:
    std::string m_strWzPath;
#endif

private:
```

Also add `#include <string>` at top if not present.

**Step 2: Verify build**

Run: `cmake --build build`
Expected: Build succeeds

**Step 3: Commit**

```bash
git add src/wz/WzCanvas.h
git commit -m "feat(debug): add WZ path storage to WzCanvas"
```

---

### Task 3: Create DebugOverlay header

**Files:**
- Create: `src/debug/DebugOverlay.h`

**Step 1: Create the header file**

```cpp
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
```

**Step 2: Verify build**

Run: `cmake --build build`
Expected: Build succeeds (header only, not yet included)

**Step 3: Commit**

```bash
git add src/debug/DebugOverlay.h
git commit -m "feat(debug): add DebugOverlay header"
```

---

### Task 4: Create DebugOverlay implementation

**Files:**
- Create: `src/debug/DebugOverlay.cpp`

**Step 1: Create the implementation file**

```cpp
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
```

**Step 2: Add to CMakeLists.txt SOURCES**

In CMakeLists.txt, add to SOURCES list (around line 77):
```cmake
    src/debug/DebugOverlay.cpp
```

And add to HEADERS list (around line 111):
```cmake
    src/debug/DebugOverlay.h
```

**Step 3: Verify build**

Run: `cmake --build build`
Expected: Build succeeds

**Step 4: Commit**

```bash
git add src/debug/DebugOverlay.cpp CMakeLists.txt
git commit -m "feat(debug): implement DebugOverlay"
```

---

### Task 5: Integrate DebugOverlay into WzGr2D rendering

**Files:**
- Modify: `src/graphics/WzGr2D.cpp:1-10` (includes)
- Modify: `src/graphics/WzGr2D.cpp:256-259` (before SDL_RenderPresent)

**Step 1: Add conditional include**

Add after line 4 (`#include "wz/WzCanvas.h"`):

```cpp
#ifdef MS_DEBUG_CANVAS
#include "debug/DebugOverlay.h"
#endif
```

**Step 2: Add debug overlay render call**

Insert before line 259 (`SDL_RenderPresent(m_pRenderer);`):

```cpp
    // Render debug overlay (always on top)
#ifdef MS_DEBUG_CANVAS
    DebugOverlay::GetInstance().Render(m_pRenderer);
#endif
```

**Step 3: Verify build**

Run: `cmake --build build`
Expected: Build succeeds

**Step 4: Commit**

```bash
git add src/graphics/WzGr2D.cpp
git commit -m "feat(debug): integrate DebugOverlay rendering"
```

---

### Task 6: Integrate DebugOverlay into Application input handling

**Files:**
- Modify: `src/app/Application.cpp:1-15` (includes)
- Modify: `src/app/Application.cpp:132-136` (mouse click handling)
- Modify: `src/app/Application.cpp:144-149` (key down handling)

**Step 1: Add conditional include**

Add after line 11 (`#include "wz/WzResMan.h"`):

```cpp
#ifdef MS_DEBUG_CANVAS
#include "debug/DebugOverlay.h"
#endif
```

**Step 2: Add mouse click interception**

Replace lines 132-136 (SDL_EVENT_MOUSE_BUTTON_DOWN case):

```cpp
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
#ifdef MS_DEBUG_CANVAS
                    if (DebugOverlay::GetInstance().OnMouseClick(
                            static_cast<int>(event.button.x),
                            static_cast<int>(event.button.y)))
                    {
                        break;
                    }
#endif
                    m_pStage->OnMouseDown(static_cast<std::int32_t>(event.button.x),
                                          static_cast<std::int32_t>(event.button.y),
                                          static_cast<std::int32_t>(event.button.button));
                    break;
```

**Step 3: Add key down interception**

Replace lines 144-149 (SDL_EVENT_KEY_DOWN case):

```cpp
                case SDL_EVENT_KEY_DOWN:
                    if (!event.key.repeat)
                    {
#ifdef MS_DEBUG_CANVAS
                        if (DebugOverlay::GetInstance().OnKeyDown(event.key.key))
                        {
                            break;
                        }
#endif
                        m_pStage->OnKeyDown(static_cast<std::int32_t>(event.key.key));
                    }
                    break;
```

**Step 4: Verify build**

Run: `cmake --build build`
Expected: Build succeeds

**Step 5: Commit**

```bash
git add src/app/Application.cpp
git commit -m "feat(debug): integrate DebugOverlay input handling"
```

---

### Task 7: Set WZ path when loading canvas (WzProperty)

**Files:**
- Modify: `src/wz/WzProperty.cpp` (in SetCanvas or canvas loading)
- Modify: `src/wz/WzProperty.h` (if needed)

**Step 1: Find where canvas is set and add path tracking**

In `src/wz/WzProperty.cpp`, find the `SetCanvas` method and modify:

```cpp
void WzProperty::SetCanvas(std::shared_ptr<WzCanvas> canvas)
{
    m_value = std::move(canvas);
    m_nodeType = WzNodeType::Canvas;
}
```

The WZ path needs to be set when the canvas is created during WZ file parsing.
This requires modifying `WzFile.cpp` or `WzReader.cpp` where canvases are created.

Check `src/wz/WzFile.cpp` and `src/wz/WzReader.cpp` to find canvas creation.

**Step 2: Modify canvas creation to include path**

In the file where canvases are created (likely WzReader or WzFile), after creating a canvas:

```cpp
#ifdef MS_DEBUG_CANVAS
canvas->SetWzPath(currentPath);  // currentPath = full WZ path to this canvas
#endif
```

**Step 3: Verify build**

Run: `cmake --build build`
Expected: Build succeeds

**Step 4: Commit**

```bash
git add src/wz/WzProperty.cpp src/wz/WzReader.cpp src/wz/WzFile.cpp
git commit -m "feat(debug): set WZ path on canvas creation"
```

---

### Task 8: Register layers in MapLoadable

**Files:**
- Modify: `src/stage/MapLoadable.cpp` (where layers are created)
- Modify: `src/stage/MapLoadable.h` (if needed)

**Step 1: Add conditional include**

In `src/stage/MapLoadable.cpp`, add:

```cpp
#ifdef MS_DEBUG_CANVAS
#include "debug/DebugOverlay.h"
#endif
```

**Step 2: Register layers when created**

Find where layers are created (likely in a method like `CreateLayer` or during stage init).
After creating each layer:

```cpp
#ifdef MS_DEBUG_CANVAS
DebugOverlay::GetInstance().RegisterLayer(layer, layerName);
#endif
```

**Step 3: Clear layers on stage close**

In the Close() or destructor:

```cpp
#ifdef MS_DEBUG_CANVAS
DebugOverlay::GetInstance().ClearLayers();
#endif
```

**Step 4: Verify build**

Run: `cmake --build build`
Expected: Build succeeds

**Step 5: Commit**

```bash
git add src/stage/MapLoadable.cpp src/stage/MapLoadable.h
git commit -m "feat(debug): register layers with DebugOverlay"
```

---

### Task 9: Final integration test

**Step 1: Build with debug flag enabled**

```bash
cd build
cmake -DMS_DEBUG_CANVAS=ON ..
cmake --build .
```

**Step 2: Build without debug flag (verify no compile errors)**

```bash
cmake -DMS_DEBUG_CANVAS=OFF ..
cmake --build .
```

**Step 3: Commit any final fixes**

```bash
git add -A
git commit -m "feat(debug): complete debug canvas overlay implementation"
```

---

## Summary

Total tasks: 9

Files to create:
- `src/debug/DebugOverlay.h`
- `src/debug/DebugOverlay.cpp`

Files to modify:
- `CMakeLists.txt`
- `src/wz/WzCanvas.h`
- `src/wz/WzProperty.cpp` (or WzReader/WzFile)
- `src/graphics/WzGr2D.cpp`
- `src/app/Application.cpp`
- `src/stage/MapLoadable.cpp`
