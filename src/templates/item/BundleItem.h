#pragma once

#include "util/ZXString.h"

#include <cstdint>
#include <map>
#include <string>

namespace ms
{

/// Bundle item data (consumable / setup / etc)
/// Original size: 0x180 = 384 bytes, 70 fields
/// Decompiled from: GetBundleItemInfoData @ 0xaf57a0
struct BundleItem
{
    // --- Identity ---
    ZXString<char> sItemName;
    std::int32_t nItemID{};
    std::int32_t bTimeLimited{};

    // --- Restrictions ---
    std::int32_t bOnly{};
    std::int32_t bTradeBlock{};
    std::int32_t bNotSale{};
    std::int32_t bExpireOnLogout{};
    std::int32_t bQuest{};
    std::int32_t bPartyQuest{};
    std::int32_t bAccountSharable{};
    std::int32_t bSharableOnce{};
    std::int32_t bAccountShareTagApplicable{};

    // --- Stats ---
    std::int32_t nPAD{};
    std::int32_t nrLevel{};                // Required level
    std::int32_t nrSTR{};
    std::int32_t nSellPrice{};

    // --- Cash ---
    std::int32_t bCash{};
    std::int32_t nSlotMax{};
    std::int32_t nMaxPerSlot{};

    // --- Type ---
    std::int32_t nBagType{};
    std::int32_t bSpecialGrade{};
    std::int32_t bMobEquip{};

    // --- Soul ---
    std::int32_t nSoulItemType{};
    std::uint32_t dwSummonSoulMobID{};

    // --- Bonus ---
    std::int32_t nBonusEXPRate{};

    // --- Tooltips ---
    ZXString<char> sTooltipCantAccountSharable;
    ZXString<char> sTooltipCanAccountSharable;

    // --- Job restriction maps ---
    std::map<std::int32_t, std::int32_t> mCantAccountSharableJob;
    std::map<std::int32_t, std::int32_t> mCanAccountSharableJob;

    // --- Karma ---
    std::int32_t nAppliableKarmaType{};

    // --- Chair ---
    std::int32_t nUseMesoChair{};

    // Additional fields are added as methods are implemented.
};

} // namespace ms
