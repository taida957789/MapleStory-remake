#pragma once

namespace ms
{

/**
 * @brief Absolute position and velocity in extended precision
 *
 * Based on AbsPosEx from the original MapleStory client (v1029).
 * Used by CVecCtrl for high-precision physics calculations.
 */
class AbsPosEx
{
public:
    long double x{};
    long double y{};
    long double vx{};
    long double vy{};
};

} // namespace ms
