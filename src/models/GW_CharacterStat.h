#pragma once

#include "util/FileTime.h"
#include "util/security/TSecType.h"
#include "util/security/ZtlSecureTear.h"

#include <array>
#include <cstdint>

namespace ms
{

// ============================================================================
// Supporting types
// ============================================================================

/**
 * @brief Extended SP (skill points) per job advancement
 *
 * Based on ExtendSP from the original client.
 * Stores per-advancement SP for jobs that track SP by job level (e.g. Evan).
 */
struct ExtendSP
{
    static constexpr int kMaxJobLevel = 10;

    std::uint8_t nJobLevel{};
    std::array<std::int32_t, kMaxJobLevel> aSP{};
};

/**
 * @brief Daily limits for non-combat (personality) stats
 *
 * Based on NonCombatStatDayLimit from the original client.
 */
struct NonCombatStatDayLimit
{
    std::int16_t nCharismaMin{};
    std::int16_t nInsightMin{};
    std::int16_t nWillMin{};
    std::int16_t nCraftMin{};
    std::int16_t nSenseMin{};
    std::int16_t nCharmMin{};
    std::uint8_t nLastUpdateDay{};
};

/**
 * @brief Character card info
 *
 * Based on CHARACTERCARD from the original client.
 */
struct CharacterCard
{
    static constexpr int kMaxCards = 9;

    struct CardEntry
    {
        std::int32_t nCharacterID{};
        std::int32_t nLevel{};
        std::int32_t nJobCode{};
    };

    std::array<CardEntry, kMaxCards> aCard{};
};

// ============================================================================
// GW_CharacterStat
// ============================================================================

/**
 * @brief Full character stat block from the game server
 *
 * Based on GW_CharacterStat from the original MapleStory client.
 * Most numeric stats are stored using ZtlSecureTear or TSecType for
 * anti-tampering protection.
 *
 * The GW_ prefix denotes a "GameWorld" data structure — these are the
 * canonical representations of character data as received from the server.
 */
struct GW_CharacterStat
{
    // --- Identity ---
    std::uint32_t dwCharacterID{};
    std::uint32_t dwCharacterIDForLog{};
    std::uint32_t dwWorldIDForLog{};
    std::array<char, 13> sCharacterName{};

    // --- Appearance ---
    std::uint8_t nGender{};
    std::uint8_t nSkin{};
    std::int32_t nFace{};
    std::int32_t nHair{};

    // --- Hair mixing ---
    std::uint8_t nMixBaseHairColor{};
    std::uint8_t nMixAddHairColor{};
    std::uint8_t nMixHairBaseProb{};

    // --- Core stats (secure) ---
    ZtlSecureTear<std::uint8_t> nLevel;
    ZtlSecureTear<std::int16_t> nJob;
    ZtlSecureTear<std::int16_t> nSTR;
    ZtlSecureTear<std::int16_t> nDEX;
    ZtlSecureTear<std::int16_t> nINT;
    ZtlSecureTear<std::int16_t> nLUK;
    ZtlSecureTear<std::int32_t> nHP;
    ZtlSecureTear<std::int32_t> nMHP;
    ZtlSecureTear<std::int32_t> nMP;
    ZtlSecureTear<std::int32_t> nMMP;
    ZtlSecureTear<std::int16_t> nAP;
    ZtlSecureTear<std::int16_t> nSP;
    ZtlSecureTear<std::int64_t> nEXP64;
    ZtlSecureTear<std::int32_t> nPOP;
    ZtlSecureTear<std::int64_t> nMoney;
    ZtlSecureTear<std::int32_t> nWP;

    // --- Extended SP ---
    ExtendSP extendSP;

    // --- Map position (TSecType for heap-based protection) ---
    TSecType<std::uint32_t> dwPosMap;
    std::uint8_t nPortal{};

    // --- Job / appearance ---
    std::int16_t nSubJob{};
    std::int32_t nDefFaceAcc{};

    // --- Fatigue ---
    std::uint8_t nFatigue{};
    std::int32_t nLastFatigueUpdateTime{};

    // --- Personality traits (secure) ---
    ZtlSecureTear<std::int32_t> nCharismaEXP;
    ZtlSecureTear<std::int32_t> nInsightEXP;
    ZtlSecureTear<std::int32_t> nWillEXP;
    ZtlSecureTear<std::int32_t> nCraftEXP;
    ZtlSecureTear<std::int32_t> nSenseEXP;
    ZtlSecureTear<std::int32_t> nCharmEXP;

    // --- Personality daily limits ---
    NonCombatStatDayLimit DayLimit;

    // --- PvP stats (secure) ---
    ZtlSecureTear<std::int32_t> nPvPExp;
    ZtlSecureTear<std::uint8_t> nPvPGrade;
    ZtlSecureTear<std::int32_t> nPvPPoint;
    ZtlSecureTear<std::uint8_t> nPvPModeLevel;
    ZtlSecureTear<std::uint8_t> nPvPModeType;

    // --- Event / Part-time (Alba) ---
    ZtlSecureTear<std::int32_t> nEventPoint;
    ZtlSecureTear<std::uint8_t> nAlbaActivityID;
    FileTime ftAlbaStartTime;
    ZtlSecureTear<std::int32_t> nAlbaDuration;
    ZtlSecureTear<std::int32_t> bAlbaSpecialReward;

    // --- Misc ---
    std::int32_t bBurning{};
    CharacterCard characterCard;
    SystemTime stAccount_LastLogout;
};

} // namespace ms
