# Gr2D Animation Architecture (動畫架構)

## 原版 Animate 函數設計

### 核心概念

原版 MapleStory 的動畫系統使用**位元旗標組合**，而非固定的動畫模式。這讓系統更靈活、更強大。

### 位元旗標定義

```cpp
enum GR2D_ANITYPE : __int32
{
    GA_STOP = 0x0,              // 停止/無動畫
    GA_WAIT = 0x100,            // 暫停(等待)
    GA_CLEAR = 0x200,           // 結束時清除所有 canvas

    // 基礎旗標
    GA_FIRST = 0x10,            // 從第一幀開始
    GA_REPEAT = 0x20,           // 重複播放
    GA_REVERSE = 0x40,          // 反向播放

    // 組合範例
    GA_REVERSE_WITH_CLEAR = 0x240,  // REVERSE | CLEAR
};
```

### 函數簽名推測

```cpp
// 原版可能的簽名 (基於 COM 介面):
HRESULT IWzGr2DLayer::Animate(
    GR2D_ANITYPE aniType,        // 動畫類型旗標
    Ztl_variant_t const& delay,  // 延遲率 (可能是百分比, 1000 = 100%)
    Ztl_variant_t const& repeat  // 重複次數 (-1 = 無限)
);
```

## 旗標組合行為

### 1. 單一旗標

| 旗標 | 十六進位 | 行為 |
|------|----------|------|
| `GA_STOP` | 0x0 | 停止動畫，保持當前幀 |
| `GA_FIRST` | 0x10 | 跳到第一幀，不播放 |
| `GA_REPEAT` | 0x20 | 從當前幀重複播放 |
| `GA_REVERSE` | 0x40 | 從當前幀反向播放一次 |
| `GA_WAIT` | 0x100 | 暫停動畫 |
| `GA_CLEAR` | 0x200 | 立即清除所有 canvas |

### 2. 常見組合

#### GA_FIRST | GA_REPEAT (0x30) - 正常循環
```
行為: 從第一幀開始 → 向前播放 → 到最後一幀 → 回到第一幀 → 重複
用途: 技能特效、怪物動作、NPC 動畫
```

#### GA_REVERSE | GA_REPEAT (0x60) - 反向循環
```
行為: 從最後一幀開始 → 向後播放 → 到第一幀 → 回到最後一幀 → 重複
用途: 某些特殊效果
```

#### GA_FIRST | GA_REPEAT | GA_REVERSE (0x70) - 乒乓循環
```
行為: 第一幀 → 最後一幀 → 第一幀 → 重複 (來回播放)
用途: 呼吸動畫、浮動效果、UI 閃爍
```

#### GA_REVERSE | GA_CLEAR (0x240) - 反向後消失
```
行為: 反向播放一次 → 播完後清除圖層
用途: 技能消失效果、UI 淡出動畫
```

#### GA_FIRST (0x10) 單獨使用
```
行為: 跳到第一幀並停止
用途: 重置動畫狀態
```

### 3. 複雜組合範例

```cpp
// 從頭循環播放 3 次後清除
layer->Animate(GA_FIRST | GA_REPEAT | GA_CLEAR, 1000, 3);

// 立即清除 (常見於切換場景)
layer->Animate(GA_CLEAR, 1000, 0);

// 反向乒乓循環
layer->Animate(GA_REVERSE | GA_REPEAT, 1000, -1);

// 暫停當前動畫
layer->Animate(GA_WAIT, 1000, 0);
```

## 內部狀態機

### 狀態定義

```cpp
enum class AnimationState : std::int32_t
{
    Idle = 0,       // 閒置(未播放)
    Forward = 1,    // 向前播放中
    Backward = 2,   // 向後播放中
    Stopped = 3,    // 已停止
};
```

### 狀態轉換

```
Idle ──(Animate with FIRST|REPEAT)──> Forward
     ──(Animate with REVERSE)──────> Backward
     ──(Animate with WAIT)─────────> Idle (paused)

Forward ──(到達最後一幀 + 無REPEAT)──> Stopped
        ──(到達最後一幀 + REPEAT)────> Forward (frame=0)
        ──(到達最後一幀 + REVERSE+REPEAT)──> Backward (ping-pong)
        ──(CLEAR)────────────────────> Idle (清除所有canvas)

Backward ──(到達第一幀 + 無REPEAT)──> Stopped
         ──(到達第一幀 + REPEAT)────> Backward (frame=last)
         ──(到達第一幀 + REVERSE+REPEAT)──> Forward (ping-pong)
         ──(CLEAR)────────────────────> Idle

Stopped ──(任何 Animate 呼叫)────────> 對應狀態
```

## Update 與 AdvanceFrame 流程

### Update(tCur) 函數職責

```cpp
void WzGr2DLayer::Update(std::int32_t tCur)
{
    // 1. 更新位置動畫 (background type 4-7 的移動效果)
    if (m_bPositionAnimating) {
        UpdatePositionAnimation(tCur);
    }

    // 2. 更新幀動畫
    if (!m_bAnimating || m_canvases.empty()) {
        return;
    }

    // 3. 初始化時間
    if (m_tLastFrameTime == 0) {
        m_tLastFrameTime = tCur;
        return;
    }

    // 4. 計算當前幀延遲 (考慮 delayRate)
    auto delay = (currentFrame.delay * m_nDelayRate) / 1000;

    // 5. 更新插值 (alpha, zoom 的平滑過渡)
    UpdateFrameInterpolation(tCur);

    // 6. 檢查是否該換幀
    if (tCur - m_tLastFrameTime >= delay) {
        AdvanceFrame();
        m_tLastFrameTime = tCur;
    }
}
```

### AdvanceFrame() 函數邏輯

```cpp
void WzGr2DLayer::AdvanceFrame()
{
    auto flags = static_cast<int32_t>(m_animType);
    bool hasRepeat = flags & GA_REPEAT;
    bool hasReverse = flags & GA_REVERSE;
    bool hasClear = flags & GA_CLEAR;

    // === 判斷動畫行為 ===

    // 情況 1: 乒乓模式 (REPEAT + REVERSE 同時存在)
    if (hasRepeat && hasReverse) {
        if (m_bReverseDirection) {
            // 向後播放中
            if (m_nCurrentFrame > 0) {
                --m_nCurrentFrame;
            } else {
                // 到達起點，轉向前
                m_bReverseDirection = false;
                CheckRepeatCount();  // 檢查是否達到重複次數
            }
        } else {
            // 向前播放中
            if (m_nCurrentFrame < lastFrame) {
                ++m_nCurrentFrame;
            } else {
                // 到達終點，轉向後
                m_bReverseDirection = true;
            }
        }
    }

    // 情況 2: 純反向播放
    else if (hasReverse) {
        if (m_nCurrentFrame > 0) {
            --m_nCurrentFrame;
        } else {
            // 到達起點
            if (hasRepeat) {
                m_nCurrentFrame = lastFrame;  // 循環回終點
                CheckRepeatCount();
            } else {
                StopAnimation();  // 停止
                if (hasClear) ClearCanvases();
            }
        }
    }

    // 情況 3: 正向播放
    else {
        if (m_nCurrentFrame < lastFrame) {
            ++m_nCurrentFrame;
        } else {
            // 到達終點
            if (hasRepeat) {
                m_nCurrentFrame = 0;  // 循環回起點
                CheckRepeatCount();
            } else {
                StopAnimation();  // 停止
                if (hasClear) ClearCanvases();
            }
        }
    }
}
```

## 插值系統

### Alpha 和 Zoom 插值

每個 canvas frame 有兩組值:
- `alpha0` → `alpha1`: 起始到結束的透明度變化
- `zoom0` → `zoom1`: 起始到結束的縮放變化

```cpp
void UpdateFrameInterpolation(int32_t tCur)
{
    auto& frame = m_canvases[m_nCurrentFrame].frameInfo;
    auto delay = (frame.delay * m_nDelayRate) / 1000;
    auto elapsed = tCur - m_tLastFrameTime;

    // 計算插值因子 (0.0 ~ 1.0)
    float t = clamp(elapsed / delay, 0.0f, 1.0f);

    // 線性插值
    m_nCurrentAlpha = lerp(frame.alpha0, frame.alpha1, t);
    m_nCurrentZoom = lerp(frame.zoom0, frame.zoom1, t);
}
```

### 用途範例

```cpp
// 漸入效果: alpha 從 0 → 255
InsertCanvas(canvas, delay=100, alpha0=0, alpha1=255, zoom0=1000, zoom1=1000);

// 縮放彈跳: 從 80% → 120% → 100%
InsertCanvas(canvas1, 50, 255, 255, 800, 1200);   // 80% → 120%
InsertCanvas(canvas2, 50, 255, 255, 1200, 1000);  // 120% → 100%

// 淡出並縮小
InsertCanvas(canvas, 100, 255, 0, 1000, 500);  // 消失效果
```

## DelayRate 參數

### 概念
`delayRate` 以千分比控制播放速度:
- `1000` = 100% (正常速度)
- `2000` = 200% (兩倍速)
- `500` = 50% (慢動作)

### 計算公式
```cpp
實際延遲 = (frame.delay * delayRate) / 1000
```

### 範例
```cpp
// 原始: 每幀 100ms
frame.delay = 100;

// 正常速度: 100 * 1000 / 1000 = 100ms
Animate(GA_FIRST | GA_REPEAT, 1000, -1);

// 兩倍速: 100 * 500 / 1000 = 50ms (播放快兩倍)
Animate(GA_FIRST | GA_REPEAT, 500, -1);

// 慢動作: 100 * 2000 / 1000 = 200ms (播放慢兩倍)
Animate(GA_FIRST | GA_REPEAT, 2000, -1);
```

## Repeat 參數

### 值的意義
- `-1`: 無限循環
- `0`: 播放一次 (不重複)
- `n > 0`: 循環 n 次

### 實作邏輯
```cpp
void CheckRepeatCount()
{
    if (m_nRepeatCount <= 0) {
        return;  // 無限循環，不檢查
    }

    ++m_nCurrentRepeat;
    if (m_nCurrentRepeat >= m_nRepeatCount) {
        StopAnimation();
        if (hasClearFlag) {
            ClearCanvases();
        }
    }
}
```

## 實際使用範例

### 1. 技能特效 (播放一次後消失)
```cpp
auto layer = gr.CreateLayer(x, y, w, h, z);
for (auto& canvas : skillEffect) {
    layer->InsertCanvas(canvas, 100, 255, 255, 1000, 1000);
}
// 從頭播放一次，結束後清除
layer->Animate(GA_FIRST | GA_CLEAR, 1000, 0);
```

### 2. 怪物待機動畫 (無限循環)
```cpp
for (auto& canvas : mobStand) {
    layer->InsertCanvas(canvas, 150);
}
// 從頭開始無限循環
layer->Animate(GA_FIRST | GA_REPEAT, 1000, -1);
```

### 3. UI 按鈕 hover 效果 (乒乓)
```cpp
layer->InsertCanvas(normal, 100);
layer->InsertCanvas(highlight, 100);
// 來回播放: normal → highlight → normal
layer->Animate(GA_FIRST | GA_REPEAT | GA_REVERSE, 1000, -1);
```

### 4. NPC 對話氣泡 (淡入淡出)
```cpp
// 淡入階段
layer->InsertCanvas(bubble, 200, 0, 255, 1000, 1000);
// 顯示階段
layer->InsertCanvas(bubble, 2000, 255, 255, 1000, 1000);
// 淡出階段
layer->InsertCanvas(bubble, 200, 255, 0, 1000, 1000);
// 播放一次後清除
layer->Animate(GA_FIRST | GA_CLEAR, 1000, 0);
```

### 5. 背景滾動 (配合位置動畫)
```cpp
layer->InsertCanvas(bgCanvas, 100);
layer->SetTiling(bgWidth, 0);  // 水平平鋪
layer->SetParallax(50, 0);     // 50% 視差
// 靜止顯示 (不需要幀動畫)
layer->Animate(GA_STOP, 1000, 0);
```

## 關鍵設計優勢

### 1. 靈活性
- 位元旗標可自由組合，創造各種動畫模式
- 不需要為每種模式寫獨立邏輯

### 2. 效能
- 單一狀態機處理所有情況
- 分支預測友好 (位元檢查很快)

### 3. 擴展性
- 新增旗標不影響現有功能
- 向後兼容

### 4. 可讀性
- 旗標名稱清楚表達意圖
- 組合語意明確

## 與現代系統比較

### 傳統方式 (枚舉)
```cpp
enum AnimType { Loop, Once, PingPong, Reverse };
// 需要 4 種獨立邏輯
```

### MapleStory 方式 (位元旗標)
```cpp
flags = FIRST | REPEAT | REVERSE;
// 單一邏輯處理所有組合
```

## 總結

MapleStory 的 Gr2D 動畫系統是一個經典的**位元旗標架構**範例，透過:

1. **基礎旗標**: FIRST, REPEAT, REVERSE
2. **特殊旗標**: WAIT, CLEAR
3. **組合語意**: 創造豐富的動畫行為

這種設計在 2000 年代中期非常先進，展現了良好的軟體工程思維。

---

*參考資料: IWzGr2DLayer 反編譯介面、GR2D_ANITYPE 枚舉定義*
