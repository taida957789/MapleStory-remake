#pragma once

#include "GW_ItemSlotBase.h"
#include "util/security/ZtlSecureTear.h"

#include <array>
#include <cstdint>

namespace ms
{

/**
 * @brief nAttribute bit flags for pet items
 */
struct PetAttr
{
    enum
    {
        PossibleTrading        = 0x01,
        NotPossiblePickUp      = 0x02, // inverted: 0 = possible
        NotPossibleSetEvolution = 0x04, // inverted: 0 = possible
    };
};

/**
 * @brief Pet item slot data
 *
 * Based on GW_ItemSlotPet (__cppobj : GW_ItemSlotBase) from the original
 * MapleStory client. Size: 159 bytes (0x9F).
 *
 * Layout:
 *   +0x000  GW_ItemSlotBase (base)
 *   +0x02C  sPetName
 *   +0x039  nLevel, nTameness, nRepleteness, nPetAttribute, usPetSkill
 *   +0x05D  dateDead, nRemainLife, nAttribute, nActiveState
 *   +0x07F  nAutoBuffSkill, nPetHue, nGiantRate
 */
class GW_ItemSlotPet : public GW_ItemSlotBase
{
public:
    // === GW_ItemSlotBase overrides ===
    [[nodiscard]] auto GetType() -> std::int32_t override { return GW_ItemSlotPet_TYPE; }
    [[nodiscard]] auto GetSN() -> std::int64_t override { return 0; }
    [[nodiscard]] auto GetDataSize() -> std::int32_t override { return 147; }
    [[nodiscard]] auto GetItemNumber() -> std::int32_t override { return 1; }

    [[nodiscard]] auto GetItemTitle() -> std::string override { return {}; }
    void SetItemTitle(const std::string& /*s*/) override {}

    [[nodiscard]] auto GetItemAttribute() -> std::int16_t override
    {
        return nAttribute.Get();
    }

    void SetItemAttribute(std::int16_t nAttr) override
    {
        nAttribute.Put(nAttr);
    }

    [[nodiscard]] auto GetActiveState() -> std::uint8_t override
    {
        return nActiveState.Get();
    }

    void SetActiveState(std::uint8_t nState) override
    {
        nActiveState.Put(nState);
    }

    [[nodiscard]] auto IsPossibleTradingItem() -> std::int32_t override
    {
        // TODO: check CSpecialServerMan singleton
        return nAttribute.Get() & PetAttr::PossibleTrading;
    }

    void SetPossibleTrading() override
    {
        nAttribute.Put(nAttribute.Get() | PetAttr::PossibleTrading);
    }

    void ResetPossibleTrading() override
    {
        nAttribute.Put(nAttribute.Get() & ~PetAttr::PossibleTrading);
    }

    [[nodiscard]] auto IsPossibleSetEvolutionData() -> std::int32_t override
    {
        return (nAttribute.Get() & PetAttr::NotPossibleSetEvolution) == 0;
    }

    void BackwardUpdateCashItem(GW_ItemSlotBase* pOther) override;
    void RawDecode(InPacket& iPacket) override;
    void RawEncode(OutPacket& oPacket, bool bForInternal) override;

    [[nodiscard]] auto IsSetItem() -> std::int32_t override;
    [[nodiscard]] auto GetSetItemID() -> std::int32_t override;

    // === Pet-specific virtuals (not in base) ===
    [[nodiscard]] virtual auto IsPossiblePickUp() -> std::int32_t
    {
        return (nAttribute.Get() & PetAttr::NotPossiblePickUp) == 0;
    }

    virtual void SetPossiblePickUp()
    {
        nAttribute.Put(nAttribute.Get() | PetAttr::NotPossiblePickUp);
    }

    virtual void ResetPossiblePickUp()
    {
        nAttribute.Put(nAttribute.Get() & ~PetAttr::NotPossiblePickUp);
    }

    virtual void SetPossibleSetEvolutionData()
    {
        nAttribute.Put(nAttribute.Get() | PetAttr::NotPossibleSetEvolution);
    }

    virtual void ResetPossibleSetEvolutionData()
    {
        nAttribute.Put(nAttribute.Get() & ~PetAttr::NotPossibleSetEvolution);
    }

    [[nodiscard]] virtual auto IsAllowedOverlappedSet() -> bool;

    // === Non-virtual methods ===
    [[nodiscard]] auto IsDead() const -> bool;
    [[nodiscard]] auto IsDeadByDate() const -> bool;
    [[nodiscard]] auto IsReinforced() const -> bool { return nRepleteness.Get() > 100; }
    [[nodiscard]] auto IsPetSkillExist(std::uint16_t usSkillID) const -> bool
    {
        return (usPetSkill.Get() & usSkillID) != 0;
    }
    [[nodiscard]] auto GetPetHue() const -> std::int32_t { return nPetHue.Get(); }
    [[nodiscard]] auto GetAutoBuffSkill() const -> std::int32_t { return nAutoBuffSkill.Get(); }

    // === Data members ===
    std::array<char, 13> sPetName{};                   // Pet name
    ZtlSecureTear<std::uint8_t> nLevel;                // Pet level
    ZtlSecureTear<std::int16_t> nTameness;             // Closeness / tameness
    ZtlSecureTear<std::uint8_t> nRepleteness;          // Fullness
    ZtlSecureTear<std::int16_t> nPetAttribute;         // Pet attribute flags
    ZtlSecureTear<std::uint16_t> usPetSkill;           // Pet skill flags
    FileTime dateDead = kDbDate19000101;               // Death / expiry date
    ZtlSecureTear<std::int32_t> nRemainLife;           // Remaining life time
    ZtlSecureTear<std::int16_t> nAttribute;            // Item attribute flags
    ZtlSecureTear<std::uint8_t> nActiveState;          // Active state
    ZtlSecureTear<std::int32_t> nAutoBuffSkill;        // Auto-buff skill ID
    ZtlSecureTear<std::int32_t> nPetHue;               // Pet color / hue
    ZtlSecureTear<std::int16_t> nGiantRate{100};       // Giant pet rate
};

} // namespace ms
