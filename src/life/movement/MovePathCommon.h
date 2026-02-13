#pragma once

#include "util/security/ZtlSecureTear.h"

#include <cstdint>
#include <list>
#include <vector>

namespace ms
{

/**
 * @brief Common move path data shared by users, mobs, etc.
 *
 * Based on CMovePathCommon from the original MapleStory client (v1029).
 * Stores position/velocity as ZtlSecureTear-protected values,
 * a list of movement elements, and keypad state.
 */
class MovePathCommon
{
public:
    /// CMovePathCommon::ELEM — a single movement element in the path
    struct Elem
    {
        ZtlSecureTear<std::int16_t> x;
        ZtlSecureTear<std::int16_t> xOffset;
        ZtlSecureTear<std::int16_t> vy;
        ZtlSecureTear<std::int16_t> fhFallStart;
        ZtlSecureTear<std::int16_t> fh;
        ZtlSecureTear<std::int16_t> tElapse;
        ZtlSecureTear<std::int16_t> vx;
        ZtlSecureTear<std::int16_t> yOffset;
        ZtlSecureTear<std::int16_t> y;
        ZtlSecureTear<std::uint8_t> nAttr;
        ZtlSecureTear<std::uint8_t> bSN;
        ZtlSecureTear<std::uint8_t> bMoveAction;
        ZtlSecureTear<std::uint8_t> bForcedStop;
    };

    virtual ~MovePathCommon() = default;

protected:
    ZtlSecureTear<std::int16_t> m_x;
    ZtlSecureTear<std::int16_t> m_y;
    ZtlSecureTear<std::int16_t> m_vx;
    ZtlSecureTear<std::int16_t> m_vy;
    std::int32_t m_tEncodedGatherDuration{};
    std::list<Elem> m_lElem;
    std::vector<std::uint8_t> m_aKeyPadState;
};

} // namespace ms
