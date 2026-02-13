#pragma once

#include "util/security/TSecType.h"

namespace ms
{

/**
 * @brief Foothold attribute data
 *
 * Based on CAttrFoothold (__cppobj : ZRefCounted) from the original
 * MapleStory client (v1029). Stores physics modifiers for foothold
 * segments (walk speed, drag, force, fall/pass-through restrictions).
 */
class CAttrFoothold
{
public:
    TSecType<double> walk;
    TSecType<double> drag;
    TSecType<double> force;
    TSecType<std::int32_t> forbidfalldown;
    TSecType<std::int32_t> cantThrough;
};

} // namespace ms
