#pragma once

#include "UIManager.h"
#include "util/Singleton.h"

#include <cstdint>
#include <memory>
#include <string>

namespace ms
{

class Stage;
class UIElement;

/**
 * @brief Global window manager singleton
 *
 * Based on CWndMan from the original MapleStory client (v1029).
 * Handles global input routing and UI management.
 *
 * Architecture:
 * - CWndMan receives all Windows messages (WM_MOUSEMOVE, WM_KEYDOWN, etc.)
 * - CWndMan manages global focus (m_pFocus) and capture (m_lpCapture)
 * - CWndMan forwards events to g_pStage (current stage)
 * - CWndMan can also have global UI elements (tooltips, notices, modal dialogs)
 *
 * Event flow (original client):
 * 1. CWvsApp::WndProc receives Windows message
 * 2. CWvsApp forwards to CWndMan::OnXXX
 * 3. CWndMan::OnXXX forwards to g_pStage->OnXXX
 *
 * Event flow (this implementation):
 * 1. Application::ProcessInput receives SDL events
 * 2. Application forwards to WndMan::OnXXX
 * 3. WndMan::OnXXX forwards to current stage->OnXXX
 */
class WndMan final : public Singleton<WndMan>
{
    friend class Singleton<WndMan>;

public:
    /**
     * @brief Initialize the window manager
     */
    void Initialize();

    /**
     * @brief Shutdown and cleanup
     */
    void Shutdown();

    /**
     * @brief Set the current stage (g_pStage)
     *
     * Based on CWndMan's reference to g_pStage.
     * WndMan forwards events to this stage.
     */
    void SetStage(Stage* pStage);

    /**
     * @brief Get the current stage
     */
    [[nodiscard]] auto GetStage() const noexcept -> Stage* { return m_pStage; }

    // ========== Input Event Handlers ==========
    // Based on CWndMan's message handlers

    /**
     * @brief Handle mouse enter/leave window
     *
     * Based on CWndMan::OnMouseEnter
     * Forwards to g_pStage->OnMouseEnter
     */
    void OnMouseEnter(bool bEnter);

    /**
     * @brief Handle mouse movement
     *
     * Based on CWndMan::OnMouseMove
     * First checks captured element, then forwards to stage
     */
    void OnMouseMove(std::int32_t x, std::int32_t y);

    /**
     * @brief Handle mouse button down
     *
     * Based on CWndMan::OnLButtonDown/OnRButtonDown
     */
    void OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button);

    /**
     * @brief Handle mouse button up
     *
     * Based on CWndMan::OnLButtonUp/OnRButtonUp
     */
    void OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button);

    /**
     * @brief Handle mouse double click
     *
     * Based on CWndMan::OnLButtonDblClk/OnRButtonDblClk
     */
    void OnMouseDoubleClick(std::int32_t x, std::int32_t y, std::int32_t button);

    /**
     * @brief Handle mouse wheel
     *
     * Based on CWndMan::OnMouseWheel
     */
    void OnMouseWheel(std::int32_t x, std::int32_t y, float delta);

    /**
     * @brief Handle key down
     *
     * Based on CWndMan::OnKeyDown
     */
    void OnKeyDown(std::int32_t keyCode);

    /**
     * @brief Handle key up
     *
     * Based on CWndMan::OnKeyUp
     */
    void OnKeyUp(std::int32_t keyCode);

    /**
     * @brief Handle text input (IME/keyboard)
     *
     * Based on CWndMan::OnIMEChar
     */
    void OnTextInput(const std::string& text);

    /**
     * @brief Handle window focus change
     *
     * Based on CWndMan::OnSetFocus
     */
    void OnSetFocus(bool bFocus);

    // ========== Focus Management ==========
    // Based on CWndMan::m_pFocus and SetFocus

    /**
     * @brief Set the focused element for keyboard input
     *
     * Based on CWndMan::SetFocusImp
     */
    void SetFocus(std::shared_ptr<UIElement> pElement);

    /**
     * @brief Get the currently focused element
     */
    [[nodiscard]] auto GetFocus() const -> std::shared_ptr<UIElement> { return m_pFocus; }

    // ========== Capture Management ==========
    // Based on CWndMan::m_lpCapture

    /**
     * @brief Set capture to an element
     *
     * Based on setting CWndMan::m_lpCapture
     * Captured element receives all mouse input
     */
    void SetCapture(std::shared_ptr<UIElement> pElement);

    /**
     * @brief Release capture from an element
     *
     * Based on CWndMan::ReleaseCaptureWnd
     */
    void ReleaseCapture(UIElement* pElement);

    /**
     * @brief Get the captured element
     */
    [[nodiscard]] auto GetCapture() const -> std::shared_ptr<UIElement> { return m_lpCapture; }

    // ========== Global UI Management ==========

    /**
     * @brief Get the global UI manager
     *
     * For global UI elements like tooltips, notices, modal dialogs
     */
    [[nodiscard]] auto GetUIManager() -> UIManager& { return m_uiManager; }

    /**
     * @brief Update global UI elements
     */
    void Update();

    /**
     * @brief Draw global UI elements
     */
    void Draw();

private:
    WndMan();
    ~WndMan() override;

    // Current stage (like g_pStage in original)
    Stage* m_pStage{nullptr};

    // Global focus (CWndMan::m_pFocus)
    std::shared_ptr<UIElement> m_pFocus;

    // Global capture (CWndMan::m_lpCapture)
    std::shared_ptr<UIElement> m_lpCapture;

    // Global UI manager for tooltips, notices, etc.
    UIManager m_uiManager;

    // Mouse state
    std::int32_t m_nLastMouseX{0};
    std::int32_t m_nLastMouseY{0};
    bool m_bMouseInWindow{false};
};

} // namespace ms
