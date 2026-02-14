#include "CtrlButton.h"

#include "Wnd.h"
#include "WndMan.h"
#include "input/InputSystem.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "app/Application.h"
#include "wz/WzResMan.h"

#include <algorithm>

namespace ms
{

namespace
{

/// Windows message constants
constexpr std::uint32_t kWM_LBUTTONDOWN  = 0x0201;
constexpr std::uint32_t kWM_LBUTTONUP    = 0x0202;
constexpr std::uint32_t kWM_LBUTTONDBLCLK = 0x0203;

/// Virtual key codes
constexpr std::uint32_t kVK_SPACE = 0x20;

/// Button click notification code
constexpr std::uint32_t kBN_CLICKED = 100;

/// WZ sub-property names for button states
constexpr const char* kStateName[CtrlButton::kNumStates] = {
    "normal",           // [0]
    "pressed",          // [1]
    "disabled",         // [2]
    "mouseOver",        // [3]
    "selected",         // [4]  (checked)
    "selectedMouseOver" // [5]  (checkedMouseOver)
};
constexpr const char* kFocusFrameName = "keyFocused";

/// Get current update time for animation timing
auto get_update_time() -> std::uint32_t;

} // anonymous namespace

// =============================================================================
// Construction / Destruction
// =============================================================================

CtrlButton::CtrlButton() = default;

CtrlButton::~CtrlButton() = default;

// =============================================================================
// CreateCtrl / Destroy
// =============================================================================

void CtrlButton::CreateCtrl(
    Wnd* pParent, std::uint32_t nCtrlId,
    std::int32_t x, std::int32_t y,
    std::int32_t /*cx*/, std::int32_t /*cy*/,
    void* pParam)
{
    // CCtrlButton::CreateCtrl at 0x760a90
    auto* p = static_cast<CREATEPARAM*>(pParam);

    // Copy CREATEPARAM flags
    m_bAcceptFocus        = p->bAcceptFocus;
    m_bDrawBack           = p->bDrawBack;
    m_bOnMoveInvalidRect  = p->bOnMoveInvalidRect;
    m_bAnimateOnce        = p->bAnimateOnce;
    m_bDisableTooltip     = p->bDisableTooltip;
    m_bShowTooltipAnystate = p->bShowTooltipAnystate;
    m_bToggle             = p->bToggle;
    m_bSetOrigin          = p->bSetOrigin;

    // Reset runtime state
    m_bMouseEnter         = 0;
    m_bMouseEnterForTooltip = 0;
    m_nDecClickArea       = 0;
    m_bPressed            = 0;
    m_bPressedByKey       = 0;
    m_bKeyFocused         = 0;
    m_bChecked            = 0;
    m_bCursorDefault      = 0;
    m_bToolTip            = 0;
    m_bToolTipZ           = 0;
    m_bVisible            = 1;
    m_sToolTipTitle.clear();
    m_sToolTipDesc.clear();

    // TODO: Create alpha animation vector (IWzVector2D)
    // m_pAlpha = PcCreateObject("BrokenLine");
    // m_pAlpha->RelMove(nAlpha, 0);

    // Load button images from WZ path
    if (!p->sUOL.empty())
        SetButtonImage(p->sUOL);

    // Compute size from largest canvas across all states
    std::int32_t width = 0;
    std::int32_t height = 0;

    for (std::int32_t i = 0; i < kNumStates; ++i)
    {
        if (!m_apPropButton[i])
            continue;

        auto nFrames = static_cast<std::int32_t>(m_apPropButton[i]->GetChildCount());
        for (std::int32_t j = 0; j < nFrames; ++j)
        {
            auto frame = m_apPropButton[i]->GetChild(std::to_string(j));
            if (!frame) continue;

            auto canvas = frame->GetCanvas();
            if (!canvas) continue;

            if (canvas->GetWidth() > width)
                width = canvas->GetWidth();
            if (canvas->GetHeight() > height)
                height = canvas->GetHeight();
        }
    }

    m_bSelfDisable = 0;

    // Call base class creation with computed size
    CtrlWnd::CreateCtrl(pParent, nCtrlId, x, y, width, height, pParam);
}

void CtrlButton::Destroy()
{
    // CCtrlButton::Destroy at 0x75c1f0
    if (m_pButtonEntered == this)
        m_pButtonEntered = nullptr;

    m_pLayerFocusFrame.reset();
    ClearToolTip();

    CtrlWnd::Destroy();
}

// =============================================================================
// SetButtonImage
// =============================================================================

void CtrlButton::SetButtonImage(const std::string& sUOL)
{
    // CCtrlButton::SetButtonImage at 0x75d930
    // Loads button state properties from WZ path

    auto& resMan = WzResMan::GetInstance();
    auto prop = resMan.GetProperty(sUOL);
    if (!prop)
        return;

    // Load each button state sub-property
    for (std::int32_t i = 0; i < kNumStates; ++i)
        m_apPropButton[i] = prop->GetChild(kStateName[i]);

    // Fill missing states with normal state
    for (std::int32_t i = 1; i < kNumStates; ++i)
    {
        if (!m_apPropButton[i])
            m_apPropButton[i] = m_apPropButton[0];
    }

    // Load focus frame
    m_pPropFocusFrame = prop->GetChild(kFocusFrameName);

    // Initialize display
    ChangeDisplayState(STATE_NORMAL);
}

// =============================================================================
// GetButtonCanvas
// =============================================================================

auto CtrlButton::GetButtonCanvas() const -> std::shared_ptr<WzCanvas>
{
    // CCtrlButton::GetButtonCanvas at 0x75e100
    if (m_nDisplayState < 0 || m_nDisplayState >= kNumStates)
        return nullptr;

    auto& prop = m_apPropButton[m_nDisplayState];
    if (!prop)
        return nullptr;

    // Get the canvas at the current animation frame
    auto frame = prop->GetChild(std::to_string(m_nDisplayFrame));
    if (!frame)
    {
        // Try frame 0
        frame = prop->GetChild("0");
        if (!frame)
            return nullptr;
    }

    return frame->GetCanvas();
}

// =============================================================================
// ChangeDisplayState / ChangeDisplayFrame
// =============================================================================

void CtrlButton::ChangeDisplayState(std::int32_t nState)
{
    // CCtrlButton::ChangeDisplayState at 0x75e270
    if (nState < 0 || nState > 5)
        return;

    // Override with checked state if applicable
    if (m_bChecked && m_apPropButton[STATE_CHECKED])
        nState = STATE_CHECKED;

    // Handle tooltip on mouseOver state
    if (nState == STATE_MOUSEOVER && !m_bDisableTooltip)
        _SetToolTip();
    else
        ClearToolTip();

    m_nDisplayState = nState;
    m_nDisplayFrame = 0;
    m_dwDisplayStarted = get_update_time();

    // Update animation frame count for new state
    if (nState < kNumStates && m_apPropButton[nState])
        m_nAniCount = static_cast<std::int32_t>(
            m_apPropButton[nState]->GetChildCount());
    else
        m_nAniCount = 0;

    // Read animation delay from canvas property
    auto canvas = GetButtonCanvas();
    if (canvas)
    {
        auto& prop = m_apPropButton[m_nDisplayState];
        if (prop)
        {
            auto frame = prop->GetChild(std::to_string(m_nDisplayFrame));
            if (frame)
            {
                auto delayProp = frame->GetChild("delay");
                m_nAniDelay = delayProp ? delayProp->GetInt(120) : 120;
            }
        }
    }

    // Invalidate parent for redraw
    if (m_pParent)
        m_pParent->InvalidateRect(nullptr);
}

void CtrlButton::ChangeDisplayFrame()
{
    // CCtrlButton::ChangeDisplayFrame at 0x75e430
    auto nextFrame = m_nDisplayFrame + 1;

    if (nextFrame >= m_nAniCount && m_bAnimateOnce)
        return;

    m_nDisplayFrame = nextFrame % m_nAniCount;
    m_dwDisplayStarted = get_update_time();

    // Read delay for new frame
    if (m_apPropButton[m_nDisplayState])
    {
        auto frame = m_apPropButton[m_nDisplayState]->GetChild(
            std::to_string(m_nDisplayFrame));
        if (frame)
        {
            auto delayProp = frame->GetChild("delay");
            m_nAniDelay = delayProp ? delayProp->GetInt(120) : 120;
        }
    }

    if (m_pParent)
        m_pParent->InvalidateRect(nullptr);
}

// =============================================================================
// Update
// =============================================================================

void CtrlButton::Update()
{
    // CCtrlButton::Update at 0x761410
    CtrlWnd::Update();

    // If this button is entered but was released externally
    if (m_pButtonEntered != this && m_bPressed)
    {
        if (!InputSystem::GetInstance().IsKeyPressed(1))
        {
            m_bPressed = 0;
            m_bPressedByKey = 0;
            ChangeDisplayState(STATE_NORMAL);
            return;
        }
    }

    // Advance animation if multi-frame
    if (m_nAniCount > 1)
    {
        auto tNow = get_update_time();
        if (tNow - m_dwDisplayStarted > static_cast<std::uint32_t>(m_nAniDelay))
            ChangeDisplayFrame();
    }
}

// =============================================================================
// Draw
// =============================================================================

void CtrlButton::Draw(
    std::int32_t rx, std::int32_t ry, const Rect* /*pRect*/)
{
    // CCtrlButton::Draw at 0x760f90 (simplified)
    if (!m_bVisible)
        return;

    auto canvas = GetButtonCanvas();
    if (!canvas)
        return;

    // Center the canvas within the control bounds
    auto nOffX = (m_width - canvas->GetWidth()) / 2;
    auto nOffY = (m_height - canvas->GetHeight()) / 2;

    // TODO: Actually render via WzGr2DLayer/Canvas system
    // The original uses:
    //   alpha = m_pAlpha->get_rx()
    //   pCanvas->Copy(rx + nOffX, ry + nOffY, pButtonCanvas, alpha)
    (void)nOffX;
    (void)nOffY;
    (void)rx;
    (void)ry;
}

// =============================================================================
// HitTest
// =============================================================================

auto CtrlButton::HitTest(std::int32_t x, std::int32_t y) -> std::int32_t
{
    // CCtrlButton::HitTest at 0x761480
    if (!isInButton(x, y))
    {
        if (m_bMouseEnterForTooltip && m_bShowTooltipAnystate)
        {
            m_bMouseEnterForTooltip = 0;
            ClearToolTip();
        }
        return 0;
    }

    // Show tooltip for disabled buttons if configured
    if (m_bDisableTooltip && !IsEnabled())
    {
        if (m_pParent)
            m_pParent->ClearToolTip();
        _SetToolTip();
    }

    if (!m_bMouseEnterForTooltip && m_bShowTooltipAnystate)
    {
        m_bMouseEnterForTooltip = 1;
        if (m_pParent)
            m_pParent->ClearToolTip();
        _SetToolTip();
    }

    return 1;
}

auto CtrlButton::isInButton(std::int32_t x, std::int32_t y) -> bool
{
    // CCtrlButton::isInButton at 0x75e580
    // Check custom click area first
    if (m_rcAreaForClick.left || m_rcAreaForClick.top ||
        m_rcAreaForClick.right || m_rcAreaForClick.bottom)
    {
        return x >= m_rcAreaForClick.left && x < m_rcAreaForClick.right &&
               y >= m_rcAreaForClick.top && y < m_rcAreaForClick.bottom;
    }

    // Pixel-perfect checking
    if (m_bPixelAreaCheck)
    {
        auto canvas = GetButtonCanvas();
        if (!canvas)
            return false;
        // TODO: Check pixel alpha at (x, y) in canvas
        // For now, fall through to bounds check
    }

    // Standard bounds check with dec area
    return m_nDecClickArea <= x &&
           x < m_width - m_nDecClickArea &&
           m_nDecClickArea <= y &&
           y < m_height - m_nDecClickArea;
}

// =============================================================================
// Mouse event handlers
// =============================================================================

void CtrlButton::OnMouseButton(
    std::uint32_t nType, std::uint32_t /*nFlag*/,
    std::int32_t x, std::int32_t y)
{
    // CCtrlButton::OnMouseButton at 0x75b9a0
    if (!isInButton(x, y))
        return;

    if (nType == kWM_LBUTTONDOWN || nType == kWM_LBUTTONDBLCLK)
    {
        MouseDown();
    }
    else if (nType == kWM_LBUTTONUP)
    {
        InputSystem::GetInstance().SetCursorState(4, false);
        MouseUp();
    }
}

auto CtrlButton::OnMouseMove(std::int32_t x, std::int32_t y) -> std::int32_t
{
    // CCtrlButton::OnMouseMove at 0x75b390
    if (isInButton(x, y))
    {
        if (!m_bMouseEnter)
        {
            m_bMouseEnter = 1;
            MouseEnter(1, 0);
            if (m_bOnMoveInvalidRect)
                m_pParent->InvalidateRect(nullptr);
        }
    }
    else if (m_bMouseEnter)
    {
        m_bMouseEnter = 0;
        MouseEnter(0, 0);
        if (m_bOnMoveInvalidRect)
            ChangeDisplayState(STATE_NORMAL);
        if (m_bAnimateOnce)
            ChangeDisplayState(STATE_NORMAL);
    }

    return 0;
}

void CtrlButton::OnMouseEnter(std::int32_t bEnter)
{
    // CCtrlButton::OnMouseEnter at 0x75b420
    if (!bEnter)
    {
        m_bMouseEnter = 0;
        MouseEnter(0, 0);
    }
    if (m_bToggle && m_pParent)
        m_pParent->OnSetFocus(0);
}

auto CtrlButton::OnMouseWheel(
    std::int32_t nDelta,
    std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    // CCtrlButton::OnMouseWheel at 0x75bdb0
    // Forwards to parent via OnMouseWheel
    if (m_pParent)
        return m_pParent->OnMouseWheel(nDelta, 0, 0);
    return 0;
}

void CtrlButton::MouseDown()
{
    // CCtrlButton::MouseDown at 0x75b6e0
    if (!IsEnabled())
        return;

    m_bPressed = 1;
    m_bPressedByKey = 0;
    ChangeDisplayState(STATE_PRESSED);
}

void CtrlButton::MouseUp()
{
    // CCtrlButton::MouseUp at 0x75d0c0
    if (!IsEnabled() || !m_bPressed)
        return;

    m_bPressed = 0;
    m_bPressedByKey = 0;

    // Toggle checked state if toggle button
    if (m_bToggle)
    {
        m_bChecked = m_bChecked ? 0 : 1;
        m_pParent->InvalidateRect(nullptr);
        ChangeDisplayState(STATE_NORMAL);
    }

    // TODO: play_ui_sound(StringPool::GetString(0x9A4)) — button click sound

    // Show hover state (still under cursor)
    ChangeDisplayState(STATE_MOUSEOVER);

    // Notify parent of click
    if (m_pParent)
        m_pParent->OnChildNotify(m_nCtrlId, kBN_CLICKED, 0);
}

void CtrlButton::MouseEnter(std::int32_t bEnter, std::int32_t bForced)
{
    // CCtrlButton::MouseEnter at 0x75cf60
    if (!IsEnabled() || m_bKeyFocused)
        return;

    m_pButtonEntered = bEnter ? this : nullptr;

    // Set cursor state on enter (non-forced)
    if (bEnter && !bForced)
        InputSystem::GetInstance().SetCursorState(4, false);

    if (m_bPressedByKey)
        return;

    if (m_bPressed)
    {
        ChangeDisplayState(bEnter ? STATE_PRESSED : STATE_NORMAL);
    }
    else if (bEnter)
    {
        ChangeDisplayState(STATE_MOUSEOVER);
        // TODO: play_ui_sound(StringPool::GetString(0x9A5)) — hover sound
    }
    else
    {
        ChangeDisplayState(STATE_NORMAL);
    }
}

// =============================================================================
// Keyboard handling
// =============================================================================

void CtrlButton::OnKey(std::uint32_t nKey, std::uint32_t nFlag)
{
    // CCtrlButton::OnKey at 0x75ce20
    auto lParam = static_cast<std::int32_t>(nFlag);

    if (lParam >= 0)
    {
        // Key down
        if (nKey == kVK_SPACE)
        {
            if (!m_bPressed)
            {
                m_bPressedByKey = 0;
                m_bPressed = 1;
                MouseEnter(1, 0);
            }
            return;
        }
        // Forward other keys to parent
        if (m_pParent)
            m_pParent->OnKey(nKey, nFlag);
        return;
    }

    // Key up
    if (nKey != kVK_SPACE || !m_bPressed)
    {
        if (m_pParent)
            m_pParent->OnKey(nKey, nFlag);
        return;
    }

    // Space released → trigger click
    m_bPressedByKey = 0;
    m_bPressed = 0;
    // TODO: play_ui_sound(StringPool::GetString(0x9A4))
    MouseEnter(0, 0);

    // Notify parent of click
    if (m_pParent)
        m_pParent->OnChildNotify(m_nCtrlId, kBN_CLICKED, 0);
}

// =============================================================================
// SetEnable / SetShow
// =============================================================================

void CtrlButton::SetEnable(std::int32_t bEnable)
{
    // CCtrlButton::SetEnable at 0x75b570
    if (IsEnabled() != bEnable)
    {
        m_bPixelAreaCheck = false;
        m_bPressed = 0;
        ChangeDisplayState(bEnable ? STATE_NORMAL : STATE_DISABLED);
    }
    CtrlWnd::SetEnable(bEnable);
}

void CtrlButton::SetShow(std::int32_t bShow)
{
    // CCtrlButton::SetShow at 0x75bd20 (simplified)
    if (bShow)
    {
        // Check if cursor is inside → auto-enter
        // TODO: get cursor pos relative to control and HitTest
    }
    else
    {
        MouseEnter(0, 0);
    }
    CtrlWnd::SetShow(bShow);
}

// =============================================================================
// State setters
// =============================================================================

void CtrlButton::SetCheck(std::int32_t bCheck)
{
    // CCtrlButton::SetCheck at 0x75b490
    if (m_bChecked != bCheck)
    {
        m_bChecked = bCheck;
        ChangeDisplayState(m_nDisplayState);
    }
}

void CtrlButton::SetPressed(std::int32_t bPressed)
{
    // CCtrlButton::SetPressed at 0x75b4d0
    if (m_bPressed != bPressed)
    {
        m_bPressed = bPressed;
        ChangeDisplayState(bPressed ? STATE_PRESSED : STATE_NORMAL);
    }
}

void CtrlButton::SetChecked(std::int32_t bChecked)
{
    // CCtrlButton::SetChecked at 0x75b4f0
    m_bChecked = bChecked;
    if (bChecked)
        ChangeDisplayState(STATE_CHECKED);
    else
        ChangeDisplayState(STATE_NORMAL);
}

void CtrlButton::SetAreaForClick(const Rect& rc, std::int32_t /*bEnable*/)
{
    // CCtrlButton::SetAreaForClick at 0x75b600
    m_rcAreaForClick = rc;
}

void CtrlButton::ForceToMouseEnter(std::int32_t bEnter)
{
    // CCtrlButton::ForceToMouseEnter at 0x75b6c0
    m_bMouseEnter = bEnter;
    MouseEnter(bEnter, 1);
}

void CtrlButton::SetLabel(const std::string& sLabel)
{
    // CCtrlButton::SetLabel at 0x75d650
    m_sLabel = sLabel;
}

void CtrlButton::SetToolTip(const std::string& sTitle, const std::string& sDesc,
                            std::int32_t bUpDir)
{
    // CCtrlButton::SetToolTip at 0x75d200
    m_sToolTipTitle = sTitle;
    m_sToolTipDesc = sDesc;
    m_bToolTipUpDir = bUpDir;
    m_bToolTip = 1;
}

void CtrlButton::SetEnableVisibleShow(bool bEnable)
{
    // CCtrlButton::SetEnableVisibleShow at 0x75b970
    SetEnable(bEnable ? 1 : 0);
    m_bVisible = bEnable ? 1 : 0;
    SetShow(bEnable ? 1 : 0);
}

void CtrlButton::ClearToolTip()
{
    // CCtrlButton::ClearToolTip at 0x75b5e0
    m_bToolTip = 0;
    // TODO: CUIToolTip::ClearToolTip
}

void CtrlButton::_SetToolTip()
{
    // CCtrlButton::_SetToolTip at 0x75f1f0 (simplified)
    // TODO: Create CUIToolTip from m_sToolTipTitle / m_sToolTipDesc
}

// =============================================================================
// Anonymous namespace helpers
// =============================================================================

namespace
{

auto get_update_time() -> std::uint32_t
{
    return static_cast<std::uint32_t>(Application::GetTick());
}

} // anonymous namespace

} // namespace ms
