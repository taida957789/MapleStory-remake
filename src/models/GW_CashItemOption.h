#pragma once

#include "util/FileTime.h"

#include <array>
#include <cstdint>

namespace ms
{

class InPacket;
class OutPacket;

/**
 * @brief Cash item option data
 *
 * Based on GW_CashItemOption (__cppobj : ZRefCounted) from the original
 * MapleStory client. Size: 44 bytes (excluding ZRefCounted base).
 */
class GW_CashItemOption
{
public:
    virtual ~GW_CashItemOption() = default;

    [[nodiscard]] auto HasOption() const -> bool
    {
        return anOption[0] != 0 || anOption[1] != 0 || anOption[2] != 0;
    }

    void Clear()
    {
        liCashItemSN = 0;
        anOption = {};
        ftExpireDate = kDbDate19000101;
        nGrade = 0;
    }

    void CopyTo(GW_CashItemOption& target) const
    {
        target.liCashItemSN = liCashItemSN;
        target.ftExpireDate = ftExpireDate;
        target.nGrade = nGrade;
        target.anOption = anOption;
    }

    [[nodiscard]] auto IsExpired(FileTime ftNow) const -> bool
    {
        return ftExpireDate < kDbDate20790101 && ftExpireDate > ftNow;
    }

    // === Static helpers ===
    [[nodiscard]] static auto GetCashItemOptionType(std::int32_t nOption) -> std::int32_t
    {
        return nOption / 1000;
    }

    [[nodiscard]] static auto GetCashItemOptionValue(std::int32_t nOption) -> std::int32_t
    {
        return nOption % 1000;
    }

    [[nodiscard]] static auto GetCashItemOptionGroup(std::int32_t nOption) -> std::int32_t;

    // === Serialization ===
    static void Decode(InPacket& iPacket, GW_CashItemOption& option);
    void Encode(OutPacket& oPacket) const;

    // === Data members ===
    std::int64_t liCashItemSN{};
    FileTime ftExpireDate{};
    std::int32_t nGrade{};
    std::array<std::int32_t, 3> anOption{};
};

} // namespace ms
