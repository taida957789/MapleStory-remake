#pragma once

#include "life/IVecCtrlOwner.h"
#include "util/Point.h"

#include <cstdint>
#include <list>

namespace ms
{

/**
 * @brief Base class for all field life objects (players, mobs, NPCs)
 *
 * Based on CLife (__cppobj : CFieldObj) from the original MapleStory client.
 * Inherits IVecCtrlOwner for physics/movement integration.
 * Holds field-related state: template ID, position, foothold, direction.
 */
class Life : public IVecCtrlOwner
{
public:
    ~Life() override = default;

    // --- IVecCtrlOwner stubs (to be implemented per subclass) ---
    [[nodiscard]] auto GetType() const -> VecCtrlOwnerType override
    {
        return VecCtrlOwnerType::User;
    }

    auto OnResolveMoveAction(
        std::int32_t /*nAction*/,
        std::int32_t /*nParam1*/,
        std::int32_t /*nParam2*/,
        const VecCtrl* /*pVecCtrl*/
    ) -> std::int32_t override { return 0; }

    void OnLayerZChanged() override {}

    void OnFootholdChanged(
        const StaticFoothold* /*pOld*/,
        const StaticFoothold* /*pNew*/
    ) override {}

    [[nodiscard]] auto GetShoeAttr() const -> const AttrShoe* override { return nullptr; }
    [[nodiscard]] auto GetPos() const -> Point2D override { return {}; }
    [[nodiscard]] auto GetPosPrev() const -> Point2D override { return {}; }
    [[nodiscard]] auto GetZMass() const -> std::int32_t override { return 0; }

    [[nodiscard]] auto GetVecCtrl() const -> const VecCtrl* override { return nullptr; }
    auto GetVecCtrl() -> VecCtrl* override { return nullptr; }

    void OnCompleteCalcMovePathAttrInfo(
        const std::list<MpaInfo>& /*lMpaInfo*/
    ) override {}

    // --- Template ---
    std::int32_t m_nTemplateID{0};

    // --- Field position ---
    std::int32_t m_cy{0};
    std::int32_t m_nFootholdSN{0};
    bool m_bLeft{false};

    // --- Hide ---
    std::int32_t m_tHide{0};
};

} // namespace ms
