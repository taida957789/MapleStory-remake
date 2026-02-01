#pragma once

#include "util/Point.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class WzGr2DLayer;
class WzProperty;

/**
 * @brief UI Element state for buttons and interactive elements
 */
enum class UIState : std::int32_t
{
    Normal = 0,
    MouseOver = 1,
    Pressed = 2,
    Disabled = 3
};

/**
 * @brief Origin type for positioning (from CWnd/CChildWnd)
 *
 * Determines how the position coordinates are interpreted.
 */
enum class OriginType : std::int32_t
{
    LeftTop = 0,     // Origin_LT - position is top-left corner
    RightTop = 1,    // Origin_RT - position is top-right corner
    LeftBottom = 2,  // Origin_LB - position is bottom-left corner
    RightBottom = 3, // Origin_RB - position is bottom-right corner
    Center = 4       // Origin_C - position is center
};

/**
 * @brief Base class for all UI elements
 *
 * Based on CWnd from the original MapleStory client.
 * Provides:
 * - Parent-child hierarchy (like CWnd)
 * - Relative positioning (child position relative to parent)
 * - Origin type support (Origin_LT, Origin_RB, etc.)
 * - Focus management (SetFocusChild)
 * - Notification system (NotifyToParent)
 *
 * Coordinate System (sdlms-equivalent):
 * - m_position is the LOCAL position (relative to parent)
 * - GetAbsolutePosition() returns screen coordinates
 * - Layer positions use absolute coordinates
 */
class UIElement : public std::enable_shared_from_this<UIElement>
{
public:
    using ClickCallback = std::function<void()>;

    UIElement();
    virtual ~UIElement();

    // Non-copyable, movable
    UIElement(const UIElement&) = delete;
    auto operator=(const UIElement&) -> UIElement& = delete;
    UIElement(UIElement&&) noexcept = default;
    auto operator=(UIElement&&) noexcept -> UIElement& = default;

    // ========== Parent-Child Hierarchy (CWnd style) ==========

    /**
     * @brief Get parent element
     */
    [[nodiscard]] auto GetParent() const noexcept -> UIElement* { return m_pParent; }

    /**
     * @brief Set parent element
     * @note This also updates the absolute position
     */
    void SetParent(UIElement* parent);

    /**
     * @brief Add a child element
     * @param child The child to add
     */
    void AddChild(std::shared_ptr<UIElement> child);

    /**
     * @brief Remove a child element
     * @param child The child to remove
     */
    void RemoveChild(UIElement* child);

    /**
     * @brief Get all children
     */
    [[nodiscard]] auto GetChildren() const noexcept
        -> const std::vector<std::shared_ptr<UIElement>>& { return m_aChildren; }

    /**
     * @brief Set focus to a child control (CWnd::SetFocusChild)
     * @param child The child to focus, or nullptr to clear focus
     */
    void SetFocusChild(UIElement* child);

    /**
     * @brief Get the currently focused child
     */
    [[nodiscard]] auto GetFocusChild() const noexcept -> UIElement* { return m_pFocusChild; }

    /**
     * @brief Notify parent of an event (CWnd::NotifyToParent)
     * @param msg Message type
     * @param param Message parameter
     */
    virtual void NotifyToParent(std::int32_t msg, std::int32_t param);

    // ========== Position (relative to parent) ==========

    /**
     * @brief Get LOCAL position (relative to parent)
     */
    [[nodiscard]] auto GetPosition() const noexcept -> Point2D { return m_position; }

    /**
     * @brief Set LOCAL position (relative to parent)
     */
    void SetPosition(const Point2D& pos) noexcept { m_position = pos; }
    void SetPosition(std::int32_t x, std::int32_t y) noexcept { m_position = {x, y}; }

    /**
     * @brief Get ABSOLUTE position (screen coordinates)
     *
     * This is calculated as: parent.absolutePos + localPos
     * For root elements, this equals m_position.
     */
    [[nodiscard]] auto GetAbsolutePosition() const noexcept -> Point2D;

    /**
     * @brief Set origin type for positioning
     */
    void SetOriginType(OriginType origin) noexcept { m_originType = origin; }
    [[nodiscard]] auto GetOriginType() const noexcept -> OriginType { return m_originType; }

    // ========== Bounds ==========

    [[nodiscard]] auto GetBounds() const noexcept -> Rect;
    [[nodiscard]] auto GetAbsoluteBounds() const noexcept -> Rect;
    [[nodiscard]] auto GetWidth() const noexcept -> std::int32_t { return m_nWidth; }
    [[nodiscard]] auto GetHeight() const noexcept -> std::int32_t { return m_nHeight; }
    void SetSize(std::int32_t width, std::int32_t height) noexcept;

    // ========== Control ID (from CCtrlWnd) ==========

    /**
     * @brief Get control ID
     * @note Used by LayoutMan for control identification and replacement
     */
    [[nodiscard]] auto GetID() const noexcept -> std::uint32_t { return m_nCtrlId; }

    /**
     * @brief Set control ID
     * @note Used by LayoutMan when creating controls
     */
    void SetID(std::uint32_t nID) noexcept { m_nCtrlId = nID; }

    // ========== Visibility & State ==========

    [[nodiscard]] auto IsVisible() const noexcept -> bool { return m_bVisible; }
    void SetVisible(bool visible) noexcept { m_bVisible = visible; }

    [[nodiscard]] auto IsEnabled() const noexcept -> bool { return m_bEnabled; }
    void SetEnabled(bool enabled) noexcept;

    [[nodiscard]] auto GetZ() const noexcept -> std::int32_t { return m_nZ; }
    void SetZ(std::int32_t z) noexcept { m_nZ = z; }

    // ========== Hit Testing ==========

    /**
     * @brief Hit test using ABSOLUTE screen coordinates
     */
    [[nodiscard]] virtual auto HitTest(std::int32_t x, std::int32_t y) const -> bool;

    // ========== Input Handling ==========

    virtual void OnMouseMove(std::int32_t x, std::int32_t y);
    virtual void OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button);
    virtual void OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button);
    virtual void OnKeyDown(std::int32_t keyCode);
    virtual void OnKeyUp(std::int32_t keyCode);

    /**
     * @brief Called when focus state changes
     * @param bFocus true if gaining focus, false if losing focus
     * @return true if the element accepts focus, false to reject
     */
    virtual auto OnSetFocus(bool bFocus) -> bool;

    // ========== Callbacks ==========

    void SetClickCallback(ClickCallback callback) { m_clickCallback = std::move(callback); }

    // ========== Update & Render ==========

    virtual void Update();
    virtual void Draw();

    // ========== Layer Access ==========

    [[nodiscard]] auto GetLayer() const noexcept -> std::shared_ptr<WzGr2DLayer> { return m_pLayer; }
    void SetLayer(std::shared_ptr<WzGr2DLayer> layer);

    // ========== Debug ==========

#ifdef MS_DEBUG_CANVAS
    /**
     * @brief Get debug type name for this element
     * @return Friendly type name (e.g., "UIButton", "UIEdit")
     *
     * Override in derived classes to provide meaningful names.
     * Default implementation uses typeid.
     */
    [[nodiscard]] virtual auto GetDebugTypeName() const -> std::string;
#endif

protected:
    void InvokeClick();

    // Local position (relative to parent)
    Point2D m_position{0, 0};
    std::int32_t m_nWidth{0};
    std::int32_t m_nHeight{0};
    std::int32_t m_nZ{0};
    bool m_bVisible{true};
    bool m_bEnabled{true};
    UIState m_state{UIState::Normal};
    OriginType m_originType{OriginType::LeftTop};

    // Control ID (from CCtrlWnd::m_nCtrlId)
    std::uint32_t m_nCtrlId{0};

    // Parent-child hierarchy (CWnd style)
    UIElement* m_pParent{nullptr};  // Not owned
    std::vector<std::shared_ptr<UIElement>> m_aChildren;
    UIElement* m_pFocusChild{nullptr};  // Not owned

    std::shared_ptr<WzGr2DLayer> m_pLayer;
    ClickCallback m_clickCallback;
};

} // namespace ms
