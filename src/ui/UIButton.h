#pragma once

#include "UIElement.h"

#include <array>
#include <memory>
#include <string>

namespace ms
{

class WzCanvas;
class WzGr2D;

/**
 * @brief Button UI element
 *
 * Based on CCtrlButton from the original MapleStory client.
 * Supports multiple states: normal, mouseOver, pressed, disabled
 * Each state can have its own canvas texture.
 *
 * Checkbox mode (based on CCtrlButton::m_bChecked):
 * - When enabled, button toggles checked state on click
 * - Checked state uses separate canvases (indices 4-7)
 * - Used for buttons like "Save Email" checkbox in login
 */
class UIButton : public UIElement
{
public:
    UIButton();
    ~UIButton() override;

    /**
     * @brief Load button from WZ property
     *
     * Expects children named: normal, mouseOver, pressed, disabled
     * For checkbox buttons, also: checkedNormal, checkedMouseOver, etc.
     * Each child should be a canvas or have a canvas child
     *
     * @param prop WZ property containing button states
     * @return true if at least normal state was loaded
     */
    auto LoadFromProperty(const std::shared_ptr<WzProperty>& prop) -> bool;

    /**
     * @brief Set canvas for a specific state
     * @param stateIndex State index (0-3 for normal, 4-7 for checked)
     */
    void SetStateCanvas(UIState state, std::shared_ptr<WzCanvas> canvas);

    /**
     * @brief Set canvas for checked state
     * @param state Base state (Normal, MouseOver, etc.)
     * @param canvas Canvas for the checked version of this state
     */
    void SetCheckedStateCanvas(UIState state, std::shared_ptr<WzCanvas> canvas);

    /**
     * @brief Get canvas for current state
     */
    [[nodiscard]] auto GetCurrentCanvas() const -> std::shared_ptr<WzCanvas>;

    // Override input handling for button-specific behavior
    void OnMouseMove(std::int32_t x, std::int32_t y) override;
    void OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button) override;
    void OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button) override;

    /**
     * @brief Hit test using sdlms coordinate system
     *
     * sdlms style: m_position is anchor point, hit bounds = position - origin
     */
    [[nodiscard]] auto HitTest(std::int32_t x, std::int32_t y) const -> bool override;

    // Override update to handle frame changes
    void Update() override;
    void Draw() override;

    // Create the layer for this button
    // screenSpace: if true, button is positioned in screen coordinates (not affected by camera)
    void CreateLayer(WzGr2D& gr, std::int32_t z, bool screenSpace = true);

    // ========== Checkbox Mode (based on CCtrlButton) ==========

    /**
     * @brief Enable/disable checkbox mode
     *
     * When enabled, button toggles checked state on click.
     * Based on CCtrlButton's checkbox behavior.
     */
    void SetCheckMode(bool bCheckMode) noexcept { m_bCheckMode = bCheckMode; }

    /**
     * @brief Check if button is in checkbox mode
     */
    [[nodiscard]] auto IsCheckMode() const noexcept -> bool { return m_bCheckMode; }

    /**
     * @brief Set checked state
     *
     * Based on CCtrlButton::SetCheck - sets state and refreshes display.
     * @param bChecked New checked state
     */
    void SetChecked(bool bChecked);

    /**
     * @brief Get checked state
     *
     * Based on CCtrlButton::IsChecked
     */
    [[nodiscard]] auto IsChecked() const noexcept -> bool { return m_bChecked; }

    /**
     * @brief Toggle checked state
     */
    void ToggleChecked();

    /**
     * @brief Set the button state directly
     * @param state New state to set
     */
    void SetState(UIState state) noexcept { m_state = state; }

#ifdef MS_DEBUG_CANVAS
    [[nodiscard]] auto GetDebugTypeName() const -> std::string override { return "UIButton"; }
#endif

private:
    void UpdateLayerCanvas();

    // Canvas for each state (0-3 = normal states, 4-7 = checked states)
    std::array<std::shared_ptr<WzCanvas>, 8> m_stateCanvases;

    // Track if we need to update the layer
    UIState m_lastState{UIState::Normal};
    bool m_lastChecked{false};

    // Checkbox mode (based on CCtrlButton::m_bChecked)
    bool m_bCheckMode{false};  // Enable checkbox behavior
    bool m_bChecked{false};    // Current checked state
};

} // namespace ms
