#pragma once

#include <cstdint>

namespace ms
{

// Forward declarations
class IDraggable;
class CDraggableSkill;
class CDraggableItem;
class GW_ItemSlotBase;

template <typename T> class ZRef;
template <typename T> class ZArray;
template <typename T> class ZList;
template <typename T> class ZXString;

/**
 * @brief Interface for UI message handling
 *
 * Based on IUIMsgHandler from the original MapleStory client (v1029).
 * Provides the virtual dispatch table for all UI input events,
 * visibility/enable state, drag-and-drop, IME, and touch.
 *
 * Implemented by CWnd (UIElement) and its derived classes.
 *
 * VFT layout (sizeof=0x80):
 *   0x00  OnKey
 *   0x04  OnSetFocus
 *   0x08  OnMouseButton
 *   0x0C  OnMouseMove
 *   0x10  OnMouseWheel
 *   0x14  OnMouseEnter
 *   0x18  OnDraggableMove
 *   0x1C  OnDragEnd (CDraggableSkill)
 *   0x20  OnDragEnd (CDraggableItem)
 *   0x24  IsUsingDragEnd
 *   0x28  CanPutItemIntoSlot
 *   0x2C  SetEnable
 *   0x30  IsEnabled
 *   0x34  SetShow
 *   0x38  IsShown
 *   0x3C  GetAbsLeft
 *   0x40  GetAbsTop
 *   0x44  ClearToolTip
 *   0x48  OnIMEModeChange
 *   0x4C  OnIMEResult
 *   0x50  OnIMEComp
 *   0x54  OnTouchPanBegin
 *   0x58  OnTouchPanEnter
 *   0x5C  OnTouchPanMoveWithDragCtx
 *   0x60  OnTouchPanMoveWithNothing
 *   0x64  OnTouchHorizontalFlick
 *   0x68  OnTouchVerticalScroll
 *   0x6C  OnTouchZoomOut
 *   0x70  OnTouchZoomIn
 *   0x74  OnTouchTwoFingerTap
 */
class IUIMsgHandler
{
public:
    virtual ~IUIMsgHandler() = default;

    // ========== Input Events ==========

    virtual void OnKey(std::uint32_t nKey, std::uint32_t nFlag) = 0;
    virtual auto OnSetFocus(std::int32_t bFocus) -> std::int32_t = 0;
    virtual void OnMouseButton(
        std::uint32_t nType, std::uint32_t nFlag,
        std::int32_t x, std::int32_t y) = 0;
    virtual auto OnMouseMove(std::int32_t x, std::int32_t y) -> std::int32_t = 0;
    virtual auto OnMouseWheel(
        std::int32_t nDelta, std::int32_t x, std::int32_t y) -> std::int32_t = 0;
    virtual void OnMouseEnter(std::int32_t bEnter) = 0;

    // ========== Drag-and-Drop ==========

    virtual void OnDraggableMove(
        std::int32_t nType, IDraggable* pDraggable,
        std::int32_t x, std::int32_t y) = 0;
    virtual auto OnDragEnd(
        CDraggableSkill* pSkill, IUIMsgHandler* pTarget,
        std::int32_t x, std::int32_t y) -> std::int32_t = 0;
    virtual auto OnDragEnd(
        CDraggableItem* pItem, IUIMsgHandler* pTarget,
        std::int32_t x, std::int32_t y) -> std::int32_t = 0;
    virtual auto IsUsingDragEnd() -> std::int32_t = 0;
    virtual auto CanPutItemIntoSlot(
        std::int32_t nSlot, const ZRef<GW_ItemSlotBase>& pItem) -> std::int32_t = 0;

    // ========== State ==========

    virtual void SetEnable(std::int32_t bEnable) = 0;
    [[nodiscard]] virtual auto IsEnabled() -> std::int32_t = 0;
    virtual void SetShow(std::int32_t bShow) = 0;
    [[nodiscard]] virtual auto IsShown() -> std::int32_t = 0;

    // ========== Position ==========

    [[nodiscard]] virtual auto GetAbsLeft() -> std::int32_t = 0;
    [[nodiscard]] virtual auto GetAbsTop() -> std::int32_t = 0;

    // ========== Tooltip ==========

    virtual void ClearToolTip() = 0;

    // ========== IME ==========

    virtual void OnIMEModeChange(char nMode) = 0;
    virtual void OnIMEResult(const char* szResult) = 0;
    virtual void OnIMEComp(
        const char* szComp,
        ZArray<std::uint32_t>* aAttr,
        std::uint32_t nCursor,
        std::int32_t bInsert,
        ZList<ZXString<char>>* lCandList,
        std::int32_t nCandIdx,
        std::int32_t nCandPageStart,
        std::int32_t nCandPageSize) = 0;

    // ========== Touch ==========

    virtual void OnTouchPanBegin(std::int32_t x, std::int32_t y) = 0;
    virtual void OnTouchPanEnter(std::int32_t bEnter) = 0;
    virtual void OnTouchPanMoveWithDragCtx(std::int32_t x, std::int32_t y) = 0;
    virtual void OnTouchPanMoveWithNothing(std::int32_t x, std::int32_t y) = 0;
    virtual void OnTouchHorizontalFlick(std::int32_t nDirection) = 0;
    virtual auto OnTouchVerticalScroll(std::int32_t nDelta) -> std::int32_t = 0;
    virtual void OnTouchZoomOut() = 0;
    virtual void OnTouchZoomIn() = 0;
    virtual void OnTouchTwoFingerTap(
        std::int32_t x, std::int32_t y, std::int32_t nParam) = 0;
};

} // namespace ms
