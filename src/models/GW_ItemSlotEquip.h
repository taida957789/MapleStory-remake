#pragma once

#include "GW_CashItemOption.h"
#include "GW_ItemSlotBase.h"
#include "GW_ItemSlotEquipBase.h"
#include "GW_ItemSlotEquipOpt.h"

#include <array>
#include <cstdint>
#include <string>

namespace ms
{

/**
 * @brief Equipment item slot data
 *
 * Based on GW_ItemSlotEquip (__cppobj : GW_ItemSlotBase) from the original
 * MapleStory client. Size: 523 bytes (0x20B).
 *
 * Layout:
 *   +0x000  GW_ItemSlotBase (base)
 *   +0x02C  liSN, sTitle, ftEquipped, nPrevBonusExpRate
 *   +0x04D  GW_ItemSlotEquipBase (equipment stats)
 *   +0x183  GW_CashItemOption (cash item options)
 *   +0x1AF  GW_ItemSlotEquipOpt (potential/soul options)
 */
class GW_ItemSlotEquip : public GW_ItemSlotBase
{
public:
    // === GW_ItemSlotBase overrides — Identity ===
    [[nodiscard]] auto GetType() -> std::int32_t override { return GW_ItemSlotEquip_TYPE; }
    [[nodiscard]] auto GetSN() -> std::int64_t override { return liSN; }
    [[nodiscard]] auto GetDataSize() -> std::int32_t override { return 511; }
    [[nodiscard]] auto GetItemNumber() -> std::int32_t override { return 1; }
    void SetItemNumber(std::int16_t /*nNumber*/) override {}
    void SetActiveState(std::uint8_t /*nState*/) override {}
    [[nodiscard]] auto GetActiveState() -> std::uint8_t override { return 0; }

    // === Level / EXP ===
    [[nodiscard]] auto GetLevelUpType() -> std::int32_t override { return item.nLevelUpType.Get(); }
    [[nodiscard]] auto GetLevel() -> std::int32_t override { return item.nLevel.Get(); }
    [[nodiscard]] auto GetEXP() -> std::int64_t override { return item.nEXP64.Get(); }
    void SetLevel(std::uint8_t nLevel) override { item.nLevel.Put(nLevel); }
    void SetEXP(std::int64_t nEXP) override { item.nEXP64.Put(nEXP); }

    // === Attribute ===
    [[nodiscard]] auto GetItemAttribute() -> std::int16_t override { return item.nAttribute.Get(); }
    void SetItemAttribute(std::int16_t nAttr) override { item.nAttribute.Put(nAttr); }

    // === Title ===
    [[nodiscard]] auto GetItemTitle() -> std::string override
    {
        return std::string(sTitle.data());
    }

    void SetItemTitle(const std::string& s) override;

    // === Grade / Look ===
    [[nodiscard]] auto GetItemGrade() -> std::uint8_t override
    {
        return option.nGrade.Get() & EquipGradeFlag::GradeMask;
    }

    [[nodiscard]] auto GetLookItemID() -> std::int32_t override;

    [[nodiscard]] auto IsLookChangeItem() -> std::int32_t override
    {
        return option.nOption5.Get() != 0;
    }

    [[nodiscard]] auto IsAdditionalOPT() -> std::int32_t override
    {
        return option.nOption4.Get() != 0;
    }

    [[nodiscard]] auto GetAdditionalGrade() -> std::int32_t override;

    // === Growth / PS enchant ===
    [[nodiscard]] auto GetGrowthEnchantID() -> std::int32_t override;
    void SetGrowthEnchantID(std::int32_t nGrowthEnchantID, std::int32_t nLevelUpType) override;
    [[nodiscard]] auto GetPSEnchantID() -> std::int32_t override;

    void SetPSEnchantID(std::int32_t nPSEnchantID) override
    {
        item.nPSEnchant.Put(nPSEnchantID != 0
            ? static_cast<std::uint8_t>(nPSEnchantID - 87)
            : static_cast<std::uint8_t>(0));
    }

    // === nAttribute flag queries ===
    [[nodiscard]] auto IsProtectedItem() -> std::int32_t override
    {
        return item.nAttribute.Get() & EquipAttr::Protected;
    }

    [[nodiscard]] auto IsPreventSlipItem() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::PreventSlip) >> 1;
    }

    [[nodiscard]] auto IsSupportWarmItem() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::SupportWarm) >> 2;
    }

    [[nodiscard]] auto IsBindedItem() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::Binded) >> 3;
    }

    [[nodiscard]] auto IsPossibleTradingItem() -> std::int32_t override
    {
        // TODO: check CSpecialServerMan singleton
        return (item.nAttribute.Get() & EquipAttr::PossibleTrading) >> 4;
    }

    [[nodiscard]] auto IsNonCombatStatExpUpItem() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::NonCombatStatExpUp) == 0;
    }

    [[nodiscard]] auto IsUsedItem() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::Used) >> 6;
    }

    [[nodiscard]] auto IsMakingSkillItem() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::MakingSkillItem) >> 7;
    }

    [[nodiscard]] auto IsBarrierEffectApplied() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::BarrierEffect) >> 8;
    }

    [[nodiscard]] auto IsLuckyDayEffectApplied() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::LuckyDayEffect) >> 9;
    }

    [[nodiscard]] auto IsAppliedAccountShareTag() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::AppliedAccountShareTag) >> 12;
    }

    [[nodiscard]] auto IsRUCBarrierApplied() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::RUCBarrier) >> 13;
    }

    [[nodiscard]] auto IsScrollBarrierApplied() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::ScrollBarrier) >> 14;
    }

    [[nodiscard]] auto IsReturnEffectApplied() -> std::int32_t override
    {
        return (item.nAttribute.Get() & EquipAttr::ReturnEffect) >> 15;
    }

    // === nAttribute flag set/reset ===
    void SetProtected() override { item.nAttribute.Put(item.nAttribute.Get() | EquipAttr::Protected); }
    void ResetProtected() override { item.nAttribute.Put(item.nAttribute.Get() & ~EquipAttr::Protected); }
    void SetPreventSlip() override { item.nAttribute.Put(item.nAttribute.Get() | EquipAttr::PreventSlip); }
    void ResetPreventSlip() override { item.nAttribute.Put(item.nAttribute.Get() & ~EquipAttr::PreventSlip); }
    void SetWarmSupport() override { item.nAttribute.Put(item.nAttribute.Get() | EquipAttr::SupportWarm); }
    void ResetWarmSupport() override { item.nAttribute.Put(item.nAttribute.Get() & ~EquipAttr::SupportWarm); }
    void SetBinded() override { item.nAttribute.Put(item.nAttribute.Get() | EquipAttr::Binded); }
    void ResetBinded() override { item.nAttribute.Put(item.nAttribute.Get() & ~EquipAttr::Binded); }
    void SetPossibleTrading() override { item.nAttribute.Put(item.nAttribute.Get() | EquipAttr::PossibleTrading); }
    void ResetPossibleTrading() override { item.nAttribute.Put(item.nAttribute.Get() & ~EquipAttr::PossibleTrading); }
    void SetNonCombatStatExpUpItem() override { item.nAttribute.Put(item.nAttribute.Get() | EquipAttr::NonCombatStatExpUp); }
    void ResetNonCombatStatExpUpItem() override { item.nAttribute.Put(item.nAttribute.Get() & ~EquipAttr::NonCombatStatExpUp); }
    void SetUsed() override { item.nAttribute.Put(item.nAttribute.Get() | EquipAttr::Used); }
    void ResetUsed() override { item.nAttribute.Put(item.nAttribute.Get() & ~EquipAttr::Used); }
    void SetMakingSkillItem() override { item.nAttribute.Put(item.nAttribute.Get() | EquipAttr::MakingSkillItem); }
    void ResetMakingSkillItem() override { item.nAttribute.Put(item.nAttribute.Get() & ~EquipAttr::MakingSkillItem); }
    void SetAppliedAccountShareTag() override { item.nAttribute.Put(item.nAttribute.Get() | EquipAttr::AppliedAccountShareTag); }
    void ResetAppliedAccountShareTag() override { item.nAttribute.Put(item.nAttribute.Get() & ~EquipAttr::AppliedAccountShareTag); }

    // === nSpecialAttribute — making skill tier ===
    [[nodiscard]] auto IsMakingSkillMeisterItem() -> bool override
    {
        return (item.nSpecialAttribute.Get() & EquipSpecialAttr::MakingSkillMeister) != 0;
    }

    void SetMakingSkillMeisterItem() override
    {
        item.nSpecialAttribute.Put(item.nSpecialAttribute.Get() | EquipSpecialAttr::MakingSkillMeister);
    }

    [[nodiscard]] auto IsMakingSkillMasterItem() -> bool override
    {
        return (item.nSpecialAttribute.Get() & EquipSpecialAttr::MakingSkillMaster) != 0;
    }

    void SetMakingSkillMasterItem() override
    {
        item.nSpecialAttribute.Put(item.nSpecialAttribute.Get() | EquipSpecialAttr::MakingSkillMaster);
    }

    [[nodiscard]] auto IsVestige() -> std::int32_t override
    {
        return (item.nSpecialAttribute.Get() & EquipSpecialAttr::Vestige) >> 7;
    }

    // === Cuttable ===
    [[nodiscard]] auto IsCuttableItem() -> bool override
    {
        return item.nCuttable.Get() <= 20 && liCashItemSN == 0;
    }

    [[nodiscard]] auto IsCuttableRemained() -> bool override
    {
        return item.nCuttable.Get() != 0 && item.nCuttable.Get() <= 20;
    }

    auto DecCuttableCount() -> bool override;

    // === nItemState — gacha/refund ===
    [[nodiscard]] auto IsRefunableGachaponItem() -> bool override
    {
        return (item.nItemState.Get() >> 3) & 1;
    }

    [[nodiscard]] auto IsRefunableEventGachaponItem() -> bool override
    {
        return (item.nItemState.Get() >> 4) & 1;
    }

    void ResetRefunableGachaponItem() override
    {
        item.nItemState.Put(item.nItemState.Get() & ~EquipItemStateFlag::RefunableGachapon);
    }

    void SetRefunableEventGachaponItem() override
    {
        item.nItemState.Put(item.nItemState.Get() | EquipItemStateFlag::RefunableEventGachapon);
    }

    // === Serialization ===
    void RawDecode(InPacket& iPacket) override;
    void RawEncode(OutPacket& oPacket, bool bToClient) override;

    // === Set item ===
    [[nodiscard]] auto IsSetItem() -> std::int32_t override;
    [[nodiscard]] auto GetSetItemID() -> std::int32_t override;

    // === Equip-specific virtual methods (not in base) ===
    [[nodiscard]] virtual auto IsReleased() const -> std::int32_t
    {
        return (option.nGrade.Get() >> 4) & 1;
    }

    [[nodiscard]] virtual auto IsAdditionalReleased() -> std::int32_t
    {
        return (option.nGrade.Get() & EquipGradeFlag::AdditionalNotReleased) == 0;
    }

    virtual void SetReleased(std::int32_t bReleased);

    [[nodiscard]] virtual auto GetIUC() const -> std::int32_t
    {
        return item.nIUC.Get();
    }

    virtual void SetIUC(std::int32_t nIUC)
    {
        item.nIUC.Put(nIUC);
    }

    // === Non-virtual methods — nSpecialAttribute queries ===
    [[nodiscard]] auto IsItemNotDestroy() const -> std::int32_t
    {
        return item.nSpecialAttribute.Get() & EquipSpecialAttr::ItemNotDestroy;
    }

    [[nodiscard]] auto IsAlwaysGradeUpgrade() const -> std::int32_t
    {
        return (item.nSpecialAttribute.Get() & EquipSpecialAttr::AlwaysGradeUpgrade) >> 1;
    }

    [[nodiscard]] auto IsAlwaysInchantSuccess() const -> std::int32_t
    {
        return (item.nSpecialAttribute.Get() & EquipSpecialAttr::AlwaysInchantSuccess) >> 2;
    }

    [[nodiscard]] auto IsItemExtended() const -> bool
    {
        return (item.nSpecialAttribute.Get() & EquipSpecialAttr::ItemExtended) >> 3;
    }

    [[nodiscard]] auto IsSellingOneMeso() const -> std::int32_t
    {
        return (item.nSpecialAttribute.Get() & EquipSpecialAttr::SellingOneMeso) >> 4;
    }

    // === Non-virtual methods — nSpecialAttribute setters ===
    void SetItemNotDestroy()
    {
        item.nSpecialAttribute.Put(item.nSpecialAttribute.Get() | EquipSpecialAttr::ItemNotDestroy);
    }

    void SetAlwaysGradeUpgrade()
    {
        item.nSpecialAttribute.Put(item.nSpecialAttribute.Get() | EquipSpecialAttr::AlwaysGradeUpgrade);
    }

    void SetAlwaysInchantSuccess()
    {
        item.nSpecialAttribute.Put(item.nSpecialAttribute.Get() | EquipSpecialAttr::AlwaysInchantSuccess);
    }

    void SetItemExtended()
    {
        item.nSpecialAttribute.Put(item.nSpecialAttribute.Get() | EquipSpecialAttr::ItemExtended);
    }

    void SetSellingOneMeso()
    {
        item.nSpecialAttribute.Put(item.nSpecialAttribute.Get() | EquipSpecialAttr::SellingOneMeso);
    }

    // === Non-virtual methods — durability ===
    [[nodiscard]] auto GetDurabilityMax() const -> std::int32_t
    {
        auto nMax = item.nDurabilityMax.Get();
        return nMax != 0 ? nMax : -1;
    }

    void SetDurabilityMax(std::int32_t nMax)
    {
        item.nDurabilityMax.Put(nMax);
    }

    // === Non-virtual methods — grade manipulation ===
    void SetAdditionalReleased(std::int32_t bReleased);
    [[nodiscard]] auto GetCubeExOptLv() const -> std::int32_t;

    // === Non-virtual methods — potential options ===
    [[nodiscard]] auto GetPotentialOption(std::int32_t nIdx) const -> std::uint16_t;
    void SetPotentialOption(std::int32_t nIdx, std::uint16_t usOption);
    void ResetItemGrade();
    void ResetAdditionalGrade();
    void ResetSoulSocketAndOption();

    // === Non-virtual methods — nItemState queries ===
    [[nodiscard]] auto IsRedLabelItem() const -> bool
    {
        return (item.nItemState.Get() >> 5) & 1;
    }

    [[nodiscard]] auto IsBlackLabelItem() const -> bool
    {
        return (item.nItemState.Get() >> 6) & 1;
    }

    [[nodiscard]] auto IsInnocentRUCItem() const -> std::int32_t
    {
        return (item.nItemState.Get() >> 7) & 1;
    }

    [[nodiscard]] auto IsAmazingHyperUpgradeChecked() const -> std::int32_t
    {
        return (item.nItemState.Get() >> 8) & 1;
    }

    void SetAmazingHyperUpgradeChecked()
    {
        item.nItemState.Put(item.nItemState.Get() | EquipItemStateFlag::AmazingHyperUpgradeChecked);
    }

    [[nodiscard]] auto IsAmazingHyperUpgradeUsed_Log() const -> std::int32_t
    {
        return (item.nItemState.Get() >> 9) & 1;
    }

    [[nodiscard]] auto IsAmazingHyperUpgradeUsed_Stat() const -> std::int32_t
    {
        return (item.nItemState.Get() >> 10) & 1;
    }

    [[nodiscard]] auto IsAmazingHyperUpgradeUsed_Sync() const -> std::int32_t
    {
        return (item.nItemState.Get() >> 11) & 1;
    }

    [[nodiscard]] auto IsVestigeBinded() const -> std::int32_t
    {
        return (item.nItemState.Get() >> 9) & 1;
    }

    [[nodiscard]] auto IsVestigePossibleTrading() const -> std::int32_t
    {
        return (item.nItemState.Get() >> 10) & 1;
    }

    [[nodiscard]] auto IsVestigeAppliedAccountShareTag() const -> std::int32_t
    {
        return (item.nItemState.Get() >> 11) & 1;
    }

    // === Data members ===
    std::int64_t liSN{};                            // Serial number
    std::array<char, 13> sTitle{};                  // Item title string
    FileTime ftEquipped{};                          // Time when equipped
    std::int32_t nPrevBonusExpRate{};               // Previous bonus EXP rate

    GW_ItemSlotEquipBase item;                      // Equipment stats
    GW_CashItemOption cashItemOption;               // Cash item options
    GW_ItemSlotEquipOpt option;                     // Potential / soul options
};

} // namespace ms
