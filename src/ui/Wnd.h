#pragma once

#include "IUIMsgHandler.h"
#include "app/IGObj.h"
#include "util/Point.h"
#include "util/security/SecPoint.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ms
{

class WzGr2DLayer;
class WzGr2DCanvas;
class CtrlWnd;
struct DRAGCTX;

/**
 * @brief Base window class for all UI elements
 *
 * Based on CWnd from the original MapleStory client (v1029).
 * Inherits from IGObj (game object update), IUIMsgHandler (input handling).
 *
 * Original: __cppobj CWnd : IGObj, IUIMsgHandler, ZRefCounted
 *
 * Layer hierarchy:
 *   m_pLayer          - main content layer
 *   m_pAnimationLayer - animation overlay layer
 *   m_pOverlabLayer   - overlap/tooltip layer
 *   m_pCtrlLayer      - child control layer
 */
class Wnd : public IGObj, public IUIMsgHandler
{
    friend class WndMan;

public:
    /// Origin type for positioning (CWnd::UIOrigin)
    enum UIOrigin : std::int32_t
    {
        Origin_LT = 0, // Left-Top
        Origin_RT = 1, // Right-Top
        Origin_LB = 2, // Left-Bottom
        Origin_RB = 3, // Right-Bottom
        Origin_C  = 4  // Center
    };

    /// Sentinel value for invalid position
    static constexpr std::int32_t INV_POS = static_cast<std::int32_t>(0x80000000);

    /// Window creation parameters
    struct CREATEWND_PARAM
    {
        std::int32_t nLeft{};
        std::int32_t nTop{};
        std::int32_t nWidth{};
        std::int32_t nHeight{};
        std::int32_t nZ{};
        std::int32_t bScreenCoord{};
        std::int32_t bSetFocus{};
        UIOrigin org{Origin_LT};
    };

    Wnd();
    ~Wnd() override;

    // Non-copyable
    Wnd(const Wnd&) = delete;
    auto operator=(const Wnd&) -> Wnd& = delete;

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

    // === Wnd vtable ===
    virtual auto OnDragDrop(
        std::int32_t nType, DRAGCTX* pCtx,
        std::int32_t x, std::int32_t y) -> std::int32_t;
    virtual void PreCreateWnd(
        std::int32_t nLeft, std::int32_t nTop,
        std::int32_t nWidth, std::int32_t nHeight,
        std::int32_t nZ, std::int32_t bScreenCoord,
        void* pParam);
    virtual void OnCreate(void* pParam);
    virtual void OnDestroy();
    virtual void OnMoveWnd(std::int32_t x, std::int32_t y);
    virtual void OnEndMoveWnd();
    virtual void OnChildNotify(
        std::uint32_t nId, std::uint32_t param1, std::uint32_t param2);
    virtual void OnButtonClicked(std::uint32_t nId);
    virtual auto HitTest(
        std::int32_t x, std::int32_t y, CtrlWnd** ppCtrl) -> std::int32_t;
    virtual auto OnActivate(std::int32_t bActive) -> std::int32_t;
    virtual void MoveWnd(std::int32_t x, std::int32_t y);
    virtual void InvalidateRect(const Rect* pRect);
    virtual void Draw(const Rect* pRect);
    virtual auto IsMyAddon(Wnd* pWnd) -> std::int32_t;
    [[nodiscard]] virtual auto IsRaceSelectWnd() -> bool;
    [[nodiscard]] virtual auto IsStatWnd() -> bool;
    virtual void AddChildWnd(Wnd* pChild, std::uint32_t nKey);
    virtual void RemoveChildWnd(std::uint32_t nKey);
    [[nodiscard]] virtual auto GetUIType() -> std::int32_t;

    // === Non-virtual methods ===
    [[nodiscard]] auto IsActive() const -> bool;
    [[nodiscard]] auto IsFocused() const -> bool;
    [[nodiscard]] auto GetCanvas() -> std::shared_ptr<WzGr2DCanvas>;
    void SetAnimationBackgrnd(
        const std::string& sUOL,
        std::int32_t nBackgrndX, std::int32_t nBackgrndY);

protected:
    // === Member data (from IDA struct) ===
    std::uint32_t m_dwWndKey{};
    std::int32_t m_nTemp{};

    /// Main content layer (_com_ptr_t<IWzGr2DLayer>)
    std::shared_ptr<WzGr2DLayer> m_pLayer;
    /// Animation overlay layer
    std::shared_ptr<WzGr2DLayer> m_pAnimationLayer;
    /// Overlap/tooltip layer
    std::shared_ptr<WzGr2DLayer> m_pOverlabLayer;
    /// Child control layer
    std::shared_ptr<WzGr2DLayer> m_pCtrlLayer;

    std::int32_t m_width{};
    std::int32_t m_height{};
    Rect m_rcInvalidated{};

    std::int32_t m_bScreenCoord{};
    std::int32_t m_nBackgrndX{};
    std::int32_t m_nBackgrndY{};

    /// Relative cursor position (SECPOINT)
    SecPoint m_ptCursorRel;

    /// Child controls (ZList<ZRef<CCtrlWnd>>)
    std::vector<std::shared_ptr<CtrlWnd>> m_lpChildren;
    /// Currently focused child (raw pointer, not owned)
    CtrlWnd* m_pFocusChild{};

    /// Background canvas (_com_ptr_t<IWzCanvas>)
    std::shared_ptr<WzGr2DCanvas> m_pBackgrnd;

    /// Child window lookup by key (ZMap<unsigned int, CWnd*, unsigned int>)
    std::unordered_map<std::uint32_t, Wnd*> m_mChildWnd;
    /// Ordered child window keys (ZArray<unsigned int>)
    std::vector<std::uint32_t> m_aChildWnd;

    UIOrigin m_origin{Origin_LT};
    std::int32_t m_nTempKey{};
};

} // namespace ms
