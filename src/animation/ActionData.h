#pragma once

#include "util/Point.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ms
{

/// Matches ACTIONDATA from the client (__cppobj).
class ActionData
{
public:
    /// Matches ACTIONDATA::PIECE.
    struct Piece
    {
        std::int32_t m_nAction{0};
        std::int32_t m_nFrameIdx{0};
        std::int32_t m_nFrameDelay{150};
        std::int32_t m_bShowFace{0};
        std::int32_t m_bFlip{0};
        std::int32_t m_nRotate{0};
        Point2D m_ptMove{0, 0};
        std::int32_t m_bWeapon2{0};
        std::int32_t m_nEmotion{-1};
        bool m_bNoWeapon{false};
        std::uint8_t m_nAlpha{255};
        std::int32_t m_nDirectionFix{0};
    };

    std::string m_sName;
    std::int32_t m_bZigzag{0};
    std::int32_t m_bPieced{0};
    std::int32_t m_nTotalDelay{0};
    std::int32_t m_nEventDelay{0};
    std::string m_sSubAvatarAction;
    std::int32_t m_nRepeatFrame{0};
    std::vector<Piece> m_aPieces;

    ActionData() = default;
    ActionData(std::int32_t bZigzag, std::int32_t bPieced, std::string sName);
};

inline constexpr std::size_t ACTIONDATA_COUNT = 1310;

extern ActionData s_aCharacterActionData[ACTIONDATA_COUNT];

} // namespace ms
