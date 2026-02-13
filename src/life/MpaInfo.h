#pragma once

#include <cstdint>

namespace ms
{

/**
 * @brief Move path attribute info
 *
 * Based on MPA_INFO from the original MapleStory client.
 * Used by IVecCtrlOwner::OnCompleteCalcMovePathAttrInfo.
 */
struct MpaInfo
{
    std::int16_t nMPA{0};
    std::int16_t nParam1{0};
    std::int16_t nParam2{0};
    std::int16_t nParam3{0};
    std::int16_t nParam4{0};
    std::int16_t nParam5{0};
    std::int16_t nParam6{0};
};

} // namespace ms
