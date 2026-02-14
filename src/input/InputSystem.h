#pragma once

#include "util/Point.h"
#include "util/Singleton.h"

#include <SDL3/SDL.h>
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

class WzGr2DLayer;
class IWzVector2D;

/// Input system message (mirrors Windows MSG subset)
struct ISMSG
{
    std::uint32_t message{};
    std::uint32_t wParam{};
    std::int32_t lParam{};
};

/**
 * @brief Input handling system
 *
 * Based on CInputSystem from the original MapleStory client (v1029).
 * Original uses DirectInput8; this implementation uses SDL3.
 *
 * Original: __cppobj CInputSystem : TSingleton<CInputSystem>
 */
class InputSystem final : public Singleton<InputSystem>
{
    friend class Singleton<InputSystem>;

public:
    /// Device indices for UpdateDevice/TryAcquireDevice
    enum : std::int32_t
    {
        IS_KEYBOARD = 0,
        IS_MOUSE    = 1,
        IS_JOYSTICK = 2,
    };

    /// Mouse state tracking (CInputSystem::MOUSESTATE)
    struct MOUSESTATE
    {
        std::int32_t x{};
        std::int32_t y{};
        std::int32_t bLBDown{};
        std::int32_t bRBDown{};
        std::int32_t tLBDown{};
        std::int32_t tRBDown{};
        Point2D ptLBDown;
        Point2D ptRBDown;
        std::int32_t nWheel{};
    };

    // === Lifecycle ===
    void Init();
    void Close();

    // === Device management ===
    void UpdateDevice(std::int32_t nDeviceIndex);
    void OnActivate();
    auto TryAcquireDevice(std::int32_t nDeviceIndex) -> std::int32_t;
    void SetAcquireKeyboard(std::int32_t bAcquire);
    [[nodiscard]] auto IsDIKeyboard() const -> std::int32_t;
    [[nodiscard]] auto IsDIMouse() const -> std::int32_t;

    // === Key state ===
    auto IsKeyPressed(std::int32_t nVk) -> std::int32_t;
    auto GetSpecialKeyFlag() -> std::uint32_t;
    auto GenerateAutoKeyDown(ISMSG* pISMsg) -> std::int32_t;

    // === Mouse state ===
    auto GetCursorPos(Point2D* lpPoint) -> std::int32_t;
    auto SetCursorPos(std::int32_t x, std::int32_t y) -> std::int32_t;
    void SetMouseSpeed(std::int32_t nMouseSpeed);

    // === Cursor management ===
    void ShowCursor(std::int32_t bShow);
    [[nodiscard]] auto IsCursorShown() -> std::int32_t;
    [[nodiscard]] auto GetCursorState() -> std::int32_t;
    void SetCursorState(std::int32_t nState, bool bForce);
    void SetCursor(const std::shared_ptr<WzGr2DLayer>& pLayer);
    void SetCursorVectorPos(std::int32_t x, std::int32_t y);
    void LoadCursorStateWithIndex(std::int32_t nIndex);
    void UnLoadCursorStateWithIndex(std::int32_t nIndex);
    [[nodiscard]] auto GetCursorOrigin() -> std::shared_ptr<IWzVector2D>;

    // === Cursor movement constraints ===
    void SetCursorMoveableRect(const Rect& rcMoveable);
    void ResetCursorMoveableRect();
    void SetMouseCantMoveTime(std::uint32_t tTime);
    void SetCantMoveCursorOrigin(bool bSet);
    [[nodiscard]] auto IsCursorOriginMoveByMouse() -> bool;
    void SetCursorOriginMoveByMouse(bool bSet);

    // === Message queue ===
    auto GetISMessage(ISMSG* pISMsg) -> std::int32_t;

    // === Static ===
    static auto GetFPSCursorIndexByFieldType(
        std::int32_t nIndex, std::int32_t nFieldType) -> std::int32_t;

    // === SDL3 integration ===
    void ProcessEvent(const SDL_Event& event);

private:
    InputSystem();
    ~InputSystem() override;

    void UpdateKeyboard(std::int32_t bGenerate);
    void UpdateMouse();
    void AddISMessage(std::uint32_t message, std::uint32_t wParam, std::int32_t lParam);

    // === Members (from IDA struct) ===
    // DirectInput replaced by SDL — m_apDevice/m_ahEvent/m_pDI omitted

    std::int32_t m_bAcquireKeyboard{1};
    std::array<std::uint8_t, 256> m_aKeyState{};
    std::array<std::uint8_t, 256> m_aKeyStatePrev{};
    std::int32_t m_tLastKeyDown{};
    std::int32_t m_nVkLastKeyDown{};
    std::uint32_t m_dwToggleKey{};
    std::int32_t m_tKeyboardDelay{500};
    std::int32_t m_tKeyboardSpeed{30};

    MOUSESTATE m_MouseState{};
    std::int32_t m_nDoubleClkTime{500};
    std::int32_t m_nCxDoubleClk{4};
    std::int32_t m_nCyDoubleClk{4};
    std::int32_t m_nMouseSpeed{1};
    std::int32_t m_bSwapButton{};

    /// Cursor layer
    std::shared_ptr<WzGr2DLayer> m_pLayerCursor;
    /// Cursor type layers (64 cursor types)
    std::array<std::shared_ptr<WzGr2DLayer>, 64> m_pCursorType{};
    std::shared_ptr<IWzVector2D> m_pVectorCursor;
    std::int32_t m_nCursorState{};
    std::int32_t m_nLastCursorState{};
    Rect m_rcCursorMoveable{};
    std::uint32_t m_tCantMoveTime{};
    bool m_bCantMoveCursorOrigin{};
    bool m_bCursorOriginMoveByMouse{};

    /// Input message queue (ZList<ISMSG>)
    std::vector<ISMSG> m_lISMsg;

    bool m_bInitialized{};
};

} // namespace ms
