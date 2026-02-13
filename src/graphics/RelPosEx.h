#pragma once

namespace ms
{

/**
 * @brief Relative position and velocity in extended precision
 *
 * Based on RelPosEx from the original MapleStory client (v1029).
 * Used by CVecCtrl for single-axis physics calculations.
 */
class RelPosEx
{
public:
    long double pos{};
    long double v{};
};

} // namespace ms
