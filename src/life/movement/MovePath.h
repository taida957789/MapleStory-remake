#pragma once

#include "life/MpaInfo.h"
#include "life/movement/MovePathCommon.h"

namespace ms
{

/**
 * @brief Concrete move path with timing and flush control
 *
 * Based on CMovePath (__cppobj : CMovePathCommon) from the original
 * MapleStory client (v1029). Extends CMovePathCommon with interval
 * timing, forced flush state, and last move path attribute info.
 */
class MovePath : public MovePathCommon
{
public:
    std::int32_t m_bShortUpdate{};
    Elem m_elemLast;
    std::int32_t m_tGatherDuration{};
    std::int32_t m_bForcedFlush{};
    bool m_bForcedFlushByPortal{};
    std::vector<std::uint8_t> m_aKeyPadStateByFoothold;
    ZtlSecureTear<std::int32_t> m_tInterval;
    ZtlSecureTear<std::int32_t> m_tOffset;
    ZtlSecureTear<std::int32_t> m_tReceived;
    std::list<MpaInfo> m_listLastMovePathAttrInfo;
};

} // namespace ms
