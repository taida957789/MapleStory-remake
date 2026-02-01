# LayoutMan UI 管理系統實作計劃

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 實作 LayoutMan UI 自動化構建和管理系統，完全基於 IDA Pro 逆向工程分析結果，確保與原版 MapleStory v1029 行為一致。

**Architecture:** LayoutMan 負責從 WZ 資源自動創建 UI 元素（按鈕、圖層等），使用映射表（按名稱索引）管理控件，並提供統一的查找和批量操作接口。AutoBuild 解析 WZ 屬性的 `type:name` 命名約定，自動創建對應的 UI 控件。

**Tech Stack:** C++17, SDL3, WZ 資源系統, IDA Pro 逆向分析

**IDA Pro 參考:**
- CLayoutMan::AutoBuild (0xb36170)
- CLayoutMan::AddButton (0xb30310)
- CLayoutMan::ABGetButton (0xb300f0)
- CLayoutMan::ABGetLayer (0xb35f90)
- CLayoutMan::RegisterLayer (0xb35d40)

---

## Task 1: 創建 LayoutMan 基礎結構

**Files:**
- Create: `src/ui/LayoutMan.h`
- Create: `src/ui/LayoutMan.cpp`

**Step 1: 創建 LayoutMan.h 頭文件**

基於 IDA 分析的 CLayoutMan 類結構創建頭文件：

```cpp
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class UIElement;
class UIButton;
class WzGr2DLayer;

/**
 * @brief UI 自動化構建和管理系統
 *
 * 基於 IDA Pro 分析 CLayoutMan (MapleStory v1029)
 * 負責從 WZ 資源自動創建 UI 元素並提供統一管理接口
 */
class LayoutMan
{
public:
    LayoutMan();
    ~LayoutMan();

    // Non-copyable, movable
    LayoutMan(const LayoutMan&) = delete;
    auto operator=(const LayoutMan&) -> LayoutMan& = delete;
    LayoutMan(LayoutMan&&) noexcept = default;
    auto operator=(LayoutMan&&) noexcept -> LayoutMan& = default;

    /**
     * @brief 初始化 LayoutMan
     * @param pParent 父 UI 元素
     * @param nOffsetX 全局 X 偏移
     * @param nOffsetY 全局 Y 偏移
     *
     * IDA: CLayoutMan::Init
     */
    void Init(UIElement* pParent, int nOffsetX = 0, int nOffsetY = 0);

    /**
     * @brief 從 WZ 資源自動構建 UI
     * @param sRootUOL WZ 資源根路徑
     * @param nIdBase 控件 ID 起始值
     * @param nOffsetX X 偏移
     * @param nOffsetY Y 偏移
     * @param bSetTooltip 是否設置 tooltip
     * @param bSameIDCtrl 允許相同 ID 控件
     *
     * IDA: CLayoutMan::AutoBuild (0xb36170)
     */
    void AutoBuild(
        const std::wstring& sRootUOL,
        int nIdBase,
        int nOffsetX,
        int nOffsetY,
        bool bSetTooltip,
        bool bSameIDCtrl
    );

    // ========== 按鈕管理 ==========

    /**
     * @brief 按名稱查找按鈕
     * @param sName 按鈕名稱
     * @return 按鈕指針，未找到返回 nullptr
     *
     * IDA: CLayoutMan::ABGetButton (0xb300f0)
     */
    [[nodiscard]] auto ABGetButton(const std::wstring& sName) -> std::shared_ptr<UIButton>;

    /**
     * @brief 設置所有按鈕的顯示狀態
     * @param bShow 是否顯示
     *
     * IDA: CLayoutMan::ABSetButtonShowAll (0xb2e680)
     */
    void ABSetButtonShowAll(bool bShow);

    /**
     * @brief 設置所有按鈕的啟用狀態
     * @param bEnable 是否啟用
     *
     * IDA: CLayoutMan::ABSetButtonEnableAll (0xb2e770)
     */
    void ABSetButtonEnableAll(bool bEnable);

    /**
     * @brief 設置指定按鈕的啟用狀態
     * @param sName 按鈕名稱
     * @param bEnable 是否啟用
     *
     * IDA: CLayoutMan::ABSetButtonEnable (0xb392f0)
     */
    void ABSetButtonEnable(const std::wstring& sName, bool bEnable);

    // ========== 圖層管理 ==========

    /**
     * @brief 註冊圖層到映射表
     * @param pLayer 圖層指針
     * @param sName 圖層名稱
     *
     * IDA: CLayoutMan::RegisterLayer (0xb35d40)
     */
    void RegisterLayer(std::shared_ptr<WzGr2DLayer> pLayer, const std::wstring& sName);

    /**
     * @brief 按名稱查找圖層
     * @param sName 圖層名稱
     * @return 圖層指針，未找到返回 nullptr
     *
     * IDA: CLayoutMan::ABGetLayer (0xb35f90)
     */
    [[nodiscard]] auto ABGetLayer(const std::wstring& sName) -> std::shared_ptr<WzGr2DLayer>;

    /**
     * @brief 設置指定圖層的可見性
     * @param sName 圖層名稱
     * @param bVisible 是否可見
     *
     * IDA: CLayoutMan::ABSetLayerVisible (0xb38ba0)
     */
    void ABSetLayerVisible(const std::wstring& sName, bool bVisible);

    /**
     * @brief 設置所有圖層的可見性
     * @param bVisible 是否可見
     *
     * IDA: CLayoutMan::ABSetLayerVisibleAll (0xb2ce20)
     */
    void ABSetLayerVisibleAll(bool bVisible);

private:
    /**
     * @brief 添加按鈕到管理系統
     * @param sButtonUOL 按鈕 WZ 資源完整路徑
     * @param nID 控件 ID
     * @param nOffsetX X 偏移
     * @param nOffsetY Y 偏移
     * @param bToggle 是否為 toggle 按鈕
     * @param bSkipIDCheck 跳過 ID 檢查
     * @return 創建的按鈕指針
     *
     * IDA: CLayoutMan::AddButton (0xb30310)
     */
    auto AddButton(
        const std::wstring& sButtonUOL,
        unsigned int nID,
        int nOffsetX,
        int nOffsetY,
        bool bToggle = false,
        bool bSkipIDCheck = false
    ) -> std::shared_ptr<UIButton>;

    UIElement* m_pParent{nullptr};  // 父 UI 元素

    // 按鈕映射表（按名稱索引）
    std::map<std::wstring, std::shared_ptr<UIButton>> m_mButtons;

    // 圖層映射表（按名稱索引）
    std::map<std::wstring, std::shared_ptr<WzGr2DLayer>> m_mLayers;

    // 所有控件陣列（按創建順序）
    std::vector<std::shared_ptr<UIElement>> m_aCtrl;

    // 所有圖層陣列
    std::vector<std::shared_ptr<WzGr2DLayer>> m_aLayer;

    // 全局位置偏移
    int m_nOffsetX{0};
    int m_nOffsetY{0};
};

} // namespace ms
```

**Step 2: 創建 LayoutMan.cpp 實作文件骨架**

```cpp
#include "LayoutMan.h"
#include "UIButton.h"
#include "UIElement.h"
#include "graphics/WzGr2DLayer.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

namespace ms
{

LayoutMan::LayoutMan() = default;

LayoutMan::~LayoutMan() = default;

void LayoutMan::Init(UIElement* pParent, int nOffsetX, int nOffsetY)
{
    m_pParent = pParent;
    m_nOffsetX = nOffsetX;
    m_nOffsetY = nOffsetY;
}

void LayoutMan::AutoBuild(
    const std::wstring& sRootUOL,
    int nIdBase,
    int nOffsetX,
    int nOffsetY,
    bool bSetTooltip,
    bool bSameIDCtrl)
{
    // TODO: Task 4
}

auto LayoutMan::ABGetButton(const std::wstring& sName) -> std::shared_ptr<UIButton>
{
    // TODO: Task 2
    return nullptr;
}

void LayoutMan::ABSetButtonShowAll(bool bShow)
{
    // TODO: Task 3
}

void LayoutMan::ABSetButtonEnableAll(bool bEnable)
{
    // TODO: Task 3
}

void LayoutMan::ABSetButtonEnable(const std::wstring& sName, bool bEnable)
{
    // TODO: Task 3
}

void LayoutMan::RegisterLayer(std::shared_ptr<WzGr2DLayer> pLayer, const std::wstring& sName)
{
    // TODO: Task 3
}

auto LayoutMan::ABGetLayer(const std::wstring& sName) -> std::shared_ptr<WzGr2DLayer>
{
    // TODO: Task 2
    return nullptr;
}

void LayoutMan::ABSetLayerVisible(const std::wstring& sName, bool bVisible)
{
    // TODO: Task 3
}

void LayoutMan::ABSetLayerVisibleAll(bool bVisible)
{
    // TODO: Task 3
}

auto LayoutMan::AddButton(
    const std::wstring& sButtonUOL,
    unsigned int nID,
    int nOffsetX,
    int nOffsetY,
    bool bToggle,
    bool bSkipIDCheck) -> std::shared_ptr<UIButton>
{
    // TODO: Task 4
    return nullptr;
}

} // namespace ms
```

**Step 3: 更新 CMakeLists.txt**

在 `src/ui/` 的源文件列表中添加：

```cmake
# 在 CMakeLists.txt 中找到 ui 相關的部分，添加：
src/ui/LayoutMan.cpp
```

**Step 4: 編譯測試**

```bash
cd /home/t4si/Desktop/repos/maplestory/.worktrees/layoutman-implementation
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

Expected: 編譯成功，無錯誤

**Step 5: Commit**

```bash
git add src/ui/LayoutMan.h src/ui/LayoutMan.cpp CMakeLists.txt
git commit -m "feat(ui): add LayoutMan base structure

- Create LayoutMan.h with IDA-based API design
- Create LayoutMan.cpp with method stubs
- Add Init() for initialization
- Reference: IDA Pro CLayoutMan analysis (0xb36170+)"
```

---

## Task 2: 實作查找 API（ABGetButton, ABGetLayer）

**Files:**
- Modify: `src/ui/LayoutMan.cpp`

**Step 1: 實作 ABGetButton**

基於 IDA 分析 (0xb300f0)：

```cpp
auto LayoutMan::ABGetButton(const std::wstring& sName) -> std::shared_ptr<UIButton>
{
    // IDA: ZMap::GetPos
    auto it = m_mButtons.find(sName);
    if (it != m_mButtons.end())
    {
        return it->second;
    }

    // IDA: 返回 static empty reference
    return nullptr;
}
```

**Step 2: 實作 ABGetLayer**

基於 IDA 分析 (0xb35f90)：

```cpp
auto LayoutMan::ABGetLayer(const std::wstring& sName) -> std::shared_ptr<WzGr2DLayer>
{
    // IDA: ZMap::Insert (如果不存在則插入 nullptr)
    auto it = m_mLayers.find(sName);
    if (it != m_mLayers.end())
    {
        return it->second;
    }

    // 未找到返回 nullptr
    return nullptr;
}
```

**Step 3: 編譯測試**

```bash
cd /home/t4si/Desktop/repos/maplestory/.worktrees/layoutman-implementation/build
make -j$(nproc)
```

Expected: 編譯成功

**Step 4: Commit**

```bash
git add src/ui/LayoutMan.cpp
git commit -m "feat(ui): implement ABGetButton and ABGetLayer

- ABGetButton: find button by name in map (IDA: 0xb300f0)
- ABGetLayer: find layer by name in map (IDA: 0xb35f90)
- Return nullptr if not found"
```

---

## Task 3: 實作批量操作和圖層管理 API

**Files:**
- Modify: `src/ui/LayoutMan.cpp`

**Step 1: 實作 ABSetButtonShowAll**

基於 IDA 分析 (0xb2e680)：

```cpp
void LayoutMan::ABSetButtonShowAll(bool bShow)
{
    // IDA: 遍歷 m_mButtons 映射表
    for (auto& [name, pButton] : m_mButtons)
    {
        if (pButton)
        {
            pButton->SetVisible(bShow);
        }
    }
}
```

**Step 2: 實作 ABSetButtonEnableAll**

基於 IDA 分析 (0xb2e770)：

```cpp
void LayoutMan::ABSetButtonEnableAll(bool bEnable)
{
    // IDA: 遍歷 m_mButtons 映射表
    for (auto& [name, pButton] : m_mButtons)
    {
        if (pButton)
        {
            pButton->SetEnabled(bEnable);
        }
    }
}
```

**Step 3: 實作 ABSetButtonEnable**

基於 IDA 分析 (0xb392f0)：

```cpp
void LayoutMan::ABSetButtonEnable(const std::wstring& sName, bool bEnable)
{
    // IDA: 先查找按鈕
    auto pButton = ABGetButton(sName);
    if (pButton)
    {
        pButton->SetEnabled(bEnable);
    }
}
```

**Step 4: 實作 RegisterLayer**

基於 IDA 分析 (0xb35d40)：

```cpp
void LayoutMan::RegisterLayer(std::shared_ptr<WzGr2DLayer> pLayer, const std::wstring& sName)
{
    if (!pLayer)
    {
        return;
    }

    // IDA: 添加到圖層陣列
    m_aLayer.push_back(pLayer);

    // IDA: 如果提供名稱，註冊到映射表
    if (!sName.empty())
    {
        m_mLayers[sName] = pLayer;
    }
}
```

**Step 5: 實作 ABSetLayerVisible**

基於 IDA 分析 (0xb38ba0)：

```cpp
void LayoutMan::ABSetLayerVisible(const std::wstring& sName, bool bVisible)
{
    // IDA: 先查找圖層
    auto pLayer = ABGetLayer(sName);
    if (pLayer)
    {
        pLayer->SetVisible(bVisible);
    }
}
```

**Step 6: 實作 ABSetLayerVisibleAll**

基於 IDA 分析 (0xb2ce20)：

```cpp
void LayoutMan::ABSetLayerVisibleAll(bool bVisible)
{
    // IDA: 遍歷 m_aLayer 陣列
    for (auto& pLayer : m_aLayer)
    {
        if (pLayer)
        {
            pLayer->SetVisible(bVisible);
        }
    }
}
```

**Step 7: 編譯測試**

```bash
cd /home/t4si/Desktop/repos/maplestory/.worktrees/layoutman-implementation/build
make -j$(nproc)
```

Expected: 編譯成功

**Step 8: Commit**

```bash
git add src/ui/LayoutMan.cpp
git commit -m "feat(ui): implement batch operations and layer management

- ABSetButtonShowAll: set visibility for all buttons (IDA: 0xb2e680)
- ABSetButtonEnableAll: set enabled state for all buttons (IDA: 0xb2e770)
- ABSetButtonEnable: set enabled state for specific button (IDA: 0xb392f0)
- RegisterLayer: register layer to map and array (IDA: 0xb35d40)
- ABSetLayerVisible: set layer visibility (IDA: 0xb38ba0)
- ABSetLayerVisibleAll: set visibility for all layers (IDA: 0xb2ce20)"
```

---

## Task 4: 實作 AddButton 方法

**Files:**
- Modify: `src/ui/LayoutMan.cpp`
- Modify: `src/ui/UIButton.h`
- Modify: `src/ui/UIButton.cpp`

**Step 1: 在 UIButton 添加 LoadFromUOL 方法聲明**

修改 `src/ui/UIButton.h`：

```cpp
// 在 UIButton 類中添加：

/**
 * @brief 從 WZ UOL 路徑加載按鈕資源
 * @param sUOL WZ 資源完整路徑
 * @return 是否成功加載
 */
auto LoadFromUOL(const std::wstring& sUOL) -> bool;
```

**Step 2: 實作 UIButton::LoadFromUOL**

修改 `src/ui/UIButton.cpp`：

```cpp
auto UIButton::LoadFromUOL(const std::wstring& sUOL) -> bool
{
    // 從 WzResMan 獲取屬性
    auto& resMan = WzResMan::GetInstance();

    // 轉換 wstring 到 string (WzResMan 使用 UTF-8)
    std::string sPath;
    sPath.reserve(sUOL.size());
    for (wchar_t wc : sUOL)
    {
        if (wc < 128)
        {
            sPath.push_back(static_cast<char>(wc));
        }
    }

    auto pProp = resMan.GetProperty(sPath);
    if (!pProp)
    {
        return false;
    }

    // 使用現有的 LoadFromProperty
    return LoadFromProperty(pProp);
}
```

**Step 3: 實作 AddButton**

基於 IDA 分析 (0xb30310)：

```cpp
auto LayoutMan::AddButton(
    const std::wstring& sButtonUOL,
    unsigned int nID,
    int nOffsetX,
    int nOffsetY,
    bool bToggle,
    bool bSkipIDCheck) -> std::shared_ptr<UIButton>
{
    // IDA: 創建按鈕對象 (0xb30424)
    auto pButton = std::make_shared<UIButton>();

    // IDA: 從 UOL 路徑加載資源
    if (!pButton->LoadFromUOL(sButtonUOL))
    {
        return nullptr;
    }

    // IDA: 設置位置 (0xb3048b-0xb30498)
    // 使用全局偏移 + 局部偏移
    pButton->SetPosition(nOffsetX + m_nOffsetX, nOffsetY + m_nOffsetY);

    // IDA: 設置父元素
    if (m_pParent)
    {
        pButton->SetParent(m_pParent);
    }

    // TODO: 設置控件 ID (需要在 UIElement 添加 m_nCtrlId)
    // pButton->m_nCtrlId = nID;

    // IDA: ID 檢查和替換邏輯 (0xb304a1-0xb304d0)
    if (!bSkipIDCheck && nID != 0)
    {
        // 檢查是否已存在相同 ID 的控件
        for (size_t i = 0; i < m_aCtrl.size(); ++i)
        {
            // TODO: 需要 GetID() 方法
            // if (m_aCtrl[i]->GetID() == nID)
            // {
            //     // 替換現有控件
            //     m_aCtrl[i] = pButton;
            //     return pButton;
            // }
        }
    }

    // IDA: 添加到控件陣列 (0xb305a1-0xb30696)
    m_aCtrl.push_back(pButton);

    return pButton;
}
```

**Step 4: 編譯測試**

```bash
cd /home/t4si/Desktop/repos/maplestory/.worktrees/layoutman-implementation/build
make -j$(nproc)
```

Expected: 編譯成功

**Step 5: Commit**

```bash
git add src/ui/LayoutMan.cpp src/ui/UIButton.h src/ui/UIButton.cpp
git commit -m "feat(ui): implement AddButton method

- AddButton: create button from UOL path (IDA: 0xb30310)
- UIButton::LoadFromUOL: load button resources from WZ path
- Apply global offset (m_nOffsetX, m_nOffsetY)
- Add to control array (m_aCtrl)
- TODO: ID check mechanism (需要 UIElement::GetID())"
```

---

## Task 5: 實作 AutoBuild 核心邏輯

**Files:**
- Modify: `src/ui/LayoutMan.cpp`

**Step 1: 實作 AutoBuild - WZ 屬性枚舉**

基於 IDA 分析 (0xb36170 - 0xb364d3)：

```cpp
void LayoutMan::AutoBuild(
    const std::wstring& sRootUOL,
    int nIdBase,
    int nOffsetX,
    int nOffsetY,
    bool bSetTooltip,
    bool bSameIDCtrl)
{
    // IDA: 從 WzResMan 獲取根屬性 (0xb362a7-0xb36330)
    auto& resMan = WzResMan::GetInstance();

    // 轉換 wstring 到 string
    std::string sRootPath;
    sRootPath.reserve(sRootUOL.size());
    for (wchar_t wc : sRootUOL)
    {
        if (wc < 128)
        {
            sRootPath.push_back(static_cast<char>(wc));
        }
    }

    auto pRoot = resMan.GetProperty(sRootPath);
    if (!pRoot)
    {
        return;
    }

    // IDA: 枚舉所有子屬性 (0xb36435-0xb364d3)
    int currentId = nIdBase;

    for (auto& [name, pProp] : pRoot->GetChildren())
    {
        // 轉換 string 到 wstring
        std::wstring wName;
        wName.reserve(name.size());
        for (char c : name)
        {
            wName.push_back(static_cast<wchar_t>(c));
        }

        // TODO: 下一步處理
        ProcessChildProperty(wName, pProp, sRootUOL, currentId,
                           nOffsetX, nOffsetY, bSetTooltip, bSameIDCtrl);
    }
}
```

**Step 2: 添加 ProcessChildProperty 私有方法**

在 `LayoutMan.h` 中添加聲明：

```cpp
private:
    /**
     * @brief 處理單個子屬性
     */
    void ProcessChildProperty(
        const std::wstring& wName,
        std::shared_ptr<WzProperty> pProp,
        const std::wstring& sRootUOL,
        int& currentId,
        int nOffsetX,
        int nOffsetY,
        bool bSetTooltip,
        bool bSameIDCtrl
    );
```

**Step 3: 實作 type:name 解析邏輯**

基於 IDA 分析 (0xb36553 - 0xb365a8)：

```cpp
void LayoutMan::ProcessChildProperty(
    const std::wstring& wName,
    std::shared_ptr<WzProperty> pProp,
    const std::wstring& sRootUOL,
    int& currentId,
    int nOffsetX,
    int nOffsetY,
    bool bSetTooltip,
    bool bSameIDCtrl)
{
    // IDA: 查找冒號 (0xb36553-0xb3655f)
    auto colonPos = wName.find(L':');
    if (colonPos == std::wstring::npos)
    {
        // 沒有冒號，跳過
        return;
    }

    // IDA: 分割 type 和 name (0xb365a8-0xb365b2)
    std::wstring sType = wName.substr(0, colonPos);       // "button"
    std::wstring sCtrlName = wName.substr(colonPos + 1);  // "GoWorld"

    // IDA: 比較類型字符串 (0xb365ed)
    if (sType == L"button")
    {
        ProcessButton(wName, sCtrlName, pProp, sRootUOL, currentId,
                     nOffsetX, nOffsetY, bSameIDCtrl);
    }
    else if (sType == L"layer")
    {
        // TODO: Task 6
    }
    // 其他類型...
}
```

**Step 4: 添加 ProcessButton 方法**

在 `LayoutMan.h` 添加聲明：

```cpp
private:
    void ProcessButton(
        const std::wstring& wFullName,
        const std::wstring& sCtrlName,
        std::shared_ptr<WzProperty> pProp,
        const std::wstring& sRootUOL,
        int& currentId,
        int nOffsetX,
        int nOffsetY,
        bool bSameIDCtrl
    );
```

在 `LayoutMan.cpp` 實作，基於 IDA 分析 (0xb36625 - 0xb36b90)：

```cpp
void LayoutMan::ProcessButton(
    const std::wstring& wFullName,
    const std::wstring& sCtrlName,
    std::shared_ptr<WzProperty> pProp,
    const std::wstring& sRootUOL,
    int& currentId,
    int nOffsetX,
    int nOffsetY,
    bool bSameIDCtrl)
{
    // IDA: 讀取配置參數

    // 1. 讀取 "id" (0xb36707-0xb36755)
    int nID = currentId;
    auto pIdProp = pProp->GetChild("id");
    if (pIdProp)
    {
        nID = pIdProp->GetInt(currentId);
    }
    nID += currentId;  // IDA: 加上基礎 ID
    currentId++;       // 自動遞增

    // 2. 讀取 "drawBack" (0xb3679f-0xb367e6)
    bool bDrawBack = false;
    auto pDrawBackProp = pProp->GetChild("drawBack");
    if (pDrawBackProp)
    {
        bDrawBack = pDrawBackProp->GetInt(0) != 0;
    }

    // 3. 讀取 "toggle" (0xb3687c-0xb368de)
    bool bToggle = false;
    auto pToggleProp = pProp->GetChild("toggle");
    if (pToggleProp)
    {
        bToggle = pToggleProp->GetInt(0) != 0;
    }

    // 4. 構建完整 UOL 路徑 (0xb368b0-0xb368ce)
    std::wstring sButtonUOL = sRootUOL + L"/" + sCtrlName;

    // 5. 調用 AddButton (0xb36908)
    auto pButton = AddButton(sButtonUOL, nID, nOffsetX, nOffsetY, bToggle, bSameIDCtrl);
    if (!pButton)
    {
        return;
    }

    // 6. 讀取並設置 "enable" (0xb369c4-0xb36a18)
    bool bEnable = true;
    auto pEnableProp = pProp->GetChild("enable");
    if (pEnableProp)
    {
        bEnable = pEnableProp->GetInt(1) != 0;
    }
    pButton->SetEnabled(bEnable);

    // 7. 讀取並設置 "visible" (0xb36a63-0xb36ab0)
    bool bVisible = true;
    auto pVisibleProp = pProp->GetChild("visible");
    if (pVisibleProp)
    {
        bVisible = pVisibleProp->GetInt(1) != 0;
    }
    pButton->SetVisible(bVisible);

    // 8. 註冊到映射表
    m_mButtons[sCtrlName] = pButton;
}
```

**Step 5: 編譯測試**

```bash
cd /home/t4si/Desktop/repos/maplestory/.worktrees/layoutman-implementation/build
make -j$(nproc)
```

Expected: 編譯成功

**Step 6: Commit**

```bash
git add src/ui/LayoutMan.h src/ui/LayoutMan.cpp
git commit -m "feat(ui): implement AutoBuild core logic

- AutoBuild: enumerate WZ children (IDA: 0xb36170)
- ProcessChildProperty: parse 'type:name' format (IDA: 0xb36553)
- ProcessButton: handle button creation (IDA: 0xb36625)
- Read config params: id, drawBack, toggle, enable, visible
- Build full UOL path and call AddButton
- Register button to m_mButtons map"
```

---

## Task 6: 實作 Layer 類型處理

**Files:**
- Modify: `src/ui/LayoutMan.cpp`

**Step 1: 添加 ProcessLayer 方法**

在 `LayoutMan.h` 添加聲明：

```cpp
private:
    void ProcessLayer(
        const std::wstring& wFullName,
        const std::wstring& sCtrlName,
        std::shared_ptr<WzProperty> pProp,
        const std::wstring& sRootUOL,
        int nOffsetX,
        int nOffsetY
    );
```

**Step 2: 在 ProcessChildProperty 中調用**

修改 `ProcessChildProperty`：

```cpp
else if (sType == L"layer")
{
    ProcessLayer(wName, sCtrlName, pProp, sRootUOL, nOffsetX, nOffsetY);
}
```

**Step 3: 實作 ProcessLayer**

```cpp
void LayoutMan::ProcessLayer(
    const std::wstring& wFullName,
    const std::wstring& sCtrlName,
    std::shared_ptr<WzProperty> pProp,
    const std::wstring& sRootUOL,
    int nOffsetX,
    int nOffsetY)
{
    // 1. 獲取 canvas
    auto pCanvas = pProp->GetCanvas();
    if (!pCanvas)
    {
        return;
    }

    // 2. 獲取 origin
    auto origin = pCanvas->GetOrigin();

    // 3. 創建圖層
    if (!m_pParent)
    {
        return;
    }

    // TODO: 需要從 m_pParent 獲取 WzGr2D 實例
    // auto& gr = m_pParent->GetGraphics();
    // auto pLayer = gr.CreateLayer(
    //     nOffsetX + m_nOffsetX + origin.x,
    //     nOffsetY + m_nOffsetY + origin.y,
    //     pCanvas->GetWidth(),
    //     pCanvas->GetHeight(),
    //     100  // z-order
    // );

    // 4. 插入 canvas
    // pLayer->InsertCanvas(pCanvas, 0, 255, 255);

    // 5. 註冊圖層
    // RegisterLayer(pLayer, sCtrlName);
}
```

**Step 4: 編譯測試**

```bash
cd /home/t4si/Desktop/repos/maplestory/.worktrees/layoutman-implementation/build
make -j$(nproc)
```

Expected: 編譯成功

**Step 5: Commit**

```bash
git add src/ui/LayoutMan.h src/ui/LayoutMan.cpp
git commit -m "feat(ui): add ProcessLayer method skeleton

- ProcessLayer: handle layer type in AutoBuild
- Get canvas from WZ property
- TODO: Create layer (需要從 UIElement 獲取 WzGr2D)
- TODO: Register layer to m_mLayers"
```

---

## Task 7: 測試 LayoutMan 與 UIChannelSelect 整合

**Files:**
- Modify: `src/ui/UIChannelSelect.h`
- Modify: `src/ui/UIChannelSelect.cpp`

**Step 1: 在 UIChannelSelect 添加 LayoutMan 成員**

修改 `UIChannelSelect.h`：

```cpp
#include "LayoutMan.h"

class UIChannelSelect
{
private:
    // 添加：
    std::unique_ptr<LayoutMan> m_pLayoutMan;
};
```

**Step 2: 在 UIChannelSelect::OnCreate 中使用 LayoutMan**

修改 `UIChannelSelect.cpp`：

```cpp
void UIChannelSelect::OnCreate(...)
{
    // 創建 LayoutMan
    m_pLayoutMan = std::make_unique<LayoutMan>();
    m_pLayoutMan->Init(this, 0, 0);

    // 使用 AutoBuild 替代手動創建按鈕
    std::wstring sRootUOL = L"UI/Login.img/WorldSelect/BtChannel/test";
    m_pLayoutMan->AutoBuild(sRootUOL, 0, 0, 0, true, false);

    // 測試查找按鈕
    auto pGoWorldBtn = m_pLayoutMan->ABGetButton(L"GoWorld");
    if (pGoWorldBtn)
    {
        LOG_INFO("Found GoWorld button via LayoutMan");
    }

    // ... 其餘代碼
}
```

**Step 3: 編譯測試**

```bash
cd /home/t4si/Desktop/repos/maplestory/.worktrees/layoutman-implementation/build
make -j$(nproc)
```

Expected: 編譯成功

**Step 4: 運行測試**

```bash
cd /home/t4si/Desktop/repos/maplestory/.worktrees/layoutman-implementation/build
./maplestory
```

Expected:
- 進入 Login 界面
- 選擇 World 後顯示 ChannelSelect
- 按鈕通過 LayoutMan 創建
- 日誌顯示 "Found GoWorld button via LayoutMan"

**Step 5: Commit**

```bash
git add src/ui/UIChannelSelect.h src/ui/UIChannelSelect.cpp
git commit -m "feat(ui): integrate LayoutMan with UIChannelSelect

- Add m_pLayoutMan member to UIChannelSelect
- Use AutoBuild to create buttons from WZ
- Test ABGetButton with 'GoWorld' button
- Replace manual button creation with LayoutMan"
```

---

## Task 8: 文檔更新和最終測試

**Files:**
- Create: `docs/LayoutMan.md`
- Modify: `docs/ARCHITECTURE.md`

**Step 1: 創建 LayoutMan 使用文檔**

```markdown
# LayoutMan 使用指南

## 概述

LayoutMan 是 UI 自動化構建和管理系統，從 WZ 資源自動創建 UI 元素。

## 基本使用

### 初始化

\`\`\`cpp
auto layoutMan = std::make_unique<LayoutMan>();
layoutMan->Init(parentElement, 0, 0);
\`\`\`

### 自動構建

\`\`\`cpp
layoutMan->AutoBuild(
    L"UI/Login.img/WorldSelect/BtChannel/test",  // WZ 根路徑
    0,      // ID 起始值
    0,      // X 偏移
    0,      // Y 偏移
    true,   // 設置 tooltip
    false   // 允許相同 ID
);
\`\`\`

### 查找控件

\`\`\`cpp
auto pButton = layoutMan->ABGetButton(L"GoWorld");
auto pLayer = layoutMan->ABGetLayer(L"bg");
\`\`\`

### 批量操作

\`\`\`cpp
layoutMan->ABSetButtonShowAll(false);
layoutMan->ABSetButtonEnableAll(false);
layoutMan->ABSetButtonEnable(L"GoWorld", true);
\`\`\`

## WZ 資源命名約定

AutoBuild 使用 `type:name` 格式：

- `button:GoWorld` - 按鈕，名稱 "GoWorld"
- `layer:bg` - 圖層，名稱 "bg"

## IDA Pro 參考

完整實作基於 IDA Pro 分析：
- AutoBuild: 0xb36170
- AddButton: 0xb30310
- ABGetButton: 0xb300f0

詳見：`docs/plans/2026-02-01-layoutman-design.md`
\`\`\`

**Step 2: 更新架構文檔**

在 `docs/ARCHITECTURE.md` 的 UI 系統部分添加：

```markdown
#### LayoutMan

UI 自動化構建和管理系統。負責：
- 從 WZ 資源自動創建 UI 元素
- 按名稱管理按鈕和圖層
- 提供批量操作接口

使用映射表 (std::map) 管理控件，支持 `type:name` 格式的 WZ 屬性解析。

詳見：`docs/LayoutMan.md`
```

**Step 3: 最終編譯測試**

```bash
cd /home/t4si/Desktop/repos/maplestory/.worktrees/layoutman-implementation/build
make clean
make -j$(nproc)
```

Expected: 完整編譯成功，無警告

**Step 4: 完整功能測試**

運行程序並測試：
1. 啟動遊戲
2. 進入 Login 界面
3. 選擇 World
4. 驗證 ChannelSelect 按鈕正確顯示
5. 測試按鈕點擊功能
6. 檢查日誌輸出

**Step 5: Commit**

```bash
git add docs/LayoutMan.md docs/ARCHITECTURE.md
git commit -m "docs: add LayoutMan documentation

- Create LayoutMan.md usage guide
- Update ARCHITECTURE.md with LayoutMan section
- Reference IDA Pro analysis addresses"
```

---

## 驗證清單

完成所有 Task 後，驗證以下項目：

### 編譯檢查
- [ ] 無編譯錯誤
- [ ] 無編譯警告
- [ ] 所有新文件已添加到 CMakeLists.txt

### 功能檢查
- [ ] LayoutMan::Init 正確初始化
- [ ] ABGetButton 可以查找按鈕
- [ ] ABGetLayer 可以查找圖層
- [ ] ABSetButtonShowAll 批量設置可見性
- [ ] AutoBuild 正確解析 type:name 格式
- [ ] ProcessButton 正確讀取配置參數
- [ ] UIChannelSelect 使用 LayoutMan 創建按鈕

### IDA Pro 一致性檢查
- [ ] 函數簽名與 IDA 分析一致
- [ ] 參數順序與 IDA 分析一致
- [ ] 邏輯流程與 IDA 分析一致
- [ ] 數據結構訪問與 IDA 分析一致

### 測試
- [ ] 遊戲可正常啟動
- [ ] Login 界面顯示正確
- [ ] ChannelSelect 按鈕正確創建
- [ ] 按鈕可正常點擊
- [ ] 日誌無錯誤輸出

### 文檔
- [ ] LayoutMan.md 完整
- [ ] ARCHITECTURE.md 已更新
- [ ] 設計文檔已創建

---

## 已知限制和 TODO

### 未實作功能
- [ ] ID 檢查機制（需要 UIElement::GetID()）
- [ ] Edit 控件類型處理
- [ ] Combo 控件類型處理
- [ ] Tooltip 控件類型處理
- [ ] Layer 創建（需要從 UIElement 獲取 WzGr2D）

### 需要後續改進
- [ ] 字符串轉換優化（wstring ↔ string）
- [ ] 錯誤處理增強
- [ ] 性能優化（大量控件時）

---

## 參考資料

- **設計文檔**: `docs/plans/2026-02-01-layoutman-design.md`
- **IDA Pro 分析**: CLayoutMan 類 (MapleStory v1029)
- **使用示例**: `src/ui/UIChannelSelect.cpp`
- **相關文檔**: `docs/CUIChannelSelect_decompiled.md`
