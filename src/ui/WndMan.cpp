#include "WndMan.h"
#include "UIElement.h"
#include "stage/Stage.h"

namespace ms
{

WndMan::WndMan() = default;

WndMan::~WndMan() = default;

void WndMan::Initialize()
{
    m_pStage = nullptr;
    m_pFocus.reset();
    m_lpCapture.reset();
    m_uiManager.Clear();
}

void WndMan::Shutdown()
{
    m_pStage = nullptr;
    m_pFocus.reset();
    m_lpCapture.reset();
    m_uiManager.Clear();
}

void WndMan::SetStage(Stage* pStage)
{
    // Clear focus/capture when changing stages
    if (m_pStage != pStage)
    {
        m_pFocus.reset();
        m_lpCapture.reset();
    }
    m_pStage = pStage;
}

// ========== Input Event Handlers ==========

void WndMan::OnMouseEnter(bool bEnter)
{
    // Based on CWndMan::OnMouseEnter
    // Forwards to g_pStage->OnMouseEnter
    m_bMouseInWindow = bEnter;

    // Note: Stage doesn't have OnMouseEnter in current implementation
    // Could be added if needed
}

void WndMan::OnMouseMove(std::int32_t x, std::int32_t y)
{
    // Based on CWndMan::OnMouseMove
    m_nLastMouseX = x;
    m_nLastMouseY = y;

    // Check capture first (CWndMan::GetHandlerFromPoint)
    if (m_lpCapture)
    {
        if (m_lpCapture->IsVisible())
        {
            m_lpCapture->OnMouseMove(x, y);
            return;
        }
    }

    // Check global UI
    if (m_uiManager.OnMouseMove(x, y))
    {
        return;
    }

    // Forward to stage
    if (m_pStage)
    {
        m_pStage->OnMouseMove(x, y);
    }
}

void WndMan::OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button)
{
    // Based on CWndMan::OnLButtonDown/OnRButtonDown

    // Check capture first
    if (m_lpCapture)
    {
        if (m_lpCapture->IsVisible() && m_lpCapture->IsEnabled())
        {
            m_lpCapture->OnMouseDown(x, y, button);
            return;
        }
    }

    // Check global UI
    if (m_uiManager.OnMouseDown(x, y, button))
    {
        return;
    }

    // Forward to stage
    if (m_pStage)
    {
        m_pStage->OnMouseDown(x, y, button);
    }
}

void WndMan::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button)
{
    // Based on CWndMan::OnLButtonUp/OnRButtonUp

    // Check capture first
    if (m_lpCapture)
    {
        if (m_lpCapture->IsVisible())
        {
            m_lpCapture->OnMouseUp(x, y, button);
            return;
        }
    }

    // Check global UI
    if (m_uiManager.OnMouseUp(x, y, button))
    {
        return;
    }

    // Forward to stage
    if (m_pStage)
    {
        m_pStage->OnMouseUp(x, y, button);
    }
}

void WndMan::OnMouseDoubleClick(std::int32_t x, std::int32_t y, std::int32_t button)
{
    // Based on CWndMan::OnLButtonDblClk/OnRButtonDblClk
    // Currently treat as regular click - can be enhanced later
    OnMouseDown(x, y, button);
}

void WndMan::OnMouseWheel(std::int32_t x, std::int32_t y, float delta)
{
    // Based on CWndMan::OnMouseWheel
    // Note: Current UI elements don't have wheel handler
    // Forward to stage if needed
    (void)x;
    (void)y;
    (void)delta;
}

void WndMan::OnKeyDown(std::int32_t keyCode)
{
    // Based on CWndMan::OnKeyDown

    // Dispatch to focused element first
    if (m_pFocus && m_pFocus->IsVisible() && m_pFocus->IsEnabled())
    {
        m_pFocus->OnKeyDown(keyCode);
        return;
    }

    // Check global UI
    m_uiManager.OnKeyDown(keyCode);

    // Forward to stage
    if (m_pStage)
    {
        m_pStage->OnKeyDown(keyCode);
    }
}

void WndMan::OnKeyUp(std::int32_t keyCode)
{
    // Based on CWndMan::OnKeyUp

    // Dispatch to focused element first
    if (m_pFocus && m_pFocus->IsVisible() && m_pFocus->IsEnabled())
    {
        m_pFocus->OnKeyUp(keyCode);
        return;
    }

    // Check global UI
    m_uiManager.OnKeyUp(keyCode);

    // Forward to stage
    if (m_pStage)
    {
        m_pStage->OnKeyUp(keyCode);
    }
}

void WndMan::OnTextInput(const std::string& text)
{
    // Based on CWndMan::OnIMEChar

    // Dispatch to focused element
    m_uiManager.OnTextInput(text);

    // Forward to stage
    if (m_pStage)
    {
        m_pStage->OnTextInput(text);
    }
}

void WndMan::OnSetFocus(bool bFocus)
{
    // Based on CWndMan::OnSetFocus
    // Notify stage of window focus change
    if (m_pStage)
    {
        m_pStage->OnSetFocus(bFocus ? 1 : 0);
    }
}

// ========== Focus Management ==========

void WndMan::SetFocus(std::shared_ptr<UIElement> pElement)
{
    // Based on CWndMan::SetFocusImp

    // Skip if setting focus to the same element
    if (m_pFocus == pElement)
    {
        return;
    }

    // Check if new element accepts focus (or if clearing focus)
    if (pElement && !pElement->IsEnabled())
    {
        return;
    }

    // Try to set focus on new element first
    if (pElement)
    {
        if (!pElement->OnSetFocus(true))
        {
            // Element rejected focus
            return;
        }
    }

    // Notify old element it's losing focus
    if (m_pFocus)
    {
        m_pFocus->OnSetFocus(false);
    }

    // Update focus pointer
    m_pFocus = std::move(pElement);
}

// ========== Capture Management ==========

void WndMan::SetCapture(std::shared_ptr<UIElement> pElement)
{
    // Based on setting CWndMan::m_lpCapture
    m_lpCapture = std::move(pElement);
}

void WndMan::ReleaseCapture(UIElement* pElement)
{
    // Based on CWndMan::ReleaseCaptureWnd
    if (m_lpCapture.get() == pElement)
    {
        m_lpCapture.reset();
    }
}

// ========== Global UI Management ==========

void WndMan::Update()
{
    m_uiManager.Update();
}

void WndMan::Draw()
{
    m_uiManager.Draw();
}

} // namespace ms
