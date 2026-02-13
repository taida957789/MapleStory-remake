#pragma once

#include <cstdint>

namespace ms
{

enum class MoveActionType : std::int32_t
{
    Walk = 0x1,
    Move = 0x1,
    Stand = 0x2,
    Jump = 0x3,
    Alert = 0x4,
    Prone = 0x5,
    Fly1 = 0x6,
    Ladder = 0x7,
    Rope = 0x8,
    Dead = 0x9,
    Sit = 0xA,
    Stand0 = 0xB,
    Hungry = 0xC,
    Rest0 = 0xD,
    Rest1 = 0xE,
    Hang = 0xF,
    Chase = 0x10,
    Fly2 = 0x11,
    Fly2Move = 0x12,
    Dash2 = 0x13,
    RocketBooster = 0x14,
    TeslaCoilTriangle = 0x15,
    Backwalk = 0x16,
    Bladestance = 0x17,
    Fevermode = 0x18,
    No = 0x19,
};

} // namespace ms
