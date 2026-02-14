#include "EquipDataPath.h"

#include "WeaponConstants.h"

#include <cstdio>

namespace ms
{

namespace
{

/// Format item ID as 8-digit zero-padded string
auto format_item_id(std::int32_t nItemID) -> std::string
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%08d", nItemID);
    return buf;
}

/// Check if item is a gathering tool (herbalism/mining pickaxe)
auto is_gather_tool_item(std::int32_t nItemID) -> bool
{
    auto prefix = nItemID / 10000;
    return prefix == 150 || prefix == 151;
}

} // anonymous namespace

std::string get_equip_data_path(std::int32_t nItemID)
{
    auto prefix = nItemID / 10000;
    auto sID = format_item_id(nItemID);

    switch (prefix)
    {
    case 0:
    case 1:
        return "Character/" + sID + ".img";

    case 2:
        return "Character/Face/" + sID + ".img";

    case 3:
    case 4:
        return "Character/Hair/" + sID + ".img";

    case 100:
        return "Character/Cap/" + sID + ".img";

    case 101:
    case 102:
    case 103:
    case 112:
    case 113:
    case 114:
    case 115:
    case 116:
    case 118:
    case 119:
        return "Character/Accessory/" + sID + ".img";

    case 104:
        return "Character/Coat/" + sID + ".img";

    case 105:
        return "Character/Longcoat/" + sID + ".img";

    case 106:
    case 107:
        return "Character/Pants/" + sID + ".img";

    case 108:
        return "Character/Glove/" + sID + ".img";

    case 109:
        return "Character/Shield/" + sID + ".img";

    case 110:
        return "Character/Cape/" + sID + ".img";

    case 111:
        return "Character/Ring/" + sID + ".img";

    case 161:
    case 162:
    case 163:
    case 164:
    case 165:
        return "Character/Mechanic/" + sID + ".img";

    case 166:
    case 167:
        return "Character/Android/" + sID + ".img";

    case 168:
        return "Character/Bits/" + sID + ".img";

    case 180:
        return "Character/PetEquip/" + sID + ".img";

    case 184:
    case 185:
    case 186:
    case 187:
    case 188:
    case 189:
        return "Character/MonsterBattle/" + sID + ".img";

    case 190:
    case 191:
    case 193:
    case 198:
        return "Character/TamingMob/" + sID + ".img";

    case 194:
    case 195:
    case 196:
    case 197:
        return "Character/Dragon/" + sID + ".img";

    default:
        break;
    }

    // Check if weapon or weapon-like item
    if (get_weapon_type(nItemID) != 0
        || nItemID / 100000 == 16
        || nItemID / 100000 == 17
        || is_gather_tool_item(nItemID))
    {
        return "Character/Weapon/" + sID + ".img";
    }

    return {};
}

bool is_accessory(std::int32_t nItemID)
{
    auto prefix = nItemID / 10000;
    switch (prefix)
    {
    case 101:
    case 102:
    case 103:
    case 112:
    case 113:
    case 114:
    case 115:
    case 116:
    case 118:
    case 119:
        return true;
    default:
        return false;
    }
}

bool is_davenger_job(std::int32_t nJob)
{
    return nJob / 100 == 31;
}

void action_mapping_for_battle_pvp(std::int32_t& nAction)
{
    // BattlePvP action codes are in blocks of 13 per character.
    // Each character block maps to standard actions.
    // Blocks: 1051-1063 (Manji), 1064-1075 (Mike), 1076-1088 (Darklord),
    //         1089-1100 (Heinz), 1101-1112 (Mugong), 1113-1124 (Helena),
    //         1125-1138 (Lange), 1139-1151 (Leemalnyun)

    if (nAction < 1051 || nAction > 1151)
        return;

    // The first action in each block is walk, which maps to Walk1 (0)
    // Map based on offset within the block
    auto blockOffset = (nAction - 1051) % 13;

    switch (blockOffset)
    {
    case 0: nAction = 0; break;  // walk -> Walk1
    case 1: nAction = 2; break;  // stand -> Stand1
    case 2: nAction = 31; break; // rope -> Rope
    case 3: nAction = 25; break; // prone -> Prone
    case 4: nAction = 24; break; // proneStab -> ProneStab
    case 5: nAction = 28; break; // jump -> Jump
    case 6: nAction = 32; break; // die -> Dead
    default:
        // Attack actions — map to Stand1 as fallback
        nAction = 2;
        break;
    }
}

} // namespace ms
