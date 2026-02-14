#pragma once

#include <cstdint>

namespace ms
{

/// Body part equipment slots.
/// Based on BodyPartEnum from the C# reference and decompiled CAvatar.
enum class BodyPart : std::int32_t
{
    BP_HAIR = 0,
    BP_CAP = 1,
    BP_FACEACC = 2,
    BP_EYEACC = 3,
    BP_EARACC = 4,
    BP_CLOTHES = 5,
    BP_PANTS = 6,
    BP_SHOES = 7,
    BP_GLOVES = 8,
    BP_CAPE = 9,
    BP_SHIELD = 10,
    BP_WEAPON = 11,
    BP_RING1 = 12,
    BP_RING2 = 13,
    BP_PETWEAR = 14,
    BP_RING3 = 15,
    BP_RING4 = 16,
    BP_PENDANT = 17,
    BP_TAMINGMOB = 18,
    BP_SADDLE = 19,
    BP_MOBEQUIP = 20,
    BP_MEDAL = 21,
    BP_BELT = 22,
    BP_SHOULDER = 23,
    BP_PETWEAR2 = 24,
    BP_PETWEAR3 = 25,
    BP_CHARMACC = 26,
    BP_ANDROID = 27,
    BP_MACHINEHEART = 28,
    BP_BADGE = 29,
    BP_EMBLEM = 30,
    BP_COUNT = 31,
};

/// Ring/pendant body part indices used by LoadCharacterAction's equipment zeroing.
/// Indices [1]=13(RING2), [2]=15(RING3), [3]=16(RING4) are used for clearing.
inline constexpr std::int32_t g_anRingBodyPart[] = {12, 13, 15, 16, 17, 31, 0, 30};

} // namespace ms
