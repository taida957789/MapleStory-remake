#include "UIButton.h"
#include "audio/SoundSystem.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "util/Logger.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

namespace
{
// Button sound paths (from IDA: StringPool 0xDEC = "Sound/UI.img/")
constexpr const char* SOUND_BUTTON_CLICK = "Sound/UI.img/BtMouseClick";
constexpr const char* SOUND_BUTTON_HOVER = "Sound/UI.img/BtMouseOver";
} // namespace

namespace ms
{

UIButton::UIButton() = default;

UIButton::~UIButton() = default;

auto UIButton::LoadFromProperty(const std::shared_ptr<WzProperty>& prop) -> bool
{
    if (!prop)
    {
        LOG_DEBUG("UIButton::LoadFromProperty - prop is null");
        return false;
    }

    LOG_DEBUG("UIButton::LoadFromProperty - loading button, {} children", prop->GetChildren().size());

    bool hasNormal = false;

    // Try to load each state
    // MapleStory button properties typically have: normal, mouseOver, pressed, disabled
    static const std::array<std::pair<const char*, UIState>, 4> stateNames = {{
        {"normal", UIState::Normal},
        {"mouseOver", UIState::MouseOver},
        {"pressed", UIState::Pressed},
        {"disabled", UIState::Disabled}
    }};

    for (const auto& [name, state] : stateNames)
    {
        auto stateProp = prop->GetChild(name);
        if (!stateProp)
        {
            continue;
        }

        LOG_DEBUG("UIButton::LoadFromProperty - found state: {}", name);

        // Try to get canvas directly
        auto canvas = stateProp->GetCanvas();
        if (!canvas)
        {
            // Try to get from child "0"
            auto childProp = stateProp->GetChild("0");
            if (childProp)
            {
                canvas = childProp->GetCanvas();
            }
        }

        if (canvas)
        {
            SetStateCanvas(state, canvas);
            LOG_DEBUG("UIButton::LoadFromProperty - loaded canvas for state: {}", name);

            if (state == UIState::Normal)
            {
                hasNormal = true;

                // Set size from normal canvas
                m_nWidth = static_cast<std::int32_t>(canvas->GetWidth());
                m_nHeight = static_cast<std::int32_t>(canvas->GetHeight());
            }
        }
        else
        {
            LOG_DEBUG("UIButton::LoadFromProperty - no canvas for state: {}", name);
        }
    }

    // If no states found, try to load numbered frames (0, 1, 2, 3)
    if (!hasNormal)
    {
        for (int i = 0; i < 4; ++i)
        {
            auto frameProp = prop->GetChild(std::to_string(i));
            if (!frameProp)
            {
                continue;
            }

            auto canvas = frameProp->GetCanvas();
            if (canvas)
            {
                SetStateCanvas(static_cast<UIState>(i), canvas);

                if (i == 0)
                {
                    hasNormal = true;
                    m_nWidth = static_cast<std::int32_t>(canvas->GetWidth());
                    m_nHeight = static_cast<std::int32_t>(canvas->GetHeight());
                }
            }
        }
    }

    // Try to load checked states for checkbox buttons
    // Format: checkedNormal, checkedMouseOver, checkedPressed, checkedDisabled
    // Or numbered: 4, 5, 6, 7
    static const std::array<std::pair<const char*, UIState>, 4> checkedStateNames = {{
        {"checkedNormal", UIState::Normal},
        {"checkedMouseOver", UIState::MouseOver},
        {"checkedPressed", UIState::Pressed},
        {"checkedDisabled", UIState::Disabled}
    }};

    for (const auto& [name, state] : checkedStateNames)
    {
        auto stateProp = prop->GetChild(name);
        if (!stateProp)
        {
            continue;
        }

        auto canvas = stateProp->GetCanvas();
        if (!canvas)
        {
            auto childProp = stateProp->GetChild("0");
            if (childProp)
            {
                canvas = childProp->GetCanvas();
            }
        }

        if (canvas)
        {
            SetCheckedStateCanvas(state, canvas);
        }
    }

    // Also try numbered frames 4-7 for checked states
    for (int i = 4; i < 8; ++i)
    {
        auto frameProp = prop->GetChild(std::to_string(i));
        if (!frameProp)
        {
            continue;
        }

        auto canvas = frameProp->GetCanvas();
        if (canvas)
        {
            // Map 4->Normal, 5->MouseOver, etc.
            SetCheckedStateCanvas(static_cast<UIState>(i - 4), canvas);
        }
    }

    // If still no normal state found, try to use the property itself as a canvas
    // This handles the case where the button is just a single canvas (no states)
    if (!hasNormal)
    {
        auto canvas = prop->GetCanvas();
        if (canvas)
        {
            LOG_DEBUG("UIButton::LoadFromProperty - using property itself as canvas (no states)");
            // Use same canvas for all states
            SetStateCanvas(UIState::Normal, canvas);
            SetStateCanvas(UIState::MouseOver, canvas);
            SetStateCanvas(UIState::Pressed, canvas);
            SetStateCanvas(UIState::Disabled, canvas);

            m_nWidth = static_cast<std::int32_t>(canvas->GetWidth());
            m_nHeight = static_cast<std::int32_t>(canvas->GetHeight());
            hasNormal = true;
        }
    }

    return hasNormal;
}

auto UIButton::LoadFromUOL(const std::wstring& sUOL) -> bool
{
    // 從 WzResMan 獲取屬性
    auto& resMan = WzResMan::GetInstance();

    // 轉換 wstring 到 string (簡化處理：WZ 路徑通常是 ASCII)
    std::string sPath;
    sPath.reserve(sUOL.size());
    for (wchar_t wc : sUOL)
    {
        if (wc > 127)
        {
            // 非 ASCII 字符，返回失敗
            LOG_DEBUG("UIButton::LoadFromUOL - non-ASCII character in path");
            return false;
        }
        sPath.push_back(static_cast<char>(wc));
    }

    LOG_DEBUG("UIButton::LoadFromUOL - trying: {}", sPath);

    auto pProp = resMan.GetProperty(sPath);
    if (!pProp)
    {
        LOG_DEBUG("UIButton::LoadFromUOL - property not found: {}", sPath);
        return false;
    }

    LOG_DEBUG("UIButton::LoadFromUOL - property found, has {} children", pProp->GetChildren().size());

    // 使用現有的 LoadFromProperty
    bool result = LoadFromProperty(pProp);
    if (!result)
    {
        LOG_DEBUG("UIButton::LoadFromUOL - LoadFromProperty failed for: {}", sPath);
    }
    return result;
}

void UIButton::SetStateCanvas(UIState state, std::shared_ptr<WzCanvas> canvas)
{
    auto index = static_cast<std::size_t>(state);
    if (index < 4)
    {
        m_stateCanvases[index] = std::move(canvas);
    }
}

void UIButton::SetCheckedStateCanvas(UIState state, std::shared_ptr<WzCanvas> canvas)
{
    // Checked states are stored at indices 4-7
    auto index = static_cast<std::size_t>(state) + 4;
    if (index < m_stateCanvases.size())
    {
        m_stateCanvases[index] = std::move(canvas);
    }
}

auto UIButton::GetCurrentCanvas() const -> std::shared_ptr<WzCanvas>
{
    auto baseIndex = static_cast<std::size_t>(m_state);

    // If checked mode and checked, use checked canvas (index + 4)
    if (m_bCheckMode && m_bChecked)
    {
        auto checkedIndex = baseIndex + 4;
        if (checkedIndex < m_stateCanvases.size() && m_stateCanvases[checkedIndex])
        {
            return m_stateCanvases[checkedIndex];
        }
        // Fallback to checked normal
        if (m_stateCanvases[4])
        {
            return m_stateCanvases[4];
        }
    }

    // Use normal state canvas
    if (baseIndex < 4 && m_stateCanvases[baseIndex])
    {
        return m_stateCanvases[baseIndex];
    }

    // Fallback to normal state
    return m_stateCanvases[static_cast<std::size_t>(UIState::Normal)];
}

void UIButton::OnMouseMove(std::int32_t x, std::int32_t y)
{
    if (!m_bEnabled)
    {
        return;
    }

    UIState oldState = m_state;

    if (HitTest(x, y))
    {
        if (m_state == UIState::Normal)
        {
            m_state = UIState::MouseOver;
        }
        // If pressed, stay pressed
    }
    else
    {
        // If not pressed, go back to normal
        if (m_state == UIState::MouseOver)
        {
            m_state = UIState::Normal;
        }
    }

    if (m_state != oldState)
    {
        // Play hover sound when entering MouseOver state
        // Based on CCtrlButton::OnMouseMove using StringPool 0x9A5 (BtMouseOver)
        if (m_state == UIState::MouseOver)
        {
            SoundSystem::GetInstance().PlaySE(SOUND_BUTTON_HOVER, 100);
        }

        UpdateLayerCanvas();
    }
}

void UIButton::OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button)
{
    if (!m_bEnabled || button != 1)
    {
        return;
    }

    if (HitTest(x, y))
    {
        m_state = UIState::Pressed;
        UpdateLayerCanvas();
    }
}

void UIButton::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button)
{
    if (!m_bEnabled || button != 1)
    {
        return;
    }

    if (m_state == UIState::Pressed)
    {
        if (HitTest(x, y))
        {
            // Play click sound
            // Based on CCtrlButton::OnKey using StringPool 0x9A4 (BtMouseClick)
            SoundSystem::GetInstance().PlaySE(SOUND_BUTTON_CLICK, 100);

            // Toggle checked state if in checkbox mode
            if (m_bCheckMode)
            {
                ToggleChecked();
            }

            InvokeClick();
            m_state = UIState::MouseOver;
        }
        else
        {
            m_state = UIState::Normal;
        }
        UpdateLayerCanvas();
    }
}

// ========== Checkbox Mode ==========

void UIButton::SetChecked(bool bChecked)
{
    // Based on CCtrlButton::SetCheck
    if (m_bChecked != bChecked)
    {
        m_bChecked = bChecked;
        UpdateLayerCanvas();
    }
}

void UIButton::ToggleChecked()
{
    SetChecked(!m_bChecked);
}

void UIButton::CreateLayer(WzGr2D& gr, std::int32_t z, bool screenSpace)
{
    m_nZ = z;

    // Get the current canvas
    auto canvas = GetCurrentCanvas();

    // MapleStory coordinate system:
    // - absPos = desired RENDER position on screen (where we want the button to appear)
    // - WzGr2DLayer::Render formula: renderPos = layerPos - canvasOrigin
    // - Therefore: layerPos = absPos + canvasOrigin (to compensate for origin subtraction)
    auto absPos = GetAbsolutePosition();

    // Compensate for canvas origin (matching CUIChannelSelect background logic)
    auto canvasOrigin = canvas ? canvas->GetOrigin() : Point2D{0, 0};
    std::int32_t layerX = absPos.x + canvasOrigin.x;
    std::int32_t layerY = absPos.y + canvasOrigin.y;

    // Create a layer for this button
    auto layer = gr.CreateLayer(layerX, layerY,
                                 static_cast<std::uint32_t>(m_nWidth),
                                 static_cast<std::uint32_t>(m_nHeight), z);

    if (layer)
    {
        // UI buttons are typically in screen space (not affected by camera)
        layer->SetScreenSpace(screenSpace);

        // Add the normal state canvas initially
        if (canvas)
        {
            layer->InsertCanvas(canvas, 0, 255, 255);
        }

        // Set layer (this triggers auto-registration with DebugOverlay)
        SetLayer(layer);
    }
}

void UIButton::Update()
{
    UIElement::Update();

    // Update layer position using absolute coordinates (supports parent-child hierarchy)
    // sdlms style: anchor point = where origin appears on screen
    if (m_pLayer)
    {
        auto absPos = GetAbsolutePosition();
        m_pLayer->SetPosition(absPos.x, absPos.y);
    }
}

void UIButton::Draw()
{
    // Layer rendering is handled by WzGr2D
}

auto UIButton::HitTest(std::int32_t x, std::int32_t y) const -> bool
{
    if (!m_bVisible)
    {
        return false;
    }

    // sdlms coordinate system with parent-child support:
    // - GetAbsolutePosition() = anchor point in screen coordinates
    // - Actual render bounds = (absPos - origin) to (absPos - origin + size)
    auto canvas = GetCurrentCanvas();
    auto absPos = GetAbsolutePosition();
    int left = absPos.x;
    int top = absPos.y;

    if (canvas)
    {
        auto origin = canvas->GetOrigin();
        left -= origin.x;
        top -= origin.y;
    }

    return x >= left && x < left + m_nWidth &&
           y >= top && y < top + m_nHeight;
}

void UIButton::UpdateLayerCanvas()
{
    if (!m_pLayer)
    {
        return;
    }

    // Check if state or checked status changed
    if (m_state == m_lastState && m_bChecked == m_lastChecked)
    {
        return;
    }

    m_lastState = m_state;
    m_lastChecked = m_bChecked;

    // Update the layer's canvas to match current state
    auto canvas = GetCurrentCanvas();
    if (canvas)
    {
        m_pLayer->RemoveAllCanvases();
        m_pLayer->InsertCanvas(canvas, 0, 255, 255);
        // Layer position stays at m_position (anchor point)
        // Different canvas origins will render at different offsets automatically
    }
}

} // namespace ms
