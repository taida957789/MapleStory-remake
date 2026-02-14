#include "ItemHelper.h"

namespace ms
{
namespace helper
{

auto GetItemType(std::int32_t nItemID) -> std::int32_t
{
    return nItemID / 1000000;
}

auto IsEquipItemID(std::int32_t nItemID) -> bool
{
    return GetItemType(nItemID) == kEquip;
}

auto is_script_run_pet_life_item(std::int32_t nItemID) -> bool
{
    return nItemID / 100 == 56890;
}

auto is_extend_riding_skill_period_item(std::int32_t nItemID) -> bool
{
    return nItemID / 1000 == 5501;
}

auto is_equip_req_lev_liberation_item(std::int32_t nItemID) -> bool
{
    return nItemID / 1000 == 5502;
}

// Original @ 0x5e17b0 — classifies cash item IDs into sub-types
auto get_cashslot_item_type(std::int32_t nItemID) -> std::int32_t
{
    switch (nItemID / 10000)
    {
    case 500: return 8;
    case 501: return 9;
    case 502: return 10;
    case 503: return 11;
    case 504:
        return (nItemID % 10000 / 1000 == 4) ? 63 : 22;

    case 505:
    {
        auto r = nItemID % 5050000;
        if (r == 100) return 68;
        if (r == 1000 || r == 1001) return 49;
        return (nItemID % 10 != 0) ? 24 : 23;
    }

    case 506:
        switch (nItemID / 1000)
        {
        case 5060:
            if (nItemID % 10 == 0) return 25;
            if (nItemID % 10 == 1) return 26;
            return 27;
        case 5061: return 43;
        case 5062:
        {
            auto v = nItemID % 1000;
            if (v > 301)
            {
                if (v > 501)
                {
                    if (v == 503) return 94;
                    if (v == 800 || v == 801) return 85;
                    return 46;
                }
                if (v >= 500) return 81;
                switch (v)
                {
                case 400: case 403: case 405: return 74;
                case 401: return 75;
                case 402: return 76;
                default:  return 46;
                }
            }
            if (v == 301) return 77;
            switch (v)
            {
            case 9:          return 88;
            case 10:         return 89;
            case 90:         return 86;
            case 100: case 103: return 47;
            case 200:        return 65;
            case 201:        return 66;
            case 202:        return 67;
            default:         return 46;
            }
        }
        case 5063:
            return (nItemID % 1000 / 100 == 1) ? 64 : 51;
        case 5064:
            switch (nItemID % 1000 / 100)
            {
            case 1: return 57;
            case 2: return 60;
            case 3: return 61;
            case 4: return 82;
            default: return 50;
            }
        case 5065:
            return (nItemID % 5065000 == 100) ? 72 : 53;
        case 5068:
        {
            auto h = nItemID % 1000 / 100;
            if (h == 1) return 58;
            if (h == 2) return 62;
            return 52;
        }
        case 5069: return 90;
        default:   return 0;
        }

    case 507:
        switch (nItemID % 10000 / 1000)
        {
        case 1: return 12;
        case 2: return 13;
        case 6: return 14;
        case 7: return 39;
        case 8: return 15;
        default: return 0;
        }

    case 508: return 18;
    case 509: return 21;
    case 510: return 20;
    case 512: return 16;

    case 513:
    {
        auto r = nItemID % 5130000;
        if (r >= 3000 && r <= 3001) return 69;
        if (r == 4000) return 79;
        return 7;
    }

    case 514: return 4;

    case 515:
        switch (nItemID / 1000)
        {
        case 5150: case 5151: case 5154: return 1;
        case 5152:
            switch (nItemID / 100)
            {
            case 51520: case 51522: return 2;
            case 51521: return 32;
            default:    return 0;
            }
        case 5153: return 3;
        case 5155: return 59;
        case 5157: return 92;
        case 5158: return 93;
        default:   return 0;
        }

    case 516: return 6;
    case 517: return (nItemID % 10000 == 0) ? 17 : 0;
    case 518: return 5;
    case 519: return 28;

    case 520:
        return (nItemID % 10000 / 1000 == 4) ? 73 : 19;

    case 523:
        return (nItemID % 5230000 == 3) ? 30 : 29;

    case 524: return 31;

    case 525:
        if (nItemID % 5250000 == 500) return 71;
        return (nItemID % 5251000 == 100) ? 36 : 35;

    case 528: return (nItemID / 1000 == 5281) ? 84 : 0;
    case 533: return 33;
    case 537: return 34;
    case 539: return 87;
    case 545: return 37;
    case 547: return 38;

    case 550:
        if (is_extend_riding_skill_period_item(nItemID)) return 54;
        return is_equip_req_lev_liberation_item(nItemID) ? 70 : 40;

    case 551: return 41;
    case 552: return (nItemID % 10000 == 1000) ? 56 : 42;
    case 553: return 44;
    case 562: return 45;

    case 568:
        return is_script_run_pet_life_item(nItemID) ? 5 : 0;

    case 570: return 55;

    case 578:
        return (nItemID / 1000 == 5781) ? 80 : 78;

    case 580: return 83;
    case 583: return 91;

    default: return 0;
    }
}

// Original @ 0x5e20a0 — filters get_cashslot_item_type for Etc inventory
auto get_etc_cash_item_type(std::int32_t nItemID) -> std::int32_t
{
    auto n = get_cashslot_item_type(nItemID);
    switch (n)
    {
    case 1: case 2: case 3: case 4: case 5: case 6: case 7:
    case 35: case 36: case 41: case 69: case 71: case 73:
        return n;
    default:
        return 0;
    }
}

// Original @ 0x5e2010 — filters get_cashslot_item_type for Consume inventory
auto get_consume_cash_item_type(std::int32_t nItemID) -> std::int32_t
{
    auto n = get_cashslot_item_type(nItemID);
    switch (n)
    {
    case 12: case 13: case 14: case 15: case 16: case 17: case 18: case 19:
    case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27:
    case 28: case 29: case 30: case 31: case 32: case 33: case 34:
    case 37: case 39: case 40: case 42: case 43: case 44: case 45: case 46:
    case 47: case 49: case 50: case 51: case 52: case 53: case 54: case 55:
    case 56: case 57: case 58: case 59: case 60: case 61: case 62: case 63:
    case 64: case 65: case 66: case 67: case 68: case 70: case 72:
    case 74: case 75: case 76: case 77: case 79: case 80: case 81: case 82:
    case 83: case 84: case 85: case 86: case 87: case 88: case 89: case 90:
    case 91: case 92: case 93: case 94:
        return n;
    default:
        return 0;
    }
}

// Original @ 0x787290 — filters get_cashslot_item_type for Bundle inventory
auto get_bundle_cash_item_type(std::int32_t nItemID) -> std::int32_t
{
    auto n = get_cashslot_item_type(nItemID);
    if ((n >= 8 && n <= 11) || n == 38)
        return n;
    return 0;
}

} // namespace helper
} // namespace ms
