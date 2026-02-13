#pragma once

#include "util/Point.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

class CAttrField;
class IStaticFoothold;
struct LadderOrRope;

/**
 * @brief Interface for 2D physical space queries
 *
 * Based on IWvsPhysicalSpace2D from the original MapleStory client (v1029).
 * Provides foothold lookups, ladder/rope queries, and field attribute access.
 *
 * VFT layout (sizeof=0x1C):
 *   0x00  GetCrossCandidate
 *   0x04  GetBaseZMass
 *   0x08  GetFieldAttr
 *   0x0C  GetBound
 *   0x10  GetFoothold
 *   0x14  GetLadderOrRope
 *   0x18  GetLadderOrRopeBySN
 */
class IWvsPhysicalSpace2D
{
public:
    virtual ~IWvsPhysicalSpace2D() = default;

    virtual void GetCrossCandidate(
        std::int32_t x1, std::int32_t y1,
        std::int32_t x2, std::int32_t y2,
        std::vector<const IStaticFoothold*>* apResult) = 0;

    [[nodiscard]] virtual auto GetBaseZMass() -> std::int32_t = 0;
    [[nodiscard]] virtual auto GetFieldAttr() -> std::shared_ptr<CAttrField> = 0;
    [[nodiscard]] virtual auto GetBound() -> Rect = 0;
    [[nodiscard]] virtual auto GetFoothold(std::uint32_t dwSN) -> const IStaticFoothold* = 0;
    [[nodiscard]] virtual auto GetLadderOrRope(Rect rc) -> const LadderOrRope* = 0;
    [[nodiscard]] virtual auto GetLadderOrRopeBySN(std::uint32_t dwSN) -> const LadderOrRope* = 0;
};

} // namespace ms
