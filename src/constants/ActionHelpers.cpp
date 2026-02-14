#include "ActionHelpers.h"

namespace
{

// --- Vehicle IDs ---
inline constexpr std::int32_t kMechanicTankVehicle  = 1932016;
inline constexpr std::int32_t kResistanceRiding1    = 1932051;
inline constexpr std::int32_t kResistanceRiding2    = 1932085;
inline constexpr std::int32_t kMountainRideVehicle  = 1932165;
inline constexpr std::int32_t kCatapultVehicle      = 1932214;
inline constexpr std::int32_t kGeorgVehicle         = 1932297;
inline constexpr std::int32_t kEvanDragonVehicle    = 1939007;

bool is_wild_hunter_jaguar(std::int32_t nVehicleID)
{
    switch (nVehicleID)
    {
    case 1932015:
    case 1932030:
    case 1932031:
    case 1932032:
    case 1932033:
    case 1932036:
    case 1932100:
    case 1932149:
    case 1932215:
        return true;
    default:
        return false;
    }
}

} // anonymous namespace

namespace ms
{

bool is_dance_action(CharacterAction nAction)
{
    if (nAction >= CharacterAction::Dance0
        && nAction <= CharacterAction::Dance8)
        return true;

    if (nAction >= CharacterAction::DanceStarplanet0
        && nAction <= CharacterAction::DanceStarplanet5)
        return true;

    if (nAction >= CharacterAction::DanceStarplanetEvent0
        && nAction <= CharacterAction::DanceStarplanetEvent5)
        return true;

    if (nAction >= CharacterAction::SpinoffGuitarStandM
        && nAction <= CharacterAction::SpinoffGuitarStrokeW)
        return true;

    return false;
}

bool is_weapon_hide_action(CharacterAction nAction)
{
    // Supercannon — check first since the decompiled code does
    if (nAction > CharacterAction::Supercannon)
        return false;

    if (nAction == CharacterAction::Supercannon)
        return true;

    // Specific actions
    switch (nAction)
    {
    case CharacterAction::Sanctuary:
    case CharacterAction::Showdown:
    case CharacterAction::AirStrike:
    case CharacterAction::Blade:
        return true;
    default:
        break;
    }

    // Dance actions
    if (is_dance_action(nAction))
        return true;

    if (nAction == CharacterAction::HideBody)
        return true;

    if (nAction >= CharacterAction::SpinoffGuitarStandM
        && nAction <= CharacterAction::SpinoffGuitarStrokeW)
        return true;

    if (nAction >= CharacterAction::BattlepvpManjiWalk
        && nAction <= CharacterAction::BattlepvpManjiPronestab)
        return true;

    return false;
}

void action_mapping_for_ghost(std::int32_t& nAction)
{
    switch (static_cast<CharacterAction>(nAction))
    {
    case CharacterAction::GhostWalk:
        nAction = static_cast<std::int32_t>(CharacterAction::Walk1);
        break;
    case CharacterAction::GhostStand:
        nAction = static_cast<std::int32_t>(CharacterAction::Stand1);
        break;
    case CharacterAction::GhostJump:
        nAction = static_cast<std::int32_t>(CharacterAction::Jump);
        break;
    case CharacterAction::GhostPronestab:
        nAction = static_cast<std::int32_t>(CharacterAction::Pronestab);
        break;
    case CharacterAction::GhostFly:
        nAction = static_cast<std::int32_t>(CharacterAction::Fly1);
        break;
    case CharacterAction::GhostLadder:
        nAction = static_cast<std::int32_t>(CharacterAction::Ladder);
        break;
    case CharacterAction::GhostRope:
        nAction = static_cast<std::int32_t>(CharacterAction::Rope);
        break;
    case CharacterAction::GhostSit:
        nAction = static_cast<std::int32_t>(CharacterAction::Sit);
        break;
    default:
        break;
    }
}

bool is_gather_tool_item(std::int32_t nItemID)
{
    return nItemID / 10000 == 150 || nItemID / 10000 == 151;
}

bool is_weapon_sticker_item(std::int32_t nItemID)
{
    return nItemID / 100000 == 17;
}

bool is_vari_cane_weapon(std::int32_t nItemID)
{
    switch (nItemID)
    {
    case 1332169:
    case 1332237:
    case 1342085:
    case 1382144:
    case 1402130:
    case 1422090:
    case 1432118:
    case 1452148:
    case 1462138:
    case 1472160:
    case 1482121:
    case 1492121:
    case 1532108:
        return true;
    default:
        return false;
    }
}

bool is_pronestab_action(std::int32_t nAction)
{
    auto act = static_cast<CharacterAction>(nAction);
    return act == CharacterAction::Pronestab
        || act == CharacterAction::PronestabJaguar;
}

bool is_vehicle(std::int32_t nVehicleID)
{
    // Vehicle IDs are in the 1932xxx range (taming mob items)
    return nVehicleID > 0 && nVehicleID / 10000 == 193;
}

bool is_walk_action(std::int32_t nAction)
{
    auto act = static_cast<CharacterAction>(nAction);
    return act == CharacterAction::Walk1
        || act == CharacterAction::Walk2;
}

bool is_shoot_action(std::int32_t nAction)
{
    auto act = static_cast<CharacterAction>(nAction);
    switch (act)
    {
    case CharacterAction::Shoot1:
    case CharacterAction::Shoot2:
    case CharacterAction::Shootf:
    case CharacterAction::Shoot6:
        return true;
    default:
        return false;
    }
}

bool is_battle_pvp_dead_action(std::int32_t nAction)
{
    switch (static_cast<CharacterAction>(nAction))
    {
    case CharacterAction::BattlepvpManjiDie:
    case CharacterAction::BattlepvpMikeDie:
    case CharacterAction::BattlepvpDarklordDie:
    case CharacterAction::BattlepvpHeinzDie:
    case CharacterAction::BattlepvpMugongDie:
    case CharacterAction::BattlepvpHelenaDie:
    case CharacterAction::BattlepvpLangeDie:
    case CharacterAction::BattlepvpLeemalnyunDie:
        return true;
    default:
        return false;
    }
}

bool is_battle_pvp_basic_attack_action(std::int32_t nAction)
{
    switch (static_cast<CharacterAction>(nAction))
    {
    case CharacterAction::BattlepvpManjiAttack1:
    case CharacterAction::BattlepvpManjiAttack2:
    case CharacterAction::BattlepvpMikeActdummy:
    case CharacterAction::BattlepvpMikeBasicattack1:
    case CharacterAction::BattlepvpDarklordBasicattack1:
    case CharacterAction::BattlepvpDarklordBasicattack2:
    case CharacterAction::BattlepvpHeinzMagicclaw:
    case CharacterAction::BattlepvpMugongAttack1:
    case CharacterAction::BattlepvpHelenaBasicattack:
    case CharacterAction::BattlepvpLangeBasicattack:
    case CharacterAction::BattlepvpLeemalnyunBasicattack1:
        return true;
    default:
        return false;
    }
}

bool is_battle_pvp_rope_action(std::int32_t nAction)
{
    switch (static_cast<CharacterAction>(nAction))
    {
    case CharacterAction::BattlepvpManjiRope:
    case CharacterAction::BattlepvpMikeRope:
    case CharacterAction::BattlepvpDarklordRope:
    case CharacterAction::BattlepvpHeinzRope:
    case CharacterAction::BattlepvpMugongRope:
    case CharacterAction::BattlepvpHelenaRope:
    case CharacterAction::BattlepvpLangeRope:
    case CharacterAction::BattlepvpLeemalnyunRope:
        return true;
    default:
        return false;
    }
}

bool is_battle_pvp_walk_action(std::int32_t nAction)
{
    switch (static_cast<CharacterAction>(nAction))
    {
    case CharacterAction::BattlepvpManjiWalk:
    case CharacterAction::BattlepvpMikeWalk:
    case CharacterAction::BattlepvpDarklordWalk:
    case CharacterAction::BattlepvpHeinzWalk:
    case CharacterAction::BattlepvpMugongWalk:
    case CharacterAction::BattlepvpHelenaWalk:
    case CharacterAction::BattlepvpLangeWalk:
    case CharacterAction::BattlepvpLeemalnyunWalk:
        return true;
    default:
        return false;
    }
}

bool is_battle_pvp_stand_action(std::int32_t nAction)
{
    switch (static_cast<CharacterAction>(nAction))
    {
    case CharacterAction::BattlepvpManjiStand:
    case CharacterAction::BattlepvpMikeStand:
    case CharacterAction::BattlepvpDarklordStand:
    case CharacterAction::BattlepvpHeinzStand:
    case CharacterAction::BattlepvpMugongStand:
    case CharacterAction::BattlepvpHelenaStand:
    case CharacterAction::BattlepvpLangeStand:
    case CharacterAction::BattlepvpLeemalnyunStand:
        return true;
    default:
        return false;
    }
}

bool is_stand_action(std::int32_t nAction)
{
    switch (static_cast<CharacterAction>(nAction))
    {
    case CharacterAction::Stand1:
    case CharacterAction::Stand2:
    case CharacterAction::Sit:
    case CharacterAction::GhostStand:
    case CharacterAction::Stand1Floating:
        return true;
    default:
        break;
    }
    return is_battle_pvp_stand_action(nAction);
}

bool IsAbleTamingMobOneTimeAction(
    CharacterAction nAction, std::int32_t nVehicleID)
{
    // Wild Hunter jaguar mounts
    if (is_wild_hunter_jaguar(nVehicleID))
    {
        switch (nAction)
        {
        case CharacterAction::PronestabJaguar:
        case CharacterAction::StormBreak:
        case CharacterAction::Doublejump:
        case CharacterAction::Knockback:
        case CharacterAction::CrossRoad:
        case CharacterAction::Wildbeast:
        case CharacterAction::Sonicboom:
        case CharacterAction::ClawCut:
        case CharacterAction::Howling:
        case CharacterAction::AssistantHuntingUnit:
        case CharacterAction::ExtendMagazine:
        case CharacterAction::Ride:
        case CharacterAction::Getoff:
        case CharacterAction::RampageAsOne:
        case CharacterAction::SilentRampage:
        case CharacterAction::HerbalismJaguar:
        case CharacterAction::MiningJaguar:
            return true;
        default:
            break;
        }
    }

    // Mechanic tank
    if (nVehicleID == kMechanicTankVehicle)
    {
        switch (nAction)
        {
        case CharacterAction::Swingt1:
        case CharacterAction::Swingt2:
        case CharacterAction::Ladder2:
        case CharacterAction::Rope2:
        case CharacterAction::Shot:
        case CharacterAction::RocketBoosterStart:
        case CharacterAction::RocketBoosterEnd:
        case CharacterAction::Siege1Start:
        case CharacterAction::Siege1:
        case CharacterAction::Siege1End:
        case CharacterAction::Siege2Start:
        case CharacterAction::Siege2:
        case CharacterAction::Siege2End:
        case CharacterAction::Siege2Laser:
        case CharacterAction::SiegeStart:
        case CharacterAction::Siege:
        case CharacterAction::SiegeEnd:
        case CharacterAction::HoveringStart:
        case CharacterAction::Hovering:
        case CharacterAction::HoveringEnd:
        case CharacterAction::HoveringDash:
        case CharacterAction::FlamethrowerStart:
        case CharacterAction::FlamethrowerEnd:
        case CharacterAction::Flamethrower2Start:
        case CharacterAction::Flamethrower2End:
        case CharacterAction::MechanicBooster:
        case CharacterAction::Msummon:
        case CharacterAction::Msummon2:
        case CharacterAction::Gatlingshot:
        case CharacterAction::Gatlingshot2:
        case CharacterAction::Drillrush:
        case CharacterAction::Earthslug:
        case CharacterAction::RocketPunch:
        case CharacterAction::Ride2:
        case CharacterAction::Getoff2:
        case CharacterAction::MechanicRush:
        case CharacterAction::TankMsummon:
        case CharacterAction::TankMsummon2:
        case CharacterAction::TankMrush:
        case CharacterAction::TankRboosterPre:
        case CharacterAction::TankRboosterAfter:
        case CharacterAction::DistortionField:
        case CharacterAction::DistortionFieldTank:
        case CharacterAction::DistortionFieldTankSiege:
        case CharacterAction::HerbalismMechanic:
        case CharacterAction::MiningMechanic:
        case CharacterAction::AdvancedGatling:
        case CharacterAction::HomingMissile:
        case CharacterAction::Focusfire:
        case CharacterAction::TankFocusfire:
        case CharacterAction::MultifirePre:
        case CharacterAction::Multifire:
        case CharacterAction::MultifireAfter:
        case CharacterAction::TankMultifire:
        case CharacterAction::TankGetoff2:
        case CharacterAction::TankRide2:
        case CharacterAction::TankJump:
        case CharacterAction::TankLadder:
        case CharacterAction::TankRope:
        case CharacterAction::TankHerbalismMechanic:
        case CharacterAction::TankMiningMechanic:
            return true;
        default:
            return false;
        }
    }

    // Ride3/Getoff3 vehicles
    if (nVehicleID == kResistanceRiding1 || nVehicleID == kResistanceRiding2)
    {
        return nAction == CharacterAction::Ride3
            || nAction == CharacterAction::Getoff3;
    }

    // Mountain riding
    if (nVehicleID == kMountainRideVehicle)
    {
        return nAction == CharacterAction::MountainridingFire
            || nAction == CharacterAction::MountainridingCut;
    }

    // Catapult food fight
    if (nVehicleID == kCatapultVehicle)
        return nAction == CharacterAction::CatapultFoodfight;

    // Georg attack
    if (nVehicleID == kGeorgVehicle)
        return nAction == CharacterAction::GeorgAttack;

    // Evan dragon master
    if (nVehicleID == kEvanDragonVehicle)
    {
        return nAction >= CharacterAction::EvanDragonMasterPrepare
            && nAction <= CharacterAction::EvanDragonMasterKeydowned;
    }

    return false;
}

std::int32_t get_change_pinkbean_action(std::int32_t nAction)
{
    // Remap standard actions to Pinkbean equivalents (981-1050 range)
    // TODO: implement full mapping table
    (void)nAction;
    return -1;
}

bool IsActionHold(std::int32_t /*nAction*/, std::int32_t /*nFrame*/)
{
    // TODO: implement action hold check for keydown skills
    return false;
}

bool is_back_action(std::int32_t nAction, std::int32_t /*nVehicleID*/)
{
    // Actions where the character walks/climbs — frame should freeze when
    // the avatar position hasn't changed (rope, ladder, walk, etc.)
    auto a = static_cast<CharacterAction>(nAction);
    switch (a)
    {
    case CharacterAction::Walk1:
    case CharacterAction::Walk2:
    case CharacterAction::Ladder:
    case CharacterAction::Ladder2:
    case CharacterAction::Rope:
    case CharacterAction::Rope2:
    case CharacterAction::Fly1:
    case CharacterAction::Fly2Move:
    case CharacterAction::GhostWalk:
    case CharacterAction::GhostLadder:
    case CharacterAction::GhostRope:
    case CharacterAction::GhostFly:
    case CharacterAction::Crawl:
    case CharacterAction::PinkbeanWalk:
    case CharacterAction::PinkbeanLadder:
    case CharacterAction::PinkbeanRope:
        return true;
    default:
        return false;
    }
}

bool is_pinkbean_job(std::int32_t nJob)
{
    return nJob == 13000 || nJob == 13100;
}

bool is_shoot_morph_action(std::int32_t nAction)
{
    return nAction == 18 || nAction == 19;
}

bool is_hatdance_action(std::int32_t nAction)
{
    // Dance1 = 875, Dance3 = 877, DanceStarplanet0-2 = 940-942
    return nAction == 875 || nAction == 877
        || (nAction >= 940 && nAction <= 942);
}

bool is_hide_body_action(std::int32_t nAction)
{
    // HideBody = 980, SpinoffGuitar = 1156-1159
    return nAction == 980
        || (static_cast<std::uint32_t>(nAction - 1156) <= 3u);
}

bool is_battle_pvp_not_pieced_action(std::int32_t nAction)
{
    if (static_cast<std::uint32_t>(nAction - 1051) > 0x64u)
        return false;

    // These specific BattlePvP actions ARE pieced (return false)
    switch (nAction)
    {
    case 1074: case 1075:
    case 1086: case 1087: case 1088:
    case 1097: case 1098: case 1099:
    case 1108: case 1109: case 1110: case 1111: case 1112:
    case 1121: case 1122: case 1123: case 1124:
    case 1133: case 1134: case 1135: case 1136: case 1137: case 1138:
    case 1147: case 1148: case 1149: case 1150: case 1151:
        return false;
    default:
        return true;
    }
}

bool is_not_pieced_action(std::int32_t nAction)
{
    // Ghost actions (132-139)
    if (static_cast<std::uint32_t>(nAction - 132) <= 7u)
        return true;

    // Making skill range (827-835)
    if (static_cast<std::uint32_t>(nAction - 827) <= 8u)
        return true;

    // Setitem3-4 (856-857)
    if (static_cast<std::uint32_t>(nAction - 856) <= 1u)
        return true;

    if (is_dance_action(static_cast<CharacterAction>(nAction)))
        return true;

    // Hide = 854
    if (nAction == 854)
        return true;

    if (is_hide_body_action(nAction))
        return true;

    if (is_battle_pvp_not_pieced_action(nAction))
        return true;

    return false;
}

bool is_long_coat(std::int32_t nItemID)
{
    return nItemID / 10000 == 105;
}

} // namespace ms
