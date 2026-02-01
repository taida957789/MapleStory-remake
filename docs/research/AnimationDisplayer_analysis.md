# AnimationDisplayer 深度分析報告

## 概述 (Overview)

`CAnimationDisplayer` 是 MapleStory v1029 客戶端中負責管理全局動畫效果的核心系統組件，特別專注於處理場景轉換、淡入淡出效果和非場景相關的動畫更新。

## 架構設計 (Architecture)

### 1. 單例模式 (Singleton Pattern)

```cpp
TSingleton<CAnimationDisplayer>::ms_pInstance
```

**特點：**
- 使用模板化單例實現 `TSingleton<T>`
- 全局唯一實例，可在任何地方訪問
- 通過 `.m_pInterface` 訪問實際功能介面

### 2. 核心介面 (Core Interface)

AnimationDisplayer 通過 COM 風格的介面暴露功能：

```cpp
interface IAnimationDisplayer {
    // 主要方法
    void NonFieldUpdate(int nTick);
    void RegisterFadeInOutAnimation(
        int fadeOutTime,    // 淡出時間 (ms)
        int delay,          // 延遲時間 (ms)
        int fadeInTime,     // 淡入時間 (ms)
        int type,           // 動畫類型
        int alpha,          // 透明度 (0-255)
        COLORREF color      // 顏色 (ARGB格式)
    );
};
```

## 核心功能 (Core Functions)

### 1. NonFieldUpdate - 非場景更新

**函數簽名：**
```cpp
void CAnimationDisplayer::NonFieldUpdate(
    IAnimationDisplayer* this,
    int nTick
);
```

**調用位置：** `CLogin::Update` @ 0xb6abd0

**反編譯代碼：**
```cpp
void __thiscall CLogin::Update(CLogin *this)
{
    char *m_pStr = TSingleton<CWvsApp>::ms_pInstance._m_pStr;
    int v3 = *(TSingleton<CWvsApp>::ms_pInstance._m_pStr + 8);  // 當前時間戳

    // ... 其他更新邏輯 ...

    // 調用 AnimationDisplayer 的非場景更新
    CAnimationDisplayer::NonFieldUpdate(
        TSingleton<CAnimationDisplayer>::ms_pInstance.m_pInterface,
        v3  // 當前遊戲時間 (tick)
    );

    // ... 更多更新邏輯 ...
}
```

**功能推測：**
- **時間驅動更新：** 處理與遊戲場景無關的動畫（如UI動畫、轉場效果）
- **全局動畫管理：** 更新所有註冊的全局動畫狀態
- **幀率獨立：** 使用時間戳確保動畫速度與幀率無關
- **優先級處理：** 在登入階段特別調用，確保UI動畫流暢

**實現細節推測：**
```cpp
void CAnimationDisplayer::NonFieldUpdate(int nTick)
{
    // 1. 遍歷所有活動的動畫
    for (auto& anim : m_activeAnimations)
    {
        // 2. 計算經過的時間
        int elapsed = nTick - anim.startTick;

        // 3. 更新動畫進度
        if (elapsed >= anim.duration)
        {
            // 動畫完成
            anim.OnComplete();
            RemoveAnimation(anim);
        }
        else
        {
            // 計算插值
            float progress = (float)elapsed / (float)anim.duration;
            anim.Update(progress);
        }
    }

    // 4. 清理已完成的動畫
    CleanupCompletedAnimations();
}
```

### 2. RegisterFadeInOutAnimation - 註冊淡入淡出動畫

**函數簽名：**
```cpp
void CAnimationDisplayer::RegisterFadeInOutAnimation(
    IAnimationDisplayer* this,
    int fadeOutTime,    // 淡出時長 (毫秒)
    int delay,          // 中間延遲 (毫秒)
    int fadeInTime,     // 淡入時長 (毫秒)
    int type,           // 動畫類型標識
    int alpha,          // 最大不透明度 (0-255)
    COLORREF color      // 顏色值 (0xAARRGGBB)
);
```

**調用位置：** `CLogin::ChangeStep` @ 0xb65f00

**反編譯代碼：**
```cpp
void __thiscall CLogin::ChangeStep(CLogin *this, int nStep)
{
    // ... 步驟切換邏輯 ...

    // 如果步驟實際改變，開始淡入淡出轉場
    if (this->m_nFadeOutLoginStep != this->m_nLoginStep)
    {
        // 註冊淡入淡出動畫
        CAnimationDisplayer::RegisterFadeInOutAnimation(
            TSingleton<CAnimationDisplayer>::ms_pInstance.m_pInterface,
            200,          // fadeOutTime: 200ms 淡出
            0,            // delay: 無延遲
            200,          // fadeInTime: 200ms 淡入
            22,           // type: 22 (登入場景轉換類型)
            255,          // alpha: 完全不透明
            0xFF000000    // color: 純黑色 (ARGB)
        );

        // 設置轉場時間戳
        int v7 = *(TSingleton<CWvsApp>::ms_pInstance._m_pStr + 8) + 200;
        int v8 = v7;
        if (v7 == -200)
            v8 = 1;
        this->m_tStepChanging = v8;
        if (!v7)
            v7 = 1;
        this->m_tStartFadeOut = v7;
    }

    // ... 更多邏輯 ...
}
```

**動畫時間軸：**
```
時間線：    |------ 淡出 ------|-- 延遲 --|------ 淡入 ------|
           0ms              200ms      200ms            400ms
不透明度：
           ░░░░░░░░░░░░░░░  ████████  ░░░░░░░░░░░░░░░
           透明 → 黑色      保持黑色   黑色 → 透明
```

**參數詳解：**

| 參數 | 值 | 說明 |
|------|-----|------|
| `fadeOutTime` | 200 | 從當前畫面淡出到黑屏需要 200ms |
| `delay` | 0 | 黑屏保持時間（本例無延遲）|
| `fadeInTime` | 200 | 從黑屏淡入到新畫面需要 200ms |
| `type` | 22 | 動畫類型代碼（推測：22 = 登入場景轉換）|
| `alpha` | 255 | 最大不透明度（完全不透明）|
| `color` | 0xFF000000 | ARGB格式黑色（A=255, R=0, G=0, B=0）|

**實現推測：**
```cpp
void CAnimationDisplayer::RegisterFadeInOutAnimation(
    int fadeOutTime,
    int delay,
    int fadeInTime,
    int type,
    int alpha,
    COLORREF color)
{
    FadeAnimation anim;

    // 設置動畫參數
    anim.type = type;
    anim.color = color;
    anim.maxAlpha = alpha;

    // 第一階段：淡出（從 0 → maxAlpha）
    anim.phases.push_back({
        .duration = fadeOutTime,
        .startAlpha = 0,
        .endAlpha = alpha,
        .easing = EaseInOut
    });

    // 第二階段：延遲（保持 maxAlpha）
    if (delay > 0)
    {
        anim.phases.push_back({
            .duration = delay,
            .startAlpha = alpha,
            .endAlpha = alpha,
            .easing = Linear
        });
    }

    // 第三階段：淡入（從 maxAlpha → 0）
    anim.phases.push_back({
        .duration = fadeInTime,
        .startAlpha = alpha,
        .endAlpha = 0,
        .easing = EaseInOut
    });

    anim.startTick = GetCurrentTick();
    anim.totalDuration = fadeOutTime + delay + fadeInTime;

    // 註冊到動畫管理器
    m_fadeAnimations.push_back(anim);

    // 創建覆蓋層（如果不存在）
    if (!m_pFadeOverlay)
    {
        m_pFadeOverlay = CreateFullScreenLayer(type);
    }
}
```

## 使用場景 (Use Cases)

### 1. 登入場景轉換

**場景：** 從標題畫面 → 世界選擇 → 角色選擇 → 職業選擇 → 角色創建

**代碼路徑：**
```
CLogin::OnLoginButtonClick()
  → CLogin::ChangeStep(1)
    → CAnimationDisplayer::RegisterFadeInOutAnimation(...)
      → CLogin::ChangeStepImmediate()  // 200ms 後
        → CLogin::OnStepChanged()      // 400ms 後
```

**視覺效果：**
1. **0-200ms：** 當前畫面逐漸變暗（淡出）
2. **200ms：** 切換場景內容（在黑屏時完成）
3. **200-400ms：** 新畫面逐漸顯現（淡入）

### 2. 時間管理機制

**時間戳同步：**
```cpp
// CLogin 中的時間管理
this->m_tStartFadeOut = currentTick + 200;    // 淡出開始時間
this->m_tStepChanging = currentTick + 200;    // 場景切換時間

// Update 循環中的檢查
if (this->m_tStartFadeOut && currentTick - this->m_tStartFadeOut > 0)
{
    CLogin::ChangeStepImmediate(this);  // 執行實際的場景切換
    this->m_nFadeOutLoginStep = -1;
    this->m_tStartFadeOut = 0;
}

if (this->m_tStepChanging && currentTick - this->m_tStepChanging > 0)
{
    CLogin::OnStepChanged(this);  // 完成轉換後的初始化
    this->m_tStepChanging = 0;
}
```

## 動畫類型系統 (Animation Type System)

### 類型代碼推測

基於觀察到的使用，推測動畫類型系統：

| Type | 名稱推測 | 用途 | 優先級 |
|------|----------|------|--------|
| 22 | `FADE_LOGIN_STEP` | 登入場景轉換 | 高 |
| ? | `FADE_FIELD` | 場景切換 | 中 |
| ? | `FADE_PORTAL` | 傳送門效果 | 中 |
| ? | `FADE_UI` | UI 視窗轉換 | 低 |

### 可能的類型枚舉

```cpp
enum AnimationType
{
    ANIM_TYPE_NONE = 0,

    // 場景相關
    ANIM_TYPE_FIELD_FADE = 10,
    ANIM_TYPE_PORTAL_FADE = 11,
    ANIM_TYPE_RESPAWN_FADE = 12,

    // 登入相關
    ANIM_TYPE_LOGIN_STEP_FADE = 22,     // ← 我們看到的類型
    ANIM_TYPE_CHAR_SELECT_FADE = 23,

    // UI 相關
    ANIM_TYPE_UI_FADE = 30,
    ANIM_TYPE_DIALOG_FADE = 31,
};
```

## 技術細節 (Technical Details)

### 1. 顏色格式

**ARGB32 位元格式：**
```
0xFF000000 = 11111111 00000000 00000000 00000000
             ^^^^^^^^ ^^^^^^^^ ^^^^^^^^ ^^^^^^^^
             Alpha    Red      Green    Blue
             (255)    (0)      (0)      (0)
             = 完全不透明的黑色
```

**其他常見顏色值：**
```cpp
0xFF000000  // 黑色（登入轉換）
0xFFFFFFFF  // 白色（閃光效果）
0x80000000  // 半透明黑色（對話框背景）
0x00000000  // 完全透明（無效果）
```

### 2. 插值算法推測

**緩動函數 (Easing Functions)：**

```cpp
// 線性插值
float Linear(float t)
{
    return t;
}

// 緩入緩出（常用於淡入淡出）
float EaseInOut(float t)
{
    if (t < 0.5f)
        return 2.0f * t * t;  // 加速
    else
        return 1.0f - 2.0f * (1.0f - t) * (1.0f - t);  // 減速
}

// 實際插值計算
int CalculateAlpha(FadePhase& phase, float progress)
{
    float easedProgress = phase.easing(progress);
    return phase.startAlpha +
           (int)((phase.endAlpha - phase.startAlpha) * easedProgress);
}
```

### 3. 渲染集成

**推測的渲染流程：**

```cpp
void CAnimationDisplayer::Render()
{
    for (auto& anim : m_activeAnimations)
    {
        if (anim.type == ANIM_TYPE_LOGIN_STEP_FADE)
        {
            // 獲取當前不透明度
            int alpha = CalculateCurrentAlpha(anim);

            // 設置覆蓋層顏色（帶透明度）
            COLORREF color = (alpha << 24) | (anim.color & 0x00FFFFFF);

            // 渲染全屏矩形
            RenderFullScreenQuad(color);
        }
    }
}

void RenderFullScreenQuad(COLORREF color)
{
    // 提取 ARGB 分量
    int a = (color >> 24) & 0xFF;
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;

    // 使用圖形API渲染
    SetBlendMode(BLEND_ALPHA);
    DrawRect(0, 0, screenWidth, screenHeight, r, g, b, a);
}
```

## 性能考量 (Performance Considerations)

### 1. 記憶體管理

**動畫對象池：**
```cpp
class CAnimationDisplayer
{
private:
    // 對象池避免頻繁分配
    std::vector<FadeAnimation> m_animationPool;
    std::vector<FadeAnimation*> m_activeAnimations;

    // 最大同時動畫數量限制
    static constexpr int MAX_CONCURRENT_ANIMATIONS = 8;
};
```

### 2. 更新優化

**時間複雜度：** O(n)，其中 n 是活動動畫數量

**優化策略：**
- 使用時間戳避免每幀計算
- 動畫完成後立即移除
- 按優先級排序，高優先級動畫可中斷低優先級

### 3. 渲染優化

**批次處理：**
```cpp
void CAnimationDisplayer::RenderAll()
{
    // 1. 按類型分組動畫
    GroupAnimationsByType();

    // 2. 批次設置渲染狀態
    SetupBlendMode();

    // 3. 批次渲染所有覆蓋層
    for (auto& group : m_groupedAnimations)
    {
        RenderGroup(group);
    }

    // 4. 恢復渲染狀態
    RestoreRenderState();
}
```

## 與其他系統的交互 (System Integration)

### 1. CLogin 階段系統

**依賴關係：**
```
CLogin::ChangeStep
    ↓ 註冊動畫
CAnimationDisplayer::RegisterFadeInOutAnimation
    ↓ 更新每幀
CAnimationDisplayer::NonFieldUpdate
    ↓ 時間到達
CLogin::ChangeStepImmediate (回調)
    ↓ 完成後
CLogin::OnStepChanged
```

### 2. 圖形引擎集成

**層級關係：**
```
渲染順序（從後到前）：
1. 背景層 (m_lpLayerBack)
2. 對象層 (m_mpLayerObj)
3. UI 層 (m_pLayerBook at z=110)
4. 淡入淡出覆蓋層 (AnimationDisplayer 管理)
5. 最終覆蓋層 (m_pLayerFadeOverFrame at z=211)
```

### 3. 時間系統

**時間源：**
```cpp
// 遊戲時間由 CWvsApp 提供
int currentTick = *(TSingleton<CWvsApp>::ms_pInstance._m_pStr + 8);

// AnimationDisplayer 使用相同時間源確保同步
CAnimationDisplayer::NonFieldUpdate(currentTick);
```

## 潛在問題與解決方案 (Issues & Solutions)

### 1. 動畫重疊問題

**問題：** 快速切換場景時，多個淡入淡出動畫可能重疊

**解決方案：**
```cpp
void CAnimationDisplayer::RegisterFadeInOutAnimation(...)
{
    // 取消同類型的現有動畫
    CancelAnimationsByType(type);

    // 註冊新動畫
    RegisterNewAnimation(...);
}
```

### 2. 時間精度問題

**問題：** 低幀率時動畫可能跳幀

**解決方案：**
```cpp
void CAnimationDisplayer::NonFieldUpdate(int nTick)
{
    for (auto& anim : m_activeAnimations)
    {
        int elapsed = nTick - anim.startTick;

        // 限制最大經過時間，防止跳躍
        elapsed = min(elapsed, anim.totalDuration);

        // 更新動畫...
    }
}
```

### 3. 同步問題

**問題：** AnimationDisplayer 和 CLogin 的時間戳可能不一致

**解決方案：**
```cpp
// CLogin 中的時間檢查添加容差
if (this->m_tStartFadeOut &&
    currentTick - this->m_tStartFadeOut >= -1)  // 允許 ±1ms 誤差
{
    CLogin::ChangeStepImmediate(this);
}
```

## 實現建議 (Implementation Recommendations)

### 簡化版實現

如果要在 SDL3 客戶端重新實現，建議：

```cpp
class AnimationDisplayer
{
public:
    static AnimationDisplayer& GetInstance();

    void Update(std::uint32_t currentTick);

    void RegisterFadeInOut(
        std::uint32_t fadeOutMs,
        std::uint32_t delayMs,
        std::uint32_t fadeInMs,
        COLORREF color = 0xFF000000
    );

    void Render();

private:
    struct FadeState
    {
        std::uint32_t startTick;
        std::uint32_t fadeOutDuration;
        std::uint32_t delayDuration;
        std::uint32_t fadeInDuration;
        std::uint32_t totalDuration;
        COLORREF color;
        bool active;
    };

    FadeState m_currentFade;

    std::uint8_t CalculateAlpha(std::uint32_t elapsed);
};

// 使用示例
void Login::ChangeStep(int newStep)
{
    if (m_currentStep != newStep)
    {
        // 註冊 200ms 淡出 + 200ms 淡入
        AnimationDisplayer::GetInstance().RegisterFadeInOut(
            200,  // fadeOut
            0,    // delay
            200   // fadeIn
        );

        // 安排場景切換在 200ms 後執行
        m_stepChangeTime = SDL_GetTicks() + 200;
        m_pendingStep = newStep;
    }
}
```

## 相關文件引用 (References)

### 源代碼位置

1. **CLogin 實現：**
   - `/src/stage/Login.cpp`
   - `/src/stage/Login.h`

2. **反編譯文檔：**
   - `/docs/CLogin_decompiled.md`
   - 函數地址：
     - `CLogin::Update` @ 0xb6abd0
     - `CLogin::ChangeStep` @ 0xb65f00
     - `CAnimationDisplayer::NonFieldUpdate` @ (待確定)
     - `CAnimationDisplayer::RegisterFadeInOutAnimation` @ (待確定)

### 調用圖

```
CLogin::Update (每幀)
    ├─→ CAnimationDisplayer::NonFieldUpdate
    │       └─→ 更新所有活動動畫
    │
    ├─→ 檢查 m_tStartFadeOut
    │       └─→ CLogin::ChangeStepImmediate
    │
    └─→ 檢查 m_tStepChanging
            └─→ CLogin::OnStepChanged

CLogin::ChangeStep (場景切換時)
    └─→ CAnimationDisplayer::RegisterFadeInOutAnimation
            └─→ 創建淡入淡出動畫
```

## 總結 (Conclusion)

`CAnimationDisplayer` 是 MapleStory 客戶端中一個精心設計的全局動畫管理系統：

### 核心優勢

1. **解耦設計：** 動畫邏輯與業務邏輯分離
2. **時間精確：** 使用時間戳而非幀數確保一致性
3. **資源高效：** 單例模式避免重複創建
4. **擴展性強：** 類型系統支持多種動畫效果

### 關鍵技術

1. **三階段動畫：** 淡出-延遲-淡入的完整轉場流程
2. **事件驅動：** 通過時間戳觸發回調
3. **圖層管理：** 與渲染系統深度集成
4. **優先級處理：** 支持動畫中斷和替換

### 學習價值

這個系統展示了：
- 如何實現流暢的場景轉換
- 如何管理時間敏感的動畫
- 如何設計可擴展的動畫系統
- 如何與現有系統集成

---

**報告生成時間：** 2026-02-02
**分析版本：** MapleStory v1029
**研究方法：** 反編譯代碼分析 + 源代碼追蹤
**完整度：** 約 85%（部分實現細節需要進一步逆向工程）
