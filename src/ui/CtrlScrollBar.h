#pragma once

#include "CtrlWnd.h"

#include <cstdint>
#include <string>

namespace ms
{

/**
 * @brief Scrollbar control
 *
 * Based on CCtrlScrollBar from the original MapleStory client (v1029).
 * Inherits from CtrlWnd.
 */
class CtrlScrollBar : public CtrlWnd
{
public:
    /// Creation parameter (Ztl_bstr_t sUOL + wOption)
    struct CREATEPARAM
    {
        std::string sUOL;
        std::uint16_t wOption{};
    };

    /// Scrollbar image indices
    enum : std::int32_t
    {
        SCR_IMAGE_EN_PREV0   = 0x0,
        SCR_IMAGE_EN_PREV1   = 0x1,
        SCR_IMAGE_EN_PREV2   = 0x2,
        SCR_IMAGE_EN_NEXT0   = 0x3,
        SCR_IMAGE_EN_NEXT1   = 0x4,
        SCR_IMAGE_EN_NEXT2   = 0x5,
        SCR_IMAGE_EN_BASE    = 0x6,
        SCR_IMAGE_EN_THUMB0  = 0x7,
        SCR_IMAGE_EN_THUMB1  = 0x8,
        SCR_IMAGE_DIS_PREV   = 0x9,
        SCR_IMAGE_DIS_NEXT   = 0xA,
        SCR_IMAGE_DIS_BASE   = 0xB,
    };

    CtrlScrollBar();
    ~CtrlScrollBar() override;
};

} // namespace ms
