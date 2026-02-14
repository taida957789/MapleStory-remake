#include "WndMan.h"

#include "CtrlWnd.h"
#include "input/InputSystem.h"
#include "stage/Stage.h"

#include <SDL3/SDL.h>
#include <algorithm>

namespace ms
{

namespace
{

// Windows message constants (matching InputSystem.cpp ISMSG encoding)
constexpr std::uint32_t kWM_KEYDOWN      = 0x0100;
constexpr std::uint32_t kWM_MOUSEMOVE    = 0x0200;
constexpr std::uint32_t kWM_LBUTTONDOWN  = 0x0201;
constexpr std::uint32_t kWM_LBUTTONUP    = 0x0202;
constexpr std::uint32_t kWM_RBUTTONDOWN  = 0x0204;
constexpr std::uint32_t kWM_RBUTTONUP    = 0x0205;
constexpr std::uint32_t kWM_MBUTTONDOWN  = 0x0207;
constexpr std::uint32_t kWM_MBUTTONUP    = 0x0208;
constexpr std::uint32_t kWM_MOUSEWHEEL   = 0x020A;

// Virtual key codes
constexpr std::uint32_t kVK_RETURN  = 0x0D;
constexpr std::uint32_t kVK_F4      = 0x73;
constexpr std::uint32_t kVK_F12     = 0x7B;
constexpr std::uint32_t kVK_SCROLL  = 0x91; // PrintScreen in original

} // anonymous namespace

WndMan::WndMan() = default;

WndMan::~WndMan() = default;

// === Input dispatching ===

auto WndMan::ProcessKey(std::uint32_t /*message*/, std::uint32_t wParam,
                        std::int32_t lParam) -> std::int32_t
{
    // CWndMan::ProcessKey at 0x1a6bd60
    // message is always 0x100 (WM_KEYDOWN) from ISMsgProc

    auto nFlag = static_cast<std::uint32_t>(lParam);

    // lParam < 0 → key-up transition (bit 31 set): dispatch to focus
    if (lParam < 0)
        goto dispatch;

    // VK_SCROLL (0x91) = screenshot key in original
    if (wParam == kVK_SCROLL)
    {
        // TODO: CScreenShot::SaveFullScreenToJpg
        return 0;
    }

    // Extended key flag (bit 8 of lParam)
    if ((nFlag & 0x100) != 0)
    {
        switch (wParam)
        {
        case kVK_RETURN: // Alt+Enter → fullscreen toggle
            // TODO: Fullscreen toggle via WzGr2D
            break;
        case kVK_F4: // Alt+F4 → quit
            // TODO: Send quit event or survey request
            break;
        case kVK_F12: // F12 → toggle FPS panel
            // TODO: IWzGr2D::ToggleFpsPanel
            break;
        default:
            goto dispatch;
        }
        return 0;
    }

dispatch:
    if (!m_bEnforcedWaiting)
    {
        if (!m_pFocus)
            SetFocus(static_cast<IUIMsgHandler*>(this));
        if (m_pFocus && m_pFocus->IsEnabled())
            m_pFocus->OnKey(wParam, nFlag);
    }
    return 0;
}

auto WndMan::ProcessMouse(std::uint32_t message, std::uint32_t wParam,
                          std::int32_t /*lParam*/) -> std::int32_t
{
    // CWndMan::ProcessMouse at 0x1a6b2c0 (simplified)
    // Skips: touch handling, drag-drop context, window dragging

    auto& input = InputSystem::GetInstance();

    // Get current cursor position
    Point2D pt;
    input.GetCursorPos(&pt);

    // Update internal cursor tracking
    m_ptCursor.x = pt.x;
    m_ptCursor.y = pt.y;

    // Find the handler under the cursor
    // For now, falls through to WndMan's own handlers (→ g_pStage)
    auto* pHandler = GetHandlerFromPoint(pt.x, pt.y);

    // Handle cursor enter/leave
    if (pHandler != m_pCursorHandler)
    {
        if (m_pCursorHandler)
            m_pCursorHandler->OnMouseEnter(0);
        m_pCursorHandler = pHandler;
        if (pHandler && pHandler->IsEnabled())
            pHandler->OnMouseEnter(1);
    }

    if (!pHandler)
    {
        // Check API capture handler
        if (m_pHandlerAPICapture)
            pHandler = m_pHandlerAPICapture;
        if (!pHandler)
            return 0;
    }

    // Update last mouse message time
    ms_tLastMouseMessage = static_cast<std::uint32_t>(SDL_GetTicks());

    switch (message)
    {
    case kWM_MOUSEMOVE:
        if (!input.IsCursorShown())
            input.ShowCursor(1);
        if (!m_bEnforcedWaiting && pHandler->IsEnabled())
            pHandler->OnMouseMove(pt.x, pt.y);
        return 0;

    case kWM_MOUSEWHEEL:
    {
        // wParam high word = wheel delta (from ISMSG encoding)
        auto nWheelDelta = static_cast<std::int16_t>(
            static_cast<std::uint16_t>(wParam >> 16));
        auto nWheel = static_cast<std::int32_t>(nWheelDelta) / 120;
        if (!m_bEnforcedWaiting && pHandler->IsEnabled())
            pHandler->OnMouseWheel(nWheel, pt.x, pt.y);
        return 0;
    }

    case kWM_LBUTTONDOWN:
    {
        // Update cursor state on left-click
        auto nCursorState = input.GetCursorState();
        if (nCursorState == 7)
            input.SetCursorState(9, false);
        else if (nCursorState == 8)
            input.SetCursorState(10, false);
        else
            input.SetCursorState(12, false);

        if (m_bEnforcedWaiting)
            return 0;

        SetFocus(pHandler);
        break; // fall through to OnMouseButton
    }

    case kWM_LBUTTONUP:
    {
        auto nCursorState = input.GetCursorState();
        if (nCursorState == 9 || nCursorState == 10 || nCursorState == 12)
            input.SetCursorState(-1, false);
        break; // fall through to OnMouseButton
    }

    default:
        break;
    }

    // Dispatch OnMouseButton for button events
    if (!m_bEnforcedWaiting && pHandler->IsEnabled())
        pHandler->OnMouseButton(message, wParam, pt.x, pt.y);

    return 0;
}

// === Focus management ===

void WndMan::SetFocus(IUIMsgHandler* pHandler)
{
    // CWndMan::SetFocus at 0x1a68240 (simplified)
    if ((!pHandler || pHandler->IsEnabled()) && m_pFocus != pHandler)
    {
        // Try OnSetFocus on new handler; abort if it refuses
        if (pHandler && !pHandler->OnSetFocus(1))
            return;

        auto* pOldFocus = m_pFocus;
        m_pFocus = pHandler;

        // Notify old focus that it lost focus
        if (pOldFocus)
            pOldFocus->OnSetFocus(0);

        // If focus is an edit control, disable keyboard acquire
        // (so key events go to the edit field instead of game input)
        InputSystem::GetInstance().SetAcquireKeyboard(1);
    }
}

auto WndMan::GetHandlerFromPoint(
    std::int32_t /*x*/, std::int32_t /*y*/) -> IUIMsgHandler*
{
    // CWndMan::GetHandlerFromPoint at 0x1a689f0
    // Full implementation traverses ms_lpWindow list and hit-tests children.
    // Simplified: return WndMan itself (events flow to g_pStage via overrides)
    // TODO: Implement full window hit-testing
    return static_cast<IUIMsgHandler*>(this);
}

// === Static window list management ===

void WndMan::RemoveWindow(Wnd* pWnd)
{
    auto it = std::find(ms_lpWindow.begin(), ms_lpWindow.end(), pWnd);
    if (it != ms_lpWindow.end())
        ms_lpWindow.erase(it);
}

void WndMan::RemoveUpdateWindow(Wnd* pWnd)
{
    auto it = std::find(ms_lpUpdateWindow.begin(), ms_lpUpdateWindow.end(), pWnd);
    if (it != ms_lpUpdateWindow.end())
        ms_lpUpdateWindow.erase(it);
}

void WndMan::RemoveInvalidatedWindow(Wnd* pWnd)
{
    auto it = std::find(
        ms_lpInvalidatedWindow.begin(), ms_lpInvalidatedWindow.end(), pWnd);
    if (it != ms_lpInvalidatedWindow.end())
        ms_lpInvalidatedWindow.erase(it);
}

void WndMan::s_Update()
{
    // Copy ms_lpWindow into ms_lpUpdateWindow for safe iteration
    ms_lpUpdateWindow.clear();
    ms_lpUpdateWindow = ms_lpWindow;

    while (!ms_lpUpdateWindow.empty())
    {
        auto* pWnd = ms_lpUpdateWindow.front();
        ms_lpUpdateWindow.erase(ms_lpUpdateWindow.begin());

        // Update the window itself
        pWnd->Update();

        // Update all child windows (m_mChildWnd)
        for (auto& [nKey, pChildWnd] : pWnd->m_mChildWnd)
            pChildWnd->Update();
    }

    // TODO: Cursor auto-hide after 60s of inactivity
}

// === Window management ===

void WndMan::UpdateWindowPosition(Wnd* /*pWnd*/)
{
    // TODO: CWndMan::UpdateWindowPosition
}

// === Wnd / IUIMsgHandler overrides ===
// These forward input events to g_pStage (matching IDA decompilation)

auto WndMan::OnSetFocus(std::int32_t bFocus) -> std::int32_t
{
    if (g_pStage)
        return g_pStage->OnSetFocus(bFocus);
    return 0;
}

void WndMan::OnKey(std::uint32_t nKey, std::uint32_t /*nFlag*/)
{
    // CWndMan::OnKey at 0x1a69f40 → g_pStage->OnKey
    // Bridge: IUIMsgHandler::OnKey(nKey, nFlag) → Stage::OnKeyDown(nKey)
    if (g_pStage)
        g_pStage->OnKeyDown(static_cast<std::int32_t>(nKey));
}

void WndMan::OnMouseButton(std::uint32_t nType, std::uint32_t /*nFlag*/,
                           std::int32_t x, std::int32_t y)
{
    // CWndMan::OnMouseButton at 0x1a69f80 → g_pStage->OnMouseButton
    // Bridge: nType is Windows message type → Stage::OnMouseDown/OnMouseUp
    if (!g_pStage)
        return;

    switch (nType)
    {
    case kWM_LBUTTONDOWN:
        g_pStage->OnMouseDown(x, y, 1);
        break;
    case kWM_RBUTTONDOWN:
        g_pStage->OnMouseDown(x, y, 3);
        break;
    case kWM_MBUTTONDOWN:
        g_pStage->OnMouseDown(x, y, 2);
        break;
    case kWM_LBUTTONUP:
        g_pStage->OnMouseUp(x, y, 1);
        break;
    case kWM_RBUTTONUP:
        g_pStage->OnMouseUp(x, y, 3);
        break;
    case kWM_MBUTTONUP:
        g_pStage->OnMouseUp(x, y, 2);
        break;
    default:
        break;
    }
}

auto WndMan::OnMouseMove(std::int32_t x, std::int32_t y) -> std::int32_t
{
    // CWndMan::OnMouseMove at 0x1a69fa0 → g_pStage->OnMouseMove
    if (g_pStage)
    {
        g_pStage->OnMouseMove(x, y);
        return 1;
    }
    return 0;
}

auto WndMan::OnMouseWheel(std::int32_t /*nDelta*/,
                          std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    // CWndMan::OnMouseWheel at 0x1a69fc0 → g_pStage->OnMouseWheel
    // TODO: Stage doesn't have OnMouseWheel yet
    return 0;
}

void WndMan::OnMouseEnter(std::int32_t /*bEnter*/)
{
    // CWndMan::OnMouseEnter at 0x1a69fe0
    // Forwards to g_pStage->OnMouseEnter in original
    // No-op for now: Stage doesn't have OnMouseEnter
}

void WndMan::Update()
{
    s_Update();
}

} // namespace ms
