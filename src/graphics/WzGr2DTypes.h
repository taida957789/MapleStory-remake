#pragma once

#include <cstdint>

namespace ms
{

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
 * @brief Layer animation type
 *
 * Based on GR2D_ANITYPE enum from the original MapleStory client.
 */
enum class Gr2DAnimationType : std::int32_t
{
    None = 0,
    Normal = 1,      // Play once forward
    Forward = 1,     // Alias for Normal
    Loop = 2,        // Loop forever
    PingPong = 3,    // Forward then backward
    Reverse = 4,     // Play backwards
    ReverseLoop = 5, // Loop backwards
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

} // namespace ms
