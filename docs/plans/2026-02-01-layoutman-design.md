# LayoutMan UI 管理系統設計

**基於 IDA Pro 逆向工程分析 - 確保與原版 MapleStory v1029 行為一致**

## 概述

LayoutMan 是 MapleStory 客戶端的 UI 自動化構建和管理系統。負責從 WZ 資源自動創建 UI 元素（按鈕、圖層等），並提供統一的查找和管理接口。

**IDA Pro 分析來源**：
- `CLayoutMan::AutoBuild` (0xb36170)
- `CLayoutMan::AddButton` (0xb30310)
- `CLayoutMan::ABGetButton` (0xb300f0)
- `CLayoutMan::ABGetLayer` (0xb35f90)
- `CLayoutMan::RegisterLayer` (0xb35d40)

## 核心架構

### 數據結構

基於 IDA 分析的 CLayoutMan 類結構：

```cpp
class LayoutMan
{
private:
    CWnd* m_pWnd;  // 父視窗（0x4 offset）

    // 按鈕映射表（按名稱索引）
    // IDA: ZMap<ZXString<unsigned short>, ZRef<CCtrlOriginButton>>
    std::map<std::wstring, std::shared_ptr<UIButton>> m_mButtons;

    // 圖層映射表（按名稱索引）
    // IDA: ZMap<ZXString<unsigned short>, _com_ptr_t<IWzGr2DLayer>>
    std::map<std::wstring, std::shared_ptr<WzGr2DLayer>> m_mLayers;

    // 所有控件陣列（按創建順序）
    // IDA: ZArray<ZRef<CCtrlWnd>>
    std::vector<std::shared_ptr<UIElement>> m_aCtrl;

    // 所有圖層陣列
    // IDA: ZArray<_com_ptr_t<IWzGr2DLayer>>
    std::vector<std::shared_ptr<WzGr2DLayer>> m_aLayer;

    // 全局位置偏移
    int m_nOffsetX;  // 0x8 offset
    int m_nOffsetY;  // 0xC offset
};
```

## AutoBuild 實作（核心功能）

### IDA 分析結果

**函數簽名**（0xb36170）：
```cpp
void __thiscall CLayoutMan::AutoBuild(
    CLayoutMan *this,
    wchar_t *sRootUOL,      // WZ 資源根路徑
    int nIdBase,            // 控件 ID 起始值
    int nOffsetX,           // X 偏移
    int nOffsetY,           // Y 偏移
    bool bSetTooltip,       // 是否設置 tooltip
    bool bSameIDCtrl        // 允許相同 ID 控件
);
```

### WZ 資源命名約定

**IDA 分析發現**（0xb365ed - 0xb3661f）：

AutoBuild 使用 `type:name` 格式解析 WZ 屬性名稱：
- 格式：`type:name`
- 範例：
  - `button:GoWorld` - 按鈕，名稱為 "GoWorld"
  - `layer:bg` - 圖層，名稱為 "bg"
  - `edit:username` - 編輯框，名稱為 "username"

**實作邏輯**（0xb36553 - 0xb365a8）：
```cpp
// 1. 查找冒號位置
wcschr(name, L':')

// 2. 分割字符串
sItemName = name.Left(colonPos)      // "button"
sCtrlName = name.Mid(colonPos + 1)   // "GoWorld"

// 3. 比較類型
if (sItemName == "button") { ... }
```

### Button 創建流程

**IDA 完整分析**（0xb36625 - 0xb36b90）：

```cpp
if (sType == L"button")
{
    // 1. 獲取按鈕屬性節點
    // 0xb3662d: pProp->Getitem(name) → pButtonProp
    auto pButtonProp = pRoot->Getitem(name);  // "button:GoWorld" 節點

    // 2. 讀取配置參數
    // 0xb36707-0xb36755: 讀取 "id"
    int nID = pButtonProp->Getitem("id")->GetInt(nIdBase);
    nID += nIdBase;  // 加上基礎 ID

    // 0xb3679f-0xb367e6: 讀取 "drawBack"
    bool bDrawBack = pButtonProp->Getitem("drawBack")->GetInt(0) != 0;

    // 0xb36830-0xb3687b: 讀取 "moveIVRect"
    bool bOnMoveIVRect = pButtonProp->Getitem("moveIVRect")->GetInt(0) != 0;

    // 0xb3687c-0xb368de: 讀取 "toggle"
    bool bToggle = pButtonProp->Getitem("toggle")->GetInt(0) != 0;

    // 3. 構建完整 UOL 路徑
    // 0xb368b0-0xb368ce: 字符串連接
    std::wstring sButtonUOL = std::wstring(sRootUOL) + L"/" + sCtrlName;

    // 4. 調用 AddButton
    // 0xb36908: 調用 AddButton
    auto pButton = AddButton(
        sButtonUOL.c_str(),  // 完整 UOL 路徑
        nID,
        nOffsetX,
        nOffsetY,
        nullptr,  // pParam
        0xFF,     // nAlpha
        bToggle,
        bOnMoveIVRect,
        bSameIDCtrl
    );

    // 5. 讀取並設置額外屬性
    // 0xb369c4-0xb36a18: 讀取 "enable"
    bool bEnable = pButtonProp->Getitem("enable")->GetInt(1) != 0;
    pButton->SetEnable(bEnable);

    // 0xb36a63-0xb36ab0: 讀取 "visible"
    bool bVisible = pButtonProp->Getitem("visible")->GetInt(1) != 0;
    pButton->SetVisible(bVisible);

    // 0xb36b02-0xb36b08: 設置 drawBack
    pButton->SetDrawBack(bDrawBack);

    // 0xb36b08-0xb36b90: 讀取 "selfdisable"
    bool bSelfDisable = pButtonProp->Getitem("selfdisable")->GetInt(0) != 0;
    // (設置 self-disable 屬性)
}
```

### AddButton 實作

**IDA 分析**（0xb30310）：

```cpp
ZRef<CCtrlOriginButton>* CLayoutMan::AddButton(
    wchar_t* sButtonUOL,           // 按鈕 WZ 資源完整路徑
    unsigned int nID,              // 控件 ID
    int nOffsetX,                  // X 偏移
    int nOffsetY,                  // Y 偏移
    CCtrlButton::CREATEPARAM* pParam,  // 創建參數
    int nAlpha,                    // 透明度
    int bToggle,                   // 是否為 toggle 按鈕
    int bOnMoveIVRect,             // 移動時更新區域
    ZRefCounted* bSkipIDCheck      // 跳過 ID 檢查
)
{
    // 1. 準備創建參數
    CCtrlButton::CREATEPARAM paramButton;
    if (pParam) {
        paramButton = *pParam;
    }

    // 2. 設置 UOL 路徑（如果提供）
    if (sButtonUOL) {
        paramButton.sUOL = sButtonUOL;  // 儲存 UOL 路徑
    }

    paramButton.bToggle = bToggle;
    paramButton.bOnMoveIVRect = bOnMoveIVRect;

    // 3. 創建按鈕對象
    auto pButton = new CCtrlOriginButton();

    // 4. 初始化按鈕（0xb30473-0xb30498）
    // 調用按鈕的初始化方法，傳入：
    // - m_pWnd (父視窗)
    // - nID
    // - nOffsetX + m_nOffsetX (絕對位置)
    // - nOffsetY + m_nOffsetY
    // - paramButton (包含 sUOL)
    pButton->Initialize(
        m_pWnd,
        nID,
        nOffsetX + m_nOffsetX,
        nOffsetY + m_nOffsetY,
        &paramButton,
        nAlpha
    );

    // 5. ID 檢查和控件管理（0xb304a1-0xb304d0）
    if (!bSkipIDCheck && nID != 0) {
        // 檢查是否已存在相同 ID 的按鈕
        for (int i = 0; i < m_aCtrl.size(); i++) {
            if (m_aCtrl[i]->GetID() == nID) {
                // 替換現有按鈕
                m_aCtrl[i] = pButton;
                return pButton;
            }
        }
    }

    // 6. 添加到控件陣列（0xb305a1-0xb30696）
    m_aCtrl.push_back(pButton);

    return pButton;
}
```

**關鍵點**：
- AddButton 接受 **UOL 路徑字符串**，不是 WzProperty 對象
- 按鈕內部（或其 Initialize 方法）根據 sUOL 從 WzResMan 加載資源
- 支持 ID 檢查和替換機制

## 查找 API 實作

### ABGetButton

**IDA 分析**（0xb300f0）：

```cpp
ZRef<CCtrlOriginButton>* CLayoutMan::ABGetButton(wchar_t* sName)
{
    // 1. 轉換為 ZXString
    ZXString<unsigned short> key = sName;

    // 2. 在映射表中查找（0xb30156）
    auto pos = m_mButtons.GetPos(key);

    // 3. 返回結果
    if (pos) {
        return &pos->value;  // 返回按鈕引用
    }

    // 4. 未找到，返回空引用
    static ZRef<CCtrlOriginButton> pEmpty;
    return &pEmpty;
}
```

### ABGetLayer

**IDA 分析**（0xb35f90）：

```cpp
_com_ptr_t<IWzGr2DLayer>* CLayoutMan::ABGetLayer(wchar_t* sName)
{
    // 1. 轉換為 ZXString
    ZXString<unsigned short> key = sName;

    // 2. 在映射表中查找並返回
    return &m_mLayers.Insert(key, nullptr)->value;
}
```

## 批量操作 API

### ABSetButtonShowAll

**IDA 分析**（0xb2e680）：

```cpp
void CLayoutMan::ABSetButtonShowAll(int bShow)
{
    // 遍歷所有按鈕
    for (auto& [name, pButton] : m_mButtons) {
        pButton->SetShow(bShow, 0);
    }
}
```

### ABSetLayerVisible

**IDA 分析**（0xb38ba0）：

```cpp
void CLayoutMan::ABSetLayerVisible(wchar_t* sName, int bShow)
{
    // 1. 獲取圖層
    auto pLayer = ABGetLayer(sName);

    // 2. 設置可見性
    if (pLayer && pLayer->m_pInterface) {
        pLayer->m_pInterface->SetVisible(bShow);
    }
}
```

## 圖層管理

### RegisterLayer

**IDA 分析**（0xb35d40）：

```cpp
void CLayoutMan::RegisterLayer(
    _com_ptr_t<IWzGr2DLayer> pLayer,
    const wchar_t* sName)
{
    if (pLayer.m_pInterface) {
        // 1. 添加到圖層陣列
        m_aLayer.push_back(pLayer);

        // 2. 如果提供名稱，註冊到映射表
        if (sName) {
            ZXString<unsigned short> key = sName;
            m_mLayers[key] = pLayer;
        }
    }
}
```

## 實作檢查清單

### 階段 1：基礎結構
- [ ] 創建 `LayoutMan.h` 和 `LayoutMan.cpp`
- [ ] 定義核心數據結構（m_mButtons, m_mLayers, m_aCtrl, m_aLayer）
- [ ] 實作 `Init()` 方法

### 階段 2：查找 API
- [ ] 實作 `ABGetButton()`（對照 0xb300f0）
- [ ] 實作 `ABGetLayer()`（對照 0xb35f90）
- [ ] 測試基本查找功能

### 階段 3：按鈕管理
- [ ] 實作 `AddButton()`（對照 0xb30310）
  - [ ] 參數處理和驗證
  - [ ] 按鈕創建和初始化
  - [ ] ID 檢查機制
  - [ ] 控件陣列管理
- [ ] 實作批量操作 API
  - [ ] `ABSetButtonShowAll()`（對照 0xb2e680）
  - [ ] `ABSetButtonEnableAll()`（對照 0xb2e770）
  - [ ] `ABSetButtonEnable()`（對照 0xb392f0）

### 階段 4：圖層管理
- [ ] 實作 `RegisterLayer()`（對照 0xb35d40）
- [ ] 實作 `ABSetLayerVisible()`（對照 0xb38ba0）
- [ ] 實作 `ABSetLayerVisibleAll()`（對照 0xb2ce20）

### 階段 5：AutoBuild
- [ ] 實作 WZ 枚舉邏輯
- [ ] 實作 `type:name` 解析
- [ ] 實作 button 類型處理（對照 0xb36625 - 0xb36b90）
  - [ ] 讀取配置參數（id, drawBack, toggle, enable, visible）
  - [ ] 構建完整 UOL 路徑
  - [ ] 調用 AddButton
  - [ ] 設置額外屬性
- [ ] 實作 layer 類型處理
- [ ] 實作其他控件類型（edit, combo, tooltip）

### 階段 6：整合測試
- [ ] 測試 UIChannelSelect 使用 LayoutMan
- [ ] 驗證按鈕創建和顯示
- [ ] 驗證圖層管理
- [ ] 驗證與原版行為一致性

## 驗證方法

### IDA Pro 比對

每個實作的函數都需要與 IDA 反編譯結果比對：

1. **參數順序和類型** - 確保與 IDA 函數簽名一致
2. **邏輯流程** - 確保分支條件、循環邏輯相同
3. **數據結構訪問** - 確保成員變數 offset 和類型正確
4. **API 調用** - 確保調用的子函數和參數順序一致

### 測試案例

基於 `CUIChannelSelect::OnCreate` (0xbc4780) 的實際使用：

```cpp
// 測試 AutoBuild
layoutMan->AutoBuild(L"UI/Login.img/WorldSelect/BtChannel/test", 0, 0, 0, 1, 0);

// 測試按鈕查找
auto pBtn = layoutMan->ABGetButton(L"GoWorld");
assert(pBtn != nullptr);

// 測試批量操作
layoutMan->ABSetButtonShowAll(false);
layoutMan->ABSetButtonEnableAll(false);

// 測試單個按鈕操作
layoutMan->ABSetButtonEnable(L"GoWorld", true);

// 測試圖層操作
layoutMan->ABSetLayerVisible(L"bg", true);
```

## 依賴項

### 現有組件
- `WzResMan` - WZ 資源管理器
- `WzProperty` - WZ 屬性節點
- `UIButton` / `CCtrlOriginButton` - 按鈕控件
- `WzGr2DLayer` - 圖層系統
- `CWnd` - 視窗基類

### 需要實作的組件
- `CCtrlButton::CREATEPARAM` - 按鈕創建參數結構
- `UIButton::Initialize()` - 按鈕初始化方法（接受 sUOL 參數）

## 參考資料

- IDA Pro 反編譯結果：`CLayoutMan` 類相關函數
- 文檔：`docs/CUIChannelSelect_decompiled.md` - 展示 LayoutMan 的實際使用
- 原版 MapleStory v1029 客戶端

## 注意事項

1. **座標系統**：AddButton 使用 `nOffsetX + m_nOffsetX` 計算絕對位置
2. **字符串處理**：所有名稱使用 `wchar_t*` (UTF-16)
3. **引用計數**：使用 `std::shared_ptr` 替代原版的 `ZRef<T>`
4. **COM 接口**：圖層使用 `std::shared_ptr` 替代 `_com_ptr_t`
5. **ID 管理**：支持自動遞增 ID 和 ID 衝突檢查
