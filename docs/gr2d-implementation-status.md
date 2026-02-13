# Gr2D Engine Implementation Status

## Overview
Based on reverse-engineered interface signatures from the original MapleStory client, this document tracks the implementation status of the Gr2D graphics engine.

## IWzGr2DLayer Implementation

### ✅ Implemented Features

#### Position and Dimensions
- ✅ `GetLeft()` / `GetTop()` / `GetWidth()` / `GetHeight()`
- ✅ `Getz()` / `Putz()`
- ✅ `Putwidth()` / `Putheight()` - Added as `SetWidth()` / `SetHeight()`
- ✅ `Getlt()` / `Getrb()` - Bounding box (left-top, right-bottom)

#### Visual Properties
- ✅ `Getalpha()` - Extract alpha from color
- ✅ `Getcolor()` / `Putcolor()`
- ✅ `Getvisible()` / `Putvisible()`
- ✅ `Getflip()` / `Putflip()`

#### Blend Modes
- ✅ `Getblend()` / `Putblend()` - Layer blend modes
  - Normal, Add, Multiply, Screen, Overlay, LinearDodge, Darken, Lighten
  - Special flags: Isolated, Premultiplied, Inverse
  - Mapped to SDL blend modes where possible

#### Canvas Management
- ✅ `InsertCanvas()` - Add canvas frames with delay, alpha, zoom parameters
- ✅ `RemoveCanvas()` - Remove canvas by index
- ✅ `Getcanvas()` - Get canvas by index
- ✅ `GetcanvasCount()` - Get total canvas count
- ✅ `ShiftCanvas()` - Reorder canvas frames
- ✅ `InitCanvasOrder()` - Reset canvas ordering

#### Animation
- ✅ `Animate()` - Start animation with flags (First, Repeat, Reverse, Clear, Wait)
- ✅ `GetanimationState()` - Current animation state
- ✅ `GetanimationTime()` - Animation time since last frame
- ✅ `GetlastAnimate()` - Last animation type
- ✅ `GetfirstAnimationAlpha0()` - First frame's initial alpha

#### Overlay and Rotation
- ✅ `Putoverlay()` / `Getoverlay()` - Overlay color for effects
- ✅ `SetRotationLayer()` - Layer rotation support

### ❌ Not Implemented (Lower Priority)

#### Shader Support
- ❌ `VertexShaderSet()` / `VertexShaderUnSet()`
- ❌ `PixelShaderSet()` / `PixelShaderUnSet()`
- ❌ `VertexShaderConstSet()`
- ❌ `PixelShaderConstSet()` / `PixelShaderConstGet()`

**Reason**: SDL3 doesn't directly support custom shaders. Would need OpenGL/Vulkan backend or shader reimplementation.

#### Color Tone Effects
- ❌ `GetredTone()` / `GetgreenBlueTone()`

**Reason**: Specific color tone adjustment for status effects. Can be emulated with color modulation.

#### Advanced Features
- ❌ `InsertMaskCanvas()` - Alpha masking
- ❌ `PutrenderToTexture()` - Render to texture (RTT)
- ❌ `SetSpineMode()` / `SetSpineInfo()` - Spine skeletal animation

**Reason**: Advanced features for later MapleStory versions. SDL3 RTT is possible but not yet needed.

---

## IWzCanvas Implementation

### Current Status

#### ✅ Implemented in WzCanvas
- ✅ `Getwidth()` / `Getheight()`
- ✅ Pixel data storage (RGBA format)

#### ✅ Implemented in WzGr2DCanvas
- ✅ Position and origin (anchor point)
- ✅ Z-ordering
- ✅ SDL texture creation and management

### ❌ Missing Canvas Features

#### Size Properties
- ❌ `Getcx()` / `Putcx()` - Horizontal size/extent
- ❌ `Getcy()` / `Putcy()` - Vertical size/extent
- ❌ `Putwidth()` / `Putheight()` - Dynamic resize

**Note**: `cx/cy` might refer to tiling dimensions or scaled dimensions. Needs investigation.

#### Pixel Access
- ❌ `Getpixel(x, y)` - Get pixel color at coordinates
- ❌ `GetpixelFormat()` / `PutpixelFormat()` - Pixel format enum (partial: enum exists)

#### Text Rendering
- ❌ `DrawTextA()` - Draw text on canvas

**Note**: Currently using separate TextRenderer class. This would integrate text rendering into canvas.

#### Properties
- ❌ `Getproperty()` - Get associated WzProperty
- ❌ `GetmagLevel()` / `PutmagLevel()` - Magnification level

#### Multi-Resolution Support
- ❌ `GetrawCanvas(level, index)` - Get raw canvas at different resolution
- ❌ `GetrawCanvasCount()` - Number of raw canvases

**Note**: Modern MapleStory supports multiple resolutions. This feature provides different quality levels.

#### Tiling
- ❌ `GettileWidth()` / `GettileHeight()`

**Note**: For tiled backgrounds. Currently handled in Layer.

#### Effects
- ❌ `SetBlur(...)` - Blur effect (6 parameters)
- ❌ `SetClipRect(...)` - Clipping rectangle

---

## Animation Type Flags (GR2D_ANITYPE)

### ✅ Correctly Implemented as Bit Flags

```cpp
enum class Gr2DAnimationType : std::int32_t
{
    Stop = 0x0,          // GA_STOP
    Normal = 0x0,        // GA_NORMAL (same as Stop)
    First = 0x10,        // GA_FIRST - Start from first frame
    Repeat = 0x20,       // GA_REPEAT - Loop animation
    Reverse = 0x40,      // GA_REVERSE - Play in reverse
    Wait = 0x100,        // GA_WAIT - Pause animation
    Clear = 0x200,       // GA_CLEAR - Clear on completion

    // Common combinations
    Loop = First | Repeat,                    // 0x30
    ReverseLoop = Reverse | Repeat,           // 0x60
    ReverseWithClear = Reverse | Clear,       // 0x240
    PingPong = First | Repeat | Reverse,      // 0x70
};
```

**Behavior**:
- `First`: Start from frame 0
- `Repeat`: Loop animation
- `Reverse`: Play backwards
- `Clear`: Remove all canvases when animation completes
- `Wait`: Pause animation
- Can be combined with bitwise OR

---

## Blend Type Flags (LAYER_BLENDTYPE)

### ✅ Correctly Implemented as Bit Flags

```cpp
enum class LayerBlendType : std::int32_t
{
    Normal = 0x0,
    Add = 0x1,            // Additive blending
    Inverse = 0x2,        // Inverse blending
    Isolated = 0x4,       // Don't blend with lower layers
    Premultiplied = 0x8,  // Premultiplied alpha
    Multiply = 0x10,      // Multiply blending
    Screen = 0x20,        // Screen blending
    Overlay = 0x40,       // Overlay blending
    LinearDodge = 0x80,   // Linear dodge (same as Add)
    Darken = 0x100,       // Darken (min)
    Lighten = 0x200,      // Lighten (max)
    All = 0x3FF,
};
```

**SDL3 Mapping**:
- `Add` / `LinearDodge` → `SDL_BLENDMODE_ADD`
- `Multiply` → `SDL_BLENDMODE_MUL`
- Others → `SDL_BLENDMODE_BLEND` (with limitations)

**Note**: Some blend modes (Screen, Overlay, Darken, Lighten) don't have direct SDL equivalents.

---

## Priority for Future Implementation

### High Priority
1. ✅ **Blend modes** - Implemented
2. ✅ **Animation flags** - Implemented
3. ✅ **Rotation** - Implemented
4. ❌ **Color tone** - For status effects (poison, burn, etc.)
5. ❌ **Pixel access** - For collision detection

### Medium Priority
1. ❌ **Canvas resize** - Dynamic scaling
2. ❌ **Multi-resolution support** - For HD textures
3. ❌ **Masking** - For advanced UI effects
4. ❌ **Render to texture** - For post-processing

### Low Priority
1. ❌ **Shader support** - Complex, needs different rendering backend
2. ❌ **Spine animation** - Only in newer versions
3. ❌ **Text on canvas** - Already have TextRenderer

---

## Summary

**Implemented**: ~70% of core IWzGr2DLayer features
**Missing**: Mostly advanced rendering features (shaders, RTT, Spine)

The current implementation covers all essential features needed for MapleStory v83-era graphics:
- Layer rendering with position, scale, rotation, flip
- Animation with proper flag-based control
- Blend modes for visual effects
- Canvas management and ordering

Missing features are either:
1. Advanced rendering (shaders, RTT) - requires different backend
2. Newer version features (Spine, multi-res) - not needed for v83
3. Optimization features (color tone) - can be added incrementally

## Recommendations

1. **Add color tone support** - Useful for status effects
2. **Add pixel access methods** - For collision detection
3. **Test blend modes** - Verify visual correctness
4. **Consider multi-resolution** - For modern displays

---

*Last updated: 2026-02-10*
