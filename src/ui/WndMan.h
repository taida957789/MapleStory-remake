#pragma once

#include "Wnd.h"
#include "util/Singleton.h"
#include "util/security/SecPoint.h"

#include <cstdint>
#include <vector>

namespace ms
{

class IUIMsgHandler;
class IWzVector2D;
struct DRAGCTX;

/**
 * @brief Global window manager singleton
 *
 * Based on CWndMan from the original MapleStory client (v1029).
 *
 * Original: __cppobj CWndMan : CWnd, TSingleton<CWndMan>
 *
 * Event flow (original client):
 * 1. CWvsApp::WndProc receives Windows message
 * 2. CWvsApp forwards to CWndMan IUIMsgHandler overrides
 * 3. CWndMan forwards to g_pStage
 */
class WndMan final : public Wnd, public Singleton<WndMan>
{
    friend class Singleton<WndMan>;

public:
    // === Static window list management ===
    static void RemoveWindow(Wnd* pWnd);
    static void RemoveUpdateWindow(Wnd* pWnd);
    static void RemoveInvalidatedWindow(Wnd* pWnd);
    static void s_Update();

    // === Input dispatching (CWvsApp::ISMsgProc → CWndMan) ===
    auto ProcessKey(std::uint32_t message, std::uint32_t wParam,
                    std::int32_t lParam) -> std::int32_t;
    auto ProcessMouse(std::uint32_t message, std::uint32_t wParam,
                      std::int32_t lParam) -> std::int32_t;

    // === Focus management ===
    void SetFocus(IUIMsgHandler* pHandler);
    [[nodiscard]] auto GetHandlerFromPoint(
        std::int32_t x, std::int32_t y) -> IUIMsgHandler*;

    // === Window management ===
    void UpdateWindowPosition(Wnd* pWnd);

    void SetActiveWnd(Wnd* pWnd) { m_pActiveWnd = pWnd; }
    [[nodiscard]] auto GetActiveWnd() const noexcept -> Wnd* { return m_pActiveWnd; }

    [[nodiscard]] auto GetFocus() const noexcept -> IUIMsgHandler* { return m_pFocus; }

    // === Wnd / IUIMsgHandler overrides ===
    void Update() override;
    auto OnSetFocus(std::int32_t bFocus) -> std::int32_t override;
    void OnKey(std::uint32_t nKey, std::uint32_t nFlag) override;
    void OnMouseButton(std::uint32_t nType, std::uint32_t nFlag,
                       std::int32_t x, std::int32_t y) override;
    auto OnMouseMove(std::int32_t x, std::int32_t y) -> std::int32_t override;
    auto OnMouseWheel(std::int32_t nDelta,
                      std::int32_t x, std::int32_t y) -> std::int32_t override;
    void OnMouseEnter(std::int32_t bEnter) override;

private:
    WndMan();
    ~WndMan() override;

    // === Static window lists (ZList<CWnd*>) ===
    static inline std::vector<Wnd*> ms_lpWindow;
    static inline std::vector<Wnd*> ms_lpUpdateWindow;
    static inline std::vector<Wnd*> ms_lpInvalidatedWindow;

    // === Instance members (from IDA struct) ===

    /// ZList<IUIMsgHandler *>
    std::vector<IUIMsgHandler*> m_lpCapture;
    Wnd* m_pActiveWnd{nullptr};
    Wnd* m_pDragWnd{nullptr};
    std::int32_t m_bDropByMouseUp{};
    IUIMsgHandler* m_pFocus{nullptr};
    IUIMsgHandler* m_pCursorHandler{nullptr};
    // DRAGCTX m_ctxDrag;  // TODO: define DRAGCTX struct
    SecPoint m_ptCursor;
    std::int32_t m_tLastScrShot{};
    std::int32_t m_nScrShotCount{};
    // HWND m_hWnd{};     // Windows-specific
    // HIMC m_hNewIMC{};   // Windows-specific
    // HIMC m_hOldIMC{};   // Windows-specific
    std::uint32_t m_dwIMEProperty{};
    std::int32_t m_bIMEActive{};
    std::vector<std::uint8_t> m_abIMECompAttr;
    std::vector<std::uint32_t> m_adwIMECompClause;
    std::vector<std::uint8_t> m_aIMECand;
    std::vector<char> m_sIMEResultStr;
    std::vector<char> m_sIMECompStr;
    std::int32_t m_nIMECursorPos{};
    std::int32_t m_bWndAttachEnabled{};
    bool m_bShiftEnterMode{};
    bool m_bEnforcedWaiting{};
    // std::shared_ptr<IWzVector2D> m_pOrgWindow[9];  // TODO
    // std::vector<WndConfigHelper*> m_lWndConfigHelper;  // TODO
    std::int32_t m_nAPISetCapture{};
    IUIMsgHandler* m_pHandlerAPICapture{nullptr};
    // CWndMan::GestureAPI m_gestureAPI;  // TODO: inner struct
    std::int32_t m_bIsTouchAllowed{};
    std::int32_t m_nTouchStatus{};
    std::int32_t m_tTrigger{};
    Wnd* m_pTouchDragWnd{nullptr};
    // DRAGCTX m_ctxTouchDrag;  // TODO: define DRAGCTX struct
    Point2D m_ptTouchPanLast;
    IUIMsgHandler* m_pTouchPanHandler{nullptr};
    std::uint32_t m_dwTouchZoomBeginArg{};
    std::uint32_t m_dwTouchZoomLastArg{};
    Point2D m_ptTouchZoomBegin;
    Point2D m_ptTouchZoomLast;

    static inline std::uint32_t ms_tLastMouseMessage{};
};

} // namespace ms
