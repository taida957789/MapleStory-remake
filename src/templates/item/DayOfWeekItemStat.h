#pragma once

#include <cstdint>

namespace ms
{

/// Day-of-week item stat modifier
/// Original: CItemInfo::EQUIPITEM::DAYOFWEEKITEMSTAT
struct DayOfWeekItemStat
{
    std::int32_t nDOWIMDR{};
};

} // namespace ms
