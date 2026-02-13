#pragma once

#include "util/security/ZtlSecureTear.h"

#include <cstdint>

namespace ms
{

/**
 * @brief item.nAttribute bit flags for equipment items (GW_ItemSlotEquipBase)
 */
struct EquipAttr
{
    enum
    {
        Protected              = 0x0001,
        PreventSlip            = 0x0002,
        SupportWarm            = 0x0004,
        Binded                 = 0x0008,
        PossibleTrading        = 0x0010,
        NonCombatStatExpUp     = 0x0020, // inverted: 0 = applied
        Used                   = 0x0040,
        MakingSkillItem        = 0x0080,
        BarrierEffect          = 0x0100,
        LuckyDayEffect         = 0x0200,
        CubeExOpt1             = 0x0400,
        CubeExOpt2             = 0x0800,
        AppliedAccountShareTag = 0x1000,
        RUCBarrier             = 0x2000,
        ScrollBarrier          = 0x4000,
        ReturnEffect           = 0x8000,
    };
};

/**
 * @brief item.nSpecialAttribute bit flags for equipment items
 */
struct EquipSpecialAttr
{
    enum
    {
        ItemNotDestroy       = 0x01,
        AlwaysGradeUpgrade   = 0x02,
        AlwaysInchantSuccess = 0x04,
        ItemExtended         = 0x08,
        SellingOneMeso       = 0x10,
        MakingSkillMeister   = 0x20,
        MakingSkillMaster    = 0x40,
        Vestige              = 0x80,
    };
};

/**
 * @brief item.nItemState bit flags for equipment items
 */
struct EquipItemStateFlag
{
    enum
    {
        RefunableGachapon             = 0x008,
        RefunableEventGachapon        = 0x010,
        RedLabel                      = 0x020,
        BlackLabel                    = 0x040,
        InnocentRUC                   = 0x080,
        AmazingHyperUpgradeChecked    = 0x100,
        VestigeBinded                 = 0x200,
        VestigePossibleTrading        = 0x400,
        VestigeAppliedAccountShareTag = 0x800,
    };
};

/**
 * @brief Equipment base stats (scrolls, enhancements, etc.)
 *
 * Based on GW_ItemSlotEquipBase from the original MapleStory client.
 * Size: 310 bytes (0x136). All fields are ZtlSecureTear-protected.
 */
class GW_ItemSlotEquipBase
{
public:
    ZtlSecureTear<std::uint8_t> nRUC;              // Remaining upgrade count
    ZtlSecureTear<std::uint8_t> nCUC;              // Completed upgrade count
    ZtlSecureTear<std::int16_t> niSTR;
    ZtlSecureTear<std::int16_t> niDEX;
    ZtlSecureTear<std::int16_t> niINT;
    ZtlSecureTear<std::int16_t> niLUK;
    ZtlSecureTear<std::int16_t> niMaxHP;
    ZtlSecureTear<std::int16_t> niMaxMP;
    ZtlSecureTear<std::int16_t> niPAD;             // Physical attack
    ZtlSecureTear<std::int16_t> niMAD;             // Magic attack
    ZtlSecureTear<std::int16_t> niPDD;             // Physical defense
    ZtlSecureTear<std::int16_t> niMDD;             // Magic defense
    ZtlSecureTear<std::int16_t> niACC;             // Accuracy
    ZtlSecureTear<std::int16_t> niEVA;             // Evasion
    ZtlSecureTear<std::int16_t> niCraft;           // Craft
    ZtlSecureTear<std::int16_t> niSpeed;
    ZtlSecureTear<std::int16_t> niJump;
    ZtlSecureTear<std::int16_t> nAttribute;        // Item attribute flags (EquipAttr)
    ZtlSecureTear<std::uint8_t> nLevelUpType;      // Level-up type
    ZtlSecureTear<std::uint8_t> nLevel;            // Equipment level
    ZtlSecureTear<std::int64_t> nEXP64;            // Equipment EXP
    ZtlSecureTear<std::int32_t> nDurability;       // Current durability
    ZtlSecureTear<std::int32_t> nIUC;              // Hammers applied
    ZtlSecureTear<std::int16_t> niPVPDamage;       // PVP damage
    ZtlSecureTear<std::uint8_t> niReduceReq;       // Reduce level requirement
    ZtlSecureTear<std::int16_t> nSpecialAttribute;  // Special attribute flags (EquipSpecialAttr)
    ZtlSecureTear<std::int32_t> nDurabilityMax;    // Max durability
    ZtlSecureTear<std::uint8_t> niIncReq;          // Increase requirement
    ZtlSecureTear<std::uint8_t> nGrowthEnchant;    // Growth enchant ID
    ZtlSecureTear<std::uint8_t> nPSEnchant;        // PS enchant ID
    ZtlSecureTear<std::uint8_t> nBDR;              // Boss damage rate
    ZtlSecureTear<std::uint8_t> nIMDR;             // Ignore monster defense rate
    ZtlSecureTear<std::uint8_t> nDamR;             // Damage rate %
    ZtlSecureTear<std::uint8_t> nStatR;            // All stat rate %
    ZtlSecureTear<std::uint8_t> nCuttable;         // Cuttable count (scissors)
    ZtlSecureTear<std::int64_t> nExGradeOption;    // Extra grade option
    ZtlSecureTear<std::int32_t> nItemState;        // Item state (EquipItemStateFlag)
};

} // namespace ms
