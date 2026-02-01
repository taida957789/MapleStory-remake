# Logo 載入畫面設計文件

**日期**: 2026-02-02
**狀態**: 已確認
**目標**: 在 Logo stage 新增載入狀態，顯示資源載入進度

---

## 概述

在 Logo 動畫播放完成後、進入 Login stage 之前，新增載入畫面顯示資源載入進度。載入畫面包含隨機背景、循環動畫和進度指示器，追蹤真實的資源載入狀態。

---

## 一、整體架構

### 1.1 執行時機與流程

載入畫面的執行流程：

1. **Logo 動畫階段** - Message phase (0-49 frames) + Logo phase (50+ frames) 正常播放
2. **載入模式** - Logo 動畫結束後，直接切換到載入模式（無淡入效果）
3. **資源載入** - 顯示載入畫面，追蹤資源載入進度，更新 step 指示器
4. **轉場** - 載入完成後執行淡出效果，然後轉場到 Login stage

### 1.2 狀態機擴展

Logo stage 的狀態流程：

```
Init → Logo Mode → Video Mode → Loading Mode → Login Transition
                      ↓              ↓
                   (unavail)      (complete)
```

- **Logo Mode**: 播放 Message 和 Logo 動畫
- **Video Mode**: 播放影片（目前 unavailable，直接跳過）
- **Loading Mode**: 顯示載入畫面，追蹤資源載入
- **Login Transition**: 淡出後轉場到 Login stage

### 1.3 資源預載策略

為避免「載入畫面本身需要載入」的矛盾：

- **時機**: 在 `Application::Init()` 階段提前載入 `UI/Logo.img`
- **範圍**: 確保 Loading 子節點（randomBackgrd, repeat, step）已載入
- **好處**: Logo stage 啟動時可直接使用載入畫面資源

### 1.4 進度追蹤機制

**階段式進度更新**（短期實作）：

- **Step 0**: 載入開始
- **Step 1-2**: 關鍵資源載入完成
  - 追蹤 "MapleStory: Loaded WZ file" 訊息
  - 追蹤其他相關資源（具體資源待後續定義）
- **最終 Step**: `Login::Init()` 完成，準備轉場

**未來擴展**（可選）：
- 在 WzResMan 加入載入進度回調系統
- 更精確追蹤每個 WZ 檔案載入百分比

---

## 二、視覺元素與 WZ 結構

### 2.1 WZ 資源結構

載入畫面使用三種 WZ 節點：

#### 2.1.1 隨機背景 - `UI/Logo.img/Loading/randomBackgrd/{0, 1, 2, ...}`

- **數量**: 透過 `GetChildCount()` 動態取得背景數量
- **選擇**: 啟動時隨機選擇其中一張
- **持續**: 整個載入過程保持不變，避免視覺跳動

```cpp
auto randomBackgrdProp = resMan.GetProperty("UI/Logo.img/Loading/randomBackgrd");
int bgCount = randomBackgrdProp->GetChildCount();
int randomIndex = rand() % bgCount;
auto bgCanvas = randomBackgrdProp->GetChild(std::to_string(randomIndex))->GetCanvas();
```

#### 2.1.2 循環動畫 - `UI/Logo.img/Loading/repeat/{0, 1, 2, ...}/{frame_index}`

- **結構**: 每個 repeat 節點下有多個 frame，每個 frame 有 `delay` 子節點
- **播放**: 依序循環播放 repeat/0 → repeat/1 → repeat/2 → ... → repeat/0
- **時機**: 當一個 repeat 動畫的所有 frame 播放完畢後，切換到下一個 repeat

```cpp
auto delayProp = frameNode->GetChild("delay");
int delay = delayProp ? delayProp->GetInt(100) : 100;  // 預設 100ms
layer->InsertCanvas(frameCanvas, delay, 255, 255);
```

#### 2.1.3 進度步驟 - `UI/Logo.img/Loading/step/{0, 1, 2, ...}`

- **顯示方式**: 替換式 - 根據當前載入進度只顯示對應的 step 圖片
- **範例**: 若有 4 個 step (0-3)，載入 50% 時顯示 step/2
- **更新**: 外部透過 `SetLoadingProgress(int step)` 更新當前步驟

```cpp
// 根據 step 更新進度顯示
void Logo::SetLoadingProgress(int step)
{
    m_nLoadingStep = step;
    if (step < m_stepFrames.size())
    {
        m_pLayerLoadingStep->RemoveAllCanvases();
        m_pLayerLoadingStep->InsertCanvas(m_stepFrames[step]);
        // 套用置中定位...
    }
}
```

### 2.2 動畫播放機制

repeat 動畫的播放方式與現有的 `UINewCharRaceSelect` 實作相同，複用現有動畫系統：

- 從 WZ 讀取 frame 和 delay
- 使用 `InsertCanvas()` 的 delay 參數控制幀時間
- Layer 自動處理動畫循環

---

## 三、畫面配置與渲染

### 3.1 層級結構

使用三個渲染層，z-order 由低到高：

| Layer | Z-Order | 用途 | 成員變數 |
|-------|---------|------|----------|
| 背景層 | 0 | 顯示隨機選中的背景圖片 | `m_pLayerLoadingBg` |
| 動畫層 | 1 | 顯示循環播放的 repeat 動畫 | `m_pLayerLoadingAnim` |
| 進度層 | 2 | 顯示當前進度的 step 圖片 | `m_pLayerLoadingStep` |

### 3.2 定位方式

採用與現有 Logo 相同的 **WZ origin 配合置中定位**：

```cpp
auto origin = canvas->GetOrigin();
auto& gr = get_gr();
int screenWidth = static_cast<int>(gr.GetWidth());
int screenHeight = static_cast<int>(gr.GetHeight());
int layerX = (screenWidth - canvas->GetWidth()) / 2 + origin.x;
int layerY = (screenHeight - canvas->GetHeight()) / 2 + origin.y;
layer->SetPosition(layerX, layerY);
```

**好處**：
- 保持與原版 MapleStory 一致的視覺效果
- 尊重 WZ 檔案中的設計定位
- 程式碼風格統一

### 3.3 轉場效果

#### 進入載入畫面
- **方式**: 直接顯示（無淡入效果）
- **時機**: Logo 動畫結束 / Video mode unavailable 後立即切換

#### 離開載入畫面
- **方式**: 淡出效果
- **實作**: 逐漸降低所有 layer 的 alpha 值（255 → 0）
- **時機**: 載入完成（最終 step 達成）後執行淡出，然後呼叫 `GoToLogin()`

```cpp
// 淡出載入畫面
void Logo::FadeOutLoading()
{
    // 在 Update 中逐幀降低 alpha
    // 當 alpha 達到 0 時，呼叫 GoToLogin()
}
```

---

## 四、實作方案

### 4.1 修改檔案

#### 4.1.1 Application.cpp/h

**修改內容**：
- 在 `Application::Init()` 中提前載入 `UI/Logo.img`

```cpp
void Application::Init()
{
    // ... 現有初始化代碼 ...

    // 預載 Logo 資源（包含 Loading 畫面）
    auto& resMan = WzResMan::GetInstance();
    resMan.GetProperty("UI/Logo.img");  // 觸發載入

    // ... 其他初始化 ...
}
```

#### 4.1.2 Logo.h

**新增成員變數**：

```cpp
// Loading mode state
bool m_bLoadingMode{false};              // 是否在載入模式
std::int32_t m_nLoadingStep{0};          // 當前載入步驟
std::int32_t m_nLoadingStepCount{0};     // 總步驟數
std::uint8_t m_loadingAlpha{255};        // 載入畫面 alpha（用於淡出）

// Loading layers
std::shared_ptr<WzGr2DLayer> m_pLayerLoadingBg;     // 背景層 (z=0)
std::shared_ptr<WzGr2DLayer> m_pLayerLoadingAnim;   // 動畫層 (z=1)
std::shared_ptr<WzGr2DLayer> m_pLayerLoadingStep;   // 進度層 (z=2)

// Loading frames
std::shared_ptr<WzCanvas> m_loadingBgCanvas;                       // 選中的隨機背景
std::vector<std::vector<std::shared_ptr<WzCanvas>>> m_repeatAnims; // repeat 動畫 [n][frame]
std::vector<std::shared_ptr<WzCanvas>> m_stepFrames;               // step 圖片
std::int32_t m_nCurrentRepeat{0};       // 當前播放的 repeat index
```

**新增方法**：

```cpp
/**
 * @brief 初始化載入畫面資源
 *
 * 從 UI/Logo.img/Loading 載入：
 * - randomBackgrd: 隨機選擇一張背景
 * - repeat: 載入所有動畫節點
 * - step: 載入所有進度步驟
 */
void InitLoading();

/**
 * @brief 開始載入模式
 *
 * 從 Logo/Video 模式切換到載入模式
 * 顯示載入畫面 layers
 */
void StartLoadingMode();

/**
 * @brief 更新載入畫面
 *
 * 更新 repeat 動畫循環
 * 處理淡出效果
 */
void UpdateLoading();

/**
 * @brief 設定載入進度
 * @param step 當前步驟（0-based）
 *
 * 更新 step 圖片顯示
 */
void SetLoadingProgress(std::int32_t step);

/**
 * @brief 開始淡出載入畫面
 *
 * 逐漸降低 alpha，完成後轉場到 Login
 */
void FadeOutLoading();
```

#### 4.1.3 Logo.cpp

**InitWZLogo() 擴展**：
```cpp
void Logo::InitWZLogo()
{
    // ... 現有代碼（載入 Logo/Grade/Message）...

    // 初始化載入畫面
    InitLoading();
}
```

**InitLoading() 實作**：
```cpp
void Logo::InitLoading()
{
    auto& resMan = WzResMan::GetInstance();
    auto loadingProp = resMan.GetProperty("UI/Logo.img/Loading");

    if (!loadingProp)
    {
        LOG_WARN("UI/Logo.img/Loading not found");
        return;
    }

    // 1. 載入隨機背景
    auto randomBgProp = loadingProp->GetChild("randomBackgrd");
    if (randomBgProp && randomBgProp->HasChildren())
    {
        int bgCount = static_cast<int>(randomBgProp->GetChildCount());
        int randomIndex = rand() % bgCount;
        auto bgChild = randomBgProp->GetChild(std::to_string(randomIndex));
        m_loadingBgCanvas = bgChild ? bgChild->GetCanvas() : nullptr;
        LOG_DEBUG("Selected random background: {}/{}", randomIndex, bgCount);
    }

    // 2. 載入 repeat 動畫
    auto repeatProp = loadingProp->GetChild("repeat");
    if (repeatProp && repeatProp->HasChildren())
    {
        for (int n = 0; n < 100; ++n)  // 最多 100 個 repeat
        {
            auto repeatN = repeatProp->GetChild(std::to_string(n));
            if (!repeatN) break;

            std::vector<std::shared_ptr<WzCanvas>> frames;
            frames = LoadLogoFrames(repeatN);  // 複用現有方法

            if (!frames.empty())
            {
                m_repeatAnims.push_back(frames);
            }
        }
        LOG_DEBUG("Loaded {} repeat animations", m_repeatAnims.size());
    }

    // 3. 載入 step 進度
    auto stepProp = loadingProp->GetChild("step");
    if (stepProp && stepProp->HasChildren())
    {
        m_stepFrames = LoadLogoFrames(stepProp);
        m_nLoadingStepCount = static_cast<int>(m_stepFrames.size());
        LOG_DEBUG("Loaded {} loading steps", m_nLoadingStepCount);
    }

    // 4. 建立 layers（初始隱藏）
    auto& gr = get_gr();

    // 背景層 (z=10, 高於 Logo layers)
    m_pLayerLoadingBg = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 10);
    if (m_pLayerLoadingBg)
    {
        m_pLayerLoadingBg->SetVisible(false);
        m_pLayerLoadingBg->SetScreenSpace(true);
    }

    // 動畫層 (z=11)
    m_pLayerLoadingAnim = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 11);
    if (m_pLayerLoadingAnim)
    {
        m_pLayerLoadingAnim->SetVisible(false);
        m_pLayerLoadingAnim->SetScreenSpace(true);
    }

    // 進度層 (z=12)
    m_pLayerLoadingStep = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 12);
    if (m_pLayerLoadingStep)
    {
        m_pLayerLoadingStep->SetVisible(false);
        m_pLayerLoadingStep->SetScreenSpace(true);
    }
}
```

**StartLoadingMode() 實作**：
```cpp
void Logo::StartLoadingMode()
{
    m_bLoadingMode = true;
    m_nLoadingStep = 0;
    m_loadingAlpha = 255;
    m_nCurrentRepeat = 0;

    // 隱藏 Logo layers
    if (m_pLayerBackground) m_pLayerBackground->SetVisible(false);
    if (m_pLayerMain) m_pLayerMain->SetVisible(false);

    // 顯示載入 layers
    if (m_pLayerLoadingBg)
    {
        m_pLayerLoadingBg->RemoveAllCanvases();
        if (m_loadingBgCanvas)
        {
            m_pLayerLoadingBg->InsertCanvas(m_loadingBgCanvas);
            // 套用置中定位
            auto origin = m_loadingBgCanvas->GetOrigin();
            auto& gr = get_gr();
            int layerX = (gr.GetWidth() - m_loadingBgCanvas->GetWidth()) / 2 + origin.x;
            int layerY = (gr.GetHeight() - m_loadingBgCanvas->GetHeight()) / 2 + origin.y;
            m_pLayerLoadingBg->SetPosition(layerX, layerY);
        }
        m_pLayerLoadingBg->SetVisible(true);
    }

    if (m_pLayerLoadingAnim)
    {
        m_pLayerLoadingAnim->RemoveAllCanvases();
        // 載入第一個 repeat 動畫
        if (!m_repeatAnims.empty())
        {
            auto& firstRepeat = m_repeatAnims[0];
            for (auto& frameCanvas : firstRepeat)
            {
                auto delayProp = /* 取得 delay */;
                int delay = delayProp ? delayProp->GetInt(100) : 100;
                m_pLayerLoadingAnim->InsertCanvas(frameCanvas, delay, 255, 255);
            }
            // 套用置中定位
        }
        m_pLayerLoadingAnim->SetVisible(true);
    }

    if (m_pLayerLoadingStep)
    {
        m_pLayerLoadingStep->SetVisible(true);
        SetLoadingProgress(0);  // 顯示第一個 step
    }

    LOG_INFO("Started loading mode");
}
```

**UpdateLoading() 實作**：
```cpp
void Logo::UpdateLoading()
{
    // 處理淡出
    if (m_loadingAlpha < 255)
    {
        // 逐幀降低 alpha（例如每幀 -5）
        m_loadingAlpha = std::max(0, m_loadingAlpha - 5);

        std::uint32_t color = (static_cast<std::uint32_t>(m_loadingAlpha) << 24) | 0x00FFFFFF;
        if (m_pLayerLoadingBg) m_pLayerLoadingBg->SetColor(color);
        if (m_pLayerLoadingAnim) m_pLayerLoadingAnim->SetColor(color);
        if (m_pLayerLoadingStep) m_pLayerLoadingStep->SetColor(color);

        if (m_loadingAlpha == 0)
        {
            // 淡出完成，轉場到 Login
            GoToLogin();
        }
        return;
    }

    // repeat 動畫循環處理
    // Layer 的動畫系統會自動處理 frame 播放
    // 這裡只需要處理 repeat 之間的切換（可選）
}
```

**SetLoadingProgress() 實作**：
```cpp
void Logo::SetLoadingProgress(std::int32_t step)
{
    m_nLoadingStep = step;

    if (!m_pLayerLoadingStep || step >= m_nLoadingStepCount)
    {
        return;
    }

    m_pLayerLoadingStep->RemoveAllCanvases();

    if (step < static_cast<int>(m_stepFrames.size()))
    {
        auto& stepCanvas = m_stepFrames[step];
        m_pLayerLoadingStep->InsertCanvas(stepCanvas);

        // 套用置中定位
        auto origin = stepCanvas->GetOrigin();
        auto& gr = get_gr();
        int layerX = (gr.GetWidth() - stepCanvas->GetWidth()) / 2 + origin.x;
        int layerY = (gr.GetHeight() - stepCanvas->GetHeight()) / 2 + origin.y;
        m_pLayerLoadingStep->SetPosition(layerX, layerY);
    }

    LOG_DEBUG("Loading progress: step {}/{}", step, m_nLoadingStepCount - 1);

    // 如果達到最終 step，開始淡出
    if (step >= m_nLoadingStepCount - 1)
    {
        FadeOutLoading();
    }
}
```

**FadeOutLoading() 實作**：
```cpp
void Logo::FadeOutLoading()
{
    // 設定 alpha 開始淡出
    // 實際淡出在 UpdateLoading() 中處理
    m_loadingAlpha = 254;  // 觸發淡出
    LOG_DEBUG("Starting loading screen fade out");
}
```

**Update() 修改**：
```cpp
void Logo::Update()
{
    if (m_bLoadingMode)
    {
        UpdateLoading();
    }
    else if (m_bVideoMode)
    {
        UpdateVideo();
    }
    else
    {
        UpdateLogo();
    }
}
```

**UpdateVideo() 修改**：
```cpp
void Logo::UpdateVideo()
{
    // Video unavailable - 進入載入模式
    if (m_videoState == VideoState::Unavailable)
    {
        StartLoadingMode();  // 改為啟動載入模式
        return;
    }

    // ... 其他代碼 ...
}
```

**Close() 擴展**：
```cpp
void Logo::Close()
{
    // ... 現有清理代碼 ...

    // 清理載入 layers
    auto& gr = get_gr();

    if (m_pLayerLoadingBg)
    {
        gr.RemoveLayer(m_pLayerLoadingBg);
        m_pLayerLoadingBg.reset();
    }

    if (m_pLayerLoadingAnim)
    {
        gr.RemoveLayer(m_pLayerLoadingAnim);
        m_pLayerLoadingAnim.reset();
    }

    if (m_pLayerLoadingStep)
    {
        gr.RemoveLayer(m_pLayerLoadingStep);
        m_pLayerLoadingStep.reset();
    }

    // 清理載入 frames
    m_loadingBgCanvas.reset();
    m_repeatAnims.clear();
    m_stepFrames.clear();
}
```

### 4.2 外部進度更新介面

外部系統（如 WzResMan 或 Application）可以透過 Logo stage 更新載入進度：

```cpp
// 範例：在關鍵載入點更新進度
auto logoStage = std::dynamic_pointer_cast<Logo>(Application::GetInstance().GetCurrentStage());
if (logoStage)
{
    logoStage->SetLoadingProgress(1);  // 更新到 step 1
}
```

**未來擴展**：可以在 Logo 中加入靜態方法或單例模式，讓其他模組更方便地更新進度。

### 4.3 測試要點

1. **背景隨機性** - 多次啟動確認背景會隨機變化
2. **動畫循環** - 確認 repeat 動畫正確循環播放
3. **進度更新** - 手動觸發不同 step，確認顯示正確
4. **淡出效果** - 最終 step 後確認平滑淡出
5. **資源清理** - 確認 Close() 正確釋放所有資源

---

## 五、未來擴展

### 5.1 精確進度追蹤

**WzResMan 回調系統**：

```cpp
class ILoadingProgressListener
{
public:
    virtual void OnLoadProgress(int current, int total) = 0;
    virtual void OnLoadComplete() = 0;
};

// WzResMan 支援註冊監聽器
void WzResMan::SetProgressListener(ILoadingProgressListener* listener);
```

Logo stage 實作此介面，接收即時進度更新。

### 5.2 更多視覺效果

- repeat 動畫之間的淡入淡出切換
- step 圖片更新時的動畫效果
- 載入提示文字（tips）

### 5.3 效能優化

- 延遲載入 repeat 動畫（只載入當前需要的）
- 背景圖片壓縮/快取

---

## 六、總結

本設計文件規劃了 Logo stage 的載入畫面功能，包含：

- ✅ 隨機背景選擇與顯示
- ✅ 循環動畫播放機制
- ✅ 進度步驟指示器
- ✅ 真實資源載入追蹤（階段式）
- ✅ 平滑的轉場效果
- ✅ 完整的實作方案與程式碼範例

設計遵循現有 Logo stage 的架構模式，保持程式碼風格一致，並為未來擴展預留彈性。
