#pragma once

#include "enums/VecCtrlOwnerType.h"
#include "life/MpaInfo.h"
#include "util/Point.h"

#include <cstdint>
#include <list>

namespace ms
{

class VecCtrl;
class StaticFoothold;
class AttrShoe;

/**
 * @brief Interface for objects that own a velocity controller
 *
 * Based on IVecCtrlOwner from the original MapleStory client.
 * Implemented by all field objects that participate in physics/movement:
 * users, mobs, NPCs, pets, summons, etc.
 */
class IVecCtrlOwner
{
public:
    virtual ~IVecCtrlOwner() = default;

    [[nodiscard]] virtual auto GetType() const -> VecCtrlOwnerType = 0;

    virtual auto OnResolveMoveAction(
        std::int32_t nAction,
        std::int32_t nParam1,
        std::int32_t nParam2,
        const VecCtrl* pVecCtrl
    ) -> std::int32_t = 0;

    virtual void OnLayerZChanged() = 0;

    virtual void OnFootholdChanged(
        const StaticFoothold* pOld,
        const StaticFoothold* pNew
    ) = 0;

    [[nodiscard]] virtual auto GetShoeAttr() const -> const AttrShoe* = 0;
    [[nodiscard]] virtual auto GetPos() const -> Point2D = 0;
    [[nodiscard]] virtual auto GetPosPrev() const -> Point2D = 0;
    [[nodiscard]] virtual auto GetZMass() const -> std::int32_t = 0;

    [[nodiscard]] virtual auto GetVecCtrl() const -> const VecCtrl* = 0;
    virtual auto GetVecCtrl() -> VecCtrl* = 0;

    virtual void OnCompleteCalcMovePathAttrInfo(
        const std::list<MpaInfo>& lMpaInfo
    ) = 0;
};

} // namespace ms
