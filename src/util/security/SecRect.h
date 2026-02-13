#pragma once

#include "ZtlSecureTear.h"

namespace ms
{

/// Matches SECRECT from the client.
/// A tagRECT protected by ZtlSecureTear on each member.
struct SecRect
{
    ZtlSecureTear<int> left;
    ZtlSecureTear<int> top;
    ZtlSecureTear<int> right;
    ZtlSecureTear<int> bottom;
};

} // namespace ms
