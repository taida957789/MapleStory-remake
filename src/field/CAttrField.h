#pragma once

#include "util/Point.h"
#include "util/security/TSecType.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace ms
{

namespace Geometry
{

/**
 * @brief Line segment for inclusion checking
 *
 * Based on Geometry::InclusionChecker::Line from the original client.
 */
struct InclusionLine
{
    std::int32_t y{};
    std::int32_t x1{};
    std::int32_t x2{};
    bool bOpen{};
};

/**
 * @brief Area inclusion checker using rasterized rectangles
 *
 * Based on Geometry::InclusionChecker (__cppobj : ZRefCounted) from the
 * original MapleStory client (v1029). Checks whether a point is inside
 * a set of rectangles (swim area, climb area, crawl area).
 */
class InclusionChecker
{
public:
    std::vector<Rect> vecRect;
    std::vector<std::int32_t> vecX;
    std::vector<std::int32_t> vecY;
    std::vector<std::vector<bool>> vvArea;
    std::mutex m_lock;  // original: ZFatalSection
};

/**
 * @brief Moment (force) area for physics
 *
 * Based on Geometry::CMomentArea (__cppobj : ZRefCounted) from the
 * original MapleStory client (v1029).
 */
class CMomentArea
{
public:
    // TODO: fill from decompiled fields
};

} // namespace Geometry

/**
 * @brief Field attribute data for movement physics
 *
 * Based on CAttrField from the original MapleStory client (v1029).
 * Stores field-wide physics modifiers: walk/drag/fly/gravity multipliers,
 * swim/climb/crawl area geometry, and mid-air movement parameters.
 */
class CAttrField
{
public:
    TSecType<double> walk;
    TSecType<double> drag;
    TSecType<double> fly;
    TSecType<double> g;
    TSecType<std::int32_t> nMapType;   // original: TSecType<long>
    TSecType<std::int32_t> bFloatSky;  // original: TSecType<int>
    std::int32_t bNeedSkillForFlying{};
    std::int32_t bNeedRidingSpeed{};
    std::shared_ptr<Geometry::InclusionChecker> m_pIC_SwimArea;
    std::shared_ptr<Geometry::InclusionChecker> m_pIC_ClimbArea;
    std::shared_ptr<Geometry::InclusionChecker> m_pIC_CrawlArea;
    std::shared_ptr<Geometry::CMomentArea> m_pClimbMomentArea;
    std::shared_ptr<Geometry::CMomentArea> m_pSwimMomentArea;
    long double freeFallingVX{};
    long double dMidAirAccelX{};
    long double dMidAirDecelX{};
    long double dJumpSpeedMaxR{};
    long double dJumpSpeedUPCtrl{};
    long double dJumpSpeedDownCtrl{};
    long double dJumpApplyVXRate{};

    [[nodiscard]] auto GetFreeFallingVX() const -> long double { return freeFallingVX; }
};

} // namespace ms
