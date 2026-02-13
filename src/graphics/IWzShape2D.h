#pragma once

#include <cstdint>

namespace ms
{

/**
 * @brief Abstract 2D shape interface
 *
 * Based on IWzShape2D from the original MapleStory client (COM interface).
 * Provides position (x, y), secondary point (x2, y2), and transforms.
 *
 * Original vtable (COM methods omitted):
 *   get_x / put_x, get_y / put_y,
 *   get_x2 / put_x2, get_y2 / put_y2,
 *   raw_Move, raw_Offset, raw_Scale, raw_Insert, raw_Remove, raw_Init
 */
class IWzShape2D
{
public:
    virtual ~IWzShape2D() = default;

    // --- Position (first point) ---
    [[nodiscard]] virtual auto GetX() -> std::int32_t = 0;
    virtual void PutX(std::int32_t x) = 0;
    [[nodiscard]] virtual auto GetY() -> std::int32_t = 0;
    virtual void PutY(std::int32_t y) = 0;

    // --- Second point (for shapes; vectors default to 0) ---
    [[nodiscard]] virtual auto GetX2() -> std::int32_t { return 0; }
    virtual void PutX2(std::int32_t /*x2*/) {}
    [[nodiscard]] virtual auto GetY2() -> std::int32_t { return 0; }
    virtual void PutY2(std::int32_t /*y2*/) {}

    // --- Transforms ---
    virtual void Move(std::int32_t x, std::int32_t y) = 0;
    virtual void Offset(std::int32_t dx, std::int32_t dy) = 0;
    virtual void Scale(std::int32_t sx, std::int32_t divx,
                       std::int32_t sy, std::int32_t divy,
                       std::int32_t cx, std::int32_t cy) = 0;
    virtual void Init(std::int32_t x, std::int32_t y) = 0;
};

} // namespace ms
