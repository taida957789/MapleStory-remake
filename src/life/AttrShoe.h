#pragma once

#include "util/security/TSecType.h"

namespace ms
{

/**
 * @brief Shoe attribute modifiers for movement physics
 *
 * Based on CAttrShoe from the original MapleStory client (v1029).
 * Stores multipliers for walk, swim, and fly physics parameters.
 * Used by CVecCtrl via IVecCtrlOwner::GetShoeAttr().
 */
class AttrShoe
{
public:
    AttrShoe()
        : mass(100.0)
        , walkAcc(1.0), walkSpeed(1.0), walkDrag(1.0)
        , walkSlant(0.9), walkJump(1.0)
        , swimAcc(1.0), swimSpeedH(1.0), swimSpeedV(1.0)
        , flyAcc(0.0), flySpeed(0.0)
        , compulsionSlant(0.0)
    {
    }

    virtual ~AttrShoe() = default;

    TSecType<double> mass;
    TSecType<double> walkAcc;
    TSecType<double> walkSpeed;
    TSecType<double> walkDrag;
    TSecType<double> walkSlant;
    TSecType<double> walkJump;
    TSecType<double> swimAcc;
    TSecType<double> swimSpeedH;
    TSecType<double> swimSpeedV;
    TSecType<double> flyAcc;
    TSecType<double> flySpeed;
    TSecType<double> compulsionSlant;
};

} // namespace ms
