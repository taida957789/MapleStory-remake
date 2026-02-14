#include "CtrlWnd.h"
#include <vector>

namespace ms
{

CtrlWnd::CtrlWnd() = default;

CtrlWnd::~CtrlWnd() = default;

// === IGObj ===

void CtrlWnd::Update()
{
}

// === IUIMsgHandler ===

void CtrlWnd::OnKey(std::uint32_t /*nKey*/, std::uint32_t /*nFlag*/)
{
}

auto CtrlWnd::OnSetFocus(std::int32_t /*bFocus*/) -> std::int32_t
{
    return 0;
}

void CtrlWnd::OnMouseButton(
    std::uint32_t /*nType*/, std::uint32_t /*nFlag*/,
    std::int32_t /*x*/, std::int32_t /*y*/)
{
}

auto CtrlWnd::OnMouseMove(std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    return 0;
}

auto CtrlWnd::OnMouseWheel(
    std::int32_t /*nDelta*/,
    std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    return 0;
}

void CtrlWnd::OnMouseEnter(std::int32_t /*bEnter*/)
{
}

void CtrlWnd::OnDraggableMove(
    std::int32_t /*nType*/, IDraggable* /*pDraggable*/,
    std::int32_t /*x*/, std::int32_t /*y*/)
{
}

auto CtrlWnd::OnDragEnd(
    CDraggableSkill* /*pSkill*/, IUIMsgHandler* /*pTarget*/,
    std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    return 0;
}

auto CtrlWnd::OnDragEnd(
    CDraggableItem* /*pItem*/, IUIMsgHandler* /*pTarget*/,
    std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    return 0;
}

auto CtrlWnd::IsUsingDragEnd() -> std::int32_t
{
    return 0;
}

auto CtrlWnd::CanPutItemIntoSlot(
    std::int32_t /*nSlot*/,
    const std::shared_ptr<GW_ItemSlotBase>& /*pItem*/) -> std::int32_t
{
    return 0;
}

void CtrlWnd::SetEnable(std::int32_t bEnable)
{
    m_bEnabled = bEnable;
}

auto CtrlWnd::IsEnabled() -> std::int32_t
{
    return m_bEnabled;
}

void CtrlWnd::SetShow(std::int32_t bShow)
{
    m_bShown = bShow;
}

auto CtrlWnd::IsShown() -> std::int32_t
{
    return m_bShown;
}

auto CtrlWnd::GetAbsLeft() -> std::int32_t
{
    return 0;
}

auto CtrlWnd::GetAbsTop() -> std::int32_t
{
    return 0;
}

void CtrlWnd::ClearToolTip()
{
}

void CtrlWnd::OnIMEModeChange(char /*nMode*/)
{
}

void CtrlWnd::OnIMEResult(const char* /*szResult*/)
{
}

void CtrlWnd::OnIMEComp(
    const char* /*szComp*/,
    std::vector<std::uint32_t>* /*aAttr*/,
    std::uint32_t /*nCursor*/,
    std::int32_t /*bInsert*/,
    std::vector<std::string>* /*lCandList*/,
    std::int32_t /*nCandIdx*/,
    std::int32_t /*nCandPageStart*/,
    std::int32_t /*nCandPageSize*/)
{
}

void CtrlWnd::OnTouchPanBegin(std::int32_t /*x*/, std::int32_t /*y*/)
{
}

void CtrlWnd::OnTouchPanEnter(std::int32_t /*bEnter*/)
{
}

void CtrlWnd::OnTouchPanMoveWithDragCtx(std::int32_t /*x*/, std::int32_t /*y*/)
{
}

void CtrlWnd::OnTouchPanMoveWithNothing(std::int32_t /*x*/, std::int32_t /*y*/)
{
}

void CtrlWnd::OnTouchHorizontalFlick(std::int32_t /*nDirection*/)
{
}

auto CtrlWnd::OnTouchVerticalScroll(std::int32_t /*nDelta*/) -> std::int32_t
{
    return 0;
}

void CtrlWnd::OnTouchZoomOut()
{
}

void CtrlWnd::OnTouchZoomIn()
{
}

void CtrlWnd::OnTouchTwoFingerTap(
    std::int32_t /*x*/, std::int32_t /*y*/, std::int32_t /*nParam*/)
{
}

// === CtrlWnd vtable ===

auto CtrlWnd::OnDragDrop(
    std::int32_t /*nType*/, DRAGCTX* /*pCtx*/,
    std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    return 0;
}

void CtrlWnd::CreateCtrl(
    Wnd* pParent, std::uint32_t nCtrlId,
    std::int32_t /*x*/, std::int32_t /*y*/,
    std::int32_t cx, std::int32_t cy,
    void* /*pParam*/)
{
    m_pParent = pParent;
    m_nCtrlId = nCtrlId;
    m_width = cx;
    m_height = cy;
}

void CtrlWnd::Destroy()
{
    OnDestroy();
}

void CtrlWnd::OnCreate(void* /*pParam*/)
{
}

void CtrlWnd::OnDestroy()
{
}

auto CtrlWnd::HitTest(std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    return 0;
}

auto CtrlWnd::GetRect() -> Rect
{
    return {};
}

void CtrlWnd::SetAbove(CtrlWnd* /*pCtrl*/)
{
}

void CtrlWnd::Draw(
    std::int32_t /*x*/, std::int32_t /*y*/, const Rect* /*pRect*/)
{
}

auto CtrlWnd::GetX() -> std::int32_t
{
    return 0;
}

auto CtrlWnd::GetY() -> std::int32_t
{
    return 0;
}

void CtrlWnd::CreateCtrl(
    Wnd* pParent, std::uint32_t nCtrlId,
    std::int32_t x, std::int32_t y,
    std::int32_t cx, std::int32_t cy,
    std::int32_t /*nParam*/, void* pParam)
{
    CreateCtrl(pParent, nCtrlId, x, y, cx, cy, pParam);
}

} // namespace ms
