#include "UIElement.h"
#include "graphics/WzGr2DLayer.h"

#ifdef MS_DEBUG_CANVAS
#include "debug/DebugOverlay.h"
#endif

#include <algorithm>
#include <typeinfo>

namespace ms
{

UIElement::UIElement() = default;

UIElement::~UIElement()
{
#ifdef MS_DEBUG_CANVAS
    DebugOverlay::GetInstance().UnregisterUIElement(this);
#endif
}

// ========== Parent-Child Hierarchy (CWnd style) ==========

void UIElement::SetParent(UIElement* parent)
{
    m_pParent = parent;
}

void UIElement::AddChild(std::shared_ptr<UIElement> child)
{
    if (!child)
    {
        return;
    }

    // Set this element as parent
    child->SetParent(this);

    // Add to children collection
    m_aChildren.push_back(std::move(child));
}

void UIElement::RemoveChild(UIElement* child)
{
    if (!child)
    {
        return;
    }

    // Clear focus if this child was focused
    if (m_pFocusChild == child)
    {
        m_pFocusChild = nullptr;
    }

    // Clear child's parent
    child->SetParent(nullptr);

    // Remove from collection
    auto it = std::find_if(m_aChildren.begin(), m_aChildren.end(),
        [child](const std::shared_ptr<UIElement>& elem) {
            return elem.get() == child;
        });

    if (it != m_aChildren.end())
    {
        m_aChildren.erase(it);
    }
}

void UIElement::SetFocusChild(UIElement* child)
{
    // Clear focus on previous child
    if (m_pFocusChild && m_pFocusChild != child)
    {
        m_pFocusChild->OnSetFocus(false);
    }

    m_pFocusChild = child;

    // Set focus on new child
    if (m_pFocusChild)
    {
        m_pFocusChild->OnSetFocus(true);
    }
}

void UIElement::NotifyToParent(std::int32_t msg, std::int32_t param)
{
    // Forward notification to parent (CWnd::NotifyToParent style)
    if (m_pParent)
    {
        m_pParent->NotifyToParent(msg, param);
    }
}

// ========== Position ==========

auto UIElement::GetAbsolutePosition() const noexcept -> Point2D
{
    // sdlms-equivalent: absolutePos = parent.absolutePos + localPos
    if (m_pParent)
    {
        auto parentAbsPos = m_pParent->GetAbsolutePosition();
        return {
            parentAbsPos.x + m_position.x,
            parentAbsPos.y + m_position.y
        };
    }

    // Root element: absolute = local
    return m_position;
}

// ========== Bounds ==========

auto UIElement::GetBounds() const noexcept -> Rect
{
    return {
        m_position.x,
        m_position.y,
        m_position.x + m_nWidth,
        m_position.y + m_nHeight
    };
}

auto UIElement::GetAbsoluteBounds() const noexcept -> Rect
{
    auto absPos = GetAbsolutePosition();
    return {
        absPos.x,
        absPos.y,
        absPos.x + m_nWidth,
        absPos.y + m_nHeight
    };
}

void UIElement::SetSize(std::int32_t width, std::int32_t height) noexcept
{
    m_nWidth = width;
    m_nHeight = height;
}

void UIElement::SetEnabled(bool enabled) noexcept
{
    m_bEnabled = enabled;
    if (!enabled)
    {
        m_state = UIState::Disabled;
    }
    else if (m_state == UIState::Disabled)
    {
        m_state = UIState::Normal;
    }
}

auto UIElement::HitTest(std::int32_t x, std::int32_t y) const -> bool
{
    if (!m_bVisible)
    {
        return false;
    }

    // Use absolute bounds for hit testing (screen coordinates)
    auto absBounds = GetAbsoluteBounds();
    return x >= absBounds.left && x < absBounds.right &&
           y >= absBounds.top && y < absBounds.bottom;
}

void UIElement::OnMouseMove(std::int32_t x, std::int32_t y)
{
    if (!m_bEnabled || !m_bVisible)
    {
        return;
    }

    // Propagate to children first (CWnd style - reverse order for z-ordering)
    for (auto it = m_aChildren.rbegin(); it != m_aChildren.rend(); ++it)
    {
        if (*it && (*it)->IsVisible())
        {
            (*it)->OnMouseMove(x, y);
        }
    }

    // Handle own state
    if (HitTest(x, y))
    {
        if (m_state == UIState::Normal)
        {
            m_state = UIState::MouseOver;
        }
    }
    else
    {
        if (m_state == UIState::MouseOver)
        {
            m_state = UIState::Normal;
        }
    }
}

void UIElement::OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button)
{
    if (!m_bEnabled || !m_bVisible)
    {
        return;
    }

    // Propagate to children first (reverse order for z-ordering)
    for (auto it = m_aChildren.rbegin(); it != m_aChildren.rend(); ++it)
    {
        if (*it && (*it)->IsVisible() && (*it)->HitTest(x, y))
        {
            (*it)->OnMouseDown(x, y, button);
            return;  // Stop propagation if child handled it
        }
    }

    // Handle own state
    if (button != 1) // Left button only
    {
        return;
    }

    if (HitTest(x, y))
    {
        m_state = UIState::Pressed;
    }
}

void UIElement::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button)
{
    if (!m_bEnabled || !m_bVisible)
    {
        return;
    }

    // Propagate to children first
    for (auto it = m_aChildren.rbegin(); it != m_aChildren.rend(); ++it)
    {
        if (*it && (*it)->IsVisible())
        {
            (*it)->OnMouseUp(x, y, button);
        }
    }

    // Handle own state
    if (button != 1)
    {
        return;
    }

    if (m_state == UIState::Pressed)
    {
        if (HitTest(x, y))
        {
            InvokeClick();
            m_state = UIState::MouseOver;
        }
        else
        {
            m_state = UIState::Normal;
        }
    }
}

void UIElement::OnKeyDown(std::int32_t keyCode)
{
    // Forward to focused child (CWnd style)
    if (m_pFocusChild)
    {
        m_pFocusChild->OnKeyDown(keyCode);
    }
}

void UIElement::OnKeyUp(std::int32_t keyCode)
{
    // Forward to focused child (CWnd style)
    if (m_pFocusChild)
    {
        m_pFocusChild->OnKeyUp(keyCode);
    }
}

auto UIElement::OnSetFocus(bool /*bFocus*/) -> bool
{
    // Default implementation returns false (doesn't accept focus)
    // Based on IUIMsgHandler::OnSetFocus from original client
    return false;
}

void UIElement::Update()
{
    // Update layer visibility based on element visibility
    if (m_pLayer)
    {
        m_pLayer->SetVisible(m_bVisible);
    }

    // Update all children (CWnd style)
    for (auto& child : m_aChildren)
    {
        if (child)
        {
            child->Update();
        }
    }
}

void UIElement::Draw()
{
    // Layer rendering is handled by WzGr2D
    // Subclasses can override for custom drawing

    // Draw all children (CWnd style - draw in order, later = on top)
    for (auto& child : m_aChildren)
    {
        if (child && child->IsVisible())
        {
            child->Draw();
        }
    }
}

void UIElement::InvokeClick()
{
    if (m_clickCallback)
    {
        m_clickCallback();
    }
}

void UIElement::SetLayer(std::shared_ptr<WzGr2DLayer> layer)
{
    m_pLayer = std::move(layer);

#ifdef MS_DEBUG_CANVAS
    if (m_pLayer)
    {
        auto typeName = GetDebugTypeName();
        DebugOverlay::GetInstance().RegisterUIElement(
            this, m_pLayer, typeName + " Main Layer"
        );
    }
#endif
}

#ifdef MS_DEBUG_CANVAS
auto UIElement::GetDebugTypeName() const -> std::string
{
    // Default implementation using typeid
    // Derived classes should override for friendly names
    const char* name = typeid(*this).name();

    // Try to demangle C++ type names
    // For GCC/Clang, names are mangled (e.g., "N2ms9UIButtonE")
    // For MSVC, names are readable (e.g., "class ms::UIButton")
    std::string result(name);

    // Simple demangling for common patterns
    // Look for "UIButton", "UIEdit", etc. in the mangled name
    std::size_t pos = result.find("UI");
    if (pos != std::string::npos)
    {
        // Found "UI" prefix, extract until non-alphanumeric
        std::size_t end = pos;
        while (end < result.length() &&
               (std::isalnum(result[end]) || result[end] == '_'))
        {
            ++end;
        }
        return result.substr(pos, end - pos);
    }

    // Fallback: return raw typeid name
    return result;
}
#endif

} // namespace ms
