# MapleStory 背景類型系統 (Background Type System)

## 概述

MapleStory 使用 8 種背景類型來控制背景圖層的平鋪 (tiling) 和動畫行為。這個系統結合了視差捲動 (parallax scrolling) 和位置動畫,創造出豐富的視覺深度效果。

---

## BackgroundType 列舉

```cpp
enum class BackgroundType : std::int32_t
{
    Normal = 0,      // 無平鋪,僅視差捲動
    HTiled = 1,      // 水平平鋪
    VTiled = 2,      // 垂直平鋪
    Tiled = 3,       // 雙向平鋪 (水平+垂直)
    HMoveA = 4,      // 水平動畫移動 → 水平平鋪
    VMoveA = 5,      // 垂直動畫移動 → 垂直平鋪
    HMoveB = 6,      // 水平動畫移動 → 雙向平鋪
    VMoveB = 7,      // 垂直動畫移動 → 雙向平鋪
};
```

---

## 類型詳解

### 靜態類型 (0-3)

這些類型不包含位置動畫,只有平鋪和視差效果。

#### Type 0: Normal (無平鋪)
```cpp
BackgroundType::Normal
```

**特徵**:
- ❌ 無水平平鋪
- ❌ 無垂直平鋪
- ✅ 使用 rx/ry 視差捲動

**用途**:
- 單一大圖背景 (如山脈輪廓)
- 需要精確定位的裝飾元素
- 固定大小的背景圖片

**範例**:
```cpp
// WZ 資料
type = 0
cx = 0    // 不使用
cy = 0    // 不使用
rx = 50   // 50% 水平視差
ry = 50   // 50% 垂直視差
```

#### Type 1: HTiled (水平平鋪)
```cpp
BackgroundType::HTiled
```

**特徵**:
- ✅ 水平平鋪 (橫向重複)
- ❌ 無垂直平鋪
- ✅ 使用 rx/ry 視差捲動

**用途**:
- 地平線天空
- 雲層
- 海平面
- 地面紋理

**平鋪邏輯**:
```cpp
tileCx = (cx > 0) ? cx : canvasWidth;  // 平鋪間距
tileCy = 0;                             // 不垂直平鋪

// 渲染: [IMG][IMG][IMG][IMG]... (橫向重複)
```

**範例**:
```cpp
// 雲層背景
type = 1         // HTiled
cx = 800         // 每 800px 重複
cy = 0           // 不使用
rx = 30          // 30% 視差 (遠景)
ry = 0           // 不垂直捲動
```

#### Type 2: VTiled (垂直平鋪) ⭐
```cpp
BackgroundType::VTiled
```

**特徵**:
- ❌ 無水平平鋪
- ✅ 垂直平鋪 (縱向重複)
- ✅ 使用 rx/ry 視差捲動

**用途**:
- 瀑布
- 高塔牆面
- 垂直柱子
- 洞穴岩壁

**平鋪邏輯**:
```cpp
tileCx = 0;                             // 不水平平鋪
tileCy = (cy > 0) ? cy : canvasHeight;  // 平鋪間距

// 渲染:
// [IMG]
// [IMG]  ← cy 間距後重複
// [IMG]
// [IMG]
```

**範例**:
```cpp
// 瀑布背景
type = 2         // VTiled
cx = 0           // 不使用
cy = 600         // 每 600px 重複
rx = 100         // 100% 水平跟隨
ry = 50          // 50% 垂直視差
```

#### Type 3: Tiled (雙向平鋪)
```cpp
BackgroundType::Tiled
```

**特徵**:
- ✅ 水平平鋪
- ✅ 垂直平鋪
- ✅ 使用 rx/ry 視差捲動

**用途**:
- 磚牆紋理
- 星空背景
- 重複圖案
- 地磚

**平鋪邏輯**:
```cpp
tileCx = (cx > 0) ? cx : canvasWidth;
tileCy = (cy > 0) ? cy : canvasHeight;

// 渲染: (橫向+縱向都重複)
// [IMG][IMG][IMG]...
// [IMG][IMG][IMG]...
// [IMG][IMG][IMG]...
```

**範例**:
```cpp
// 星空背景
type = 3         // Tiled
cx = 800         // 橫向每 800px 重複
cy = 600         // 縱向每 600px 重複
rx = 10          // 10% 視差 (極遠景)
ry = 10
```

---

### 動畫類型 (4-7)

這些類型包含**位置動畫** + **平鋪**,創造移動效果。

#### Type 4: HMoveA (水平移動 + 水平平鋪)
```cpp
BackgroundType::HMoveA
```

**特徵**:
- ✅ 水平位置動畫 (來回移動)
- ✅ 水平平鋪
- ❌ 無垂直平鋪
- ✅ 使用 rx 參數控制移動距離

**動畫行為**:
```cpp
// 動畫參數
moveRel = rx;
moveOffsetX = -abs(rx);  // 移動距離
duration = 20000 / abs(rx);  // 移動週期

// 位置動畫: 來回移動
// 起點 → (起點 + offsetX) → 起點 (循環)
layer->StartPositionAnimation(moveOffsetX, 0, duration, true);

// 同時設定水平平鋪
effectiveType = HTiled;
tileCx = cx;
tileCy = 0;
```

**用途**:
- 緩慢移動的雲層
- 漂浮的霧氣
- 橫向捲動的遠景

**範例**:
```cpp
// 移動雲層
type = 4         // HMoveA
cx = 800         // 水平平鋪間距
cy = 0
rx = 100         // 移動距離 100px, 週期 200ms
ry = 0

// 效果: 雲層每 200ms 來回移動 100px,同時水平平鋪
```

#### Type 5: VMoveA (垂直移動 + 垂直平鋪)
```cpp
BackgroundType::VMoveA
```

**特徵**:
- ✅ 垂直位置動畫
- ❌ 無水平平鋪
- ✅ 垂直平鋪
- ✅ 使用 ry 參數控制移動距離

**動畫行為**:
```cpp
moveRel = ry;
moveOffsetY = -abs(ry);
duration = 20000 / abs(ry);

layer->StartPositionAnimation(0, moveOffsetY, duration, true);

effectiveType = VTiled;
tileCx = 0;
tileCy = cy;
```

**用途**:
- 向下流動的瀑布
- 上升的蒸氣
- 垂直移動的霧

**範例**:
```cpp
// 瀑布動畫
type = 5         // VMoveA
cx = 0
cy = 600         // 垂直平鋪間距
rx = 0
ry = 200         // 向下移動 200px, 週期 100ms

// 效果: 瀑布每 100ms 向下移動 200px 並循環,創造流動效果
```

#### Type 6: HMoveB (水平移動 + 雙向平鋪)
```cpp
BackgroundType::HMoveB
```

**特徵**:
- ✅ 水平位置動畫
- ✅ 水平平鋪
- ✅ 垂直平鋪
- ✅ 使用 rx 參數控制移動距離

**動畫行為**:
```cpp
moveRel = rx;
moveOffsetX = -abs(rx);
duration = 20000 / abs(rx);

layer->StartPositionAnimation(moveOffsetX, 0, duration, true);

effectiveType = Tiled;
tileCx = cx;
tileCy = cy;
```

**用途**:
- 橫向捲動的星空
- 移動的雲海
- 橫向飄動的雪花背景

**範例**:
```cpp
// 移動星空
type = 6         // HMoveB
cx = 800
cy = 600
rx = 50          // 橫向移動
ry = 0
```

#### Type 7: VMoveB (垂直移動 + 雙向平鋪)
```cpp
BackgroundType::VMoveB
```

**特徵**:
- ✅ 垂直位置動畫
- ✅ 水平平鋪
- ✅ 垂直平鋪
- ✅ 使用 ry 參數控制移動距離

**動畫行為**:
```cpp
moveRel = ry;
moveOffsetY = -abs(ry);
duration = 20000 / abs(ry);

layer->StartPositionAnimation(0, moveOffsetY, duration, true);

effectiveType = Tiled;
tileCx = cx;
tileCy = cy;
```

**用途**:
- 下雨效果
- 下雪效果
- 垂直移動的霧氣

**範例**:
```cpp
// 下雨背景
type = 7         // VMoveB
cx = 800
cy = 600
rx = 0
ry = 300         // 向下移動

// 效果: 雨滴向下移動並雙向平鋪,覆蓋整個螢幕
```

---

## 動畫週期公式

動畫類型 (4-7) 使用以下公式計算移動週期:

```cpp
duration = 20000 / abs(moveRel)  // 毫秒
```

### 範例計算

| moveRel (rx/ry) | duration (ms) | 說明 |
|-----------------|---------------|------|
| 10 | 2000 | 慢速移動 (2 秒) |
| 50 | 400 | 中速移動 |
| 100 | 200 | 快速移動 (0.2 秒) |
| 200 | 100 | 極快移動 |

---

## 視差捲動 (Parallax Scrolling)

所有類型都支援視差捲動,透過 `rx` 和 `ry` 參數控制:

### rx/ry 參數意義

```cpp
// rx/ry 範圍: -100 ~ 100
// 負值或零: 全速跟隨相機
// 正值: 部分跟隨 (視差效果)

rx/ry = 0    → 完全不跟隨相機 (固定在螢幕上)
rx/ry = 20   → 20% 捲動速度 (極遠景)
rx/ry = 50   → 50% 捲動速度 (中景)
rx/ry = 100  → 100% 捲動速度 (前景,隨相機完全移動)
rx/ry < 0    → 全速跟隨 (固定世界座標)
```

### 視差計算

```cpp
// 在 WzGr2DLayer::CalculateBasePosition() 中:
if (m_nParallaxRx <= 0) {
    baseX = m_nLeft + offsetX;  // 全速跟隨
} else {
    auto parallaxOffsetX = (offsetX * m_nParallaxRx) / 100;
    baseX = m_nLeft + parallaxOffsetX;  // 部分跟隨
}
```

### 多層視差範例

```cpp
// 遠景 (天空)
layer0->SetParallax(10, 10);    // 10% 速度,移動很慢

// 中景 (雲層)
layer1->SetParallax(30, 30);    // 30% 速度

// 近景 (樹木)
layer2->SetParallax(70, 70);    // 70% 速度

// 前景 (地面)
layer3->SetParallax(100, 100);  // 100% 速度,完全跟隨

// 效果: 創造深度感,遠處移動慢,近處移動快
```

---

## 實際應用範例

### 1. 登入畫面天空

```cpp
// 遠景雲層 - 慢速移動
type = BackgroundType::HMoveA
cx = 1024
cy = 0
rx = 50      // 移動距離
ry = 0
alpha = 200  // 半透明
```

### 2. 村莊瀑布

```cpp
// 瀑布 - 向下流動
type = BackgroundType::VMoveA
cx = 0
cy = 600
rx = 0
ry = 200     // 快速向下移動
alpha = 255
```

### 3. 洞穴岩壁

```cpp
// 垂直重複的岩石紋理
type = BackgroundType::VTiled
cx = 0
cy = 400
rx = 100
ry = 100
alpha = 255
```

### 4. 星空背景

```cpp
// 緩慢橫向捲動的星空
type = BackgroundType::HMoveB
cx = 800
cy = 600
rx = 20      // 慢速移動
ry = 0
alpha = 255
```

### 5. 雨天效果

```cpp
// 下雨 - 快速垂直移動
type = BackgroundType::VMoveB
cx = 800
cy = 600
rx = 0
ry = 400     // 快速向下
alpha = 180  // 半透明
```

---

## WZ 資料格式

背景資料儲存在 `Map.wz/Back/*.img` 中:

```xml
<imgdir name="0">
    <string name="bS" value="back0"/>   <!-- 背景圖片路徑 -->
    <int name="type" value="2"/>        <!-- BackgroundType -->
    <int name="cx" value="0"/>          <!-- 水平平鋪間距 -->
    <int name="cy" value="600"/>        <!-- 垂直平鋪間距 -->
    <int name="rx" value="50"/>         <!-- 水平視差/移動 -->
    <int name="ry" value="50"/>         <!-- 垂直視差/移動 -->
    <int name="x" value="0"/>           <!-- X 位置 -->
    <int name="y" value="-300"/>        <!-- Y 位置 -->
    <int name="a" value="255"/>         <!-- Alpha 透明度 -->
    <int name="front" value="0"/>       <!-- 是否在前景 -->
    <int name="flip" value="0"/>        <!-- 是否翻轉 -->
</imgdir>
```

---

## 程式碼實作

### MakeBack 函數流程

```cpp
void MapLoadable::MakeBack(...)
{
    // 1. 建立圖層
    auto layer = gr.CreateLayer(x, y, width, height, z);

    // 2. 插入背景圖片
    layer->InsertCanvas(canvas, delay, alpha0, alpha1);

    // 3. 設定視差
    layer->SetParallax(rx, ry);

    // 4. 根據類型設定平鋪
    switch (static_cast<BackgroundType>(type))
    {
    case BackgroundType::VTiled:
        layer->SetTiling(0, cy);
        break;

    case BackgroundType::HTiled:
        layer->SetTiling(cx, 0);
        break;

    case BackgroundType::Tiled:
        layer->SetTiling(cx, cy);
        break;

    // ... 動畫類型處理
    }

    // 5. 對於動畫類型,啟動位置動畫
    if (isAnimatedType)
    {
        auto duration = 20000 / abs(moveRel);
        layer->StartPositionAnimation(moveOffsetX, moveOffsetY, duration, true);
    }
}
```

---

## 設計優勢

### 1. 靈活性
- 8 種類型覆蓋大多數背景需求
- 視差 + 平鋪 + 動畫的組合創造豐富效果

### 2. 效能
- 靜態類型 (0-3) 無動畫開銷
- 動畫類型 (4-7) 只更新位置,不重新渲染

### 3. 簡潔性
- 單一 `type` 參數決定行為
- 參數重用 (rx/ry 兼做視差和移動距離)

### 4. 可擴展性
- 可輕易新增新類型
- 使用列舉保證型別安全

---

## Clean Code 改進

### 改進前 (魔術數字):
```cpp
if (type >= 4 && type <= 7) { ... }

switch (type) {
    case 0: // NORMAL
    case 1: // HTILED
    case 2: // VTILED
    ...
}
```

### 改進後 (列舉):
```cpp
if (type >= static_cast<int>(BackgroundType::HMoveA) &&
    type <= static_cast<int>(BackgroundType::VMoveB)) { ... }

switch (static_cast<BackgroundType>(type)) {
    case BackgroundType::Normal:
    case BackgroundType::HTiled:
    case BackgroundType::VTiled:
    ...
}
```

**優點**:
- ✅ 自我文檔化
- ✅ IDE 自動完成
- ✅ 編譯期型別檢查
- ✅ 重構更安全

---

## 總結

MapleStory 背景類型系統是一個**簡潔而強大**的設計:

1. **8 種類型** 涵蓋靜態與動畫背景
2. **視差捲動** 創造深度感
3. **平鋪系統** 實現無限背景
4. **位置動畫** 增添生動效果

透過這個系統,美術人員只需設定幾個參數,就能創造出豐富的背景效果,無需程式修改。

這是 2D 遊戲引擎設計的經典範例! 🎨

---

## 相關文件

- [gr2d-complete-architecture.md](./gr2d-complete-architecture.md) - Gr2D 引擎完整架構
- [gr2d-clean-code-improvements.md](./gr2d-clean-code-improvements.md) - Clean Code 優化
- [CLogo_decompiled.md](./CLogo_decompiled.md) - Logo 場景分析

---

*最後更新: 2026-02-10*
*基於原版 MapleStory v83 反編譯分析*
