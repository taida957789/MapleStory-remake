#include "ActionHelpers.h"

namespace ms
{

bool is_dance_action(CharacterAction nAction)
{
    auto n = static_cast<std::int32_t>(nAction);

    // Dance0(874)..Dance8(882)
    if (n >= 874 && n <= 882)
        return true;

    // DanceStarplanet0(940)..DanceStarplanet5(945)
    if (n >= 940 && n <= 945)
        return true;

    // DanceStarplanetEvent0(946)..DanceStarplanetEvent5(951)
    if (n >= 946 && n <= 951)
        return true;

    // SpinoffGuitarStandM(1156)..SpinoffGuitarStrokeW(1159)
    if (n >= 1156 && n <= 1159)
        return true;

    return false;
}

bool is_weapon_hide_action(CharacterAction nAction)
{
    auto n = static_cast<std::int32_t>(nAction);

    // Supercannon(313) — check first since the decompiled code does
    if (n > static_cast<std::int32_t>(CharacterAction::Supercannon))
        return false;

    if (nAction == CharacterAction::Supercannon)
        return true;

    // Specific actions
    switch (nAction)
    {
    case CharacterAction::Sanctuary:   // 95
    case CharacterAction::Showdown:    // 104
    case CharacterAction::AirStrike:   // 120
    case CharacterAction::Blade:       // 146
        return true;
    default:
        break;
    }

    // Dance actions
    if (is_dance_action(nAction))
        return true;

    // HideBody(980)
    if (nAction == CharacterAction::HideBody)
        return true;

    // SpinoffGuitarStandM(1156)..SpinoffGuitarStrokeW(1159)
    if (n >= 1156 && n <= 1159)
        return true;

    // BattlepvpManjiWalk(1051)..BattlepvpManjiPronestab(1055)
    if (n >= 1051 && n <= 1055)
        return true;

    return false;
}

void action_mapping_for_ghost(std::int32_t& nAction)
{
    switch (static_cast<CharacterAction>(nAction))
    {
    case CharacterAction::GhostWalk:      // 132
        nAction = static_cast<std::int32_t>(CharacterAction::Walk1);
        break;
    case CharacterAction::GhostStand:     // 133
        nAction = static_cast<std::int32_t>(CharacterAction::Stand1);
        break;
    case CharacterAction::GhostJump:      // 134
        nAction = static_cast<std::int32_t>(CharacterAction::Jump);
        break;
    case CharacterAction::GhostPronestab: // 135
        nAction = static_cast<std::int32_t>(CharacterAction::Pronestab);
        break;
    case CharacterAction::GhostFly:       // 136
        nAction = static_cast<std::int32_t>(CharacterAction::Fly1);
        break;
    case CharacterAction::GhostLadder:    // 137
        nAction = static_cast<std::int32_t>(CharacterAction::Ladder);
        break;
    case CharacterAction::GhostRope:      // 138
        nAction = static_cast<std::int32_t>(CharacterAction::Rope);
        break;
    case CharacterAction::GhostSit:       // 139
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
    // BattlePvP dead actions: PVPA1_die(1057), PVPA2_die(1070), etc.
    // Each PvP avatar has a die action at specific offsets
    switch (nAction)
    {
    case 1057: // PVPA1_die
    case 1070: // PVPA2_die
    case 1082: // PVPA3_die
    case 1095: // PVPA4_die
    case 1106: // PVPA5_die
    case 1119: // PVPA6_die
    case 1131: // PVPA7_die
    case 1145: // PVPA8_die
        return true;
    default:
        return false;
    }
}

bool is_battle_pvp_basic_attack_action(std::int32_t nAction)
{
    // BattlePvP basic attack actions
    switch (nAction)
    {
    case 1058: // PVPA1_attack1
    case 1059: // PVPA1_attack2
    case 1071: // PVPA2_attack1
    case 1072: // PVPA2_attack2
    case 1084: // PVPA3_attack1
    case 1085: // PVPA3_attack2
    case 1097: // PVPA4_attack
    case 1108: // PVPA5_attack
    case 1121: // PVPA6_attack
    case 1133: // PVPA7_attack
    case 1147: // PVPA8_attack
        return true;
    default:
        return false;
    }
}

bool IsAbleTamingMobOneTimeAction(
    CharacterAction /*nAction*/, std::int32_t /*nVehicleID*/)
{
    // TODO: implement taming mob one-time action check
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

} // namespace ms
