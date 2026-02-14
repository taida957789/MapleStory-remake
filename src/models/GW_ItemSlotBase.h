#pragma once

#include "util/FileTime.h"
#include "util/security/TSecType.h"
#include "templates/item/ItemHelper.h"
#include <cstdint>
#include <memory>
#include <string>

namespace ms
{

class InPacket;
class OutPacket;

/**
 * @brief Base item slot data from the game server
 *
 * Based on GW_ItemSlotBase (__cppobj : ZRefCounted) from the original
 * MapleStory client. Base class for all inventory item types
 * (GW_ItemSlotEquip, GW_ItemSlotBundle, GW_ItemSlotPet).
 */
class GW_ItemSlotBase
{
public:
    static constexpr std::int32_t GW_ItemSlotEquip_TYPE = 1;
    static constexpr std::int32_t GW_ItemSlotBundle_TYPE = 2;
    static constexpr std::int32_t GW_ItemSlotPet_TYPE = 3;

    virtual ~GW_ItemSlotBase() = default;

    // === Item flag queries ===
    [[nodiscard]] virtual auto IsUsedItem() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsProtectedItem() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsPreventSlipItem() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsSupportWarmItem() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsBindedItem() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsPossibleTradingItem() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsNonCombatStatExpUpItem() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsMakingSkillItem() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsBarrierEffectApplied() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsLuckyDayEffectApplied() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsAppliedAccountShareTag() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsRUCBarrierApplied() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsScrollBarrierApplied() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsReturnEffectApplied() -> std::int32_t { return 0; }

    // === Identity / type ===
    [[nodiscard]] virtual auto GetSN() -> std::int64_t { return 0; }
    [[nodiscard]] virtual auto GetType() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto GetDataSize() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto GetItemNumber() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto GetLevelUpType() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto GetLevel() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto GetEXP() -> std::int64_t { return 0; }
    virtual void SetItemNumber(std::int16_t /*nNumber*/) {}

    // === Title ===
    [[nodiscard]] virtual auto GetItemTitle() -> std::string { return {}; }
    virtual void SetItemTitle(const std::string& /*sTitle*/) {}

    // === Flag set/reset ===
    virtual void SetUsed() {}
    virtual void ResetUsed() {}
    virtual void SetProtected() {}
    virtual void ResetProtected() {}
    virtual void SetPreventSlip() {}
    virtual void ResetPreventSlip() {}
    virtual void SetWarmSupport() {}
    virtual void ResetWarmSupport() {}
    virtual void SetBinded() {}
    virtual void ResetBinded() {}
    virtual void SetPossibleTrading() {}
    virtual void ResetPossibleTrading() {}
    virtual void SetNonCombatStatExpUpItem() {}
    virtual void ResetNonCombatStatExpUpItem() {}
    virtual void SetMakingSkillItem() {}
    virtual void ResetMakingSkillItem() {}
    virtual void SetAppliedAccountShareTag() {}
    virtual void ResetAppliedAccountShareTag() {}

    // === Attribute ===
    virtual void SetItemAttribute(std::int16_t /*nAttr*/) {}
    [[nodiscard]] virtual auto GetItemAttribute() -> std::int16_t { return 0; }

    // === Look / grade ===
    [[nodiscard]] virtual auto GetLookItemID() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto GetItemGrade() -> std::uint8_t { return 0; }

    // === Level / EXP setters ===
    virtual void SetLevel(std::uint8_t /*nLevel*/) {}
    virtual void SetEXP(std::int64_t /*nEXP*/) {}

    // === Active state ===
    virtual void SetActiveState(std::uint8_t /*nState*/) {}
    [[nodiscard]] virtual auto GetActiveState() -> std::uint8_t { return 0; }

    // === Look change / additional ===
    [[nodiscard]] virtual auto IsLookChangeItem() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto IsAdditionalOPT() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto GetAdditionalGrade() -> std::int32_t { return 0; }

    // === Growth / PS enchant ===
    [[nodiscard]] virtual auto GetGrowthEnchantID() -> std::int32_t { return 0; }
    virtual void SetGrowthEnchantID(std::int32_t /*nID*/, std::int32_t /*nParam*/) {}
    [[nodiscard]] virtual auto GetPSEnchantID() -> std::int32_t { return 0; }
    virtual void SetPSEnchantID(std::int32_t /*nID*/) {}

    // === Cuttable ===
    virtual auto DecCuttableCount() -> bool { return false; }
    [[nodiscard]] virtual auto IsCuttableItem() -> bool { return false; }
    [[nodiscard]] virtual auto IsCuttableRemained() -> bool { return false; }

    // === Gacha / refund ===
    [[nodiscard]] virtual auto IsRefunableGachaponItem() -> bool { return false; }
    [[nodiscard]] virtual auto IsRefunableEventGachaponItem() -> bool { return false; }
    virtual void SetRefunableEventGachaponItem() {}
    virtual void ResetRefunableGachaponItem() {}
    virtual void SetBuyMaplePoint() {}

    // === Making skill tier ===
    [[nodiscard]] virtual auto IsMakingSkillMeisterItem() -> bool { return false; }
    virtual void SetMakingSkillMeisterItem() {}
    [[nodiscard]] virtual auto IsMakingSkillMasterItem() -> bool { return false; }
    [[nodiscard]] virtual auto IsVestige() -> std::int32_t { return 0; }
    virtual void SetMakingSkillMasterItem() {}

    // === Evolution / set ===
    [[nodiscard]] virtual auto IsPossibleSetEvolutionData() -> std::int32_t { return 0; }
    virtual void BackwardUpdateCashItem(GW_ItemSlotBase* /*pOther*/) {}
    [[nodiscard]] virtual auto DumpString() -> std::string { return {}; }

    // === Serialization ===
    virtual void RawDecode(InPacket& /*iPacket*/) {}
    virtual void RawEncode(OutPacket& /*oPacket*/, bool /*bForInternal*/) {}

    // === Set item ===
    [[nodiscard]] virtual auto IsSetItem() -> std::int32_t { return 0; }
    [[nodiscard]] virtual auto GetSetItemID() -> std::int32_t { return 0; }

    // === Factory ===
    /// GW_ItemSlotBase::CreateItem — creates Equip(1), Bundle(2), or Pet(3)
    [[nodiscard]] static auto CreateItem(std::int32_t nType) -> std::shared_ptr<GW_ItemSlotBase>;

    // === Non-virtual methods ===
    [[nodiscard]] auto IsCashItem() const noexcept -> bool { return liCashItemSN != 0; }
    [[nodiscard]] auto IsTimeLimitedItem() const noexcept -> bool { return dateExpire < kDbDate20790101; }
    [[nodiscard]] auto GetBagIndex() const noexcept -> std::int32_t { return nBagIndex; }
    [[nodiscard]] auto GetTypeIndex() const -> std::int32_t { return helper::GetItemType(nItemID); }

    [[nodiscard]] auto IsBagOpened() const -> bool;

    // === Data members ===
    TSecType<std::int32_t> nItemID;
    std::int64_t liCashItemSN{};
    FileTime dateExpire{};
    std::int32_t nBagIndex{};
};

} // namespace ms
