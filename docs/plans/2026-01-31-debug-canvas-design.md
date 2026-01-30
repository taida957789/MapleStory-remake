# Debug Canvas Overlay 設計

## 概述

新增開發用的 debug 功能，點擊 sprite 後顯示半透明 popup 顯示該圖的 WZ path。

## 需求

- 編譯時 flag `MS_DEBUG_CANVAS` 控制是否啟用
- 點擊 sprite 顯示 WZ path
- 重疊時顯示清單讓使用者選擇
- 再次點擊或按 ESC 關閉
- 簡單 tooltip 風格：黑色半透明背景 + 白色文字

## 設計

### 1. WzCanvas 修改

新增 WZ path 欄位，只在 debug flag 啟用時編譯：

```cpp
// WzCanvas.h
#ifdef MS_DEBUG_CANVAS
private:
    std::string m_strWzPath;

public:
    [[nodiscard]] auto GetWzPath() const noexcept -> const std::string& { return m_strWzPath; }
    void SetWzPath(const std::string& path) noexcept { m_strWzPath = path; }
#endif
```

### 2. DebugOverlay 類別

檔案位置：`src/debug/DebugOverlay.h` / `DebugOverlay.cpp`

```cpp
struct CanvasHitInfo {
    std::string wzPath;
    std::string layerName;
    int zOrder;
};

class DebugOverlay : public Singleton<DebugOverlay>
{
public:
    // 回傳 true 表示消費了此事件
    auto OnMouseClick(int screenX, int screenY) -> bool;
    auto OnKeyDown(SDL_Keycode key) -> bool;
    void Render();

private:
    auto FindCanvasesAt(int x, int y) -> std::vector<CanvasHitInfo>;
    void RenderSelectionList();
    void RenderInfoPopup();

    bool m_bActive = false;
    std::vector<CanvasHitInfo> m_hitList;
    int m_nSelectedIndex = -1;
    Point2D m_clickPos;
};
```

### 3. 整合點

**WzGr2D::RenderFrame()：**
```cpp
#ifdef MS_DEBUG_CANVAS
DebugOverlay::GetInstance().Render();
#endif
```

**Input 處理：**
```cpp
#ifdef MS_DEBUG_CANVAS
if (DebugOverlay::GetInstance().OnMouseClick(x, y))
    return;
#endif
```

### 4. CMakeLists.txt

```cmake
option(MS_DEBUG_CANVAS "Enable debug canvas overlay" OFF)
if(MS_DEBUG_CANVAS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MS_DEBUG_CANVAS)
endif()
```

## 使用方式

編譯時啟用：
```bash
cmake -DMS_DEBUG_CANVAS=ON ..
```

執行時：
1. 點擊任意 sprite
2. 若有重疊，從清單選擇
3. 查看 WZ path
4. 點擊其他地方或按 ESC 關閉

## 修改檔案清單

1. `CMakeLists.txt` - 新增編譯選項
2. `src/wz/WzCanvas.h` - 新增 WZ path 欄位
3. `src/wz/WzResMan.cpp` - 載入時設定 WZ path
4. `src/debug/DebugOverlay.h` - 新增（debug overlay 類別）
5. `src/debug/DebugOverlay.cpp` - 新增（實作）
6. `src/graphics/WzGr2D.cpp` - 整合 render
7. `src/input/*.cpp` - 整合滑鼠/鍵盤事件
