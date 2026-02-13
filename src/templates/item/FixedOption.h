#pragma once

#include <cstdint>

namespace ms
{

/// Fixed potential option entry
/// Original: CItemInfo::EQUIPITEM::FIXEDOPTION
struct FixedOption
{
    std::int32_t nOption{};
    std::int32_t nLevel{};
};

} // namespace ms
