#include "UIEdit.h"
#include "app/Application.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "text/TextRenderer.h"
#include "wz/WzCanvas.h"

#include <SDL3/SDL.h>
#include <algorithm>

namespace ms
{

// Caret blink interval in milliseconds
constexpr std::uint64_t kCaretBlinkInterval = 500;

UIEdit::UIEdit() = default;

UIEdit::~UIEdit() = default;

void UIEdit::SetText(const std::string& text)
{
    m_sText = text;
    if (m_nMaxLength > 0 && static_cast<std::int32_t>(m_sText.length()) > m_nMaxLength)
    {
        m_sText = m_sText.substr(0, static_cast<std::size_t>(m_nMaxLength));
    }
    m_nCaretPos = static_cast<std::int32_t>(m_sText.length());
    ClearSelection();
    UpdateCaretPosition();
    UpdateLayerContent();
}

void UIEdit::Clear()
{
    m_sText.clear();
    m_nCaretPos = 0;
    ClearSelection();
    UpdateCaretPosition();
    UpdateLayerContent();
    NotifyTextChanged();
}

void UIEdit::SetPlaceholderCanvas(std::shared_ptr<WzCanvas> canvas)
{
    m_pPlaceholderCanvas = std::move(canvas);
    UpdateLayerContent();
}

void UIEdit::SelectAll()
{
    if (!m_sText.empty())
    {
        m_nSelStart = 0;
        m_nSelEnd = static_cast<std::int32_t>(m_sText.length());
    }
}

void UIEdit::ClearSelection()
{
    m_nSelStart = -1;
    m_nSelEnd = -1;
}

void UIEdit::MoveCaretToEnd()
{
    m_nCaretPos = static_cast<std::int32_t>(m_sText.length());
    ClearSelection();
    UpdateCaretPosition();
}

void UIEdit::MoveCaretToStart()
{
    m_nCaretPos = 0;
    ClearSelection();
    UpdateCaretPosition();
}

void UIEdit::SetFocus(bool focus)
{
    m_bFocused = focus;
    if (focus)
    {
        m_bCaretVisible = true;
        m_tLastCaretBlink = Application::GetTick();
    }
    UpdateLayerContent();
}

auto UIEdit::OnSetFocus(bool bFocus) -> bool
{
    // Based on CCtrlEdit::OnSetFocus from original client
    // Always accept focus (return true)
    SetFocus(bFocus);
    return true;
}

void UIEdit::OnMouseMove(std::int32_t x, std::int32_t y)
{
    if (!m_bEnabled)
    {
        return;
    }

    // Update hover state
    if (HitTest(x, y))
    {
        // Could change cursor to I-beam here
    }
}

void UIEdit::OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button)
{
    if (!m_bEnabled || button != 1)
    {
        return;
    }

    if (HitTest(x, y))
    {
        SetFocus(true);

        // Calculate caret position based on click position
        // Use absolute position for proper parent-child coordinate handling
        auto absPos = GetAbsolutePosition();
        std::int32_t relX = x - absPos.x - m_textOffsetX;

        // Simple approximation: assume ~8 pixels per character
        constexpr int kCharWidth = 8;
        std::int32_t clickPos = relX / kCharWidth;
        clickPos = std::clamp(clickPos, 0, static_cast<std::int32_t>(m_sText.length()));
        m_nCaretPos = clickPos;

        ClearSelection();
        UpdateCaretPosition();
    }
    else
    {
        SetFocus(false);
    }
}

void UIEdit::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button)
{
    (void)x;
    (void)y;
    (void)button;
}

void UIEdit::OnKeyDown(std::int32_t keyCode)
{
    if (!m_bEnabled || !m_bFocused)
    {
        return;
    }

    switch (keyCode)
    {
    case SDLK_BACKSPACE:
        if (m_nSelStart >= 0 && m_nSelEnd >= 0 && m_nSelStart != m_nSelEnd)
        {
            // Delete selection
            std::int32_t start = std::min(m_nSelStart, m_nSelEnd);
            std::int32_t end = std::max(m_nSelStart, m_nSelEnd);
            m_sText.erase(static_cast<std::size_t>(start),
                          static_cast<std::size_t>(end - start));
            m_nCaretPos = start;
            ClearSelection();
            NotifyTextChanged();
        }
        else
        {
            DeleteCharacter(false);
        }
        break;

    case SDLK_DELETE:
        if (m_nSelStart >= 0 && m_nSelEnd >= 0 && m_nSelStart != m_nSelEnd)
        {
            // Delete selection
            std::int32_t start = std::min(m_nSelStart, m_nSelEnd);
            std::int32_t end = std::max(m_nSelStart, m_nSelEnd);
            m_sText.erase(static_cast<std::size_t>(start),
                          static_cast<std::size_t>(end - start));
            m_nCaretPos = start;
            ClearSelection();
            NotifyTextChanged();
        }
        else
        {
            DeleteCharacter(true);
        }
        break;

    case SDLK_LEFT:
        if (m_nCaretPos > 0)
        {
            --m_nCaretPos;
            ClearSelection();
            UpdateCaretPosition();
        }
        break;

    case SDLK_RIGHT:
        if (m_nCaretPos < static_cast<std::int32_t>(m_sText.length()))
        {
            ++m_nCaretPos;
            ClearSelection();
            UpdateCaretPosition();
        }
        break;

    case SDLK_HOME:
        MoveCaretToStart();
        break;

    case SDLK_END:
        MoveCaretToEnd();
        break;

    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        if (m_enterPressedCallback)
        {
            m_enterPressedCallback(m_sText);
        }
        break;

    case SDLK_A:
        // Ctrl+A to select all
        if (SDL_GetModState() & SDL_KMOD_CTRL)
        {
            SelectAll();
        }
        break;

    default:
        break;
    }

    UpdateLayerContent();
}

void UIEdit::OnTextInput(const std::string& text)
{
    if (!m_bEnabled || !m_bFocused)
    {
        return;
    }

    // Delete selection first if exists
    if (m_nSelStart >= 0 && m_nSelEnd >= 0 && m_nSelStart != m_nSelEnd)
    {
        std::int32_t start = std::min(m_nSelStart, m_nSelEnd);
        std::int32_t end = std::max(m_nSelStart, m_nSelEnd);
        m_sText.erase(static_cast<std::size_t>(start),
                      static_cast<std::size_t>(end - start));
        m_nCaretPos = start;
        ClearSelection();
    }

    // Insert text character by character
    for (char c : text)
    {
        InsertCharacter(c);
    }

    NotifyTextChanged();
    UpdateLayerContent();
}

void UIEdit::Update()
{
    UIElement::Update();

    // Update layer positions using absolute coordinates (supports parent-child hierarchy)
    auto absPos = GetAbsolutePosition();
    if (m_pLayer)
    {
        m_pLayer->SetPosition(absPos.x, absPos.y);
    }
    if (m_pTextLayer)
    {
        m_pTextLayer->SetPosition(absPos.x + m_textOffsetX, absPos.y + m_textOffsetY);
    }
    if (m_pCaretLayer)
    {
        m_pCaretLayer->SetPosition(absPos.x + m_nCaretX, absPos.y + m_textOffsetY);
    }

    if (m_bFocused)
    {
        // Caret blinking
        auto tNow = Application::GetTick();
        if (tNow - m_tLastCaretBlink >= kCaretBlinkInterval)
        {
            m_bCaretVisible = !m_bCaretVisible;
            m_tLastCaretBlink = tNow;

            // Update caret layer visibility
            if (m_pCaretLayer)
            {
                m_pCaretLayer->SetVisible(m_bCaretVisible);
            }
        }
    }
}

void UIEdit::Draw()
{
    // Layer rendering is handled by WzGr2D
}

void UIEdit::CreateLayer(WzGr2D& gr, std::int32_t z, bool screenSpace)
{
    m_nZ = z;

    // MapleStory coordinate system:
    // - absPos = desired RENDER position on screen (where we want the edit box to appear)
    // - WzGr2DLayer::Render formula: renderPos = layerPos - canvasOrigin
    // - Therefore: layerPos = absPos + canvasOrigin (to compensate for origin subtraction)
    auto absPos = GetAbsolutePosition();

    // Compensate for canvas origin (matching UIButton logic)
    auto canvasOrigin = m_pBackgroundCanvas ? m_pBackgroundCanvas->GetOrigin() : Point2D{0, 0};
    std::int32_t layerX = absPos.x + canvasOrigin.x;
    std::int32_t layerY = absPos.y + canvasOrigin.y;

    // Create main layer for the edit box
    m_pLayer = gr.CreateLayer(layerX, layerY,
                               static_cast<std::uint32_t>(m_nWidth),
                               static_cast<std::uint32_t>(m_nHeight), z);

    if (m_pLayer)
    {
        m_pLayer->SetScreenSpace(screenSpace);

        // Add background canvas if available
        if (m_pBackgroundCanvas)
        {
            m_pLayer->InsertCanvas(m_pBackgroundCanvas, 0, 255, 255);
        }
    }

    // Create caret layer (slightly higher z)
    // Width = 1 pixel for Windows-style thin caret
    m_pCaretLayer = gr.CreateLayer(layerX + m_textOffsetX, layerY + m_textOffsetY,
                                    1, static_cast<std::uint32_t>(m_nHeight - m_textOffsetY * 2),
                                    z + 2);  // Above text layer
    if (m_pCaretLayer)
    {
        m_pCaretLayer->SetScreenSpace(screenSpace);
        m_pCaretLayer->SetVisible(false);

        // Create a simple caret canvas (1 pixel wide black line, like Windows)
        auto caretCanvas = std::make_shared<WzCanvas>(1, m_nHeight - m_textOffsetY * 2);
        std::vector<std::uint8_t> caretPixels(static_cast<std::size_t>(1 * (m_nHeight - m_textOffsetY * 2) * 4));
        for (std::size_t i = 0; i < caretPixels.size(); i += 4)
        {
            caretPixels[i] = 0;       // R (black)
            caretPixels[i + 1] = 0;   // G
            caretPixels[i + 2] = 0;   // B
            caretPixels[i + 3] = 255; // A
        }
        caretCanvas->SetPixelData(std::move(caretPixels));
        m_pCaretLayer->InsertCanvas(caretCanvas, 0, 255, 255);
    }

    // Create text layer (slightly higher z than background, lower than caret)
    m_pTextLayer = gr.CreateLayer(layerX + m_textOffsetX, layerY + m_textOffsetY,
                                   static_cast<std::uint32_t>(m_nWidth - m_textOffsetX * 2),
                                   static_cast<std::uint32_t>(m_nHeight - m_textOffsetY * 2),
                                   z + 1);
    if (m_pTextLayer)
    {
        m_pTextLayer->SetScreenSpace(screenSpace);
    }
}

void UIEdit::SetBackgroundCanvas(std::shared_ptr<WzCanvas> canvas)
{
    m_pBackgroundCanvas = std::move(canvas);
    if (m_pBackgroundCanvas)
    {
        m_nWidth = static_cast<std::int32_t>(m_pBackgroundCanvas->GetWidth());
        m_nHeight = static_cast<std::int32_t>(m_pBackgroundCanvas->GetHeight());
    }
}

void UIEdit::UpdateCaretPosition()
{
    // Simple calculation: assume ~8 pixels per character
    constexpr int kCharWidth = 8;
    m_nCaretX = m_textOffsetX + m_nCaretPos * kCharWidth;

    // Use absolute position for parent-child coordinate support
    if (m_pCaretLayer)
    {
        auto absPos = GetAbsolutePosition();
        m_pCaretLayer->SetPosition(absPos.x + m_nCaretX, absPos.y + m_textOffsetY);
    }
}

void UIEdit::InsertCharacter(char c)
{
    // Check max length
    if (m_nMaxLength > 0 && static_cast<std::int32_t>(m_sText.length()) >= m_nMaxLength)
    {
        return;
    }

    // Insert at caret position
    m_sText.insert(static_cast<std::size_t>(m_nCaretPos), 1, c);
    ++m_nCaretPos;
    UpdateCaretPosition();
}

void UIEdit::DeleteCharacter(bool forward)
{
    if (forward)
    {
        // Delete character after caret
        if (m_nCaretPos < static_cast<std::int32_t>(m_sText.length()))
        {
            m_sText.erase(static_cast<std::size_t>(m_nCaretPos), 1);
            NotifyTextChanged();
        }
    }
    else
    {
        // Backspace: delete character before caret
        if (m_nCaretPos > 0)
        {
            --m_nCaretPos;
            m_sText.erase(static_cast<std::size_t>(m_nCaretPos), 1);
            UpdateCaretPosition();
            NotifyTextChanged();
        }
    }
}

void UIEdit::NotifyTextChanged()
{
    if (m_textChangedCallback)
    {
        m_textChangedCallback(m_sText);
    }
}

auto UIEdit::GetDisplayText() const -> std::string
{
    if (m_bPasswordMode)
    {
        // Return asterisks for password mode
        return std::string(m_sText.length(), '*');
    }
    return m_sText;
}

void UIEdit::UpdateLayerContent()
{
    // Update caret visibility
    if (m_pCaretLayer)
    {
        m_pCaretLayer->SetVisible(m_bFocused && m_bCaretVisible);
    }

    // Update background layer
    if (m_pLayer)
    {
        m_pLayer->RemoveAllCanvases();
        if (m_pBackgroundCanvas)
        {
            m_pLayer->InsertCanvas(m_pBackgroundCanvas, 0, 255, 255);
        }

        // Show placeholder when empty and not focused
        if (m_sText.empty() && !m_bFocused && m_pPlaceholderCanvas)
        {
            m_pLayer->InsertCanvas(m_pPlaceholderCanvas, 0, 255, 255);
        }
    }

    // Update text layer
    if (m_pTextLayer)
    {
        std::string displayText = GetDisplayText();

        // Only re-render if text changed
        if (displayText != m_sLastRenderedText)
        {
            m_sLastRenderedText = displayText;
            m_pTextLayer->RemoveAllCanvases();

            if (!displayText.empty())
            {
                auto& textRenderer = TextRenderer::GetInstance();
                if (textRenderer.IsInitialized())
                {
                    // Set font size based on edit height
                    textRenderer.SetFontSize(m_nHeight - m_textOffsetY * 2 - 2);

                    // Render text with font color
                    auto textCanvas = textRenderer.RenderText(displayText, m_dwFontColor);
                    if (textCanvas)
                    {
                        m_pTextLayer->InsertCanvas(textCanvas, 0, 255, 255);
                    }
                }
            }
        }
    }
}

} // namespace ms
