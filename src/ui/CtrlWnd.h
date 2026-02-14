#pragma once

#include "IUIMsgHandler.h"
#include "app/IGObj.h"
#include "util/Point.h"

#include <cstdint>
#include <memory>

namespace ms
{

class Wnd;
class IWzVector2D;
struct DRAGCTX;

/**
 * @brief Base class for child controls within a Wnd
 *
 * Based on CCtrlWnd from the original MapleStory client (v1029).
 * Inherits from IGObj (game object update), IUIMsgHandler (input handling).
 *
 * Original: __cppobj CCtrlWnd : IGObj, IUIMsgHandler, ZRefCounted
 *
 * Each CtrlWnd has a parent Wnd and a position vector (m_pLTCtrl)
 * that is chained to the parent's control layer.
 */
class CtrlWnd : public IGObj, public IUIMsgHandler
{
public:
    CtrlWnd();
    ~CtrlWnd() override;

    // Non-copyable
    CtrlWnd(const CtrlWnd&) = delete;
    auto operator=(const CtrlWnd&) -> CtrlWnd& = delete;

    // === IGObj ===
    void Update() override;

    // === IUIMsgHandler default implementations ===
    void OnKey(std::uint32_t nKey, std::uint32_t nFlag) override;
    auto OnSetFocus(std::int32_t bFocus) -> std::int32_t override;
    void OnMouseButton(
        std::uint32_t nType, std::uint32_t nFlag,
        std::int32_t x, std::int32_t y) override;
    auto OnMouseMove(std::int32_t x, std::int32_t y) -> std::int32_t override;
    auto OnMouseWheel(
        std::int32_t nDelta,
        std::int32_t x, std::int32_t y) -> std::int32_t override;
    void OnMouseEnter(std::int32_t bEnter) override;

    void OnDraggableMove(
        std::int32_t nType, IDraggable* pDraggable,
        std::int32_t x, std::int32_t y) override;
    auto OnDragEnd(
        CDraggableSkill* pSkill, IUIMsgHandler* pTarget,
        std::int32_t x, std::int32_t y) -> std::int32_t override;
    auto OnDragEnd(
        CDraggableItem* pItem, IUIMsgHandler* pTarget,
        std::int32_t x, std::int32_t y) -> std::int32_t override;
    auto IsUsingDragEnd() -> std::int32_t override;
    auto CanPutItemIntoSlot(
        std::int32_t nSlot,
        const ZRef<GW_ItemSlotBase>& pItem) -> std::int32_t override;

    void SetEnable(std::int32_t bEnable) override;
    [[nodiscard]] auto IsEnabled() -> std::int32_t override;
    void SetShow(std::int32_t bShow) override;
    [[nodiscard]] auto IsShown() -> std::int32_t override;

    [[nodiscard]] auto GetAbsLeft() -> std::int32_t override;
    [[nodiscard]] auto GetAbsTop() -> std::int32_t override;

    void ClearToolTip() override;

    void OnIMEModeChange(char nMode) override;
    void OnIMEResult(const char* szResult) override;
    void OnIMEComp(
        const char* szComp,
        ZArray<std::uint32_t>* aAttr,
        std::uint32_t nCursor,
        std::int32_t bInsert,
        ZList<ZXString<char>>* lCandList,
        std::int32_t nCandIdx,
        std::int32_t nCandPageStart,
        std::int32_t nCandPageSize) override;

    void OnTouchPanBegin(std::int32_t x, std::int32_t y) override;
    void OnTouchPanEnter(std::int32_t bEnter) override;
    void OnTouchPanMoveWithDragCtx(std::int32_t x, std::int32_t y) override;
    void OnTouchPanMoveWithNothing(std::int32_t x, std::int32_t y) override;
    void OnTouchHorizontalFlick(std::int32_t nDirection) override;
    auto OnTouchVerticalScroll(std::int32_t nDelta) -> std::int32_t override;
    void OnTouchZoomOut() override;
    void OnTouchZoomIn() override;
    void OnTouchTwoFingerTap(
        std::int32_t x, std::int32_t y, std::int32_t nParam) override;

    // === CtrlWnd vtable ===
    virtual auto OnDragDrop(
        std::int32_t nType, DRAGCTX* pCtx,
        std::int32_t x, std::int32_t y) -> std::int32_t;
    virtual void CreateCtrl(
        Wnd* pParent, std::uint32_t nCtrlId,
        std::int32_t x, std::int32_t y,
        std::int32_t cx, std::int32_t cy,
        void* pParam);
    virtual void Destroy();
    virtual void OnCreate(void* pParam);
    virtual void OnDestroy();
    virtual auto HitTest(std::int32_t x, std::int32_t y) -> std::int32_t;
    virtual auto GetRect() -> Rect;
    virtual void SetAbove(CtrlWnd* pCtrl);
    virtual void Draw(
        std::int32_t x, std::int32_t y, const Rect* pRect);
    virtual auto GetX() -> std::int32_t;
    virtual auto GetY() -> std::int32_t;
    virtual void CreateCtrl(
        Wnd* pParent, std::uint32_t nCtrlId,
        std::int32_t x, std::int32_t y,
        std::int32_t cx, std::int32_t cy,
        std::int32_t nParam, void* pParam);

protected:
    // === Member data (from IDA struct) ===
    std::uint32_t m_nCtrlId{};
    std::int32_t m_tRelMove{};

    /// Position vector (_com_ptr_t<IWzVector2D>)
    std::shared_ptr<IWzVector2D> m_pLTCtrl;

    std::int32_t m_width{};
    std::int32_t m_height{};

    /// Parent window (raw pointer, not owned)
    Wnd* m_pParent{};

    std::int32_t m_bAcceptFocus{};
    std::int32_t m_bEnabled{1};
    std::int32_t m_bShown{1};
};

} // namespace ms
