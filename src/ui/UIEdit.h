#pragma once

#include "UIElement.h"

#include <memory>
#include <string>

namespace ms
{

class WzCanvas;
class WzGr2D;
class WzGr2DLayer;

/**
 * @brief Text input UI element
 *
 * Based on CCtrlEdit from the original MapleStory client (v1029).
 * Supports:
 * - Text input and editing
 * - Caret (cursor) display
 * - Password mode (asterisk masking)
 * - Placeholder image for empty state
 * - Max character limit
 *
 * CCtrlEdit::CREATEPARAM from original:
 * - sText: Initial text
 * - sEmptyImageUOL: UOL for empty placeholder image
 * - ptText: Text offset within control (default: 6, 6)
 * - nFontColor: Font color
 * - nHorzMax: Max characters
 * - bPasswd: Password mode
 */
class UIEdit : public UIElement
{
public:
    using TextChangedCallback = std::function<void(const std::string&)>;
    using EnterPressedCallback = std::function<void(const std::string&)>;

    UIEdit();
    ~UIEdit() override;

    // Configuration
    void SetMaxLength(std::int32_t maxLen) noexcept { m_nMaxLength = maxLen; }
    [[nodiscard]] auto GetMaxLength() const noexcept -> std::int32_t { return m_nMaxLength; }

    void SetPasswordMode(bool password) noexcept { m_bPasswordMode = password; }
    [[nodiscard]] auto IsPasswordMode() const noexcept -> bool { return m_bPasswordMode; }

    void SetTextOffset(std::int32_t x, std::int32_t y) noexcept { m_textOffsetX = x; m_textOffsetY = y; }

    void SetFontColor(std::uint32_t color) noexcept { m_dwFontColor = color; }
    [[nodiscard]] auto GetFontColor() const noexcept -> std::uint32_t { return m_dwFontColor; }

    // Text content
    void SetText(const std::string& text);
    [[nodiscard]] auto GetText() const -> std::string { return m_sText; }
    void Clear();

    // Placeholder
    void SetPlaceholderCanvas(std::shared_ptr<WzCanvas> canvas);
    void SetGuideText(const std::string& text) { m_sGuideText = text; }

    // Selection
    void SelectAll();
    void ClearSelection();

    // Caret control
    void MoveCaretToEnd();
    void MoveCaretToStart();

    // Focus
    [[nodiscard]] auto IsFocused() const noexcept -> bool { return m_bFocused; }
    void SetFocus(bool focus);

    // Callbacks
    void SetTextChangedCallback(TextChangedCallback callback) { m_textChangedCallback = std::move(callback); }
    void SetEnterPressedCallback(EnterPressedCallback callback) { m_enterPressedCallback = std::move(callback); }

    // Input handling
    void OnMouseMove(std::int32_t x, std::int32_t y) override;
    void OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button) override;
    void OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button) override;
    void OnKeyDown(std::int32_t keyCode) override;
    void OnTextInput(const std::string& text);

    /**
     * @brief Handle focus change (from CWndMan)
     *
     * Based on CCtrlEdit::OnSetFocus - returns true to accept focus.
     * When gaining focus: shows caret, enables IME
     * When losing focus: hides caret
     */
    auto OnSetFocus(bool bFocus) -> bool override;

    // Update and render
    void Update() override;
    void Draw() override;

    // Create the layer for this edit
    void CreateLayer(WzGr2D& gr, std::int32_t z, bool screenSpace = true);

    // Background canvas (for the input box frame)
    void SetBackgroundCanvas(std::shared_ptr<WzCanvas> canvas);

#ifdef MS_DEBUG_CANVAS
    [[nodiscard]] auto GetDebugTypeName() const -> std::string override { return "UIEdit"; }
#endif

private:
    void UpdateCaretPosition();
    void InsertCharacter(char c);
    void DeleteCharacter(bool forward);
    void NotifyTextChanged();
    [[nodiscard]] auto GetDisplayText() const -> std::string;
    void UpdateLayerContent();

    // Text content
    std::string m_sText;
    std::string m_sGuideText;  // Placeholder text when empty

    // Configuration
    std::int32_t m_nMaxLength{64};
    bool m_bPasswordMode{false};
    std::int32_t m_textOffsetX{6};
    std::int32_t m_textOffsetY{6};
    std::uint32_t m_dwFontColor{0xFFFFFFFF};  // White by default

    // Caret state
    std::int32_t m_nCaretPos{0};      // Caret position in text
    std::int32_t m_nCaretX{0};        // Caret X position in pixels
    bool m_bCaretVisible{true};       // Caret blink state
    std::uint64_t m_tLastCaretBlink{0};

    // Selection
    std::int32_t m_nSelStart{-1};     // Selection start (-1 = no selection)
    std::int32_t m_nSelEnd{-1};       // Selection end

    // Focus state
    bool m_bFocused{false};

    // Visual elements
    std::shared_ptr<WzCanvas> m_pBackgroundCanvas;
    std::shared_ptr<WzCanvas> m_pPlaceholderCanvas;
    std::shared_ptr<WzGr2DLayer> m_pCaretLayer;
    std::shared_ptr<WzGr2DLayer> m_pTextLayer;  // Layer for rendered text
    std::string m_sLastRenderedText;             // Track last rendered text

    // Callbacks
    TextChangedCallback m_textChangedCallback;
    EnterPressedCallback m_enterPressedCallback;
};

} // namespace ms
