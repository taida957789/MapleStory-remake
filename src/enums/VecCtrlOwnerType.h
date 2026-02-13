#pragma once

#include <cstdint>

namespace ms
{

/**
 * @brief Type of velocity controller owner
 *
 * Based on IVecCtrlOwner::OwnerType from the original MapleStory client.
 * Identifies the kind of field object that owns a CVecCtrl instance.
 */
enum class VecCtrlOwnerType : std::int32_t
{
    User = 0x0,
    UserZeroSub = 0x1,
    Mob = 0x2,
    Npc = 0x3,
    Pet = 0x4,
    Summon = 0x5,
    TownPortal = 0x6,
    Employee = 0x7,
    Grenade = 0x8,
    Dragon = 0x9,
    Android = 0xA,
    UserControlMob = 0xB,
    FoxMan = 0xC,
    SkillPet = 0xD,
};

} // namespace ms
