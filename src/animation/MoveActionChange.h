#pragma once

#include <cstdint>

namespace ms
{

/// Matches MOVE_ACTION_CHANGE from the client.
struct MoveActionChange
{
    std::int32_t m_nAction{0};
    std::int32_t m_nProb{0};
};

} // namespace ms
