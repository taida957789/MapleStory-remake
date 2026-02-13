#pragma once

#include "IWzShape2D.h"

#include <cstdint>
#include <vector>

namespace ms
{

// Forward declare for FlyKeyframe
class IWzVector2D;

/**
 * @brief Keyframe for Fly (cubic Hermite spline) animation
 *
 * Uses live IWzVector2D* control points with explicit tangent vectors.
 */
struct FlyKeyframe
{
    IWzVector2D* point = nullptr;
    double vel_x = 0.0;
    double vel_y = 0.0;
    double accel_x = 0.0;
    double accel_y = 0.0;
    std::int32_t time = 0;
};

/**
 * @brief Abstract 2D vector interface with hierarchical animation
 *
 * Based on IWzVector2D (COM interface : IWzShape2D) from the original client.
 * Extends IWzShape2D with animation chains, parent-child origin hierarchy,
 * rotation, flip, snapshot, and various animation commands.
 *
 * Original vtable additions over IWzShape2D (COM methods omitted):
 *   get/put_currentTime, get/put_origin, get/put_rx, get/put_ry,
 *   get_a, get/put_ra, get/put_flipX, raw__GetSnapshot,
 *   raw_RelMove, raw_RelOffset, raw_Ratio, raw_WrapClip,
 *   raw_Rotate, get/put_looseLevel, raw_Fly
 */
class IWzVector2D : public IWzShape2D
{
public:
    ~IWzVector2D() override = default;

    // --- Time ---
    [[nodiscard]] virtual auto GetCurrentTime() -> std::int32_t = 0;
    virtual void PutCurrentTime(std::int32_t t) = 0;

    // --- Parent-child origin ---
    [[nodiscard]] virtual auto GetOrigin() const -> IWzVector2D* = 0;
    virtual void PutOrigin(IWzVector2D* parent) = 0;

    // --- Relative (local) position ---
    [[nodiscard]] virtual auto GetRX() -> std::int32_t = 0;
    virtual void PutRX(std::int32_t x) = 0;
    [[nodiscard]] virtual auto GetRY() -> std::int32_t = 0;
    virtual void PutRY(std::int32_t y) = 0;

    // --- Angle ---
    [[nodiscard]] virtual auto GetA() -> double = 0;
    [[nodiscard]] virtual auto GetRA() -> double = 0;
    virtual void PutRA(double a) = 0;

    // --- Flip ---
    [[nodiscard]] virtual auto GetFlipX() -> bool = 0;
    virtual void PutFlipX(std::int32_t f) = 0;

    // --- Snapshot ---
    virtual void GetSnapshot(std::int32_t* x, std::int32_t* y,
                             std::int32_t* rx, std::int32_t* ry,
                             std::int32_t* ox, std::int32_t* oy,
                             double* a, double* ra,
                             std::int32_t time = -1) = 0;

    // --- Animation commands ---
    virtual void RelMove(std::int32_t x, std::int32_t y,
                         std::int32_t startTime = 0, std::int32_t endTime = 0,
                         bool bounce = false, bool pingpong = false,
                         bool replace = false) = 0;

    virtual void RelOffset(std::int32_t dx, std::int32_t dy,
                           std::int32_t startTime = 0, std::int32_t endTime = 0) = 0;

    virtual void Ratio(IWzVector2D* target,
                       std::int32_t denomX, std::int32_t denomY,
                       std::int32_t scaleX, std::int32_t scaleY) = 0;

    virtual void WrapClip(IWzVector2D* bounds,
                          std::int32_t x, std::int32_t y,
                          std::int32_t w, std::int32_t h,
                          bool clampMode) = 0;

    virtual void Rotate(double angle, std::int32_t period,
                        std::int32_t easeFrames = 0) = 0;

    // --- Loose level ---
    [[nodiscard]] virtual auto GetLooseLevel() -> std::int32_t = 0;
    virtual void PutLooseLevel(std::int32_t level) = 0;

    // --- Fly (spline animation) ---
    virtual void Fly(const std::vector<FlyKeyframe>& keyframes,
                     IWzVector2D* completionTarget = nullptr) = 0;
};

} // namespace ms
