#include "CtrlToolTip.h"

#include "Wnd.h"
#include "app/Application.h"

namespace ms
{

CtrlToolTip::CtrlToolTip() = default;

CtrlToolTip::~CtrlToolTip() = default;

// =============================================================================
// CreateCtrl
// =============================================================================

void CtrlToolTip::CreateCtrl(
    Wnd* pParent, std::uint32_t nCtrlId,
    std::int32_t x, std::int32_t y,
    std::int32_t cx, std::int32_t cy,
    void* pParam)
{
    // CCtrlToolTip::CreateCtrl at 0x7903b0
    if (pParam)
    {
        auto* p = static_cast<CREATEPARAM*>(pParam);
        m_sText = p->sText;
        m_nToolTipWidth = p->nToolTipWidth;
        m_nDelay = p->nDelay;
    }

    CtrlWnd::CreateCtrl(pParent, nCtrlId, x, y, cx, cy, pParam);
}

// =============================================================================
// HitTest
// =============================================================================

auto CtrlToolTip::HitTest(std::int32_t x, std::int32_t y) -> std::int32_t
{
    // CCtrlToolTip::HitTest at 0x7904b0
    auto nRet = CtrlWnd::HitTest(x, y);

    if (!nRet)
    {
        // Mouse left → clear tooltip and reset enter time
        // TODO: CUIToolTip::ClearToolTip(&m_uiToolTip)
        m_nEnterTime = static_cast<std::int32_t>(Application::GetTick());
        return 0;
    }

    // Mouse inside → check if delay has elapsed
    auto tNow = static_cast<std::int32_t>(Application::GetTick());

    if (m_sText.empty())
        return nRet;

    if (tNow - m_nEnterTime < m_nDelay)
        return nRet;

    if (m_nDelay && IsEnabled())
    {
        auto nX = GetAbsLeft() + x + 20;
        auto nY = GetAbsTop() + y + 20;
        // TODO: CUIToolTip::SetToolTip_String_MultiLine(
        //     &m_uiToolTip, nX, nY, m_sText, m_nToolTipWidth, 1)
        (void)nX;
        (void)nY;
    }

    return nRet;
}

// =============================================================================
// OnMouseMove
// =============================================================================

auto CtrlToolTip::OnMouseMove(std::int32_t x, std::int32_t y) -> std::int32_t
{
    // CCtrlToolTip::OnMouseMove at 0x790410
    // Shows tooltip at mouse position when inside hit area

    if (m_sText.empty())
        return 0;

    if (!m_nToolTipWidth)
    {
        // No fixed width → show at cursor position
        auto nX = GetAbsLeft() + x + 20;
        auto nY = GetAbsTop() + y + 20;
        // TODO: CUIToolTip::SetToolTip_String_MultiLine(
        //     &m_uiToolTip, nX, nY, m_sText, 0, 1)
        (void)nX;
        (void)nY;
    }
    else
    {
        // Has fixed width → record enter time for delayed display
        m_nEnterTime = static_cast<std::int32_t>(Application::GetTick());
    }

    return 0;
}

// =============================================================================
// OnMouseEnter
// =============================================================================

void CtrlToolTip::OnMouseEnter(std::int32_t bEnter)
{
    // CCtrlToolTip::OnMouseEnter at 0xb2f1b0
    CtrlWnd::OnMouseEnter(bEnter);

    if (bEnter)
    {
        m_nEnterTime = static_cast<std::int32_t>(Application::GetTick());
    }
    else
    {
        // TODO: CUIToolTip::ClearToolTip(&m_uiToolTip)
    }
}

// =============================================================================
// SetShow
// =============================================================================

void CtrlToolTip::SetShow(std::int32_t bShow)
{
    // CCtrlToolTip::SetShow at 0xb2f1e0
    CtrlWnd::SetShow(bShow);

    if (!bShow)
    {
        // TODO: CUIToolTip::ClearToolTip(&m_uiToolTip)
    }
}

// =============================================================================
// SetText
// =============================================================================

void CtrlToolTip::SetText(const std::string& sText)
{
    // CCtrlToolTip::SetText at 0xb30280
    SetShow(1);
    m_sText = sText;
}

} // namespace ms
