#pragma once

#include "util/security/ZtlSecureTear.h"

#include <cstdint>

namespace ms
{

/**
 * @brief option.nGrade bit flags for equipment items
 */
struct EquipGradeFlag
{
    enum
    {
        GradeMask             = 0x0F,
        Released              = 0x10,
        AdditionalNotReleased = 0x20,
    };
};

/**
 * @brief Equipment potential / soul options
 *
 * Based on GW_ItemSlotEquipOpt from the original MapleStory client.
 * Size: 92 bytes (0x5C). All fields are ZtlSecureTear-protected.
 */
class GW_ItemSlotEquipOpt
{
public:
    ZtlSecureTear<std::uint8_t> nGrade;            // Potential rank + flags (EquipGradeFlag)
    ZtlSecureTear<std::uint8_t> nCHUC;             // Star force enhancement count
    ZtlSecureTear<std::uint16_t> nOption1;          // Potential line 1
    ZtlSecureTear<std::uint16_t> nOption2;          // Potential line 2
    ZtlSecureTear<std::uint16_t> nOption3;          // Potential line 3
    ZtlSecureTear<std::uint16_t> nOption4;          // Bonus potential line 1
    ZtlSecureTear<std::uint16_t> nOption5;          // Look change item ID (low bits)
    ZtlSecureTear<std::uint16_t> nOption6;          // Bonus potential line 2
    ZtlSecureTear<std::uint16_t> nOption7;          // Bonus potential line 3
    ZtlSecureTear<std::int16_t> nSoulOptionID;      // Soul weapon option ID
    ZtlSecureTear<std::int16_t> nSoulSocketID;      // Soul socket ID
    ZtlSecureTear<std::uint16_t> nSoulOption;       // Soul option value
};

} // namespace ms
