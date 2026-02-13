#pragma once

#include "ZtlSecureTear.h"

namespace ms
{

/// Matches SECPOINT from the client.
/// A tagPOINT protected by ZtlSecureTear on each coordinate.
struct SecPoint
{
    ZtlSecureTear<int> x;
    ZtlSecureTear<int> y;
};

} // namespace ms
