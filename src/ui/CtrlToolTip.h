#pragma once

#include "CtrlWnd.h"
#include "UIToolTip.h"

#include <cstdint>
#include <string>

namespace ms
{

/**
 * @brief Tooltip control
 *
 * Based on CCtrlToolTip from the original MapleStory client (v1029).
 *
 * Original: __cppobj CCtrlToolTip : CCtrlWnd
 *
 * Displays a tooltip after a configurable delay when the mouse
 * hovers over the control's hit area. Uses CUIToolTip internally
 * to render the tooltip text.
 */
class CtrlToolTip : public CtrlWnd
{
public:
    /// Creation parameters (CCtrlToolTip::CREATEPARAM at 0xb2f090)
    struct CREATEPARAM
    {
        std::string sText;
        std::int32_t nToolTipWidth{};
        std::int32_t nDelay{};
    };

    CtrlToolTip();
    ~CtrlToolTip() override;

    // === CtrlWnd overrides ===
    void CreateCtrl(
        Wnd* pParent, std::uint32_t nCtrlId,
        std::int32_t x, std::int32_t y,
        std::int32_t cx, std::int32_t cy,
        void* pParam) override;
    auto HitTest(
        std::int32_t x, std::int32_t y) -> std::int32_t override;

    // === IUIMsgHandler overrides ===
    auto OnMouseMove(
        std::int32_t x, std::int32_t y) -> std::int32_t override;
    void OnMouseEnter(std::int32_t bEnter) override;
    void SetShow(std::int32_t bShow) override;

    // === CtrlToolTip-specific ===

    /// Set tooltip text and show the control (0xb30280)
    void SetText(const std::string& sText);

    /// Get tooltip text (0x113b7f0)
    [[nodiscard]] auto GetText() const -> const std::string& { return m_sText; }

    /// Get tooltip width (0x113a330)
    [[nodiscard]] auto GetToolTipWidth() const noexcept -> std::int32_t
    {
        return m_nToolTipWidth;
    }

private:
    UIToolTip m_uiToolTip;
    std::string m_sText;
    std::int32_t m_nToolTipWidth{};
    std::int32_t m_nDelay{};
    std::int32_t m_nEnterTime{};
    std::int32_t m_bTime{};
};

} // namespace ms
