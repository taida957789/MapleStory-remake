#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ms
{

class UIElement;

/**
 * @brief UI Manager - manages UI elements and dispatches input
 *
 * Handles mouse/keyboard input routing to UI elements.
 * Elements are sorted by Z-order for proper event dispatch.
 */
class UIManager
{
public:
    UIManager();
    ~UIManager();

    // Non-copyable
    UIManager(const UIManager&) = delete;
    auto operator=(const UIManager&) -> UIManager& = delete;
    UIManager(UIManager&&) noexcept = default;
    auto operator=(UIManager&&) noexcept -> UIManager& = default;

    /**
     * @brief Add a UI element
     * @param name Element name for lookup
     * @param element The element to add
     */
    void AddElement(const std::string& name, std::shared_ptr<UIElement> element);

    /**
     * @brief Remove a UI element by name
     */
    void RemoveElement(const std::string& name);

    /**
     * @brief Get a UI element by name
     */
    [[nodiscard]] auto GetElement(const std::string& name) -> std::shared_ptr<UIElement>;

    /**
     * @brief Clear all elements
     */
    void Clear();

    /**
     * @brief Process mouse movement
     * @return true if any element handled the input
     */
    auto OnMouseMove(std::int32_t x, std::int32_t y) -> bool;

    /**
     * @brief Process mouse button down
     * @return true if any element handled the input
     */
    auto OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button) -> bool;

    /**
     * @brief Process mouse button up
     * @return true if any element handled the input
     */
    auto OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button) -> bool;

    /**
     * @brief Process key down
     */
    void OnKeyDown(std::int32_t keyCode);

    /**
     * @brief Process key up
     */
    void OnKeyUp(std::int32_t keyCode);

    /**
     * @brief Process text input (for typing)
     */
    void OnTextInput(const std::string& text);

    /**
     * @brief Set the focused element for keyboard input
     *
     * Based on CWndMan::SetFocus from the original client.
     * Calls OnSetFocus(false) on old element, OnSetFocus(true) on new element.
     * Only sets focus if the element accepts it (OnSetFocus returns true).
     */
    void SetFocusedElement(std::shared_ptr<UIElement> element);

    /**
     * @brief Get the focused element
     */
    [[nodiscard]] auto GetFocusedElement() const -> std::shared_ptr<UIElement> { return m_pFocusedElement; }

    /**
     * @brief Set capture to an element (receives all mouse input)
     *
     * Based on CWndMan::m_lpCapture from the original client.
     * Captured element receives all mouse events regardless of hit testing.
     */
    void SetCapture(std::shared_ptr<UIElement> element);

    /**
     * @brief Release capture from an element
     */
    void ReleaseCapture(UIElement* element);

    /**
     * @brief Get the captured element
     */
    [[nodiscard]] auto GetCapture() const -> std::shared_ptr<UIElement> { return m_pCapturedElement; }

    /**
     * @brief Update all elements
     */
    void Update();

    /**
     * @brief Draw all elements (triggers layer updates)
     */
    void Draw();

private:
    void SortElements();

    std::unordered_map<std::string, std::shared_ptr<UIElement>> m_elementMap;
    std::vector<std::shared_ptr<UIElement>> m_elements;
    bool m_bSorted{false};

    // Track focused element for keyboard input (CWndMan::m_pFocus)
    std::shared_ptr<UIElement> m_pFocusedElement;

    // Track captured element for mouse input (CWndMan::m_lpCapture)
    std::shared_ptr<UIElement> m_pCapturedElement;
};

} // namespace ms
