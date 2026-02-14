#pragma once

#include <cstdint>
#include <vector>

namespace ms
{

class Gr2DVector;

/**
 * @brief Canvas pixel format
 *
 * Based on CANVAS_PIXFORMAT enum from the original MapleStory client.
 */
enum class CanvasPixelFormat : std::int32_t
{
    ARGB4444 = 1,
    ARGB8888 = 2,
    RGB565 = 513,
    DXT3 = 517,
    DXT5 = 1026,
};

/**
 * @brief Canvas alpha blending type
 *
 * Based on CANVAS_ALPHATYPE enum from the original MapleStory client.
 */
enum class CanvasAlphaType : std::int32_t
{
    None = 0,
    Default = 1,
    Additive = 2,
    Screen = 3,
};

/**
 * @brief Layer blend modes (bit flags)
 *
 * Based on LAYER_BLENDTYPE enum from the original MapleStory client.
 * These are bit flags that can be combined.
 */
enum class LayerBlendType : std::int32_t
{
    Normal = 0x0,         // Normal blending
    Add = 0x1,            // Additive blending (src + dst)
    Inverse = 0x2,        // Inverse blending (1 - src)
    Isolated = 0x4,       // Isolated layer (don't blend with layers below)
    Premultiplied = 0x8,  // Premultiplied alpha (texture already multiplied by alpha)
    Multiply = 0x10,      // Multiply blending (src * dst)
    Screen = 0x20,        // Screen blending (1 - (1-src)*(1-dst))
    Overlay = 0x40,       // Overlay blending
    LinearDodge = 0x80,   // Linear dodge (same as Add in most implementations)
    Darken = 0x100,       // Darken (min(src, dst))
    Lighten = 0x200,      // Lighten (max(src, dst))
    All = 0x3FF,          // All blend mode bits
};

/**
 * @brief Layer animation type (bit flags)
 *
 * Based on GR2D_ANITYPE enum from the original MapleStory client.
 * Note: GA_FIRST (0x10) and GA_REPEAT (0x20) are mutually exclusive.
 */
enum class Gr2DAnimationType : std::int32_t
{
    Stop = 0x0,                // GA_STOP
    Normal = 0x0,              // GA_NORMAL (same as Stop)
    None = 0x0,                // Alias
    First = 0x10,              // GA_FIRST - Play from first frame (one-shot)
    Repeat = 0x20,             // GA_REPEAT - Loop animation
    Reverse = 0x40,            // GA_REVERSE - Play in reverse
    Wait = 0x100,              // GA_WAIT - Pause
    Clear = 0x200,             // GA_CLEAR - Clear on completion
    ReverseWithClear = 0x240,  // GA_REVERSE_WITH_CLEAR
    Loop = 0x20,               // Alias for Repeat
};

/**
 * @brief Layer flip state
 */
enum class LayerFlipState : std::int32_t
{
    None = 0,
    Horizontal = 1,
    Vertical = 2,
    Both = 3,
};

/**
 * @brief Animation playback state
 *
 * Based on the original MapleStory animation state machine (this[91] in CGr2DLayer).
 */
enum class AnimationState : std::int32_t
{
    Idle = 0,      // Not playing
    Forward = 1,   // Playing forward
    Backward = 2,  // Playing backward
    Stopped = 3,   // Stopped (finished)
};

/**
 * @brief Canvas frame info for animated layers
 *
 * Based on the original MapleStory animation system.
 * The original uses 1000 as the base for zoom (1000 = 100%).
 *
 * Note: The zoom center/origin is determined by the Canvas's origin property,
 * not stored here. This matches the original CGr2DLayer_ApplyScaleTransform behavior.
 */
struct CanvasFrameInfo
{
    std::int32_t nDelay{100};     // Frame delay in ms
    std::uint8_t nAlpha0{255};    // Start alpha (0-255)
    std::uint8_t nAlpha1{255};    // End alpha (0-255)
    std::int32_t nZoom0{1000};    // Start zoom in thousandths (1000 = 100%)
    std::int32_t nZoom1{1000};    // End zoom in thousandths (1000 = 100%)
};

// Animation and rendering constants
namespace Gr2DConstants
{
    // Zoom scale factor (1000 = 100%)
    constexpr std::int32_t ZoomScaleFactor = 1000;
    constexpr std::int32_t ZoomNormal = 1000;

    // Delay rate scale factor (1000 = 100% speed)
    constexpr std::int32_t DelayRateScaleFactor = 1000;
    constexpr std::int32_t DelayRateNormal = 1000;

    // Alpha range
    constexpr std::uint8_t AlphaTransparent = 0;
    constexpr std::uint8_t AlphaOpaque = 255;

    // Default frame delay (milliseconds)
    constexpr std::int32_t DefaultFrameDelay = 100;

    // Infinite repeat
    constexpr std::int32_t InfiniteRepeat = -1;
}

/**
 * @brief Abstract canvas interface
 *
 * Based on IWzCanvas from original MapleStory client.
 * Implementors provide texture data for rendering.
 */
class ICanvas
{
public:
    virtual ~ICanvas() = default;
    virtual auto getWidth() const -> int = 0;
    virtual auto getHeight() const -> int = 0;
    virtual auto getOriginX() const -> int = 0;
    virtual auto getOriginY() const -> int = 0;
    virtual auto getTextureHandle() const -> std::intptr_t = 0;
    virtual auto isReady() const -> bool = 0;

    virtual auto getSrcX() const -> int { return 0; }
    virtual auto getSrcY() const -> int { return 0; }
    virtual auto getSrcW() const -> int { return getWidth(); }
    virtual auto getSrcH() const -> int { return getHeight(); }
};

/**
 * @brief Animation frame node (doubly-linked list with hash chain)
 *
 * From raw_InsertCanvas (0x5322E550). Each frame contains an ICanvas*
 * plus playback parameters.
 */
struct FrameNode
{
    FrameNode* next = nullptr;
    FrameNode* prev = nullptr;
    FrameNode* hashNext = nullptr;
    std::int32_t frameId = 0;
    ICanvas* canvas = nullptr;
    std::int32_t duration = 0;
    std::int32_t alphaA = -1;
    std::int32_t alphaB = -1;
    std::int32_t blendSrc = 0;
    std::int32_t blendDst = 0;
};

/**
 * @brief Render command output from Animate()
 *
 * From sub_5322F150 (17-DWORD structure). External renderer draws
 * the scene from this list after each Animate() call.
 */
struct RenderCommand
{
    std::int32_t frameIndex = 0;
    std::int32_t timestamp = 0;
    std::int32_t currentFrameTime = -1;
    std::int32_t alpha = 255;
    std::int32_t colorMod = 255;
    std::int32_t blendSrc = 0;
    std::int32_t blendDst = 0;
    std::intptr_t textureHandle = 0;
    std::int32_t srcX = 0, srcY = 0;
    std::int32_t srcW = 0, srcH = 0;
    std::int32_t dstW = 0, dstH = 0;
};

/**
 * @brief Single particle state (180 bytes)
 *
 * From sub_5321BD40. [N] = float offset in original structure.
 */
struct Particle
{
    float posX = 0, posY = 0;
    float baseVelX = 0, baseVelY = 0;
    float pad4_7[4] = {};
    float colorR = 0, colorG = 0;
    float colorB = 0, colorA = 0;
    float startR = 0, startG = 0;
    float startB = 0, startA = 0;
    float endR = 0, endG = 0;
    float endB = 0, endA = 0;
    float alphaKeys[4] = {};
    float pad24_27[4] = {};
    float sizeCurrent = 0;
    float sizeRate = 0;
    float sizeScale = 0;
    float rotationRate = 0;
    float rotationAccel = 0;
    float timeRemaining = 0;
    float totalLifetime = 0;
    float pad35_36[2] = {};
    float driftX = 0, driftY = 0;
    float forceScaleA = 0;
    float forceScaleB = 0;
    float angularData[4] = {};
};
static_assert(sizeof(Particle) == 180, "Particle must be 180 bytes");

/**
 * @brief Particle emitter system
 *
 * From sub_5321BD40 and internal object this[9].
 */
struct ParticleEmitter
{
    std::vector<Particle> particles;
    std::int32_t maxParticles = 0;
    std::int32_t activeCount = 0;
    float emitInterval = 0.0F;
    float frameAccumulator = 0.0F;
    float elapsedTime = 0.0F;
    float maxLifetime = -1.0F;
    float opacityMultiplier = 1.0F;
    float timeScale = 1.0F;

    bool usePhysics = false;
    bool affectGravity = false;
    std::int32_t mirrorDirection = 1;
    std::int32_t positionType = 0;
    std::int32_t originX = 0, originY = 0;
    std::int32_t parallaxX = 0, parallaxY = 0;

    float forceXA = 0, forceXB = 0;
    float forceYA = 0, forceYB = 0;
    float velocityX = 0, velocityY = 0;

    Gr2DVector* animOrigin = nullptr;

    void update(float deltaTime, float param3, int param4, float param5);
    void reset();
};

} // namespace ms
