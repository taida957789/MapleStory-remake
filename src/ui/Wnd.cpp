#include "Wnd.h"

#include "CtrlWnd.h"
#include "WndMan.h"
#include "animation/AnimationDisplayer.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DCanvas.h"
#include "graphics/WzGr2DLayer.h"

namespace ms
{

Wnd::Wnd() = default;

Wnd::~Wnd() = default;

// === IGObj ===

void Wnd::Update()
{
}

// === IUIMsgHandler ===

void Wnd::OnKey(std::uint32_t /*nKey*/, std::uint32_t /*nFlag*/)
{
}

auto Wnd::OnSetFocus(std::int32_t /*bFocus*/) -> std::int32_t
{
    return 0;
}

void Wnd::OnMouseButton(
    std::uint32_t /*nType*/, std::uint32_t /*nFlag*/,
    std::int32_t /*x*/, std::int32_t /*y*/)
{
}

auto Wnd::OnMouseMove(std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    return 0;
}

auto Wnd::OnMouseWheel(
    std::int32_t /*nDelta*/,
    std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    return 0;
}

void Wnd::OnMouseEnter(std::int32_t bEnter)
{
    if (bEnter)
    {
        // TODO: CInputSystem::SetCursorState(0, 0) when m_nCursorState != 0
    }
}

void Wnd::OnDraggableMove(
    std::int32_t /*nType*/, IDraggable* /*pDraggable*/,
    std::int32_t /*x*/, std::int32_t /*y*/)
{
}

auto Wnd::OnDragEnd(
    CDraggableSkill* /*pSkill*/, IUIMsgHandler* /*pTarget*/,
    std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    return 0;
}

auto Wnd::OnDragEnd(
    CDraggableItem* /*pItem*/, IUIMsgHandler* /*pTarget*/,
    std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    return 0;
}

auto Wnd::IsUsingDragEnd() -> std::int32_t
{
    return 0;
}

auto Wnd::CanPutItemIntoSlot(
    std::int32_t /*nSlot*/,
    const ZRef<GW_ItemSlotBase>& /*pItem*/) -> std::int32_t
{
    return 0;
}

void Wnd::SetEnable(std::int32_t /*bEnable*/)
{
}

auto Wnd::IsEnabled() -> std::int32_t
{
    return 1;
}

void Wnd::SetShow(std::int32_t /*bShow*/)
{
}

auto Wnd::IsShown() -> std::int32_t
{
    return 1;
}

auto Wnd::GetAbsLeft() -> std::int32_t
{
    if (!m_pLayer)
        return 0;
    // this->layer.GetX() - WndMan::GetOrgWindow(Origin_LT).GetX()
    const auto nX = m_pLayer->GetX();
    // TODO: subtract origin window X from WndMan::GetOrgWindow(Origin_LT)
    return nX;
}

auto Wnd::GetAbsTop() -> std::int32_t
{
    if (!m_pLayer)
        return 0;
    // this->layer.GetY() - WndMan::GetOrgWindow(Origin_LT).GetY()
    const auto nY = m_pLayer->GetY();
    // TODO: subtract origin window Y from WndMan::GetOrgWindow(Origin_LT)
    return nY;
}

void Wnd::ClearToolTip()
{
}

void Wnd::OnIMEModeChange(char /*nMode*/)
{
}

void Wnd::OnIMEResult(const char* /*szResult*/)
{
}

void Wnd::OnIMEComp(
    const char* /*szComp*/,
    ZArray<std::uint32_t>* /*aAttr*/,
    std::uint32_t /*nCursor*/,
    std::int32_t /*bInsert*/,
    ZList<ZXString<char>>* /*lCandList*/,
    std::int32_t /*nCandIdx*/,
    std::int32_t /*nCandPageStart*/,
    std::int32_t /*nCandPageSize*/)
{
}

void Wnd::OnTouchPanBegin(std::int32_t /*x*/, std::int32_t /*y*/)
{
}

void Wnd::OnTouchPanEnter(std::int32_t /*bEnter*/)
{
}

void Wnd::OnTouchPanMoveWithDragCtx(std::int32_t /*x*/, std::int32_t /*y*/)
{
}

void Wnd::OnTouchPanMoveWithNothing(std::int32_t /*x*/, std::int32_t /*y*/)
{
}

void Wnd::OnTouchHorizontalFlick(std::int32_t /*nDirection*/)
{
}

auto Wnd::OnTouchVerticalScroll(std::int32_t /*nDelta*/) -> std::int32_t
{
    return 0;
}

void Wnd::OnTouchZoomOut()
{
}

void Wnd::OnTouchZoomIn()
{
}

void Wnd::OnTouchTwoFingerTap(
    std::int32_t /*x*/, std::int32_t /*y*/, std::int32_t /*nParam*/)
{
}

// === Wnd vtable ===

auto Wnd::OnDragDrop(
    std::int32_t /*nType*/, DRAGCTX* /*pCtx*/,
    std::int32_t /*x*/, std::int32_t /*y*/) -> std::int32_t
{
    return 0;
}

void Wnd::PreCreateWnd(
    std::int32_t /*nLeft*/, std::int32_t /*nTop*/,
    std::int32_t /*nWidth*/, std::int32_t /*nHeight*/,
    std::int32_t /*nZ*/, std::int32_t /*bScreenCoord*/,
    void* /*pParam*/)
{
}

void Wnd::OnCreate(void* /*pParam*/)
{
}

void Wnd::OnDestroy()
{
}

void Wnd::OnMoveWnd(std::int32_t /*x*/, std::int32_t /*y*/)
{
}

void Wnd::OnEndMoveWnd()
{
    m_ptCursorRel.x = -1;
    m_ptCursorRel.y = -1;
}

void Wnd::OnChildNotify(
    std::uint32_t nId, std::uint32_t param1, std::uint32_t /*param2*/)
{
    if (param1 == 100)
        OnButtonClicked(nId);
}

void Wnd::OnButtonClicked(std::uint32_t /*nId*/)
{
}

auto Wnd::HitTest(
    std::int32_t rx, std::int32_t ry, CtrlWnd** ppCtrl) -> std::int32_t
{
    if (ppCtrl)
        *ppCtrl = nullptr;

    // Iterate children in reverse order (last = topmost)
    for (auto it = m_lpChildren.rbegin(); it != m_lpChildren.rend(); ++it)
    {
        auto& pCtrl = *it;
        if (!pCtrl || !pCtrl->IsShown())
            continue;

        const auto cx = pCtrl->GetX();
        const auto cy = pCtrl->GetY();

        if (pCtrl->HitTest(rx - cx, ry - cy))
        {
            if (ppCtrl)
                *ppCtrl = pCtrl.get();
            return 2;
        }
    }

    // Check own bounds
    if (rx >= 0 && ry >= 0 && rx < m_width && ry < m_height)
        return 2;

    return 0;
}

auto Wnd::OnActivate(std::int32_t bActive) -> std::int32_t
{
    if (bActive)
        WndMan::GetInstance().UpdateWindowPosition(this);
    return 1;
}

void Wnd::MoveWnd(std::int32_t /*x*/, std::int32_t /*y*/)
{
}

void Wnd::InvalidateRect(const Rect* pRect)
{
    if (pRect)
    {
        // Union with existing invalidated rect
        if (pRect->left < m_rcInvalidated.left)
            m_rcInvalidated.left = pRect->left;
        if (pRect->top < m_rcInvalidated.top)
            m_rcInvalidated.top = pRect->top;
        if (pRect->right > m_rcInvalidated.right)
            m_rcInvalidated.right = pRect->right;
        if (pRect->bottom > m_rcInvalidated.bottom)
            m_rcInvalidated.bottom = pRect->bottom;
    }
    else
    {
        m_rcInvalidated = Rect{0, 0, m_width, m_height};
    }
    // TODO: CWndMan::InsertInvalidatedWindow(this)
}

void Wnd::Draw(const Rect* pRect)
{
    // Draw background: either static canvas or overlap layer fill
    if (m_pBackgrnd)
    {
        // TODO: GetCanvas()->Copy(m_nBackgrndX, m_nBackgrndY, m_pBackgrnd)
    }
    else if (m_pOverlabLayer)
    {
        // Fill overlap region on the canvas
        // TODO: GetCanvas()->FillRect(m_nBackgrndX, m_nBackgrndY,
        //     m_pOverlabLayer->get_width(), m_pOverlabLayer->get_height(), 0xFFFFFF)
    }

    // Draw child controls via m_pCtrlLayer
    if (m_pCtrlLayer)
    {
        std::int32_t left = 0;
        std::int32_t top = 0;
        auto right = m_pCtrlLayer->get_width();
        auto bottom = m_pCtrlLayer->get_height();

        if (pRect)
        {
            left = pRect->left;
            top = pRect->top;
            right = pRect->right;
            bottom = pRect->bottom;
        }

        // TODO: m_pCtrlLayer->GetCanvas(0)->FillRect(left, top, right - left, bottom - top, 0xFFFFFF)
    }
}

auto Wnd::IsMyAddon(Wnd* /*pWnd*/) -> std::int32_t
{
    return 0;
}

auto Wnd::IsRaceSelectWnd() -> bool
{
    return false;
}

auto Wnd::IsStatWnd() -> bool
{
    return false;
}

void Wnd::AddChildWnd(Wnd* pChild, std::uint32_t nKey)
{
    m_mChildWnd[nKey] = pChild;
    m_aChildWnd.push_back(nKey);
}

void Wnd::RemoveChildWnd(std::uint32_t nKey)
{
    m_mChildWnd.erase(nKey);
    for (auto it = m_aChildWnd.begin(); it != m_aChildWnd.end(); ++it)
    {
        if (*it == nKey)
        {
            m_aChildWnd.erase(it);
            return;
        }
    }
}

auto Wnd::GetUIType() -> std::int32_t
{
    return 0;
}

// === Non-virtual methods ===

auto Wnd::IsActive() const -> bool
{
    return WndMan::GetInstance().GetActiveWnd() == this;
}

auto Wnd::IsFocused() const -> bool
{
    // Original: CWndMan::m_pFocus == &this->IUIMsgHandler
    return WndMan::GetInstance().GetFocus() ==
           static_cast<const IUIMsgHandler*>(this);
}

auto Wnd::GetCanvas() -> std::shared_ptr<WzGr2DCanvas>
{
    if (m_pOverlabLayer)
        return m_pOverlabLayer->GetCurrentCanvas();
    if (m_pLayer)
        return m_pLayer->GetCurrentCanvas();
    return nullptr;
}

void Wnd::SetAnimationBackgrnd(
    const std::string& sUOL,
    std::int32_t nBackgrndX, std::int32_t nBackgrndY)
{
    if (m_pBackgrnd || !m_pLayer)
        return;

    m_nBackgrndX = nBackgrndX;
    m_nBackgrndY = nBackgrndY;

    // Release existing animation and overlap layers
    m_pAnimationLayer.reset();
    m_pOverlabLayer.reset();

    // Get z from main layer
    const auto nZ = m_pLayer->get_z();

    // Load animation layer from UOL
    // Origin chained to m_pLayer, offset by (nBackgrndX, nBackgrndY)
    m_pAnimationLayer = AnimationDisplayer::LoadLayer(
        sUOL, 0, {}, nBackgrndX, nBackgrndY,
        m_pLayer, nZ + 1, 255, 0,
        nullptr, 0, 0, false);

    if (!m_pAnimationLayer)
        return;

    // Start repeating animation
    m_pAnimationLayer->Animate(Gr2DAnimationType::Repeat);

    // Create overlap layer at background position with animation dimensions
    auto& gr = get_gr();
    m_pOverlabLayer = gr.CreateLayer(
        m_nBackgrndX, m_nBackgrndY,
        m_pAnimationLayer->GetWidth(),
        m_pAnimationLayer->GetHeight(),
        nZ + 2);

    if (m_pOverlabLayer)
    {
        // Chain overlap layer origin and overlay to main layer
        m_pOverlabLayer->PutOrigin(m_pLayer.get());
        m_pOverlabLayer->put_overlay(m_pLayer);
        m_pOverlabLayer->put_color(0xFFFFFFFF);
    }
}

} // namespace ms
