# Gr2D 引擎完整架構

## 目錄
1. [系統概述](#系統概述)
2. [核心類別](#核心類別)
3. [渲染流程](#渲染流程)
4. [動畫系統](#動畫系統)
5. [混合模式](#混合模式)
6. [座標系統](#座標系統)
7. [實作狀態](#實作狀態)

---

## 系統概述

### 設計哲學

Gr2D 是 MapleStory 的 2D 圖形引擎，基於 COM 介面設計，採用**圖層-畫布-幀**三層架構:

```
WzGr2D (引擎)
    └─ WzGr2DLayer[] (圖層陣列，按 Z-order 排序)
           └─ WzGr2DCanvas[] (畫布陣列，用於幀動畫)
                  └─ WzCanvas (像素資料)
```

### 職責分離

| 類別 | 職責 | 對應原版 |
|------|------|----------|
| `WzGr2D` | 渲染引擎、視窗管理、圖層排序 | `IWzGr2D` |
| `WzGr2DLayer` | 圖層邏輯、動畫控制、座標轉換 | `IWzGr2DLayer` |
| `WzGr2DCanvas` | 渲染資料包裝、SDL 紋理管理 | (封裝層) |
| `WzCanvas` | 像素資料、WZ 檔案解析 | `IWzCanvas` |

---

## 核心類別

### 1. WzGr2D (圖形引擎)

#### 定義
```cpp
class WzGr2D : public Singleton<WzGr2D>
{
    // 初始化
    bool Initialize(width, height, window, bpp, refreshRate);
    void Shutdown();

    // 圖層管理
    shared_ptr<WzGr2DLayer> CreateLayer(left, top, width, height, z, canvas, filter);
    void RemoveLayer(layer);
    void RemoveAllLayers();

    // 渲染
    bool RenderFrame(tCur);

    // 相機控制
    Point2D GetCameraPosition();
    void SetCameraPosition(x, y);
    float GetCameraRotate();
    void SetCameraRotate(angle);

    // 座標轉換
    Point2D ScreenToWorld(screenPos);
    Point2D WorldToScreen(worldPos);
};
```

#### 重要成員
```cpp
private:
    SDL_Window* m_pWindow;
    SDL_Renderer* m_pRenderer;
    vector<shared_ptr<WzGr2DLayer>> m_layers;  // 按 Z-order 排序
    Point2D m_cameraPos;                        // 相機位置
    float m_fCameraRotate;                      // 相機旋轉
    int32_t m_tCurrent;                         // 當前時間
    uint32_t m_uFps100;                         // FPS * 100
```

#### 全域存取
```cpp
auto& gr = get_gr();  // 匹配原版 get_gr() 函數
```

### 2. WzGr2DLayer (圖層)

#### 定義
```cpp
class WzGr2DLayer
{
public:
    // === 位置與尺寸 ===
    int32_t GetLeft/GetTop();
    uint32_t GetWidth/GetHeight();
    void SetPosition(left, top);
    void SetWidth/SetHeight(value);

    // === 深度 ===
    int32_t GetZ();
    void SetZ(z);

    // === 視覺屬性 ===
    LayerFlipState GetFlip();
    void SetFlip(flip);

    uint32_t GetColor();      // ARGB
    void SetColor(color);
    uint8_t GetAlpha();
    void SetAlpha(alpha);

    bool IsVisible();
    void SetVisible(visible);

    // === 混合模式 ===
    LayerBlendType GetBlend();
    void SetBlend(blend);

    uint32_t GetOverlay();    // 疊加顏色
    void SetOverlay(color);

    float GetRotation();      // 旋轉角度
    void SetRotation(degrees);

    // === 畫布管理 ===
    size_t InsertCanvas(canvas, delay, alpha0, alpha1, zoom0, zoom1);
    shared_ptr<WzGr2DCanvas> RemoveCanvas(index);
    void RemoveAllCanvases();
    bool ShiftCanvas(fromIndex, toIndex);
    void InitCanvasOrder();

    size_t GetCanvasCount();
    shared_ptr<WzGr2DCanvas> GetCanvas(index);
    shared_ptr<WzGr2DCanvas> GetCurrentCanvas();

    // === 動畫控制 ===
    bool Animate(type, delayRate = 1000, repeat = -1);
    void StopAnimation();
    bool IsAnimating();

    size_t GetCurrentFrame();
    void SetCurrentFrame(frame);
    AnimationState GetAnimationState();
    int32_t GetAnimationTime();
    Gr2DAnimationType GetLastAnimationType();

    uint8_t GetFirstAnimationAlpha0();
    uint8_t GetCurrentInterpolatedAlpha();
    int32_t GetCurrentInterpolatedZoom();

    // === 平鋪與視差 ===
    void SetTiling(cx, cy);
    void SetParallax(rx, ry);
    int32_t GetTileCx/GetTileCy();
    int32_t GetParallaxRx/GetParallaxRy();

    // === 位置動畫 ===
    void StartPositionAnimation(offsetX, offsetY, duration, loop);
    void StopPositionAnimation();
    bool IsPositionAnimating();

    // === 座標空間 ===
    bool IsScreenSpace();
    void SetScreenSpace(screenSpace);
    bool IsCenterBased();
    void SetCenterBased(centerBased);

    // === 更新與渲染 ===
    void Update(tCur);
    void Render(renderer, offsetX, offsetY);
};
```

#### 內部狀態
```cpp
private:
    // 位置與尺寸
    int32_t m_nLeft, m_nTop;
    uint32_t m_uWidth, m_uHeight;
    int32_t m_nZ;

    // 視覺屬性
    LayerFlipState m_flipState;
    uint32_t m_dwColor;           // ARGB
    uint32_t m_dwOverlayColor;
    float m_fRotation;
    bool m_bVisible;
    bool m_bScreenSpace;
    bool m_bCenterBased;
    LayerBlendType m_blendMode;

    // 平鋪與視差
    int32_t m_nTileCx, m_nTileCy;
    int32_t m_nParallaxRx, m_nParallaxRy;

    // 畫布與動畫
    vector<CanvasEntry> m_canvases;
    size_t m_nCurrentFrame;
    bool m_bAnimating;
    AnimationState m_animState;
    Gr2DAnimationType m_animType;
    int32_t m_nDelayRate;
    int32_t m_nRepeatCount;
    int32_t m_nCurrentRepeat;
    int32_t m_tLastFrameTime;
    bool m_bReverseDirection;

    // 插值值
    uint8_t m_nCurrentAlpha;
    int32_t m_nCurrentZoom;   // 以千分比儲存 (1000 = 100%)

    // 位置動畫
    bool m_bPositionAnimating;
    int32_t m_nAnimOffsetX, m_nAnimOffsetY;
    int32_t m_nAnimDuration;
    bool m_bAnimLoop;
    int32_t m_tAnimStart;
    int32_t m_nInitialLeft, m_nInitialTop;
};
```

### 3. WzGr2DCanvas (圖形畫布)

#### 定義
```cpp
class WzGr2DCanvas
{
public:
    // WzCanvas 存取
    shared_ptr<WzCanvas> GetCanvas();
    void SetCanvas(canvas);

    // 尺寸 (轉發自 WzCanvas)
    int GetWidth();
    int GetHeight();

    // 位置 (相對於圖層)
    Point2D GetPosition();
    void SetPosition(x, y);

    // 原點 (錨點)
    Point2D GetOrigin();
    void SetOrigin(origin);

    // Z 值 (圖層內排序)
    int GetZ();
    void SetZ(z);

    // SDL 紋理
    SDL_Texture* GetTexture();
    void SetTexture(texture);
    SDL_Texture* CreateTexture(renderer);

    // 狀態檢查
    bool HasPixelData();
    bool HasTexture();
};
```

#### 重要概念

**Position vs Origin**:
```
Position: 畫布在世界/螢幕中的位置
Origin: 畫布內的錨點 (pivot point)

範例: 角色精靈圖
- Position = (100, 200) - 角色在世界中的位置
- Origin = (50, 100) - 錨點在腳底 (圖片尺寸 100x100)
- 實際渲染位置 = Position - Origin = (50, 100)
```

### 4. WzCanvas (像素資料)

#### 定義
```cpp
class WzCanvas
{
public:
    int GetWidth();
    int GetHeight();

    const vector<uint8_t>& GetPixelData();  // RGBA 格式
    void SetPixelData(data);

    bool HasPixelData();
};
```

---

## 渲染流程

### 完整渲染流程

```cpp
// 1. 應用程式主迴圈
void Application::Update(tCur)
{
    // 更新所有圖層動畫
    for (auto& layer : layers) {
        layer->Update(tCur);
    }

    // 渲染一幀
    gr.RenderFrame(tCur);
}

// 2. WzGr2D::RenderFrame
bool WzGr2D::RenderFrame(tCur)
{
    // 清空畫面
    SDL_SetRenderDrawColor(m_pRenderer, backColor);
    SDL_RenderClear(m_pRenderer);

    // 計算相機偏移量 (世界座標 → 螢幕座標)
    auto screenCenter = GetCenter();
    auto offsetX = -m_cameraPos.x + screenCenter.x;
    auto offsetY = -m_cameraPos.y + screenCenter.y;

    // 按 Z-order 排序圖層
    if (m_bLayersDirty) {
        SortLayers();  // 根據 m_nZ 排序
    }

    // 渲染每個圖層
    for (auto& layer : m_layers) {
        layer->Render(m_pRenderer, offsetX, offsetY);
    }

    // 顯示到螢幕
    SDL_RenderPresent(m_pRenderer);

    return true;
}

// 3. WzGr2DLayer::Render
void WzGr2DLayer::Render(renderer, offsetX, offsetY)
{
    if (!m_bVisible || m_canvases.empty()) return;

    auto canvas = GetCurrentCanvas();
    auto* texture = canvas->GetTexture();

    // === 計算渲染位置 ===

    // 基礎位置計算
    float baseX, baseY;

    if (m_bScreenSpace) {
        if (m_bCenterBased) {
            // 螢幕空間 + 中心基準
            baseX = m_nLeft + offsetX;  // offsetX 包含螢幕中心
            baseY = m_nTop + offsetY;
        } else {
            // 螢幕空間 + 絕對座標
            baseX = m_nLeft;
            baseY = m_nTop;
        }
    } else {
        // 世界空間 + 視差
        if (m_nParallaxRx <= 0) {
            baseX = m_nLeft + offsetX;  // 全速跟隨相機
        } else {
            baseX = m_nLeft + (offsetX * m_nParallaxRx / 100);
        }

        if (m_nParallaxRy <= 0) {
            baseY = m_nTop + offsetY;
        } else {
            baseY = m_nTop + (offsetY * m_nParallaxRy / 100);
        }
    }

    // === 應用縮放 ===

    auto canvasPos = canvas->GetPosition();
    auto canvasOrigin = canvas->GetOrigin();
    auto zoomFactor = m_nCurrentZoom / 1000.0f;

    float renderX, renderY, renderW, renderH;

    if (m_nCurrentZoom != 1000) {
        auto scaledOriginX = canvasOrigin.x * zoomFactor;
        auto scaledOriginY = canvasOrigin.y * zoomFactor;

        renderX = baseX + canvasPos.x - scaledOriginX;
        renderY = baseY + canvasPos.y - scaledOriginY;
        renderW = canvas->GetWidth() * zoomFactor;
        renderH = canvas->GetHeight() * zoomFactor;
    } else {
        renderX = baseX + canvasPos.x - canvasOrigin.x;
        renderY = baseY + canvasPos.y - canvasOrigin.y;
        renderW = canvas->GetWidth();
        renderH = canvas->GetHeight();
    }

    // === 應用顏色與透明度 ===

    auto alpha = (m_dwColor >> 24) & 0xFF;
    auto red = (m_dwColor >> 16) & 0xFF;
    auto green = (m_dwColor >> 8) & 0xFF;
    auto blue = m_dwColor & 0xFF;

    // 結合圖層 alpha 與幀插值 alpha
    alpha = (alpha * m_nCurrentAlpha) / 255;

    SDL_SetTextureColorMod(texture, red, green, blue);
    SDL_SetTextureAlphaMod(texture, alpha);

    // === 應用混合模式 ===

    auto sdlBlendMode = ConvertToSDLBlendMode(m_blendMode);
    SDL_SetTextureBlendMode(texture, sdlBlendMode);

    // === 應用翻轉 ===

    SDL_FlipMode flipMode = SDL_FLIP_NONE;
    if (m_flipState == LayerFlipState::Horizontal) {
        flipMode = SDL_FLIP_HORIZONTAL;
    } else if (m_flipState == LayerFlipState::Vertical) {
        flipMode = SDL_FLIP_VERTICAL;
    } else if (m_flipState == LayerFlipState::Both) {
        flipMode = SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL;
    }

    // === 平鋪渲染 ===

    auto tileCx = m_nTileCx > 0 ? m_nTileCx : renderW;
    auto tileCy = m_nTileCy > 0 ? m_nTileCy : renderH;

    // 計算需要渲染的瓦片數量
    int tilesX = CalculateTileCount(renderX, renderW, tileCx, viewportW);
    int tilesY = CalculateTileCount(renderY, renderH, tileCy, viewportH);

    for (int ty = 0; ty < tilesY; ++ty) {
        for (int tx = 0; tx < tilesX; ++tx) {
            float tileX = startTileX + tx * tileCx;
            float tileY = startTileY + ty * tileCy;

            SDL_FRect dstRect{tileX, tileY, renderW, renderH};

            if (flipMode != SDL_FLIP_NONE || m_fRotation != 0.0f) {
                SDL_RenderTextureRotated(renderer, texture, nullptr,
                                          &dstRect, m_fRotation, nullptr, flipMode);
            } else {
                SDL_RenderTexture(renderer, texture, nullptr, &dstRect);
            }
        }
    }
}
```

---

## 動畫系統

### 動畫類型 (位元旗標)

```cpp
enum class Gr2DAnimationType : int32_t
{
    None = 0x0,         // 停止
    First = 0x10,       // 從第一幀開始
    Repeat = 0x20,      // 重複
    Reverse = 0x40,     // 反向
    Wait = 0x100,       // 暫停
    Clear = 0x200,      // 結束時清除

    // 常用組合
    Loop = First | Repeat,                    // 0x30 - 正常循環
    ReverseLoop = Reverse | Repeat,           // 0x60 - 反向循環
    PingPong = First | Repeat | Reverse,      // 0x70 - 乒乓
    ReverseWithClear = Reverse | Clear,       // 0x240 - 反向後清除
};
```

### 幀資訊

```cpp
struct CanvasFrameInfo
{
    int32_t nDelay;       // 幀延遲 (毫秒)
    uint8_t nAlpha0;      // 起始透明度 (0-255)
    uint8_t nAlpha1;      // 結束透明度
    int32_t nZoom0;       // 起始縮放 (千分比, 1000 = 100%)
    int32_t nZoom1;       // 結束縮放
};
```

### 動畫流程

```
1. Animate(type, delayRate, repeat)
   ↓
2. 設定動畫參數:
   - m_animType = type
   - m_nDelayRate = delayRate
   - m_nRepeatCount = repeat
   - 根據旗標設定初始幀和方向
   ↓
3. 每幀 Update(tCur):
   a. 計算實際延遲 = (frameDelay * delayRate) / 1000
   b. UpdateFrameInterpolation(tCur) - 更新 alpha/zoom 插值
   c. 如果 (tCur - lastTime >= delay):
      - AdvanceFrame() - 根據旗標推進到下一幀
      - lastTime = tCur
   ↓
4. AdvanceFrame() 邏輯:
   - 檢查 Repeat + Reverse → 乒乓模式
   - 檢查 Reverse → 反向播放
   - 否則 → 正向播放
   - 到達終點/起點時:
     * 有 Repeat → 循環
     * 無 Repeat → 停止
     * 有 Clear → 清除所有 canvas
```

詳見: [gr2d-animate-architecture.md](./gr2d-animate-architecture.md)

---

## 混合模式

### 混合類型 (位元旗標)

```cpp
enum class LayerBlendType : int32_t
{
    Normal = 0x0,
    Add = 0x1,            // 加算 (src + dst)
    Inverse = 0x2,        // 反轉
    Isolated = 0x4,       // 隔離 (不與下層混合)
    Premultiplied = 0x8,  // 預乘 Alpha
    Multiply = 0x10,      // 乘算 (src * dst)
    Screen = 0x20,        // 濾色
    Overlay = 0x40,       // 疊加
    LinearDodge = 0x80,   // 線性加亮 (同 Add)
    Darken = 0x100,       // 變暗 (min)
    Lighten = 0x200,      // 變亮 (max)
    All = 0x3FF,
};
```

### SDL 對應

| MapleStory | SDL3 | 備註 |
|------------|------|------|
| Normal | `SDL_BLENDMODE_BLEND` | 標準 alpha 混合 |
| Add | `SDL_BLENDMODE_ADD` | 完全支援 |
| Multiply | `SDL_BLENDMODE_MUL` | 完全支援 |
| LinearDodge | `SDL_BLENDMODE_ADD` | 等同 Add |
| Screen | `SDL_BLENDMODE_BLEND` | 近似 (無直接支援) |
| Overlay | `SDL_BLENDMODE_BLEND` | 近似 |
| Darken | `SDL_BLENDMODE_BLEND` | 需要自訂 |
| Lighten | `SDL_BLENDMODE_BLEND` | 需要自訂 |

---

## 座標系統

### 三種座標空間

#### 1. 世界座標 (World Space)
```cpp
// 物件在遊戲世界中的絕對位置
layer->SetScreenSpace(false);
layer->SetPosition(1000, 500);  // 世界座標 (1000, 500)

// 受相機影響:
// 螢幕位置 = 世界位置 - 相機位置 + 螢幕中心
```

#### 2. 螢幕座標 (Screen Space)
```cpp
// 固定在螢幕上的 UI 元素
layer->SetScreenSpace(true);
layer->SetCenterBased(false);
layer->SetPosition(10, 10);  // 螢幕左上角 (10, 10)

// 不受相機影響
```

#### 3. 中心基準座標 (Center-Based Screen Space)
```cpp
// 相對於螢幕中心的位置 (原版 MapleStory UI 常用)
layer->SetScreenSpace(true);
layer->SetCenterBased(true);
layer->SetPosition(-400, -300);  // 螢幕中心向左 400, 向上 300

// 範例: 800x600 螢幕
// 中心 = (400, 300)
// 實際位置 = (400 - 400, 300 - 300) = (0, 0)
```

### 視差捲動

```cpp
// rx, ry: 視差因子 (0-100)
layer->SetParallax(50, 50);  // 50% 捲動速度

// 計算:
// rx <= 0: 全速跟隨相機 (固定在世界座標)
// rx > 0: 部分跟隨 (視差效果)
//   offsetX = cameraOffset * (rx / 100)

// 範例:
// rx = 0: 完全不跟隨 (背景最遠層)
// rx = 50: 半速跟隨 (中間層)
// rx = 100: 全速跟隨 (前景)
```

### 平鋪

```cpp
// cx, cy: 平鋪間距 (0 = 不平鋪)
layer->SetTiling(800, 0);  // 水平每 800px 重複

// 渲染邏輯:
// 1. 計算需要的瓦片數量以覆蓋螢幕
// 2. 從起始位置開始,每隔 cx/cy 渲染一次
```

---

## 實作狀態

### ✅ 已實作 (核心功能)

#### WzGr2D
- ✅ 初始化與關閉
- ✅ 圖層管理 (創建、移除、排序)
- ✅ 渲染流程 (RenderFrame)
- ✅ 相機控制
- ✅ 座標轉換
- ✅ FPS 計算

#### WzGr2DLayer
- ✅ 位置與尺寸屬性
- ✅ Z-order 排序
- ✅ 翻轉、顏色、透明度
- ✅ 混合模式 (位元旗標)
- ✅ 旋轉
- ✅ 疊加顏色
- ✅ 畫布管理 (插入、移除、排序)
- ✅ 動畫系統 (位元旗標)
- ✅ 插值 (alpha, zoom)
- ✅ 視差與平鋪
- ✅ 位置動畫 (背景移動)
- ✅ 螢幕空間 / 中心基準

#### WzGr2DCanvas
- ✅ 位置與原點
- ✅ Z 值
- ✅ SDL 紋理管理
- ✅ WzCanvas 包裝

#### WzCanvas
- ✅ 尺寸屬性
- ✅ 像素資料儲存 (RGBA)

### ❌ 未實作 (進階功能)

#### WzGr2DLayer
- ❌ 著色器支援 (VertexShader, PixelShader)
- ❌ 著色器常數 (ShaderConst)
- ❌ 色調效果 (RedTone, GreenBlueTone)
- ❌ 遮罩 (MaskCanvas)
- ❌ 渲染到紋理 (RenderToTexture)
- ❌ Spine 骨骼動畫

#### WzCanvas
- ❌ 像素存取 (GetPixel)
- ❌ 文字繪製 (DrawTextA)
- ❌ 動態調整尺寸 (Putwidth/Putheight)
- ❌ cx/cy 屬性
- ❌ magLevel (放大等級)
- ❌ 多解析度支援 (rawCanvas)
- ❌ 平鋪尺寸 (tileWidth/Height)
- ❌ 模糊效果 (SetBlur)
- ❌ 裁切矩形 (SetClipRect)

### 📊 完成度統計

```
WzGr2D:         100% (核心功能完整)
WzGr2DLayer:     85% (核心功能完整,進階功能部分缺失)
WzGr2DCanvas:    70% (基礎功能完整)
WzCanvas:        40% (僅基本像素資料)

整體:            ~75% (足以運行 v83 時期的 MapleStory)
```

---

## 使用範例

### 1. 初始化引擎

```cpp
auto& gr = get_gr();
gr.Initialize(800, 600, window);
gr.SetBackColor(0xFF000000);  // 黑色背景
```

### 2. 創建靜態圖像

```cpp
// 載入圖片
auto wzCanvas = WzResMan::Get().GetCanvas("UI/Logo/Nexon");
auto grCanvas = make_shared<WzGr2DCanvas>(wzCanvas);
grCanvas->SetOrigin(wzCanvas->GetWidth()/2, wzCanvas->GetHeight()/2);

// 創建圖層
auto layer = gr.CreateLayer(400, 300, 200, 100, 0);
layer->SetScreenSpace(true);
layer->SetCenterBased(true);
layer->InsertCanvas(grCanvas);
```

### 3. 創建動畫精靈

```cpp
// 載入動畫幀
auto frames = WzResMan::Get().GetCanvases("Mob/100100/stand");
auto layer = gr.CreateLayer(x, y, 100, 100, 100);

for (auto& frame : frames) {
    auto grCanvas = make_shared<WzGr2DCanvas>(frame);
    grCanvas->SetOrigin(frame->GetOrigin());
    layer->InsertCanvas(grCanvas, 150);  // 150ms per frame
}

// 播放動畫
layer->Animate(Gr2DAnimationType::Loop, 1000, -1);
```

### 4. 創建視差背景

```cpp
// 遠景 (慢速)
auto bgBack = gr.CreateLayer(0, 0, 800, 600, -100);
bgBack->InsertCanvas(backCanvas);
bgBack->SetTiling(800, 0);
bgBack->SetParallax(20, 0);  // 20% 捲動速度

// 中景 (中速)
auto bgMid = gr.CreateLayer(0, 0, 800, 600, -50);
bgMid->InsertCanvas(midCanvas);
bgMid->SetTiling(800, 0);
bgMid->SetParallax(50, 0);  // 50% 捲動速度

// 前景 (全速)
auto bgFront = gr.CreateLayer(0, 0, 800, 600, 0);
bgFront->InsertCanvas(frontCanvas);
bgFront->SetTiling(800, 0);
bgFront->SetParallax(100, 0);  // 100% 跟隨相機
```

### 5. 技能特效

```cpp
auto layer = gr.CreateLayer(x, y, w, h, 200);

for (auto& effectFrame : skillFrames) {
    auto canvas = make_shared<WzGr2DCanvas>(effectFrame);
    // 淡入淡出效果
    layer->InsertCanvas(canvas, 50, 0, 255, 1000, 1200);  // 放大 + 淡入
}
layer->InsertCanvas(lastFrame, 50, 255, 0, 1200, 800);  // 縮小 + 淡出

// 播放一次後清除
layer->Animate(Gr2DAnimationType::First | Gr2DAnimationType::Clear, 1000, 0);
```

### 6. UI 按鈕 Hover 效果

```cpp
auto buttonLayer = gr.CreateLayer(x, y, w, h, 1000);
buttonLayer->SetScreenSpace(true);

auto normal = make_shared<WzGr2DCanvas>(normalCanvas);
auto hover = make_shared<WzGr2DCanvas>(hoverCanvas);

buttonLayer->InsertCanvas(normal, 100);
buttonLayer->InsertCanvas(hover, 100);

// 滑鼠移入時
buttonLayer->Animate(Gr2DAnimationType::PingPong, 1000, -1);

// 滑鼠移出時
buttonLayer->StopAnimation();
buttonLayer->SetCurrentFrame(0);
```

### 7. 相機控制

```cpp
// 跟隨玩家
auto& gr = get_gr();
gr.SetCameraPosition(player.x, player.y);

// 平滑相機移動 (在 Update 中)
auto currentPos = gr.GetCameraPosition();
auto targetPos = player.GetPosition();
auto newPos = lerp(currentPos, targetPos, 0.1f);  // 10% 插值
gr.SetCameraPosition(newPos);
```

### 8. 主迴圈

```cpp
void Application::Update()
{
    auto tCur = SDL_GetTicks();

    // 更新邏輯
    UpdateGame(tCur);

    // 更新所有圖層動畫
    // (通常由 Logo/Login/MapLoadable 等 Stage 管理)

    // 渲染
    auto& gr = get_gr();
    gr.UpdateCurrentTime(tCur);
    gr.RenderFrame(tCur);
}
```

---

## 設計優勢

### 1. 圖層分離
- 每個圖層獨立管理自己的動畫和渲染
- Z-order 自動排序
- 易於新增/移除視覺元素

### 2. 位元旗標系統
- 動畫類型和混合模式都使用位元旗標
- 靈活組合,無需窮舉所有可能
- 效能友好 (位元運算快速)

### 3. 插值系統
- Alpha 和 Zoom 平滑過渡
- 支援每幀不同的起始/結束值
- 創造豐富的視覺效果

### 4. 座標系統
- 支援世界座標、螢幕座標、中心基準
- 視差捲動簡化背景渲染
- 平鋪系統自動處理無限背景

### 5. 擴展性
- 基於 COM 介面設計
- 清晰的職責分離
- 易於新增功能 (如 Spine, 著色器)

---

## 性能考量

### 優化點

1. **圖層排序**
   - 只在 Z 值改變時排序 (m_bLayersDirty)
   - 使用穩定排序保持相對順序

2. **紋理快取**
   - WzGr2DCanvas 快取 SDL_Texture
   - 避免每幀重建紋理

3. **視錐剔除**
   - 平鋪渲染時跳過螢幕外的瓦片
   - 減少不必要的繪製呼叫

4. **插值優化**
   - 使用整數運算 (千分比)
   - 避免浮點數精度問題

### 潛在瓶頸

1. **過多圖層**
   - 解決: 合併靜態圖層
   - 解決: 移除不可見圖層

2. **大量平鋪**
   - 解決: 限制平鋪數量
   - 解決: 使用紋理重複 (GPU)

3. **頻繁動畫**
   - 解決: 共用 canvas 資料
   - 解決: LOD (遠處降低幀率)

---

## 與原版差異

### 相同點
- ✅ 圖層-畫布-幀 三層架構
- ✅ 位元旗標動畫系統
- ✅ 插值系統 (alpha, zoom)
- ✅ 視差與平鋪
- ✅ COM 風格介面 (Get*/Put*)

### 差異點
- ❌ 使用 SDL3 而非 DirectX
- ❌ 不支援自訂著色器
- ❌ 部分混合模式僅近似
- ❌ 無 Spine 動畫支援
- ✅ 使用現代 C++ (shared_ptr, RAII)
- ✅ 更清晰的座標系統 (ScreenSpace, CenterBased)

---

## 總結

Gr2D 引擎是一個設計優良的 2D 圖形系統,特點包括:

1. **靈活的動畫系統** - 位元旗標組合創造豐富行為
2. **強大的座標系統** - 世界/螢幕/中心三種空間
3. **視差與平鋪** - 簡化背景渲染
4. **插值效果** - 平滑的視覺過渡
5. **清晰的職責分離** - 引擎/圖層/畫布/像素資料

當前實作已覆蓋 ~75% 原版功能,足以運行 MapleStory v83 時期的遊戲內容。

---

## 相關文件

- [gr2d-implementation-status.md](./gr2d-implementation-status.md) - 實作狀態詳細追蹤
- [gr2d-animate-architecture.md](./gr2d-animate-architecture.md) - 動畫系統深入分析
- [CLogo_decompiled.md](./CLogo_decompiled.md) - Logo 場景反編譯分析

---

*最後更新: 2026-02-10*
*基於 IWzGr2D, IWzGr2DLayer, IWzCanvas 反編譯介面*
