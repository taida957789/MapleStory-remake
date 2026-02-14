#pragma once

#include "CtrlWnd.h"
#include "UIToolTip.h"
#include "util/Point.h"

#include <cstdint>
#include <memory>
#include <string>

namespace ms
{

class WzProperty;
class WzCanvas;
class WzGr2DLayer;
class IWzVector2D;

/**
 * @brief Button control
 *
 * Based on CCtrlButton from the original MapleStory client (v1029).
 *
 * Original: __cppobj CCtrlButton : CCtrlWnd
 *
 * Button states (m_nDisplayState):
 *   0 = Normal
 *   1 = Pressed
 *   2 = Disabled
 *   3 = MouseOver
 *   4 = Checked
 *   5 = CheckedMouseOver
 *
 * Each state maps to m_apPropButton[state] which contains
 * animation frames (child canvases indexed by frame number).
 */
class CtrlButton : public CtrlWnd
{
public:
    /// Number of button display states
    static constexpr std::int32_t kNumStates = 6;

    /// Display state indices
    enum DisplayState : std::int32_t
    {
        STATE_NORMAL           = 0,
        STATE_PRESSED          = 1,
        STATE_DISABLED         = 2,
        STATE_MOUSEOVER        = 3,
        STATE_CHECKED          = 4,
        STATE_CHECKED_MOUSEOVER = 5,
    };

    /// Button creation parameters (CCtrlButton::CREATEPARAM at 0x75c8e0)
    struct CREATEPARAM
    {
        std::int32_t bAcceptFocus{1};
        std::int32_t bDrawBack{};
        std::int32_t bAnimateOnce{};
        std::int32_t bDisableTooltip{};
        std::int32_t bShowTooltipAnystate{};
        std::int32_t bOnMoveInvalidRect{};
        std::int32_t bToggle{};
        std::int32_t bSetOrigin{};
        std::string sUOL; ///< WZ path for button images
    };

    CtrlButton();
    ~CtrlButton() override;

    // === CtrlWnd overrides ===
    void CreateCtrl(
        Wnd* pParent, std::uint32_t nCtrlId,
        std::int32_t x, std::int32_t y,
        std::int32_t cx, std::int32_t cy,
        void* pParam) override;
    void Destroy() override;
    void Update() override;
    void Draw(
        std::int32_t x, std::int32_t y, const Rect* pRect) override;
    auto HitTest(
        std::int32_t x, std::int32_t y) -> std::int32_t override;

    // === IUIMsgHandler overrides ===
    void OnKey(std::uint32_t nKey, std::uint32_t nFlag) override;
    void OnMouseButton(
        std::uint32_t nType, std::uint32_t nFlag,
        std::int32_t x, std::int32_t y) override;
    auto OnMouseMove(
        std::int32_t x, std::int32_t y) -> std::int32_t override;
    void OnMouseEnter(std::int32_t bEnter) override;
    auto OnMouseWheel(
        std::int32_t nDelta,
        std::int32_t x, std::int32_t y) -> std::int32_t override;
    void SetEnable(std::int32_t bEnable) override;
    void SetShow(std::int32_t bShow) override;
    void ClearToolTip() override;

    // === Button-specific interface ===

    /// Load button images from WZ path (CCtrlButton::SetButtonImage at 0x75d930)
    virtual void SetButtonImage(const std::string& sUOL);

    /// Set the check state for toggle buttons (0x75b490)
    void SetCheck(std::int32_t bCheck);

    /// Get checked state (0x75b4c0)
    [[nodiscard]] auto IsChecked() const noexcept -> std::int32_t { return m_bChecked; }

    /// Set the pressed state directly (0x75b4d0)
    void SetPressed(std::int32_t bPressed);

    /// Get pressed state (0x75b520)
    [[nodiscard]] auto IsPressed() const noexcept -> std::int32_t { return m_bPressed; }

    /// Set checked state with display update (0x75b4f0)
    void SetChecked(std::int32_t bChecked);

    /// Set visible flag (0x75b540)
    void SetVisible(std::int32_t bVisible) { m_bVisible = bVisible; }

    /// Set draw background flag (0x75b560)
    void SetDrawBack(std::int32_t bDrawBack) { m_bDrawBack = bDrawBack; }

    /// Shrink click area by nDec pixels from each edge (0x75b470)
    void SetDecClickArea(std::int32_t nDec) { m_nDecClickArea = nDec; }

    /// Use pixel-perfect hit testing (0x75b480)
    void SetPixelAreaCheck() { m_bPixelAreaCheck = true; }

    /// Set default cursor mode (0x75b530)
    void SetCursorDefault(std::int32_t bDefault) { m_bCursorDefault = bDefault; }

    /// Set tooltip Z order relative to parent (0x75b550)
    void SetToolTipZByParentZ(std::int32_t bZ) { m_bToolTipZ = bZ; }

    /// Set self-disable mode (0x75b380)
    void SetSelfDisable() { m_bSelfDisable = 1; }

    /// Clear self-disable mode (0x75b460)
    void ClearSelfDisable() { m_bSelfDisable = 0; }

    /// Disable tooltip display (0x75b710)
    void SetDisabledToolTip(bool bDisable) { m_bDisableTooltip = bDisable ? 1 : 0; }

    /// Enable/disable tooltip (0x75b720)
    void EnableToolTip(std::int32_t bEnable) { m_bToolTip = bEnable; }

    /// Set custom click area rectangle (0x75b600)
    void SetAreaForClick(const Rect& rc, std::int32_t bEnable);

    /// Force mouse enter state (0x75b6c0)
    void ForceToMouseEnter(std::int32_t bEnter);

    /// Set label text (0x75d650)
    void SetLabel(const std::string& sLabel);

    /// Set tooltip text (0x75d200)
    void SetToolTip(const std::string& sTitle, const std::string& sDesc,
                    std::int32_t bUpDir = 0);

    /// Set enable + visible + show (0x75b970)
    void SetEnableVisibleShow(bool bEnable);

    /// Get the canvas for the current display state/frame
    [[nodiscard]] auto GetButtonCanvas() const
        -> std::shared_ptr<WzCanvas>;

protected:
    // === Virtual mouse event handlers (overridable by subclasses) ===
    virtual void MouseDown();    ///< 0x75b6e0
    virtual void MouseUp();      ///< 0x75d0c0
    virtual void MouseEnter(std::int32_t bEnter, std::int32_t bForced);  ///< 0x75cf60

    /// Change display state and update animation (0x75e270)
    virtual void ChangeDisplayState(std::int32_t nState);

    /// Advance animation frame (0x75e430)
    void ChangeDisplayFrame();

    /// Test if point is inside the button area (0x75e580)
    [[nodiscard]] auto isInButton(
        std::int32_t x, std::int32_t y) -> bool;

    /// Check if mouse is currently entered (0x75b370)
    [[nodiscard]] auto IsEntered() const noexcept -> bool
    {
        return m_pButtonEntered == this;
    }

    /// Set tooltip from current button data
    virtual void _SetToolTip();

private:
    // === Display state ===
    std::int32_t m_nDisplayState{};
    std::int32_t m_nDisplayFrame{};
    std::int32_t m_nAniCount{};
    std::int32_t m_nAniDelay{};
    std::uint32_t m_dwDisplayStarted{};

    // === Mouse/focus state ===
    std::int32_t m_bMouseEnter{};
    std::int32_t m_bMouseEnterForTooltip{};
    std::int32_t m_nDecClickArea{};
    bool m_bPixelAreaCheck{};
    std::int32_t m_bPressed{};
    std::int32_t m_bPressedByKey{};
    std::int32_t m_bKeyFocused{};

    // === Behavior flags (from CREATEPARAM) ===
    std::int32_t m_bDrawBack{};
    std::int32_t m_bAnimateOnce{};
    std::int32_t m_bDisableTooltip{};
    std::int32_t m_bShowTooltipAnystate{};
    std::int32_t m_bToggle{};
    std::int32_t m_bOnMoveInvalidRect{};
    std::int32_t m_bChecked{};
    std::int32_t m_bCursorDefault{};

    // === WZ resources ===
    /// Focus frame property
    std::shared_ptr<WzProperty> m_pPropFocusFrame;
    std::shared_ptr<WzGr2DLayer> m_pLayerFocusFrame;

    /// Properties for each button state (normal, pressed, disabled, mouseOver, checked, checkedMouseOver)
    std::shared_ptr<WzProperty> m_apPropButton[kNumStates];

    // === Tooltip ===
    std::int32_t m_bToolTip{};
    std::int32_t m_bToolTipUpDir{};
    std::string m_sToolTipTitle;
    std::string m_sToolTipDesc;
    UIToolTip m_uiToolTip;
    std::int32_t m_bToolTipZ{};
    // ZRef<SimpleToolTipInfo> m_pSimpleToolTipInfo;  // TODO
    std::string m_sToolTipFromData;

    // === Appearance ===
    std::int32_t m_bSelfDisable{};
    std::shared_ptr<IWzVector2D> m_pAlpha;
    std::int32_t m_nAlpha{};
    std::int32_t m_bVisible{1};
    std::int32_t m_nFontAlpha{};
    std::int32_t m_bSetOrigin{};
    std::string m_sLabel;
    // IWzFont* m_pFont{};  // TODO: font system
    Rect m_rcAreaForClick{};

    // === Static ===
    static inline CtrlButton* m_pButtonEntered{};
};

} // namespace ms
