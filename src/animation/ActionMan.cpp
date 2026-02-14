#include "ActionMan.h"

#include "ActionFrame.h"
#include "CharacterActionFrameEntry.h"
#include "LoadItemAction.h"
#include "SpriteSource.h"
#include "constants/ActionHelpers.h"
#include "constants/EquipDataPath.h"
#include "constants/WeaponConstants.h"
#include "enums/BodyPart.h"
#include "enums/CharacterAction.h"
#include "graphics/WzGr2DCanvas.h"
#include "templates/item/ItemInfo.h"
#include "app/Application.h"
#include "util/Logger.h"
#include "util/Rand32.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>

namespace ms
{

// ---------------------------------------------------------------------------
// Static maps (matches s_mCharacterRotateAction / s_mBlinkAction in client)
// ---------------------------------------------------------------------------
static std::map<std::int32_t, std::int32_t> s_mCharacterRotateAction;
static std::map<std::int32_t, bool> s_mBlinkAction;

// ---------------------------------------------------------------------------
// s_asEmotionName — 39 emotion name strings (from dynamic initializer at
// 0x1ea5bae, populated via StringPool IDs).
// Indices 0-22 are unique; 23-38 repeat 8-22 plus "qBlue" at index 38.
// ---------------------------------------------------------------------------
static const std::string s_asEmotionName[39] = {
    "blink",      //  0  (StringPool 11031)
    "hit",        //  1  (StringPool 11056)
    "smile",      //  2  (StringPool 1699)
    "troubled",   //  3  (StringPool 1700)
    "cry",        //  4  (StringPool 1701)
    "angry",      //  5  (StringPool 1702)
    "bewildered", //  6  (StringPool 1703)
    "stunned",    //  7  (StringPool 1704)
    "vomit",      //  8  (StringPool 1705)
    "oops",       //  9  (StringPool 1706)
    "cheers",     // 10  (StringPool 1707)
    "chu",        // 11  (StringPool 1708)
    "wink",       // 12  (StringPool 1709)
    "pain",       // 13  (StringPool 1710)
    "glitter",    // 14  (StringPool 1711)
    "blaze",      // 15  (StringPool 11030)
    "shine",      // 16  (StringPool 1712)
    "love",       // 17  (StringPool 1713)
    "despair",    // 18  (StringPool 1714)
    "hum",        // 19  (StringPool 1715)
    "bowing",     // 20  (StringPool 1716)
    "hot",        // 21  (StringPool 11057)
    "dam",        // 22  (StringPool 1717)
    "vomit",      // 23  (repeat of 8)
    "oops",       // 24  (repeat of 9)
    "cheers",     // 25  (repeat of 10)
    "chu",        // 26  (repeat of 11)
    "wink",       // 27  (repeat of 12)
    "pain",       // 28  (repeat of 13)
    "glitter",    // 29  (repeat of 14)
    "blaze",      // 30  (repeat of 15)
    "shine",      // 31  (repeat of 16)
    "love",       // 32  (repeat of 17)
    "despair",    // 33  (repeat of 18)
    "hum",        // 34  (repeat of 19)
    "bowing",     // 35  (repeat of 20)
    "hot",        // 36  (repeat of 21)
    "dam",        // 37  (repeat of 22)
    "qBlue",      // 38  (StringPool 1718)
};

const std::string ActionMan::s_sEmptyEmotion;

auto ActionMan::GetEmotionName(std::int32_t nEmotion) -> const std::string&
{
    if (nEmotion < 0 || nEmotion >= 39)
        return s_sEmptyEmotion;
    return s_asEmotionName[nEmotion];
}

// ---------------------------------------------------------------------------
// blit_canvas — copies source RGBA pixels onto destination at (dstX, dstY)
// with source-over alpha compositing. If alpha == 0, the blit is skipped
// entirely (used for invisible face rendering).
// ---------------------------------------------------------------------------
static void blit_canvas(
    std::vector<std::uint8_t>& dst, int dstW, int dstH,
    int dstX, int dstY,
    const std::vector<std::uint8_t>& src, int srcW, int srcH,
    int alpha)
{
    if (alpha == 0 || src.empty())
        return;

    for (int y = 0; y < srcH; ++y)
    {
        int dy = dstY + y;
        if (dy < 0 || dy >= dstH)
            continue;

        for (int x = 0; x < srcW; ++x)
        {
            int dx = dstX + x;
            if (dx < 0 || dx >= dstW)
                continue;

            auto srcIdx = static_cast<std::size_t>((y * srcW + x) * 4);
            auto dstIdx = static_cast<std::size_t>((dy * dstW + dx) * 4);

            auto sa = src[srcIdx + 3];
            if (sa == 0)
                continue;

            auto da = dst[dstIdx + 3];
            if (da == 0)
            {
                // Destination transparent — direct copy
                dst[dstIdx]     = src[srcIdx];
                dst[dstIdx + 1] = src[srcIdx + 1];
                dst[dstIdx + 2] = src[srcIdx + 2];
                dst[dstIdx + 3] = sa;
            }
            else
            {
                // Source-over alpha compositing
                int outA = sa + da * (255 - sa) / 255;
                if (outA > 0)
                {
                    dst[dstIdx]     = static_cast<std::uint8_t>(
                        (src[srcIdx]     * sa + dst[dstIdx]     * da * (255 - sa) / 255) / outA);
                    dst[dstIdx + 1] = static_cast<std::uint8_t>(
                        (src[srcIdx + 1] * sa + dst[dstIdx + 1] * da * (255 - sa) / 255) / outA);
                    dst[dstIdx + 2] = static_cast<std::uint8_t>(
                        (src[srcIdx + 2] * sa + dst[dstIdx + 2] * da * (255 - sa) / 255) / outA);
                    dst[dstIdx + 3] = static_cast<std::uint8_t>(outA);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// is_action_on_develop  (matches decompiled is_action_on_develop)
// Determines whether a missing action in the WZ data is acceptable.
// Uses CharacterAction enum for action index constants.
// ---------------------------------------------------------------------------
static auto is_action_on_develop(std::int32_t nAction) -> bool
{
    using CA = CharacterAction;
    constexpr auto I = [](CA a) { return static_cast<std::int32_t>(a); };

    // Fallback: BattlePVP actions [BattlepvpManjiWalk, BattlepvpLeemalnyunDestroy]
    auto fallback = [&I](std::int32_t n) -> bool
    {
        return n >= I(CA::BattlepvpManjiWalk) && n <= I(CA::BattlepvpLeemalnyunDestroy);
    };

    if (nAction > I(CA::WhWildShot))
    {
        if (nAction > I(CA::Reactor0))
        {
            if (nAction == I(CA::DeadRiding))
                return true;
            if (nAction >= I(CA::KinesisPsychicAttack) && nAction <= I(CA::KinesisCrash))
                return true;
            if (nAction >= I(CA::AranSmashswing1) && nAction <= I(CA::AranSwingFinalblowMid))
                return true;
            if (nAction >= I(CA::RwGauntlepunch) && nAction <= I(CA::RwMagnumBlowFinish))
                return true;
            return fallback(nAction);
        }
        if (nAction >= I(CA::BattlefieldSheepRevival)
            || nAction == I(CA::WhWildVulcan)
            || nAction == I(CA::GeorgAttack))
            return true;
        if (nAction == I(CA::PinkbeanFlySkill))
            return true;
        return fallback(nAction);
    }

    if (nAction >= I(CA::WhDoubleShot))
        return true;

    if (nAction > I(CA::Michaellink))
    {
        switch (nAction)
        {
        case I(CA::HekatonFlightAttack):
        case I(CA::Shockwavepunch0):
        case I(CA::Shockwavepunch1):
        case I(CA::Shockwavepunch2):
        case I(CA::Groundstrike0):
        case I(CA::Groundstrike1):
        case I(CA::CancelBackstep):
        case I(CA::Deathmarker):
        case I(CA::Momentstep):
        case I(CA::Divisionsoulattack):
        case I(CA::SummonsoultentPre):
        case I(CA::Summonsoultent):
        case I(CA::Summonredemption):
        case I(CA::Spiritclaw):
        case I(CA::Bombpunch0):
        case I(CA::Bombpunch1):
        case I(CA::Bombpunch2):
        case I(CA::Bombpunch3):
        case I(CA::Megapunch0):
        case I(CA::Megapunch1):
        case I(CA::Dragpullingfront):
        case I(CA::Dragpullingturn):
        case I(CA::Dragpullingdown):
        case I(CA::Spiritbarrier):
        case I(CA::Bindarea):
        case I(CA::Spirittransformation):
        case I(CA::StarplanetBoost1):
        case I(CA::StarplanetBoost2):
            return true;
        default:
            return fallback(nAction);
        }
    }

    if (nAction == I(CA::Michaellink))
        return true;

    if (nAction > I(CA::Timedistotion))
    {
        if (nAction >= I(CA::Stand1Floating2) && nAction <= I(CA::Vampdeath))
            return true;
        return fallback(nAction);
    }

    if (nAction >= I(CA::Shadowweb))
        return true;

    return nAction == I(CA::Sharpslash);
}

// ---------------------------------------------------------------------------
const std::string ActionMan::s_sEmpty;

ActionMan::ActionMan() = default;

// ---------------------------------------------------------------------------
// Initialize  (matches CActionMan::Init(int bCallOnLoadAction))
// Called from WvsApp::SetUp with bCallOnLoadAction = false.
// ---------------------------------------------------------------------------
auto ActionMan::Initialize(bool bCallOnLoadAction) -> bool
{
    if (!bCallOnLoadAction)
        ActionFrame::LoadMappers();

    // Build name→code lookup (was InitNameMap)
    m_nameToCode.clear();
    m_nameToCode.reserve(ACTIONDATA_COUNT);
    for (std::size_t i = 0; i < ACTIONDATA_COUNT; ++i)
    {
        m_nameToCode.emplace(
            s_aCharacterActionData[i].m_sName, static_cast<std::int32_t>(i));
    }

    // Load body item (ID 0x7D0 = 2000)
    auto pBodyEntry = GetCharacterImgEntry(2000);
    if (!pBodyEntry || !pBodyEntry->m_pImg)
    {
        LOG_WARN("ActionMan: Could not load body item (ID 2000) for action init");
        return false;
    }

    auto pImg = pBodyEntry->m_pImg;

    for (std::int32_t i = 0; i < static_cast<std::int32_t>(ACTIONDATA_COUNT); ++i)
    {
        // Skip action 58 (matches: if (v4 == 58) goto LABEL_260)
        if (i == 58)
            continue;

        auto& action = s_aCharacterActionData[static_cast<std::size_t>(i)];

        auto pActionNode = pImg->GetChild(action.m_sName);
        if (!pActionNode)
        {
            if (is_action_on_develop(i))
                continue;
            LOG_ERROR("No Character Action Data : {}", i);
            continue;
        }

        // Ghost actions (132-139): navigate into sub-property "1"
        if (static_cast<std::uint32_t>(i - 132) <= 7u)
        {
            auto pSub = pActionNode->GetChild("1");
            if (pSub)
                pActionNode = pSub;
        }

        action.m_nTotalDelay = 0;

        // Get frame count
        auto nSrcCount = static_cast<std::int32_t>(pActionNode->GetChildCount());

        // Read subAvatarAction
        auto pSubAvatar = pActionNode->GetChild("subAvatarAction");
        action.m_sSubAvatarAction = pSubAvatar ? pSubAvatar->GetString("") : "";
        if (!action.m_sSubAvatarAction.empty())
            --nSrcCount;

        // Read repeat
        auto pRepeat = pActionNode->GetChild("repeat");
        action.m_nRepeatFrame = pRepeat ? pRepeat->GetInt(0) : 0;
        if (action.m_nRepeatFrame)
            --nSrcCount;

        if (action.m_bPieced)
        {
            // === Pieced mode ===
            action.m_aPieces.resize(static_cast<std::size_t>(nSrcCount));
            action.m_bZigzag = 0;
            action.m_nEventDelay = 0;

            for (std::int32_t j = 0; j < nSrcCount; ++j)
            {
                auto pFrame = pActionNode->GetChild(std::to_string(j));
                if (!pFrame)
                    continue;

                auto& piece = action.m_aPieces[static_cast<std::size_t>(j)];

                // action (StringPool 11026)
                auto pActionProp = pFrame->GetChild("action");
                if (pActionProp)
                    piece.m_nAction = GetActionCode(pActionProp->GetString(""));

                // frame (StringPool 1833)
                auto pFrameIdx = pFrame->GetChild("frame");
                piece.m_nFrameIdx = pFrameIdx ? pFrameIdx->GetInt(0) : 0;

                // delay (StringPool 11044, default 150)
                auto pDelay = pFrame->GetChild("delay");
                piece.m_nFrameDelay = pDelay ? pDelay->GetInt(150) : 150;

                // flip (StringPool 5918)
                auto pFlip = pFrame->GetChild("flip");
                piece.m_bFlip = (pFlip && pFlip->GetInt(0) != 0) ? 1 : 0;

                // rotate (StringPool 5919)
                auto pRotate = pFrame->GetChild("rotate");
                piece.m_nRotate = pRotate ? pRotate->GetInt(0) : 0;

                // weapon2
                auto pWeapon2 = pFrame->GetChild("weapon2");
                piece.m_bWeapon2 = (pWeapon2 && pWeapon2->GetInt(0) != 0) ? 1 : 0;

                // noweapon
                auto pNoWeapon = pFrame->GetChild("noweapon");
                piece.m_bNoWeapon = pNoWeapon && pNoWeapon->GetInt(0) != 0;

                // alpha (default 255)
                auto pAlpha = pFrame->GetChild("alpha");
                piece.m_nAlpha =
                    static_cast<std::uint8_t>(pAlpha ? pAlpha->GetInt(255) : 255);

                // justDir
                auto pJustDir = pFrame->GetChild("justDir");
                piece.m_nDirectionFix = pJustDir ? pJustDir->GetInt(0) : 0;

                // emotion (default -1, clamped to max 0x26)
                auto pEmotion = pFrame->GetChild("emotion");
                auto nEmotion =
                    static_cast<std::uint32_t>(pEmotion ? pEmotion->GetInt(-1) : -1);
                if (nEmotion > 0x26u)
                    nEmotion = static_cast<std::uint32_t>(-1);
                piece.m_nEmotion = static_cast<std::int32_t>(nEmotion);

                // Track actions with rotation
                if (piece.m_nRotate != 0)
                    s_mCharacterRotateAction.try_emplace(i, 1);

                // Track actions referencing blink (action code 33)
                if (piece.m_nAction == 33)
                    s_mBlinkAction.try_emplace(i, true);

                // move (StringPool 11071)
                auto pMove = pFrame->GetChild("move");
                if (pMove)
                {
                    auto vec = pMove->GetVector();
                    piece.m_ptMove = {vec.x, vec.y};
                }
                else
                {
                    piece.m_ptMove = {0, 0};
                }

                // bShowFace: COPY from referenced action (not read from WZ)
                auto nRefAction = piece.m_nAction;
                auto nRefFrame = piece.m_nFrameIdx;
                if (nRefAction >= 0
                    && static_cast<std::size_t>(nRefAction) < ACTIONDATA_COUNT)
                {
                    auto& refPieces =
                        s_aCharacterActionData[static_cast<std::size_t>(nRefAction)]
                            .m_aPieces;
                    if (nRefFrame >= 0
                        && static_cast<std::size_t>(nRefFrame) < refPieces.size())
                    {
                        piece.m_bShowFace =
                            refPieces[static_cast<std::size_t>(nRefFrame)].m_bShowFace;
                    }
                }

                // Negative delay: negate and accumulate to tEventDelay (all pieced)
                if (piece.m_nFrameDelay < 0)
                {
                    piece.m_nFrameDelay = -piece.m_nFrameDelay;
                    action.m_nEventDelay += piece.m_nFrameDelay;
                }

                action.m_nTotalDelay += piece.m_nFrameDelay;
            }
        }
        else
        {
            // === Non-pieced mode ===
            auto nDesCount =
                action.m_bZigzag ? 2 * nSrcCount - 2 : nSrcCount;

            action.m_aPieces.resize(static_cast<std::size_t>(nDesCount));

            // Range check: actions 981-1050 (PB actions) have special delay handling
            auto bIsPBRange =
                static_cast<std::uint32_t>(i - 981) <= 0x45u;

            for (std::int32_t j = 0; j < nSrcCount; ++j)
            {
                auto pFrame = pActionNode->GetChild(std::to_string(j));
                if (!pFrame)
                    continue;

                auto& piece = action.m_aPieces[static_cast<std::size_t>(j)];
                piece.m_nFrameIdx = 0;

                // delay (default 150)
                auto pDelay = pFrame->GetChild("delay");
                piece.m_nFrameDelay = pDelay ? pDelay->GetInt(150) : 150;

                // flip
                auto pFlip = pFrame->GetChild("flip");
                piece.m_bFlip = (pFlip && pFlip->GetInt(0) != 0) ? 1 : 0;

                // rotate
                auto pRotate = pFrame->GetChild("rotate");
                piece.m_nRotate = pRotate ? pRotate->GetInt(0) : 0;

                // weapon2
                auto pWeapon2 = pFrame->GetChild("weapon2");
                piece.m_bWeapon2 = (pWeapon2 && pWeapon2->GetInt(0) != 0) ? 1 : 0;

                // noweapon
                auto pNoWeapon = pFrame->GetChild("noweapon");
                piece.m_bNoWeapon = pNoWeapon && pNoWeapon->GetInt(0) != 0;

                // alpha (default 255)
                auto pAlpha = pFrame->GetChild("alpha");
                piece.m_nAlpha =
                    static_cast<std::uint8_t>(pAlpha ? pAlpha->GetInt(255) : 255);

                // justDir
                auto pJustDir = pFrame->GetChild("justDir");
                piece.m_nDirectionFix = pJustDir ? pJustDir->GetInt(0) : 0;

                // emotion (default -1, clamped to max 0x26)
                auto pEmotion = pFrame->GetChild("emotion");
                auto nEmotion =
                    static_cast<std::uint32_t>(pEmotion ? pEmotion->GetInt(-1) : -1);
                if (nEmotion > 0x26u)
                    nEmotion = static_cast<std::uint32_t>(-1);
                piece.m_nEmotion = static_cast<std::int32_t>(nEmotion);

                // Track actions with rotation
                if (piece.m_nRotate != 0)
                    s_mCharacterRotateAction.try_emplace(i, 1);

                // move
                auto pMove = pFrame->GetChild("move");
                if (pMove)
                {
                    auto vec = pMove->GetVector();
                    piece.m_ptMove = {vec.x, vec.y};
                }
                else
                {
                    piece.m_ptMove = {0, 0};
                }

                // face / bShowFace (StringPool 11046) — read from WZ
                auto pFace = pFrame->GetChild("face");
                piece.m_bShowFace = (pFace && pFace->GetInt(0) != 0) ? 1 : 0;

                // Negative delay: only for PB actions [981, 1050]
                if (bIsPBRange && piece.m_nFrameDelay < 0)
                {
                    piece.m_nFrameDelay = -piece.m_nFrameDelay;
                    action.m_nEventDelay += piece.m_nFrameDelay;
                }

                action.m_nTotalDelay += piece.m_nFrameDelay;
            }

            // Zigzag: mirror frames
            if (nSrcCount < nDesCount)
            {
                auto dst = nSrcCount;
                auto src = nDesCount - nSrcCount; // = nSrcCount - 2
                auto count = nDesCount - nSrcCount;

                for (std::int32_t k = 0; k < count; ++k)
                {
                    action.m_aPieces[static_cast<std::size_t>(dst)] =
                        action.m_aPieces[static_cast<std::size_t>(src)];

                    auto& mirrored =
                        action.m_aPieces[static_cast<std::size_t>(dst)];
                    mirrored.m_nFrameIdx = 0;

                    // Negative delay handling for mirrored frames
                    if (bIsPBRange && mirrored.m_nFrameDelay < 0)
                    {
                        mirrored.m_nFrameDelay = -mirrored.m_nFrameDelay;
                        action.m_nEventDelay += mirrored.m_nFrameDelay;
                    }

                    action.m_nTotalDelay += mirrored.m_nFrameDelay;
                    ++dst;
                    --src;
                }
            }

            // Event delay override for non-PB actions
            if (!bIsPBRange)
            {
                if (action.m_bZigzag)
                {
                    action.m_nEventDelay = 0;
                }
                else if (nDesCount > 0)
                {
                    action.m_nEventDelay =
                        action.m_nTotalDelay
                        - action.m_aPieces[static_cast<std::size_t>(nDesCount - 1)]
                              .m_nFrameDelay;
                }
                else
                {
                    action.m_nEventDelay = 0;
                }
            }
        }
    }

    LoadRandomMoveActionChange();

    LOG_INFO("ActionMan: Initialized actions from body item");
    return true;
}

// ---------------------------------------------------------------------------
// GetCharacterImgEntry
// ---------------------------------------------------------------------------
auto ActionMan::GetCharacterImgEntry(std::int32_t nItemID)
    -> std::shared_ptr<CharacterImgEntry>
{
    auto it = m_mCharacterImgEntry.find(nItemID);
    if (it != m_mCharacterImgEntry.end())
        return it->second;

    auto sPath = get_equip_data_path(nItemID);
    if (sPath.empty())
        return nullptr;

    auto& resMan = WzResMan::GetInstance();
    auto pImg = resMan.GetProperty(sPath);
    if (!pImg)
    {
        m_mCharacterImgEntry[nItemID] = nullptr;
        return nullptr;
    }

    auto entry = std::make_shared<CharacterImgEntry>();
    entry->m_pImg = pImg;

    auto pInfo = pImg->GetChild("info");
    if (pInfo)
    {
        auto pISlot = pInfo->GetChild("islot");
        entry->m_sISlot = pISlot ? pISlot->GetString("") : "";

        auto pVSlot = pInfo->GetChild("vslot");
        entry->m_sVSlot = pVSlot ? pVSlot->GetString("") : "";

        auto pAfterImage = pInfo->GetChild("afterImage");
        entry->m_sWeaponAfterimage = pAfterImage ? pAfterImage->GetString("") : "";

        auto pAttackSpeed = pInfo->GetChild("attackSpeed");
        entry->m_nAttackSpeed = pAttackSpeed ? pAttackSpeed->GetInt(0) : 0;

        auto pWalk = pInfo->GetChild("walk");
        entry->m_nWalk = pWalk ? pWalk->GetInt(0) : 0;

        auto pStand = pInfo->GetChild("stand");
        entry->m_nStand = pStand ? pStand->GetInt(0) : 0;

        auto pAttack = pInfo->GetChild("attack");
        entry->m_nAttack = pAttack ? pAttack->GetInt(0) : 0;

        auto pSfx = pInfo->GetChild("sfx");
        entry->m_sSfx = pSfx ? pSfx->GetString("") : "";

        auto pWeekly = pInfo->GetChild("weekly");
        entry->m_bWeekly = pWeekly && pWeekly->GetInt(0) != 0;

        auto pInvisibleFace = pInfo->GetChild("invisibleFace");
        entry->m_bInvisibleFace = pInvisibleFace && pInvisibleFace->GetInt(0) != 0;

        auto pExtendFrame = pInfo->GetChild("extendFrame");
        entry->m_bExtendFrame = pExtendFrame && pExtendFrame->GetInt(0) != 0;

        entry->m_pVehicleDefaultFrame = pInfo->GetChild("vehicleDefaultFrame");
    }

    entry->m_nWeapon = get_weapon_type(nItemID);

    m_mCharacterImgEntry[nItemID] = entry;
    m_lCharacterImgEntry.push_back(entry);
    return entry;
}

// ---------------------------------------------------------------------------
auto ActionMan::GetActionData(std::int32_t nAction) const -> const ActionData*
{
    if (nAction < 0 || static_cast<std::size_t>(nAction) >= ACTIONDATA_COUNT)
        return nullptr;
    return &s_aCharacterActionData[static_cast<std::size_t>(nAction)];
}

// ---------------------------------------------------------------------------
// GetActionCode  (matches decompiled get_action_code_from_name, but O(1)
// via hash map instead of the original O(n) linear scan)
// ---------------------------------------------------------------------------
auto ActionMan::GetActionCode(const std::string& sName) const -> std::int32_t
{
    auto it = m_nameToCode.find(sName);
    if (it != m_nameToCode.end())
        return it->second;
    return -1;
}

// ---------------------------------------------------------------------------
auto ActionMan::GetActionName(std::int32_t nAction) const -> const std::string&
{
    if (nAction < 0 || static_cast<std::size_t>(nAction) >= ACTIONDATA_COUNT)
        return s_sEmpty;
    return s_aCharacterActionData[static_cast<std::size_t>(nAction)].m_sName;
}

// ---------------------------------------------------------------------------
// GetWeaponAfterImage
// (matches decompiled CActionMan::GetWeaponAfterImage)
// Loads or returns cached afterimage data for a weapon UOL path.
// ---------------------------------------------------------------------------
auto ActionMan::GetWeaponAfterImage(const std::string& sUOL)
    -> std::shared_ptr<MeleeAttackAfterimage>
{
    // Cache lookup
    auto it = m_mAfterimage.find(sUOL);
    if (it != m_mAfterimage.end())
        return it->second;

    // Allocate new entry with 1310 slots
    auto p = std::make_shared<MeleeAttackAfterimage>();
    p->m_aapCanvas.resize(ACTIONDATA_COUNT);
    p->m_arcRange.resize(ACTIONDATA_COUNT);

    // Load WZ property from UOL path
    auto& resMan = WzResMan::GetInstance();
    auto pAfterimage = resMan.GetProperty(sUOL);

    if (pAfterimage)
    {
        for (std::int32_t nAction = 0;
             nAction < static_cast<std::int32_t>(ACTIONDATA_COUNT); ++nAction)
        {
            auto& sActionName = GetActionName(nAction);

            auto pAction = pAfterimage->GetChild(sActionName);
            if (!pAction)
                continue;

            // Read lt / rb vectors for attack range (StringPool 11065, 11075)
            auto pLT = pAction->GetChild("lt");
            auto pRB = pAction->GetChild("rb");

            auto idx = static_cast<std::size_t>(nAction);

            if (pLT && pRB)
            {
                auto lt = pLT->GetVector();
                auto rb = pRB->GetVector();
                p->m_arcRange[idx].left = lt.x;
                p->m_arcRange[idx].top = lt.y;
                p->m_arcRange[idx].right = rb.x;
                p->m_arcRange[idx].bottom = rb.y;
            }

            // Find first sub-property child (canvas container).
            // In the original, this QIs each child to IWzProperty;
            // vector children (lt/rb) fail the QI and are skipped.
            for (auto& [sName, pChild] : pAction->GetChildren())
            {
                if (!pChild || !pChild->HasChildren())
                    continue;

                auto nFrames = pChild->GetChildCount();
                auto& canvases = p->m_aapCanvas[idx];
                canvases.resize(nFrames);

                for (std::size_t k = 0; k < nFrames; ++k)
                {
                    auto pNode = pChild->GetChild(std::to_string(k));
                    if (pNode)
                    {
                        auto pCanvas = pNode->GetCanvas();
                        if (pCanvas)
                            canvases[k] = std::make_shared<WzGr2DCanvas>(pCanvas);
                    }
                }
                break; // Only process the first valid sub-property
            }
        }
    }

    m_mAfterimage[sUOL] = p;
    return p;
}

// ---------------------------------------------------------------------------
// GetRandomMoveActionChange
// (matches decompiled CActionMan::GetRandomMoveActionChange @ 0x4d0e40)
// Weighted random selection of an alternative action for the given action ID.
// ---------------------------------------------------------------------------
auto ActionMan::GetRandomMoveActionChange(std::int32_t nActionID) -> std::int32_t
{
    auto it = m_mMoveActionChange.find(nActionID);
    if (it == m_mMoveActionChange.end())
        return -1;

    auto& entries = it->second;
    if (entries.empty())
        return -1;

    // Sum all probabilities
    std::int32_t nTotalProb = 0;
    for (auto& e : entries)
        nTotalProb += e.m_nProb;

    if (nTotalProb <= 0)
        return -1;

    // Pick a random value in [1, nTotalProb]
    auto nRand = static_cast<std::uint32_t>(detail::get_rand().Random());
    auto nRoll = static_cast<std::int32_t>((nRand % nTotalProb) + 1);

    // Walk entries, accumulate probability
    std::int32_t nAccum = 0;
    for (auto& e : entries)
    {
        nAccum += e.m_nProb;
        if (nRoll <= nAccum)
            return e.m_nAction;
    }

    return -1;
}

// ---------------------------------------------------------------------------
// LoadRandomMoveActionChange
// (matches decompiled CActionMan::LoadRandomMoveActionChange)
// Loads random move action change data from Etc/RandomMoveAction.img.
// ---------------------------------------------------------------------------
void ActionMan::LoadRandomMoveActionChange()
{
    auto pProp = WzResMan::GetInstance().GetProperty("Etc/RandomMoveAction.img");
    if (!pProp)
        return;

    m_mMoveActionChange.clear();

    for (auto& [sName, pChild] : pProp->GetChildren())
    {
        if (!pChild)
            continue;

        auto nAction = GetActionCode(sName);
        LoadRandomMoveActionChangeInfo(nAction, pChild);
    }
}

// ---------------------------------------------------------------------------
// LoadRandomMoveActionChangeInfo
// (matches decompiled CActionMan::LoadRandomMoveActionChangeInfo)
// Loads individual move action change entries for a given action.
// ---------------------------------------------------------------------------
void ActionMan::LoadRandomMoveActionChangeInfo(
    std::int32_t nAction,
    const std::shared_ptr<WzProperty>& pRandomProp)
{
    if (!pRandomProp)
        return;

    std::vector<MoveActionChange> info;

    for (auto& [sIdx, pChild] : pRandomProp->GetChildren())
    {
        if (!pChild)
            continue;

        MoveActionChange val;

        // action
        auto pAction = pChild->GetChild("action");
        if (pAction)
            val.m_nAction = GetActionCode(pAction->GetString(""));

        // prob
        auto pProb = pChild->GetChild("prob");
        val.m_nProb = pProb ? pProb->GetInt(0) : 0;

        info.push_back(val);
    }

    if (!info.empty())
        m_mMoveActionChange[nAction] = std::move(info);
}

// ---------------------------------------------------------------------------
// IsGettableImgEntry (from decompiled CActionMan::IsGettableImgEntry @ 0x4d9010)
// Returns true if a CharacterImgEntry can be loaded for the given item ID.
// ---------------------------------------------------------------------------
auto ActionMan::IsGettableImgEntry(std::int32_t nItemID) -> bool
{
    return GetCharacterImgEntry(nItemID) != nullptr;
}

// ---------------------------------------------------------------------------
// IsInvisibleFace (from decompiled CActionMan::IsInvisibleFace @ 0x4d0f20)
// Returns true if the face should be invisible (acc has invisibleFace flag
// or job is Pinkbean 13000/13100).
// ---------------------------------------------------------------------------
auto ActionMan::IsInvisibleFace(
    const std::shared_ptr<CharacterImgEntry>& pAccEntry,
    std::int32_t nJob) -> bool
{
    bool bInvisible = false;
    if (pAccEntry)
        bInvisible = pAccEntry->m_bInvisibleFace;
    if (nJob == 13000 || nJob == 13100)
        bInvisible = true;
    return bInvisible;
}

// ---------------------------------------------------------------------------
// LoadFaceLook (from decompiled CActionMan::LoadFaceLook @ 0x4de500)
// Loads face look canvases for the given face/emotion/accessory combination.
// Composites face and face-accessory canvases into combined frames.
// ---------------------------------------------------------------------------
void ActionMan::LoadFaceLook(
    std::int32_t nSkin,
    std::int32_t nFace,
    std::int32_t nEmotion,
    std::int32_t nFaceAcc,
    std::list<std::shared_ptr<WzGr2DCanvas>>& lpEmotion,
    std::int32_t nJob,
    bool bIgnoreInvisibleFace)
{
    // Early return for PinkBean jobs (13000 = 0x32C8, 13100 = 0x332C)
    if (nJob == 13000 || nJob == 13100)
        return;

    // Build cache key
    FaceLookCodes fl{nFace, nEmotion, nFaceAcc};

    // ---- Cache lookup ----
    auto cacheIt = m_mFaceLook.find(fl);
    if (cacheIt != m_mFaceLook.end() && cacheIt->second)
    {
        auto& entry = cacheIt->second;
        entry->m_tLastAccessed = static_cast<std::int32_t>(
            Application::GetInstance().GetUpdateTime());
        lpEmotion.clear();
        for (auto& c : entry->m_lpEmotion)
            lpEmotion.push_back(c);
        return;
    }

    // ---- Load face and acc CharacterImgEntry ----
    auto pFaceEntry = GetCharacterImgEntry(nFace);

    std::shared_ptr<CharacterImgEntry> pAccEntry;
    if (nFaceAcc)
        pAccEntry = GetCharacterImgEntry(nFaceAcc);

    // Clamp emotion to [0, 38]
    auto nEmotionIdx = static_cast<std::uint32_t>(nEmotion);
    if (nEmotionIdx > 0x26)
        nEmotionIdx = 0;

    const auto& sEmotionName = s_asEmotionName[nEmotionIdx];

    // Get face root property (prefer m_pWeeklyImg, fallback to m_pImg)
    auto pFaceRoot = pFaceEntry
        ? (pFaceEntry->m_pWeeklyImg ? pFaceEntry->m_pWeeklyImg : pFaceEntry->m_pImg)
        : nullptr;
    if (!pFaceRoot)
        return;

    // Get face emotion property (e.g., "smile", "blink", etc.)
    auto pFace = pFaceRoot->GetChild(sEmotionName);
    if (!pFace)
        return;

    // Get acc emotion property (if acc exists)
    std::shared_ptr<WzProperty> pAcc;
    if (pAccEntry)
    {
        auto pAccRoot = pAccEntry->m_pWeeklyImg
            ? pAccEntry->m_pWeeklyImg
            : pAccEntry->m_pImg;
        if (pAccRoot)
        {
            // For emotion 38 (qBlue), acc uses emotion 0 (blink) instead
            auto nAccEmotionIdx = (nEmotionIdx != 38) ? nEmotionIdx : 0u;
            pAcc = pAccRoot->GetChild(s_asEmotionName[nAccEmotionIdx]);
        }
        if (!pAcc)
            return;
    }

    // ---- Invisible face check ----
    bool bInvisibleFace = IsInvisibleFace(pAccEntry, nJob);
    if (bIgnoreInvisibleFace)
        bInvisibleFace = false;

    // ---- Determine enumerator ----
    // If acc has more frames than face, enumerate acc frames (mapping to face
    // frames via modulo). Otherwise enumerate face frames directly.
    bool bAccEnumerator = false;
    if (pAcc)
    {
        auto nAccCount = static_cast<std::int32_t>(pAcc->GetChildCount());
        auto nFaceCount = static_cast<std::int32_t>(pFace->GetChildCount());
        if (nAccCount > nFaceCount)
            bAccEnumerator = true;
    }

    auto pEnumProp = bAccEnumerator ? pAcc : pFace;
    auto nFaceFrameCount = static_cast<std::int32_t>(pFace->GetChildCount());
    std::int32_t tDuration = 5000;

    // ---- Enumerate frames ----
    for (auto& [sEnumName, pEnumValue] : pEnumProp->GetChildren())
    {
        if (!pEnumValue)
            continue;

        // Determine face frame and acc frame for this iteration
        std::shared_ptr<WzProperty> pFaceFrame;
        std::shared_ptr<WzProperty> pAccFrame;

        if (bAccEnumerator)
        {
            // Enumerating acc: current entry IS the acc frame
            pAccFrame = pEnumValue;

            // Map acc frame index → face frame index via modulo
            if (nFaceFrameCount > 0)
            {
                int idx = std::atoi(sEnumName.c_str());
                pFaceFrame = pFace->GetChild(
                    std::to_string(idx % nFaceFrameCount));
            }
        }
        else
        {
            // Enumerating face: current entry IS the face frame
            pFaceFrame = pEnumValue;

            // Get matching acc frame by same name
            if (pAcc)
                pAccFrame = pAcc->GetChild(sEnumName);
        }

        if (!pFaceFrame)
            continue;

        // Check if face frame is a duration override (VT_I4 in original)
        auto frameType = pFaceFrame->GetNodeType();
        if (frameType == WzNodeType::Int || frameType == WzNodeType::UnsignedShort)
        {
            tDuration = pFaceFrame->GetInt(tDuration);
            continue;
        }

        // ---- Get "face" canvas from face frame ----
        auto pFaceCanvasNode = pFaceFrame->GetChild("face");
        if (!pFaceCanvasNode)
            continue;
        auto pFaceCanvas = pFaceCanvasNode->GetCanvas();
        if (!pFaceCanvas || !pFaceCanvas->HasPixelData())
            continue;

        // Get "delay" (default 60ms)
        auto pDelayProp = pFaceFrame->GetChild("delay");
        auto nDelay = pDelayProp ? pDelayProp->GetInt(60) : 60;

        // Get face Z from canvas property (CSpriteSource::QueryZ)
        auto nFaceZ = SpriteSource::QueryZ(pFaceCanvasNode);

        // ---- Get acc canvas ----
        std::shared_ptr<WzCanvas> pAccCanvas;
        std::shared_ptr<WzProperty> pAccCanvasNode;
        std::int32_t nAccZ = 0;

        if (pAccFrame)
        {
            // Try "default" first, then skin number
            pAccCanvasNode = pAccFrame->GetChild("default");
            if (pAccCanvasNode)
                pAccCanvas = pAccCanvasNode->GetCanvas();

            if (!pAccCanvas)
            {
                pAccCanvasNode = pAccFrame->GetChild(std::to_string(nSkin));
                if (pAccCanvasNode)
                    pAccCanvas = pAccCanvasNode->GetCanvas();
            }

            if (pAccCanvas && pAccCanvas->HasPixelData())
                nAccZ = SpriteSource::QueryZ(pAccCanvasNode);
            else
                pAccCanvas = nullptr;
        }

        // ---- Get face origin, map, brow vectors ----
        auto pFaceOriginProp = pFaceCanvasNode->GetChild("origin");
        if (!pFaceOriginProp)
            continue;
        auto faceOrigin = pFaceOriginProp->GetVector();

        auto pFaceMapProp = pFaceCanvasNode->GetChild("map");
        if (!pFaceMapProp)
            continue;
        auto pFaceBrowProp = pFaceMapProp->GetChild("brow");
        if (!pFaceBrowProp)
            continue;
        auto faceBrow = pFaceBrowProp->GetVector();

        // ---- Calculate face bounding rect ----
        int faceW = pFaceCanvas->GetWidth();
        int faceH = pFaceCanvas->GetHeight();
        Rect rcFace{
            -(faceOrigin.x + faceBrow.x),
            -(faceOrigin.y + faceBrow.y),
            faceW - faceOrigin.x - faceBrow.x,
            faceH - faceOrigin.y - faceBrow.y};

        // ---- Calculate acc bounding rect ----
        Rect rcAcc{};
        bool bHasAcc = false;

        if (pAccCanvas && pAccCanvasNode)
        {
            auto pAccOriginProp = pAccCanvasNode->GetChild("origin");
            auto pAccMapProp = pAccCanvasNode->GetChild("map");
            if (pAccOriginProp && pAccMapProp)
            {
                auto pAccBrowProp = pAccMapProp->GetChild("brow");
                if (pAccBrowProp)
                {
                    auto accOrigin = pAccOriginProp->GetVector();
                    auto accBrow = pAccBrowProp->GetVector();
                    int accW = pAccCanvas->GetWidth();
                    int accH = pAccCanvas->GetHeight();
                    rcAcc = {
                        -(accOrigin.x + accBrow.x),
                        -(accOrigin.y + accBrow.y),
                        accW - accOrigin.x - accBrow.x,
                        accH - accOrigin.y - accBrow.y};
                    bHasAcc = true;
                }
            }
        }

        // ---- Union rects ----
        Rect rcUnion;
        if (!bHasAcc)
        {
            rcUnion = rcFace;
        }
        else
        {
            rcUnion = {
                std::min(rcFace.left, rcAcc.left),
                std::min(rcFace.top, rcAcc.top),
                std::max(rcFace.right, rcAcc.right),
                std::max(rcFace.bottom, rcAcc.bottom)};
        }

        int combinedW = rcUnion.Width();
        int combinedH = rcUnion.Height();
        if (combinedW <= 0 || combinedH <= 0)
            continue;

        // ---- Composite canvases ----
        auto pixelSize = static_cast<std::size_t>(combinedW) * combinedH * 4;
        std::vector<std::uint8_t> pixels(pixelSize, 0);

        int faceX = rcFace.left - rcUnion.left;
        int faceY = rcFace.top - rcUnion.top;
        int accX = rcAcc.left - rcUnion.left;
        int accY = rcAcc.top - rcUnion.top;
        int faceAlpha = bInvisibleFace ? 0 : 255;

        if (nFaceZ < nAccZ)
        {
            // Face behind acc: draw face first, then acc on top
            blit_canvas(pixels, combinedW, combinedH, faceX, faceY,
                pFaceCanvas->GetPixelData(), faceW, faceH, faceAlpha);
            if (pAccCanvas)
            {
                blit_canvas(pixels, combinedW, combinedH, accX, accY,
                    pAccCanvas->GetPixelData(),
                    pAccCanvas->GetWidth(), pAccCanvas->GetHeight(), 255);
            }
        }
        else
        {
            // Acc behind face (or equal z): draw acc first, then face on top
            if (pAccCanvas)
            {
                blit_canvas(pixels, combinedW, combinedH, accX, accY,
                    pAccCanvas->GetPixelData(),
                    pAccCanvas->GetWidth(), pAccCanvas->GetHeight(), 255);
            }
            blit_canvas(pixels, combinedW, combinedH, faceX, faceY,
                pFaceCanvas->GetPixelData(), faceW, faceH, faceAlpha);
        }

        // ---- Create WzGr2DCanvas from composited pixel data ----
        auto pCombinedCanvas = std::make_shared<WzCanvas>(combinedW, combinedH);
        pCombinedCanvas->SetPixelData(std::move(pixels));

        auto pGr2DCanvas = std::make_shared<WzGr2DCanvas>(pCombinedCanvas);
        pGr2DCanvas->SetOrigin({-rcUnion.left, -rcUnion.top});
        pGr2DCanvas->SetDelay(nDelay);

        lpEmotion.push_back(pGr2DCanvas);
    }

    // ---- Cache result ----
    auto entry = std::make_shared<FaceLookEntry>();
    for (auto& c : lpEmotion)
        entry->m_lpEmotion.push_back(c);
    entry->m_nDuration = tDuration;
    entry->m_tLastAccessed = static_cast<std::int32_t>(
        Application::GetInstance().GetUpdateTime());

    m_lFaceLook.push_back(entry);
    m_mFaceLook[fl] = entry;
}

// ---------------------------------------------------------------------------
// load_character_action  (matches CActionMan::load_character_action @ 0x4eefe0)
// Loads body, face, and all equipment sprites for a character action.
// ---------------------------------------------------------------------------
auto ActionMan::load_character_action(
    std::int32_t nAction,
    std::int32_t nSkin,
    std::int32_t nJob,
    const std::int32_t* aAvatarHairEquip,
    std::vector<ActionFrame>& aFrame,
    std::int32_t nWeaponStickerID,
    std::int32_t nVehicleID,
    std::int32_t nGhostIndex,
    std::int32_t nGatherToolID,
    bool bDrawElfEar,
    std::int32_t nLarknessState,
    bool bInvisibleCashCape,
    std::int32_t nMixedHairId,
    std::int32_t nMixPercent,
    bool bZigZag,
    bool bRemoveBody) -> bool
{
    using CA = CharacterAction;
    constexpr auto I = [](CA a) { return static_cast<std::int32_t>(a); };

    // --- Validate weapon sticker ---
    if (!is_weapon_sticker_item(nWeaponStickerID))
        nWeaponStickerID = 0;

    // --- bCanEquip: is a cap equipped? ---
    bool bCanEquip =
        (aAvatarHairEquip[static_cast<std::int32_t>(BodyPart::BP_CAP)] != 0);

    // --- Compute body item ID (nSkin + 2000) ---
    auto nBodyID = nSkin + 2000;

    // Zero body for certain actions
    if (static_cast<std::uint32_t>(nAction - I(CA::MakingSkill2)) <= 8u
        || static_cast<std::uint32_t>(nAction - I(CA::Setitem3)) <= 1u
        || is_dance_action(static_cast<CA>(nAction))
        || (nGhostIndex == 3
            && static_cast<std::uint32_t>(nAction - 132) <= 7u)
        || nAction == I(CA::HideBody)
        || static_cast<std::uint32_t>(nAction - 1156) <= 3u
        || static_cast<std::uint32_t>(nAction - 1051) <= 0x64u
        || static_cast<std::uint32_t>(nAction - 981) <= 0x45u
        || bRemoveBody)
    {
        nBodyID = 2000;
    }

    // --- Cash cape check ---
    // v93 (bCashCape) = true if cape is cash item or bInvisibleCashCape
    auto nCapeID = aAvatarHairEquip[static_cast<std::int32_t>(BodyPart::BP_CAPE)];
    bool bCashCape = bInvisibleCashCape;
    if (!bCashCape && nCapeID != 0)
    {
        bCashCape = ItemInfo::GetInstance().IsCashItem(nCapeID);
    }

    // --- Body action override: bRemoveBody → Blink ---
    auto nBodyAction = nAction;
    if (bRemoveBody)
        nBodyAction = I(CA::Blink);

    // --- 1. Load body sprites ---
    LoadItemAction(
        nBodyAction, nJob, nBodyID, aFrame,
        /*nWeaponStickerID=*/0,
        nVehicleID, nGhostIndex,
        bCanEquip,
        /*bGatherEquip=*/false,
        bDrawElfEar, nLarknessState, bCashCape,
        /*nMixHairID=*/0,
        /*nMixPercent=*/100,
        /*bCapExtendFrame=*/false);

    // --- Ghost action mapping ---
    action_mapping_for_ghost(nAction);

    // --- Dead/DeadRiding → Jump for face ---
    auto nFaceAction = nAction;
    if (nAction == I(CA::DeadRiding))
        nAction = I(CA::Dead);
    if (nAction == I(CA::Dead))
        nFaceAction = I(CA::Jump);
    else
        nFaceAction = nAction;

    // --- 2. Load face sprites ---
    LoadItemAction(
        nFaceAction, nJob, nSkin + 12000, aFrame,
        /*nWeaponStickerID=*/0,
        nVehicleID, nGhostIndex,
        bCanEquip,
        /*bGatherEquip=*/false,
        bDrawElfEar, nLarknessState, bCashCape,
        /*nMixHairID=*/0,
        /*nMixPercent=*/100,
        /*bCapExtendFrame=*/false);

    // --- Action for equipment: Dead → Jump ---
    auto nEquipAction = I(CA::Jump);
    if (nAction != I(CA::Dead))
        nEquipAction = nAction;

    // --- Get action data for extend frame checks ---
    auto pActionData = GetActionData(nEquipAction);

    // --- Get hair/cap CharacterImgEntry ---
    auto pHairImgEntry = GetCharacterImgEntry(
        aAvatarHairEquip[static_cast<std::int32_t>(BodyPart::BP_HAIR)]);
    auto pCapImgEntry = GetCharacterImgEntry(
        aAvatarHairEquip[static_cast<std::int32_t>(BodyPart::BP_CAP)]);

    // --- Extend frame checks ---
    bool bHairExtendFrame = false;
    bool bCapExtendFrame = false;

    // Hair extend frame check
    if (pHairImgEntry && pHairImgEntry->m_bExtendFrame
        && pActionData && !pActionData->m_bPieced
        && !is_not_pieced_action(nAction))
    {
        if (pHairImgEntry->m_pWeeklyImg)
        {
            auto sActionName = GetActionName(nAction);
            auto pHairAction = pHairImgEntry->m_pWeeklyImg->GetChild(sActionName);
            if (pHairAction)
            {
                auto nWzFrameCount = pHairAction->GetChildCount();
                auto nCurrentCount = aFrame.size();
                if (nWzFrameCount > nCurrentCount)
                    bHairExtendFrame = true;
            }
        }
    }

    // Cap extend frame check
    if (pCapImgEntry && pCapImgEntry->m_bExtendFrame
        && pActionData && !pActionData->m_bPieced
        && !is_not_pieced_action(nAction))
    {
        if (pCapImgEntry->m_pWeeklyImg)
        {
            auto sActionName = GetActionName(nAction);
            auto pCapAction = pCapImgEntry->m_pWeeklyImg->GetChild(sActionName);
            if (pCapAction)
            {
                auto nWzFrameCount = pCapAction->GetChildCount();
                auto nCurrentCount = aFrame.size();
                if (nWzFrameCount > nCurrentCount)
                    bCapExtendFrame = true;
            }
        }
    }

    // --- 3. Equipment loop ---
    for (std::int32_t i = 0; i < static_cast<std::int32_t>(BodyPart::BP_COUNT); ++i)
    {
        auto ePart = static_cast<BodyPart>(i);

        // Skip logic per body part
        switch (ePart)
        {
        case BodyPart::BP_WEAPON:
            if (is_weapon_hide_action(static_cast<CA>(nAction)))
                continue;
            // Fall through to Pvpko check (shared with BP_SHIELD)
            if (nAction == I(CA::Pvpko))
                continue;
            break;

        case BodyPart::BP_TAMINGMOB:
        case BodyPart::BP_SADDLE:
        case BodyPart::BP_MOBEQUIP:
            continue;

        case BodyPart::BP_SHIELD:
            if (nAction == I(CA::Pvpko))
                continue;
            break;

        default:
            break;
        }

        // Hat-dance skip for hair (0) and cap (1)
        if (static_cast<std::uint32_t>(i) <= 1u
            && is_hatdance_action(nAction))
            continue;

        // --- BP_SHIELD special handling ---
        if (ePart == BodyPart::BP_SHIELD)
        {
            // Skip shield if gather tool equipped and shield is zero sub-weapon
            if (is_gather_tool_item(nGatherToolID)
                && is_zero_sub_weapon_item(
                    aAvatarHairEquip[static_cast<std::int32_t>(BodyPart::BP_SHIELD)]))
                continue;

            // Special case: 1332237 → load 1342085 instead
            if (nGatherToolID == 1332237)
            {
                auto nRealAction = nAction;
                if (nAction == I(CA::Dead))
                    nRealAction = I(CA::Jump);

                LoadItemAction(
                    nRealAction, nJob, 1342085, aFrame,
                    /*nWeaponStickerID=*/0,
                    nVehicleID, nGhostIndex,
                    bCanEquip,
                    /*bGatherEquip=*/false,
                    bDrawElfEar, nLarknessState, bCashCape,
                    /*nMixHairID=*/0,
                    /*nMixPercent=*/100,
                    /*bCapExtendFrame=*/false);
                continue;
            }

            // Fall through to regular equipment loading
        }

        // --- BP_HAIR (i == 0) ---
        if (i == 0)
        {
            // Skip if hair has extend frames (loaded later)
            if (bHairExtendFrame)
                continue;

            // Mixed hair special path
            if (nMixedHairId != 0)
            {
                auto nRealAction = nAction;
                if (nAction == I(CA::Dead))
                    nRealAction = I(CA::Jump);

                LoadItemAction(
                    nRealAction, nJob,
                    aAvatarHairEquip[0], aFrame,
                    /*nWeaponStickerID=*/0,
                    nVehicleID, nGhostIndex,
                    bCanEquip,
                    /*bGatherEquip=*/false,
                    bDrawElfEar, nLarknessState, bCashCape,
                    nMixedHairId, nMixPercent,
                    /*bCapExtendFrame=*/false);
                continue;
            }

            // Fall through to regular loading
        }

        // --- BP_CAP (i == 1) ---
        if (ePart == BodyPart::BP_CAP)
        {
            // Skip if cap has extend frames (loaded later)
            if (bCapExtendFrame)
                continue;
            // Fall through to regular loading
        }

        // --- BP_WEAPON (i == 11) with gather tool ---
        if (i == static_cast<std::int32_t>(BodyPart::BP_WEAPON))
        {
            if (is_gather_tool_item(nGatherToolID)
                || is_vari_cane_weapon(nGatherToolID))
            {
                auto nRealAction = nAction;
                if (nAction == I(CA::Dead))
                    nRealAction = I(CA::Jump);

                LoadItemAction(
                    nRealAction, nJob,
                    nGatherToolID, aFrame,
                    nWeaponStickerID,
                    nVehicleID, nGhostIndex,
                    bCanEquip,
                    /*bGatherEquip=*/true,
                    bDrawElfEar, nLarknessState, bCashCape,
                    /*nMixHairID=*/0,
                    /*nMixPercent=*/100,
                    /*bCapExtendFrame=*/false);
                continue;
            }
        }

        // --- Regular equipment loading ---
        auto nEquipID = aAvatarHairEquip[i];
        if (nEquipID == 0)
            continue;

        auto nRealAction = nAction;
        if (nAction == I(CA::Dead))
            nRealAction = I(CA::Jump);

        auto nStickerForSlot =
            (ePart == BodyPart::BP_WEAPON) ? nWeaponStickerID : 0;

        LoadItemAction(
            nRealAction, nJob, nEquipID, aFrame,
            nStickerForSlot,
            nVehicleID, nGhostIndex,
            bCanEquip,
            /*bGatherEquip=*/false,
            bDrawElfEar, nLarknessState, bCashCape,
            /*nMixHairID=*/0,
            /*nMixPercent=*/100,
            /*bCapExtendFrame=*/false);
    }

    // --- 4. No cap: set m_sExclVSlot = "H4H5" on all frames ---
    if (aAvatarHairEquip[static_cast<std::int32_t>(BodyPart::BP_CAP)] == 0)
    {
        // StringPool 1940 = "H4H5"
        static const std::string s_sCapHairVSlot = "H4H5";
        for (auto& frame : aFrame)
        {
            frame.SetExclusiveVSlot(s_sCapHairVSlot);
        }
    }

    // --- 5. Zigzag mirroring: [0,1,2,...,n-1] → [0,1,...,n-1,...,1,0] ---
    if (bZigZag && aFrame.size() > 1)
    {
        auto nOrigSize = aFrame.size();
        auto nNewSize = 2 * nOrigSize - 2;
        std::vector<ActionFrame> aZigzag(nNewSize);

        // Copy original frames
        for (std::size_t i = 0; i < nOrigSize; ++i)
            aZigzag[i] = aFrame[i];

        // Mirror frames
        auto dst = nOrigSize;
        auto src = static_cast<std::ptrdiff_t>(nOrigSize) - 2;
        while (dst < nNewSize)
        {
            aZigzag[dst] = aFrame[static_cast<std::size_t>(src)];
            ++dst;
            --src;
        }

        aFrame = std::move(aZigzag);
    }

    // --- 6. Extend frame loading for hair ---
    if (bHairExtendFrame)
    {
        auto nHairID = aAvatarHairEquip[static_cast<std::int32_t>(BodyPart::BP_HAIR)];

        if (!bCapExtendFrame)
        {
            // Only hair has extend frames → use LoadItemActionExtendFrame
            LoadItemActionExtendFrame(
                pHairImgEntry->m_pWeeklyImg,
                nEquipAction, nJob, nHairID, aFrame,
                /*nWeaponStickerID=*/0,
                nVehicleID, nGhostIndex,
                bCanEquip,
                /*bGatherEquip=*/false,
                bDrawElfEar, nLarknessState, bCashCape,
                nMixedHairId, nMixPercent);
        }
        else
        {
            // Both have extend frames → regular load with bCapExtendFrame=true
            LoadItemAction(
                nEquipAction, nJob, nHairID, aFrame,
                /*nWeaponStickerID=*/0,
                nVehicleID, nGhostIndex,
                bCanEquip,
                /*bGatherEquip=*/false,
                bDrawElfEar, nLarknessState, bCashCape,
                nMixedHairId, nMixPercent,
                /*bCapExtendFrame=*/true);
        }
    }

    // --- 7. Extend frame loading for cap ---
    if (bCapExtendFrame)
    {
        auto nCapID = aAvatarHairEquip[static_cast<std::int32_t>(BodyPart::BP_CAP)];

        LoadItemActionExtendFrame(
            pCapImgEntry->m_pWeeklyImg,
            nEquipAction, nJob, nCapID, aFrame,
            /*nWeaponStickerID=*/0,
            nVehicleID, nGhostIndex,
            bCanEquip,
            /*bGatherEquip=*/false,
            bDrawElfEar, nLarknessState, bCashCape,
            /*nMixHairID=*/0,
            /*nMixPercent=*/100);
    }

    return true;
}

// ---------------------------------------------------------------------------
// MergeCharacterSprite  (matches CActionMan::MergeCharacterSprite @ 0x4d7160)
// Converts ActionFrame vector into CharacterActionFrameEntry vector.
// Extracts anchor points (navel, brow, muzzle, hand) and timing data.
// ---------------------------------------------------------------------------
void ActionMan::MergeCharacterSprite(
    const std::vector<ActionFrame>& aFrame,
    std::vector<std::shared_ptr<CharacterActionFrameEntry>>& apFE)
{
    auto nCount = aFrame.size();
    apFE.resize(nCount);

    for (std::size_t i = 0; i < nCount; ++i)
    {
        auto& frame = aFrame[i];
        auto entry = std::make_shared<CharacterActionFrameEntry>();

        // Copy timing and body rect
        entry->tDelay = frame.tDelay;
        entry->rcBody = frame.rcBody;

        // Canvas compositing is deferred — our rendering pipeline handles
        // sprite layering differently from the original COM-based Draw().
        entry->pCanvasUnderFace = nullptr;
        entry->pCanvasOverFace = nullptr;

        // Search all groups for named anchor points.
        // Original CActionFrame::Draw extracts these relative to body center
        // after compositing; here we extract raw positions from groups.
        for (auto& pGroup : frame.m_lGroups)
        {
            if (!pGroup)
                continue;

            for (auto& mapInfo : *pGroup)
            {
                if (mapInfo.sName == "navel")
                    entry->ptNavel = mapInfo.pt;
                else if (mapInfo.sName == "brow")
                    entry->ptBrow = mapInfo.pt;
                else if (mapInfo.sName == "muzzle")
                    entry->ptMuzzle = mapInfo.pt;
                else if (mapInfo.sName == "hand")
                    entry->ptHand = mapInfo.pt;
                else if (mapInfo.sName == "head")
                    entry->ptHead = mapInfo.pt;
                else if (mapInfo.sName == "tail")
                    entry->ptTail = mapInfo.pt;
            }
        }

        apFE[i] = std::move(entry);
    }
}

// ---------------------------------------------------------------------------
// LoadCharacterAction  (matches CActionMan::LoadCharacterAction @ 0x4f1350)
// Outer public wrapper — preprocesses equipment array, handles special cases,
// then delegates to load_character_action + MergeCharacterSprite.
// ---------------------------------------------------------------------------
void ActionMan::LoadCharacterAction(
    std::int32_t nAction,
    std::int32_t nGender,
    std::int32_t nSkin,
    std::int32_t nJob,
    std::int32_t* aAvatarHairEquip,
    std::vector<std::shared_ptr<CharacterActionFrameEntry>>& apFE,
    std::int32_t nWeaponStickerID,
    std::int32_t nVehicleID,
    bool /*bTamingMobTired*/,
    std::int32_t nGhostIndex,
    std::int32_t nGatherToolID,
    bool bDrawElfEar,
    std::int32_t nChangeWeaponLook,
    std::int32_t nLarknessState,
    std::int32_t nPortableChair,
    std::int32_t nMixedHairColor,
    std::int32_t nMixPercent,
    std::int32_t nBattlePvPAvatar)
{
    using CA = CharacterAction;
    constexpr auto I = [](CA a) { return static_cast<std::int32_t>(a); };

    bool bRemoveBody = false;
    bool bInvisibleWeapon = false;

    // --- Portable chair: check removeBody / invisibleWeapon ---
    if (nPortableChair != 0)
    {
        auto pInfo = ItemInfo::GetInstance().GetItemInfo(nPortableChair);
        if (pInfo)
        {
            auto pRemoveBody = pInfo->GetChild("removeBody");
            if (pRemoveBody && pRemoveBody->GetInt(0) != 0)
                bRemoveBody = true;

            auto pInvisWeapon = pInfo->GetChild("invisibleWeapon");
            if (pInvisWeapon && pInvisWeapon->GetInt(0) != 0)
                bInvisibleWeapon = true;
        }
    }

    // --- Build local equipment array b[32] ---
    std::int32_t b[32]{};
    auto nLocalAction = nAction;

    if (nJob == 13000 || nJob == 13100)
    {
        // PB jobs: zero all equipment, clear sticker and skin
        std::memset(b, 0, sizeof(b));
        nWeaponStickerID = 0;
        nSkin = 0;
    }
    else if (bRemoveBody
        || nAction == I(CA::Dead)        // 32
        || nAction == I(CA::DeadRiding)  // 1165
        || (nAction >= 856 && nAction <= 857)  // Setitem3-4
        || is_dance_action(static_cast<CA>(nAction)))
    {
        // removeBody/dead/setitem/dance: zero most, keep hair(0), cap(1),
        // eyeacc(3), earacc(4)
        std::memset(b, 0, sizeof(b));
        b[0] = aAvatarHairEquip[0];  // hair
        b[1] = aAvatarHairEquip[1];  // cap
        b[3] = aAvatarHairEquip[3];  // eyeacc
        b[4] = aAvatarHairEquip[4];  // earacc
    }
    else
    {
        // Normal: copy all, then zero specific slots
        std::memcpy(b, aAvatarHairEquip, sizeof(b));
        b[14] = 0;                        // PETWEAR
        b[2]  = 0;                        // FACEACC
        b[g_anRingBodyPart[0]] = 0;       // RING1 (12)
        b[g_anRingBodyPart[1]] = 0;       // RING2 (13)
        b[g_anRingBodyPart[2]] = 0;       // RING3 (15)
        b[g_anRingBodyPart[3]] = 0;       // RING4 (16)

        // Default coat/pants by gender if missing
        auto nCoat = b[5];
        auto nPants = b[6];

        if (is_long_coat(nCoat) && nPants != 0)
        {
            // Long coat with pants: zero pants unless coat is non-cash and pants is cash
            if (!ItemInfo::GetInstance().IsCashItem(nCoat)
                && ItemInfo::GetInstance().IsCashItem(nPants))
            {
                // Cash overcoat: keep pants, fall through to default coat
            }
            else
            {
                nPants = 0;
                b[6] = 0;
            }
        }

        if (nCoat == 0)
        {
            nCoat = (nGender != 0) ? 1041046 : 1040036;
            b[5] = nCoat;
        }

        if (nPants == 0 && !is_long_coat(nCoat))
        {
            b[6] = (nGender != 0) ? 1061039 : 1060026;
        }
    }

    // --- Transparent item zeroing ---
    if (b[1] == 1002186)   b[1] = 0;   // Transparent cap
    if (b[4] == 1032024)   b[4] = 0;   // Transparent earacc
    if (b[3] == 1022079)   b[3] = 0;   // Transparent faceacc
    if (b[7] == 1072153)   b[7] = 0;   // Transparent shoes
    if (b[8] == 1082102)   b[8] = 0;   // Transparent gloves

    bool bCashCape = false;
    if (b[9] == 1102039)
    {
        bCashCape = true;
        b[9] = 0;                        // Transparent cape
    }

    if (b[10] == 1092067 || b[10] == 1342069)
        b[10] = 0;                        // Transparent shield

    if (nWeaponStickerID == 1702099
        || nWeaponStickerID == 1702190
        || nWeaponStickerID == 1702653)
    {
        b[11] = 0;                        // Transparent sticker
        nWeaponStickerID = 0;
    }

    // --- BattlePvP cap override ---
    if (static_cast<std::uint32_t>(nAction - 1051) <= 0x64u
        && nBattlePvPAvatar == 7)
    {
        b[1] = 1004502;
    }

    // --- Vehicle action remapping ---
    if (is_vehicle(nVehicleID))
    {
        b[10] = 0;  // shield
        b[11] = 0;  // weapon

        // Check for special sit/ride actions that don't remap
        if (nAction != I(CA::Ride2)       // 275
            && nAction != I(CA::Getoff2)  // 276
            && nAction != I(CA::TankRide2)   // 972
            && nAction != I(CA::TankGetoff2) // 971
            && nAction != I(CA::Ladder2)  // 67
            && nAction != I(CA::Rope2))   // 68
        {
            if (nAction == I(CA::Ladder))     // 30
                nLocalAction = I(CA::Rope);   // 31
            else if (nAction == I(CA::Rope))  // 31
                nLocalAction = I(CA::Rope);   // 31
            else
                nLocalAction = I(CA::Sit);    // 29
        }

        // Special vehicle 1932103 → Blink
        if (nVehicleID == 1932103)
            nLocalAction = I(CA::Blink);      // 33

        // PB riding action remap
        if (nJob == 13000 || nJob == 13100)
        {
            if (nAction == I(CA::PinkbeanLadder))    // 1011
                nLocalAction = I(CA::PinkbeanRope);  // 1012
            else if (nAction == I(CA::PinkbeanRope)) // 1012
                nLocalAction = I(CA::PinkbeanRope);  // 1012
            else
                nLocalAction = I(CA::PinkbeanSit);   // 1010
        }

        // TODO: Implement IsSpecialActionRiding / GetActionSpecialActionRiding
    }

    // --- Invisible weapon: zero weapon and cape ---
    if (bInvisibleWeapon)
    {
        b[11] = 0;  // weapon
        b[9]  = 0;  // cape
    }

    // --- Handgun action (108): zero shield and weapon ---
    if (nAction == I(CA::Handgun)) // 108
    {
        b[10] = 0;  // shield
        b[11] = 0;  // weapon
    }

    // --- Ghost index: zero most equipment ---
    if (nGhostIndex != 0
        && nAction != I(CA::Gather0)       // 285
        && nAction != I(CA::Gather1)       // 286
        && nAction != I(CA::MakingSkill0)  // 825
        && nAction != I(CA::MakingSkill1)) // 826
    {
        for (std::int32_t i = 0; i <= 31; ++i)
        {
            if (static_cast<std::uint32_t>(i) >= 2
                && i != 3 && i != 4)
            {
                b[i] = 0;
            }
        }
    }

    // --- Zero sticker for shoot morph actions ---
    if (nWeaponStickerID != 0
        && (nAction == 83 || nAction == 84))
    {
        nWeaponStickerID = 0;
    }

    // --- Compute nGatherToolID fallback from nChangeWeaponLook ---
    auto nFinalGatherToolID = nGatherToolID;
    if (nGatherToolID == 0 && nChangeWeaponLook != 0)
        nFinalGatherToolID = nChangeWeaponLook;

    // --- Compute mixed hair ---
    std::int32_t nMixedHairId = 0;
    if (nMixPercent != 0)
        nMixedHairId = nMixedHairColor + 10 * (b[0] / 10);

    // --- Compute bZigZag from action data ---
    bool bZigZag = false;
    if (nLocalAction >= 0
        && static_cast<std::size_t>(nLocalAction) < ACTIONDATA_COUNT)
    {
        bZigZag = s_aCharacterActionData[static_cast<std::size_t>(nLocalAction)]
                      .m_bZigzag != 0;
    }

    // --- Call inner load_character_action ---
    std::vector<ActionFrame> aCharacterFrame;
    load_character_action(
        nLocalAction, nSkin, nJob, b,
        aCharacterFrame,
        nWeaponStickerID,
        nVehicleID,
        nGhostIndex,
        nFinalGatherToolID,
        bDrawElfEar, nLarknessState,
        bCashCape,
        nMixedHairId, nMixPercent,
        bZigZag, bRemoveBody);

    // --- Convert to CharacterActionFrameEntry ---
    MergeCharacterSprite(aCharacterFrame, apFE);
}

} // namespace ms
