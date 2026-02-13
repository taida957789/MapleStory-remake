#pragma once

#include <cmath>
#include <cstdint>
#include <type_traits>

namespace ms
{

/**
 * @brief 2D Point structure
 *
 * Based on tagPOINT from the original MapleStory client.
 *
 * @tparam T Coordinate type (int32_t, float, etc.)
 */
template <typename T>
struct Point
{
    static_assert(std::is_arithmetic_v<T>, "Point requires arithmetic type");

    T x{};
    T y{};

    constexpr Point() noexcept = default;
    constexpr Point(T px, T py) noexcept : x(px), y(py) {}

    // Arithmetic operators
    [[nodiscard]] constexpr auto operator+(const Point& other) const noexcept -> Point
    {
        return Point{x + other.x, y + other.y};
    }

    [[nodiscard]] constexpr auto operator-(const Point& other) const noexcept -> Point
    {
        return Point{x - other.x, y - other.y};
    }

    [[nodiscard]] constexpr auto operator*(T scalar) const noexcept -> Point
    {
        return Point{x * scalar, y * scalar};
    }

    [[nodiscard]] constexpr auto operator/(T scalar) const noexcept -> Point
    {
        return Point{x / scalar, y / scalar};
    }

    // Compound assignment operators
    constexpr auto operator+=(const Point& other) noexcept -> Point&
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr auto operator-=(const Point& other) noexcept -> Point&
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr auto operator*=(T scalar) noexcept -> Point&
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr auto operator/=(T scalar) noexcept -> Point&
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    // Comparison operators
    [[nodiscard]] constexpr auto operator==(const Point& other) const noexcept -> bool
    {
        return x == other.x && y == other.y;
    }

    [[nodiscard]] constexpr auto operator!=(const Point& other) const noexcept -> bool
    {
        return !(*this == other);
    }

    // Utility methods
    [[nodiscard]] auto Length() const noexcept -> T
    {
        if constexpr (std::is_floating_point_v<T>)
            return std::sqrt(x * x + y * y);
        else
            return static_cast<T>(std::sqrt(static_cast<double>(x * x + y * y)));
    }

    [[nodiscard]] auto DistanceTo(const Point& other) const noexcept -> T
    {
        return (*this - other).Length();
    }

    [[nodiscard]] constexpr auto LengthSquared() const noexcept -> T
    {
        return x * x + y * y;
    }

    [[nodiscard]] constexpr auto IsZero() const noexcept -> bool
    {
        return x == T{} && y == T{};
    }
};

// Type aliases
using Point2D = Point<std::int32_t>;
using Point2DF = Point<float>;
using Point2DD = Point<double>;

/**
 * @brief Rectangle structure
 *
 * Based on RECT from the original MapleStory client.
 */
struct Rect
{
    std::int32_t left{};
    std::int32_t top{};
    std::int32_t right{};
    std::int32_t bottom{};

    constexpr Rect() noexcept = default;
    constexpr Rect(std::int32_t l, std::int32_t t, std::int32_t r, std::int32_t b) noexcept
        : left(l), top(t), right(r), bottom(b)
    {
    }

    // Factory methods
    [[nodiscard]] static constexpr auto FromXYWH(std::int32_t x, std::int32_t y,
                                                  std::int32_t w, std::int32_t h) noexcept -> Rect
    {
        return Rect{x, y, x + w, y + h};
    }

    // Dimension getters
    [[nodiscard]] constexpr auto Width() const noexcept -> std::int32_t
    {
        return right - left;
    }

    [[nodiscard]] constexpr auto Height() const noexcept -> std::int32_t
    {
        return bottom - top;
    }

    [[nodiscard]] constexpr auto Center() const noexcept -> Point2D
    {
        return Point2D{(left + right) / 2, (top + bottom) / 2};
    }

    // Hit testing
    [[nodiscard]] constexpr auto Contains(const Point2D& pt) const noexcept -> bool
    {
        return pt.x >= left && pt.x < right && pt.y >= top && pt.y < bottom;
    }

    [[nodiscard]] constexpr auto Contains(std::int32_t x, std::int32_t y) const noexcept -> bool
    {
        return x >= left && x < right && y >= top && y < bottom;
    }

    [[nodiscard]] constexpr auto Intersects(const Rect& other) const noexcept -> bool
    {
        return left < other.right && right > other.left &&
               top < other.bottom && bottom > other.top;
    }

    // Modifiers
    constexpr void Offset(std::int32_t dx, std::int32_t dy) noexcept
    {
        left += dx;
        right += dx;
        top += dy;
        bottom += dy;
    }

    constexpr void Offset(const Point2D& delta) noexcept
    {
        Offset(delta.x, delta.y);
    }

    constexpr void Inflate(std::int32_t dx, std::int32_t dy) noexcept
    {
        left -= dx;
        right += dx;
        top -= dy;
        bottom += dy;
    }

    // Comparison
    [[nodiscard]] constexpr auto operator==(const Rect& other) const noexcept -> bool
    {
        return left == other.left && top == other.top &&
               right == other.right && bottom == other.bottom;
    }

    [[nodiscard]] constexpr auto operator!=(const Rect& other) const noexcept -> bool
    {
        return !(*this == other);
    }

    // State checks
    [[nodiscard]] constexpr auto IsEmpty() const noexcept -> bool
    {
        return right <= left || bottom <= top;
    }
};

/**
 * @brief Integer range (low/high)
 *
 * Based on RANGE from the original MapleStory client.
 */
struct Range
{
    std::int32_t low{};
    std::int32_t high{};
};

} // namespace ms
