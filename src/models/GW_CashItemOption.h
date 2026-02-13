#pragma once

#include "util/FileTime.h"

#include <array>
#include <cstdint>

namespace ms
{

/**
 * @brief Cash item option data
 *
 * Based on GW_CashItemOption (__cppobj : ZRefCounted) from the original
 * MapleStory client. Size: 44 bytes (excluding ZRefCounted base).
 */
class GW_CashItemOption
{
public:
    virtual ~GW_CashItemOption() = default;

    std::int64_t liCashItemSN{};
    FileTime ftExpireDate{};
    std::int32_t nGrade{};
    std::array<std::int32_t, 3> anOption{};
};

} // namespace ms
