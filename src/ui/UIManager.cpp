#include "UIManager.h"
#include "UIEdit.h"
#include "UIElement.h"

#include <algorithm>

namespace ms
{

UIManager::UIManager() = default;

UIManager::~UIManager() = default;

void UIManager::AddElement(const std::string& name, std::shared_ptr<UIElement> element)
{
    if (!element)
    {
        return;
    }

    m_elementMap[name] = element;
    m_elements.push_back(element);
    m_bSorted = false;
}

void UIManager::RemoveElement(const std::string& name)
{
    auto it = m_elementMap.find(name);
    if (it == m_elementMap.end())
    {
        return;
    }

    auto element = it->second;
    m_elementMap.erase(it);

    // Remove from vector
    m_elements.erase(
        std::remove(m_elements.begin(), m_elements.end(), element),
        m_elements.end());

    if (m_pFocusedElement == element)
    {
        m_pFocusedElement.reset();
    }
}

auto UIManager::GetElement(const std::string& name) -> std::shared_ptr<UIElement>
{
    auto it = m_elementMap.find(name);
    if (it != m_elementMap.end())
    {
        return it->second;
    }
    return nullptr;
}

void UIManager::Clear()
{
    m_elementMap.clear();
    m_elements.clear();
    m_pFocusedElement.reset();
    m_bSorted = false;
}

auto UIManager::OnMouseMove(std::int32_t x, std::int32_t y) -> bool
{
    SortElements();

    // Based on CWndMan - check capture first
    if (m_pCapturedElement)
    {
        if (m_pCapturedElement->IsVisible())
        {
            m_pCapturedElement->OnMouseMove(x, y);
            return true;
        }
    }

    bool handled = false;

    // Make a copy to avoid iterator invalidation if handlers modify m_elements
    auto elementsCopy = m_elements;

    // Dispatch to all elements (for hover states)
    for (auto& element : elementsCopy)
    {
        if (element && element->IsVisible())
        {
            element->OnMouseMove(x, y);
            if (element->HitTest(x, y))
            {
                handled = true;
            }
        }
    }

    return handled;
}

auto UIManager::OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button) -> bool
{
    SortElements();

    // Based on CWndMan::GetHandlerFromPoint - check capture first
    if (m_pCapturedElement)
    {
        if (m_pCapturedElement->IsVisible() && m_pCapturedElement->IsEnabled())
        {
            m_pCapturedElement->OnMouseDown(x, y, button);
            return true;
        }
    }

    // Make a copy to avoid iterator invalidation if handlers modify m_elements
    auto elementsCopy = m_elements;

    // Dispatch to topmost element that handles the click
    // Iterate in reverse Z-order (highest Z first)
    for (auto it = elementsCopy.rbegin(); it != elementsCopy.rend(); ++it)
    {
        auto& element = *it;
        if (element && element->IsVisible() && element->IsEnabled())
        {
            if (element->HitTest(x, y))
            {
                element->OnMouseDown(x, y, button);
                // Set focus using the proper method
                SetFocusedElement(element);
                return true;
            }
        }
    }

    // Click on nothing - clear focus
    SetFocusedElement(nullptr);
    return false;
}

auto UIManager::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button) -> bool
{
    SortElements();

    // Based on CWndMan - check capture first
    if (m_pCapturedElement)
    {
        if (m_pCapturedElement->IsVisible())
        {
            m_pCapturedElement->OnMouseUp(x, y, button);
            return true;
        }
    }

    bool handled = false;

    // Make a copy to avoid iterator invalidation if handlers modify m_elements
    auto elementsCopy = m_elements;

    // Dispatch to all elements (for release handling)
    for (auto& element : elementsCopy)
    {
        if (element && element->IsVisible())
        {
            element->OnMouseUp(x, y, button);
            if (element->HitTest(x, y))
            {
                handled = true;
            }
        }
    }

    return handled;
}

void UIManager::OnKeyDown(std::int32_t keyCode)
{
    // Dispatch to focused element
    if (m_pFocusedElement && m_pFocusedElement->IsVisible() && m_pFocusedElement->IsEnabled())
    {
        m_pFocusedElement->OnKeyDown(keyCode);
    }
}

void UIManager::OnKeyUp(std::int32_t keyCode)
{
    if (m_pFocusedElement && m_pFocusedElement->IsVisible() && m_pFocusedElement->IsEnabled())
    {
        m_pFocusedElement->OnKeyUp(keyCode);
    }
}

void UIManager::OnTextInput(const std::string& text)
{
    // Dispatch text input to focused UIEdit element
    if (m_pFocusedElement && m_pFocusedElement->IsVisible() && m_pFocusedElement->IsEnabled())
    {
        auto* edit = dynamic_cast<UIEdit*>(m_pFocusedElement.get());
        if (edit)
        {
            edit->OnTextInput(text);
        }
    }
}

void UIManager::SetFocusedElement(std::shared_ptr<UIElement> element)
{
    // Based on CWndMan::SetFocusImp from the original client
    // Skip if setting focus to the same element
    if (m_pFocusedElement == element)
    {
        return;
    }

    // Check if new element accepts focus (or if clearing focus)
    if (element && !element->IsEnabled())
    {
        return;
    }

    // Try to set focus on new element first
    if (element)
    {
        if (!element->OnSetFocus(true))
        {
            // Element rejected focus
            return;
        }
    }

    // Notify old element it's losing focus
    if (m_pFocusedElement)
    {
        m_pFocusedElement->OnSetFocus(false);
    }

    // Update focus pointer
    m_pFocusedElement = std::move(element);
}

void UIManager::SetCapture(std::shared_ptr<UIElement> element)
{
    // Based on CWndMan::m_lpCapture from the original client
    m_pCapturedElement = std::move(element);
}

void UIManager::ReleaseCapture(UIElement* element)
{
    // Based on CWndMan::ReleaseCaptureWnd from the original client
    if (m_pCapturedElement.get() == element)
    {
        m_pCapturedElement.reset();
    }
}

void UIManager::Update()
{
    // Make a copy to avoid iterator invalidation if handlers modify m_elements
    auto elementsCopy = m_elements;

    for (auto& element : elementsCopy)
    {
        if (element)
        {
            element->Update();
        }
    }
}

void UIManager::Draw()
{
    SortElements();

    // Make a copy to avoid iterator invalidation if handlers modify m_elements
    auto elementsCopy = m_elements;

    for (auto& element : elementsCopy)
    {
        if (element && element->IsVisible())
        {
            element->Draw();
        }
    }
}

void UIManager::SortElements()
{
    if (m_bSorted)
    {
        return;
    }

    // Sort by Z-order (lowest first, so they render bottom to top)
    std::sort(m_elements.begin(), m_elements.end(),
              [](const std::shared_ptr<UIElement>& a, const std::shared_ptr<UIElement>& b) {
                  if (!a) return true;
                  if (!b) return false;
                  return a->GetZ() < b->GetZ();
              });

    m_bSorted = true;
}

} // namespace ms
