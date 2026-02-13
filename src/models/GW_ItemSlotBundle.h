#pragma once

#include "GW_ItemSlotBase.h"
#include "util/security/ZtlSecureTear.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace ms
{

/**
 * @brief nAttribute bit flags for bundle (stackable) items
 */
struct BundleAttr
{
    enum
    {
        Protected              = 0x01,
        PossibleTrading        = 0x02,
        Binded                 = 0x04, // category 265/308/433/223 only
        Used                   = 0x08,
        MakingSkillItem        = 0x10,
        AppliedAccountShareTag = 0x20,
        NonCombatStatExpUp     = 0x40, // inverted: 0 = applied
        BuyMaplePoint          = 0x80,
    };
};

/**
 * @brief Stackable item slot data (use/consume, setup, etc)
 *
 * Based on GW_ItemSlotBundle (__cppobj : GW_ItemSlotBase) from the original
 * MapleStory client. Size: 81 bytes (0x51).
 *
 * Layout:
 *   +0x000  GW_ItemSlotBase (base)
 *   +0x02C  nNumber, nAttribute
 *   +0x03C  liSN, sTitle
 */
class GW_ItemSlotBundle : public GW_ItemSlotBase
{
public:
    // === GW_ItemSlotBase overrides ===
    [[nodiscard]] auto GetType() -> std::int32_t override { return GW_ItemSlotBundle_TYPE; }
    [[nodiscard]] auto GetSN() -> std::int64_t override { return liSN; }
    [[nodiscard]] auto GetDataSize() -> std::int32_t override { return 69; }
    [[nodiscard]] auto GetItemNumber() -> std::int32_t override { return nNumber.Get(); }

    void SetItemNumber(std::int16_t n) override
    {
        nNumber.Put(static_cast<std::uint16_t>(n));
    }

    [[nodiscard]] auto GetItemAttribute() -> std::int16_t override
    {
        return nAttribute.Get();
    }

    void SetItemAttribute(std::int16_t nAttr) override
    {
        nAttribute.Put(nAttr);
    }

    [[nodiscard]] auto GetItemTitle() -> std::string override
    {
        return std::string(sTitle.data());
    }

    void SetItemTitle(const std::string& s) override
    {
        auto len = std::min(s.size(), sTitle.size() - 1);
        std::memcpy(sTitle.data(), s.data(), len);
        sTitle[len] = '\0';
    }

    // --- Flag queries ---
    [[nodiscard]] auto IsUsedItem() -> std::int32_t override
    {
        return (nAttribute.Get() & BundleAttr::Used) >> 3;
    }

    [[nodiscard]] auto IsProtectedItem() -> std::int32_t override
    {
        return nAttribute.Get() & BundleAttr::Protected;
    }

    [[nodiscard]] auto IsPossibleTradingItem() -> std::int32_t override
    {
        // TODO: check CSpecialServerMan singleton
        return (nAttribute.Get() & BundleAttr::PossibleTrading) >> 1;
    }

    [[nodiscard]] auto IsMakingSkillItem() -> std::int32_t override
    {
        return (nAttribute.Get() & BundleAttr::MakingSkillItem) >> 4;
    }

    [[nodiscard]] auto IsAppliedAccountShareTag() -> std::int32_t override
    {
        return (nAttribute.Get() & BundleAttr::AppliedAccountShareTag) >> 5;
    }

    [[nodiscard]] auto IsNonCombatStatExpUpItem() -> std::int32_t override
    {
        return (nAttribute.Get() & BundleAttr::NonCombatStatExpUp) == 0;
    }

    [[nodiscard]] auto IsBindedItem() -> std::int32_t override
    {
        auto nCategory = static_cast<std::int32_t>(nItemID) / 10000;
        return (nCategory == 265 || nCategory == 308 || nCategory == 433 || nCategory == 223)
            && (nAttribute.Get() & BundleAttr::Binded) != 0;
    }

    // --- Flag set/reset ---
    void SetUsed() override { nAttribute.Put(nAttribute.Get() | BundleAttr::Used); }
    void ResetUsed() override { nAttribute.Put(nAttribute.Get() & ~BundleAttr::Used); }
    void SetProtected() override { nAttribute.Put(nAttribute.Get() | BundleAttr::Protected); }
    void ResetProtected() override { nAttribute.Put(nAttribute.Get() & ~BundleAttr::Protected); }
    void SetPossibleTrading() override { nAttribute.Put(nAttribute.Get() | BundleAttr::PossibleTrading); }
    void ResetPossibleTrading() override { nAttribute.Put(nAttribute.Get() & ~BundleAttr::PossibleTrading); }
    void SetMakingSkillItem() override { nAttribute.Put(nAttribute.Get() | BundleAttr::MakingSkillItem); }
    void ResetMakingSkillItem() override { nAttribute.Put(nAttribute.Get() & ~BundleAttr::MakingSkillItem); }
    void SetAppliedAccountShareTag() override { nAttribute.Put(nAttribute.Get() | BundleAttr::AppliedAccountShareTag); }
    void ResetAppliedAccountShareTag() override { nAttribute.Put(nAttribute.Get() & ~BundleAttr::AppliedAccountShareTag); }
    void SetNonCombatStatExpUpItem() override { nAttribute.Put(nAttribute.Get() | BundleAttr::NonCombatStatExpUp); }
    void ResetNonCombatStatExpUpItem() override { nAttribute.Put(nAttribute.Get() & ~BundleAttr::NonCombatStatExpUp); }

    void SetBinded() override
    {
        auto nCategory = static_cast<std::int32_t>(nItemID) / 10000;
        if (nCategory == 265 || nCategory == 308 || nCategory == 433 || nCategory == 223)
            nAttribute.Put(nAttribute.Get() | BundleAttr::Binded);
    }

    // --- Set item ---
    [[nodiscard]] auto IsSetItem() -> std::int32_t override { return GetSetItemID() != 0; }
    [[nodiscard]] auto GetSetItemID() -> std::int32_t override { return 0; }

    void SetBuyMaplePoint() override { nAttribute.Put(nAttribute.Get() | BundleAttr::BuyMaplePoint); }

    // --- Bundle-specific virtuals (not in base) ---
    virtual void ResetBuyMaplePoint() { nAttribute.Put(nAttribute.Get() & ~BundleAttr::BuyMaplePoint); }
    [[nodiscard]] virtual auto IsBuyMaplePoint() -> bool
    {
        return (nAttribute.Get() & BundleAttr::BuyMaplePoint) >> 7;
    }

    // === Data members ===
    ZtlSecureTear<std::uint16_t> nNumber;             // Stack count / quantity
    ZtlSecureTear<std::int16_t> nAttribute;            // Item attribute flags
    std::int64_t liSN{};                               // Serial number
    std::array<char, 13> sTitle{};                     // Item title string
};

} // namespace ms
