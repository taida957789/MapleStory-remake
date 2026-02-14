#include "InputSystem.h"
#include "graphics/WzGr2D.h"
#include "util/Logger.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace ms
{

// =============================================================================
// Windows message / key constants used in ISMSG
// =============================================================================
namespace
{

// Windows messages (used as ISMSG::message values)
constexpr std::uint32_t kWM_KEYDOWN       = 0x0100;
constexpr std::uint32_t kWM_MOUSEMOVE     = 0x0200;
constexpr std::uint32_t kWM_LBUTTONDOWN   = 0x0201;
constexpr std::uint32_t kWM_LBUTTONUP     = 0x0202;
constexpr std::uint32_t kWM_LBUTTONDBLCLK = 0x0203;
constexpr std::uint32_t kWM_RBUTTONDOWN   = 0x0204;
constexpr std::uint32_t kWM_RBUTTONUP     = 0x0205;
constexpr std::uint32_t kWM_RBUTTONDBLCLK = 0x0206;
constexpr std::uint32_t kWM_MOUSEWHEEL    = 0x020A;

// MK_* mouse wParam flags
constexpr std::uint32_t kMK_LBUTTON  = 0x0001;
constexpr std::uint32_t kMK_RBUTTON  = 0x0002;
constexpr std::uint32_t kMK_SHIFT    = 0x0004;
constexpr std::uint32_t kMK_CONTROL  = 0x0008;
constexpr std::uint32_t kMK_MBUTTON  = 0x0010;

// Windows virtual-key codes (subset used by CInputSystem)
constexpr int kVK_LBUTTON   = 1;
constexpr int kVK_RBUTTON   = 2;
constexpr int kVK_BACK      = 8;
constexpr int kVK_TAB       = 9;
constexpr int kVK_RETURN    = 13;
constexpr int kVK_SHIFT     = 16;
constexpr int kVK_CONTROL   = 17;
constexpr int kVK_MENU      = 18;  // Alt
constexpr int kVK_PAUSE     = 19;
constexpr int kVK_CAPITAL   = 20;  // CapsLock
constexpr int kVK_ESCAPE    = 27;
constexpr int kVK_SPACE     = 32;
constexpr int kVK_PRIOR     = 33;  // Page Up
constexpr int kVK_NEXT      = 34;  // Page Down
constexpr int kVK_END       = 35;
constexpr int kVK_HOME      = 36;
constexpr int kVK_LEFT      = 37;
constexpr int kVK_UP        = 38;
constexpr int kVK_RIGHT     = 39;
constexpr int kVK_DOWN      = 40;
constexpr int kVK_SNAPSHOT  = 44;  // PrintScreen
constexpr int kVK_INSERT    = 45;
constexpr int kVK_DELETE    = 46;
constexpr int kVK_LWIN      = 91;
constexpr int kVK_RWIN      = 92;
constexpr int kVK_NUMPAD0   = 96;
constexpr int kVK_MULTIPLY  = 106;
constexpr int kVK_ADD       = 107;
constexpr int kVK_SUBTRACT  = 109;
constexpr int kVK_DECIMAL   = 110;
constexpr int kVK_DIVIDE    = 111;
constexpr int kVK_F1        = 112;
constexpr int kVK_NUMLOCK   = 144;
constexpr int kVK_SCROLL    = 145;
constexpr int kVK_LSHIFT    = 160;
constexpr int kVK_RSHIFT    = 161;
constexpr int kVK_LCONTROL  = 162;
constexpr int kVK_RCONTROL  = 163;
constexpr int kVK_LMENU     = 164;
constexpr int kVK_RMENU     = 165;
constexpr int kVK_OEM_1     = 186;  // ;:
constexpr int kVK_OEM_PLUS  = 187;  // =+
constexpr int kVK_OEM_COMMA = 188;  // ,<
constexpr int kVK_OEM_MINUS = 189;  // -_
constexpr int kVK_OEM_PERIOD = 190; // .>
constexpr int kVK_OEM_2     = 191;  // /?
constexpr int kVK_OEM_3     = 192;  // `~
constexpr int kVK_OEM_4     = 219;  // [{
constexpr int kVK_OEM_5     = 220;  // \|
constexpr int kVK_OEM_6     = 221;  // ]}
constexpr int kVK_OEM_7     = 222;  // '"

/// Pack x/y into Windows-style lParam (LOWORD=x, HIWORD=y)
constexpr auto MakeLParam(std::int32_t x, std::int32_t y) -> std::int32_t
{
    return static_cast<std::int32_t>(
        static_cast<std::uint16_t>(x) |
        (static_cast<std::uint32_t>(static_cast<std::uint16_t>(y)) << 16));
}

/// Map SDL_Scancode to Windows VK code
auto SDLScancodeToVK(SDL_Scancode sc) -> std::int32_t
{
    // Letters A-Z → 0x41-0x5A
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
        return 0x41 + (sc - SDL_SCANCODE_A);

    // Number row 0-9 → 0x30-0x39
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
        return 0x31 + (sc - SDL_SCANCODE_1);
    if (sc == SDL_SCANCODE_0)
        return 0x30;

    // Function keys F1-F12 → 0x70-0x7B
    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12)
        return kVK_F1 + (sc - SDL_SCANCODE_F1);

    // Numpad 0-9 → 0x60-0x69
    if (sc >= SDL_SCANCODE_KP_1 && sc <= SDL_SCANCODE_KP_9)
        return kVK_NUMPAD0 + 1 + (sc - SDL_SCANCODE_KP_1);
    if (sc == SDL_SCANCODE_KP_0)
        return kVK_NUMPAD0;

    switch (sc)
    {
    case SDL_SCANCODE_RETURN:       return kVK_RETURN;
    case SDL_SCANCODE_ESCAPE:       return kVK_ESCAPE;
    case SDL_SCANCODE_BACKSPACE:    return kVK_BACK;
    case SDL_SCANCODE_TAB:          return kVK_TAB;
    case SDL_SCANCODE_SPACE:        return kVK_SPACE;
    case SDL_SCANCODE_PAUSE:        return kVK_PAUSE;
    case SDL_SCANCODE_INSERT:       return kVK_INSERT;
    case SDL_SCANCODE_DELETE:       return kVK_DELETE;
    case SDL_SCANCODE_HOME:         return kVK_HOME;
    case SDL_SCANCODE_END:          return kVK_END;
    case SDL_SCANCODE_PAGEUP:       return kVK_PRIOR;
    case SDL_SCANCODE_PAGEDOWN:     return kVK_NEXT;
    case SDL_SCANCODE_LEFT:         return kVK_LEFT;
    case SDL_SCANCODE_RIGHT:        return kVK_RIGHT;
    case SDL_SCANCODE_UP:           return kVK_UP;
    case SDL_SCANCODE_DOWN:         return kVK_DOWN;
    case SDL_SCANCODE_PRINTSCREEN:  return kVK_SNAPSHOT;
    case SDL_SCANCODE_CAPSLOCK:     return kVK_CAPITAL;
    case SDL_SCANCODE_NUMLOCKCLEAR: return kVK_NUMLOCK;
    case SDL_SCANCODE_SCROLLLOCK:   return kVK_SCROLL;

    // Modifiers
    case SDL_SCANCODE_LSHIFT:       return kVK_LSHIFT;
    case SDL_SCANCODE_RSHIFT:       return kVK_RSHIFT;
    case SDL_SCANCODE_LCTRL:        return kVK_LCONTROL;
    case SDL_SCANCODE_RCTRL:        return kVK_RCONTROL;
    case SDL_SCANCODE_LALT:         return kVK_LMENU;
    case SDL_SCANCODE_RALT:         return kVK_RMENU;
    case SDL_SCANCODE_LGUI:         return kVK_LWIN;
    case SDL_SCANCODE_RGUI:         return kVK_RWIN;

    // Numpad operators
    case SDL_SCANCODE_KP_MULTIPLY:  return kVK_MULTIPLY;
    case SDL_SCANCODE_KP_PLUS:      return kVK_ADD;
    case SDL_SCANCODE_KP_MINUS:     return kVK_SUBTRACT;
    case SDL_SCANCODE_KP_PERIOD:    return kVK_DECIMAL;
    case SDL_SCANCODE_KP_DIVIDE:    return kVK_DIVIDE;
    case SDL_SCANCODE_KP_ENTER:     return kVK_RETURN;

    // OEM keys
    case SDL_SCANCODE_SEMICOLON:    return kVK_OEM_1;
    case SDL_SCANCODE_EQUALS:       return kVK_OEM_PLUS;
    case SDL_SCANCODE_COMMA:        return kVK_OEM_COMMA;
    case SDL_SCANCODE_MINUS:        return kVK_OEM_MINUS;
    case SDL_SCANCODE_PERIOD:       return kVK_OEM_PERIOD;
    case SDL_SCANCODE_SLASH:        return kVK_OEM_2;
    case SDL_SCANCODE_GRAVE:        return kVK_OEM_3;
    case SDL_SCANCODE_LEFTBRACKET:  return kVK_OEM_4;
    case SDL_SCANCODE_BACKSLASH:    return kVK_OEM_5;
    case SDL_SCANCODE_RIGHTBRACKET: return kVK_OEM_6;
    case SDL_SCANCODE_APOSTROPHE:   return kVK_OEM_7;

    default: return 0;
    }
}

} // anonymous namespace

// =============================================================================
// Constructor / Destructor
// =============================================================================

InputSystem::InputSystem()
{
    // Match original CInputSystem::CInputSystem constructor
    m_nCursorState = -1;
    m_nLastCursorState = 0;
    m_bCursorOriginMoveByMouse = true;
}

InputSystem::~InputSystem()
{
    Close();
}

// =============================================================================
// Lifecycle
// =============================================================================

void InputSystem::Init()
{
    // Original: CInputSystem::Init(HWND, void**)
    // Creates DirectInput8, enumerates devices, sets data format/cooperative level.
    // SDL3: input subsystem is already initialised via SDL_Init.

    m_tKeyboardDelay = 500;
    m_tKeyboardSpeed = 30;
    m_nDoubleClkTime = 500;
    m_nCxDoubleClk = 4;
    m_nCyDoubleClk = 4;
    m_nMouseSpeed = 1;
    m_bAcquireKeyboard = 1;

    m_bInitialized = true;
    LOG_INFO("InputSystem initialised (SDL3)");
}

void InputSystem::Close()
{
    // Original: releases DirectInput devices, cursor layers, vector cursor.
    if (!m_bInitialized)
        return;

    m_pLayerCursor.reset();
    m_pVectorCursor.reset();
    for (auto& cursor : m_pCursorType)
        cursor.reset();

    m_lISMsg.clear();
    m_aKeyState.fill(0);
    m_aKeyStatePrev.fill(0);

    m_bInitialized = false;
}

// =============================================================================
// SDL3 integration — event-driven input
// =============================================================================

void InputSystem::ProcessEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    // ── Keyboard ─────────────────────────────────────────────────────────
    case SDL_EVENT_KEY_DOWN:
    {
        if (!m_bAcquireKeyboard)
            break;

        const auto vk = SDLScancodeToVK(event.key.scancode);
        if (vk <= 0 || vk >= 256)
            break;

        const auto prev = m_aKeyState[vk];
        m_aKeyState[vk] = 1;

        // Update generic modifier keys
        if (vk == kVK_LSHIFT || vk == kVK_RSHIFT)
            m_aKeyState[kVK_SHIFT] = 1;
        else if (vk == kVK_LCONTROL || vk == kVK_RCONTROL)
            m_aKeyState[kVK_CONTROL] = 1;
        else if (vk == kVK_LMENU || vk == kVK_RMENU)
            m_aKeyState[kVK_MENU] = 1;

        if (!prev && !event.key.repeat)
        {
            // Generate ISMSG for key-down (message 0x100, bit 31 = 0)
            const auto flags = GetSpecialKeyFlag();
            AddISMessage(kWM_KEYDOWN, static_cast<std::uint32_t>(vk),
                         static_cast<std::int32_t>(flags));

            // Auto-repeat tracking: negative = awaiting initial delay
            m_nVkLastKeyDown = -vk;
            m_tLastKeyDown = static_cast<std::int32_t>(SDL_GetTicks());
        }
        break;
    }

    case SDL_EVENT_KEY_UP:
    {
        if (!m_bAcquireKeyboard)
            break;

        const auto vk = SDLScancodeToVK(event.key.scancode);
        if (vk <= 0 || vk >= 256)
            break;

        m_aKeyState[vk] = 0;

        // Update generic modifier keys
        if (vk == kVK_LSHIFT || vk == kVK_RSHIFT)
            m_aKeyState[kVK_SHIFT] =
                (m_aKeyState[kVK_LSHIFT] || m_aKeyState[kVK_RSHIFT]) ? 1 : 0;
        else if (vk == kVK_LCONTROL || vk == kVK_RCONTROL)
            m_aKeyState[kVK_CONTROL] =
                (m_aKeyState[kVK_LCONTROL] || m_aKeyState[kVK_RCONTROL]) ? 1 : 0;
        else if (vk == kVK_LMENU || vk == kVK_RMENU)
            m_aKeyState[kVK_MENU] =
                (m_aKeyState[kVK_LMENU] || m_aKeyState[kVK_RMENU]) ? 1 : 0;

        // Generate ISMSG for key-up (message 0x100, bit 31 = 1)
        const auto flags = GetSpecialKeyFlag();
        AddISMessage(kWM_KEYDOWN, static_cast<std::uint32_t>(vk),
                     static_cast<std::int32_t>(flags | 0x80000000u));

        // Clear auto-repeat if this was the repeating key
        const auto absVk = std::abs(m_nVkLastKeyDown);
        if (absVk == vk)
        {
            m_nVkLastKeyDown = 0;
            m_tLastKeyDown = 0;
        }
        break;
    }

    // ── Mouse motion ─────────────────────────────────────────────────────
    case SDL_EVENT_MOUSE_MOTION:
    {
        const auto x = static_cast<std::int32_t>(event.motion.x);
        const auto y = static_cast<std::int32_t>(event.motion.y);

        if (m_MouseState.x != x || m_MouseState.y != y)
        {
            m_MouseState.x = x;
            m_MouseState.y = y;

            std::uint32_t wFlags = 0;
            if (m_MouseState.bLBDown) wFlags |= kMK_LBUTTON;
            if (m_MouseState.bRBDown) wFlags |= kMK_RBUTTON;
            if (m_aKeyState[kVK_SHIFT])   wFlags |= kMK_SHIFT;
            if (m_aKeyState[kVK_CONTROL]) wFlags |= kMK_CONTROL;

            AddISMessage(kWM_MOUSEMOVE, wFlags, MakeLParam(x, y));
            SetCursorVectorPos(x, y);
        }
        break;
    }

    // ── Mouse button ─────────────────────────────────────────────────────
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    {
        const auto tCur = static_cast<std::uint32_t>(SDL_GetTicks());
        const auto lParam = MakeLParam(m_MouseState.x, m_MouseState.y);

        // Build wParam flags
        std::uint32_t wFlags = 0;
        if (m_MouseState.bLBDown) wFlags |= kMK_LBUTTON;
        if (m_MouseState.bRBDown) wFlags |= kMK_RBUTTON;
        if (m_aKeyState[kVK_SHIFT])   wFlags |= kMK_SHIFT;
        if (m_aKeyState[kVK_CONTROL]) wFlags |= kMK_CONTROL;

        // Determine logical button (with swap support)
        const bool isLB =
            (event.button.button == SDL_BUTTON_LEFT  && !m_bSwapButton) ||
            (event.button.button == SDL_BUTTON_RIGHT &&  m_bSwapButton);
        const bool isRB =
            (event.button.button == SDL_BUTTON_RIGHT && !m_bSwapButton) ||
            (event.button.button == SDL_BUTTON_LEFT  &&  m_bSwapButton);

        if (isLB)
        {
            wFlags |= kMK_LBUTTON;

            // Double-click detection (mirrors original UpdateMouse)
            if (m_MouseState.tLBDown
                && static_cast<std::int32_t>(tCur - m_MouseState.tLBDown) <= m_nDoubleClkTime
                && m_MouseState.x >= m_MouseState.ptLBDown.x - m_nCxDoubleClk
                && m_MouseState.x <  m_MouseState.ptLBDown.x + m_nCxDoubleClk
                && m_MouseState.y >= m_MouseState.ptLBDown.y - m_nCyDoubleClk
                && m_MouseState.y <  m_MouseState.ptLBDown.y + m_nCyDoubleClk)
            {
                AddISMessage(kWM_LBUTTONDBLCLK, wFlags, lParam);
                m_MouseState.tLBDown = 0;
            }
            else
            {
                AddISMessage(kWM_LBUTTONDOWN, wFlags, lParam);
                m_MouseState.tLBDown = static_cast<std::int32_t>(tCur);
                m_MouseState.ptLBDown = {m_MouseState.x, m_MouseState.y};
            }
            m_MouseState.bLBDown = 1;
        }

        if (isRB)
        {
            wFlags |= kMK_RBUTTON;

            if (m_MouseState.tRBDown
                && static_cast<std::int32_t>(tCur - m_MouseState.tRBDown) <= m_nDoubleClkTime
                && m_MouseState.x >= m_MouseState.ptRBDown.x - m_nCxDoubleClk
                && m_MouseState.x <  m_MouseState.ptRBDown.x + m_nCxDoubleClk
                && m_MouseState.y >= m_MouseState.ptRBDown.y - m_nCyDoubleClk
                && m_MouseState.y <  m_MouseState.ptRBDown.y + m_nCyDoubleClk)
            {
                AddISMessage(kWM_RBUTTONDBLCLK, wFlags, lParam);
                m_MouseState.tRBDown = 0;
            }
            else
            {
                AddISMessage(kWM_RBUTTONDOWN, wFlags, lParam);
                m_MouseState.tRBDown = static_cast<std::int32_t>(tCur);
                m_MouseState.ptRBDown = {m_MouseState.x, m_MouseState.y};
            }
            m_MouseState.bRBDown = 1;
        }
        break;
    }

    case SDL_EVENT_MOUSE_BUTTON_UP:
    {
        const auto lParam = MakeLParam(m_MouseState.x, m_MouseState.y);

        const bool isLB =
            (event.button.button == SDL_BUTTON_LEFT  && !m_bSwapButton) ||
            (event.button.button == SDL_BUTTON_RIGHT &&  m_bSwapButton);
        const bool isRB =
            (event.button.button == SDL_BUTTON_RIGHT && !m_bSwapButton) ||
            (event.button.button == SDL_BUTTON_LEFT  &&  m_bSwapButton);

        // Build wFlags AFTER clearing the released button
        if (isLB)
            m_MouseState.bLBDown = 0;
        if (isRB)
            m_MouseState.bRBDown = 0;

        std::uint32_t wFlags = 0;
        if (m_MouseState.bLBDown) wFlags |= kMK_LBUTTON;
        if (m_MouseState.bRBDown) wFlags |= kMK_RBUTTON;
        if (m_aKeyState[kVK_SHIFT])   wFlags |= kMK_SHIFT;
        if (m_aKeyState[kVK_CONTROL]) wFlags |= kMK_CONTROL;

        if (isLB)
            AddISMessage(kWM_LBUTTONUP, wFlags, lParam);
        if (isRB)
            AddISMessage(kWM_RBUTTONUP, wFlags, lParam);
        break;
    }

    // ── Mouse wheel ──────────────────────────────────────────────────────
    case SDL_EVENT_MOUSE_WHEEL:
    {
        const auto lParam = MakeLParam(m_MouseState.x, m_MouseState.y);

        // Windows WHEEL_DELTA = 120 per notch
        const auto wheelDelta = static_cast<std::int16_t>(event.wheel.y * 120);

        std::uint32_t wFlags = 0;
        if (m_MouseState.bLBDown) wFlags |= kMK_LBUTTON;
        if (m_MouseState.bRBDown) wFlags |= kMK_RBUTTON;
        if (m_aKeyState[kVK_SHIFT])   wFlags |= kMK_SHIFT;
        if (m_aKeyState[kVK_CONTROL]) wFlags |= kMK_CONTROL;

        // Original packs: LOWORD = wFlags, HIWORD = wheelDelta
        const auto wParam = static_cast<std::uint32_t>(
            static_cast<std::uint16_t>(wFlags) |
            (static_cast<std::uint32_t>(static_cast<std::uint16_t>(wheelDelta)) << 16));

        AddISMessage(kWM_MOUSEWHEEL, wParam, lParam);
        m_MouseState.nWheel = static_cast<std::int32_t>(event.wheel.y);
        break;
    }

    default:
        break;
    }
}

// =============================================================================
// Device management
// =============================================================================

void InputSystem::UpdateDevice(std::int32_t nDeviceIndex)
{
    // Original dispatches: 0 → UpdateKeyboard(1), 1 → UpdateMouse()
    if (nDeviceIndex == IS_KEYBOARD)
        UpdateKeyboard(1);
    else if (nDeviceIndex == IS_MOUSE)
        UpdateMouse();
}

void InputSystem::UpdateKeyboard([[maybe_unused]] std::int32_t bGenerate)
{
    // Original: polls DirectInput, detects state changes, generates ISMSG.
    // SDL3: key events are handled by ProcessEvent.
    // This function now only handles auto-repeat generation.
    // (Auto-repeat is done via GenerateAutoKeyDown, called by the game loop.)
}

void InputSystem::UpdateMouse()
{
    // Original: polls DirectInput mouse, detects button/motion changes.
    // SDL3: mouse events are handled by ProcessEvent.
}

void InputSystem::OnActivate()
{
    // Original: re-acquires all 3 DirectInput devices.
    // SDL3: nothing to re-acquire, but reset key state for safety.
    TryAcquireDevice(IS_KEYBOARD);
    TryAcquireDevice(IS_MOUSE);
    TryAcquireDevice(IS_JOYSTICK);
}

auto InputSystem::TryAcquireDevice(std::int32_t nDeviceIndex) -> std::int32_t
{
    // Original: calls IDirectInputDevice8::Acquire and reinitialises state.
    // SDL3: always "acquired" — just reset state.

    if (nDeviceIndex == IS_KEYBOARD)
    {
        m_nVkLastKeyDown = 0;
        m_tLastKeyDown = 0;
        m_aKeyState.fill(0);

        // Read toggle key state from SDL
        const auto modState = SDL_GetModState();
        m_dwToggleKey = 0;
        if (modState & SDL_KMOD_CAPS)   m_dwToggleKey |= 0x10000000u;
        if (modState & SDL_KMOD_NUM)    m_dwToggleKey |= 0x20000000u;
        // SDL3 doesn't track ScrollLock in mod state
    }
    else if (nDeviceIndex == IS_MOUSE)
    {
        SetCursorVectorPos(m_MouseState.x, m_MouseState.y);
    }

    return 1;
}

void InputSystem::SetAcquireKeyboard(std::int32_t bAcquire)
{
    if (bAcquire != m_bAcquireKeyboard)
    {
        m_bAcquireKeyboard = bAcquire;
        if (bAcquire)
            TryAcquireDevice(IS_KEYBOARD);
    }
}

auto InputSystem::IsDIKeyboard() const -> std::int32_t
{
    // Original: checks m_apDevice[0] != nullptr.
    // SDL3: always have keyboard.
    return 1;
}

auto InputSystem::IsDIMouse() const -> std::int32_t
{
    // Original: checks m_apDevice[1] != nullptr.
    // SDL3: always have mouse.
    return 1;
}

// =============================================================================
// Key state
// =============================================================================

auto InputSystem::IsKeyPressed(std::int32_t nVk) -> std::int32_t
{
    // Mirrors original CInputSystem::IsKeyPressed
    if (static_cast<std::uint32_t>(nVk) > 0xFF)
        return 0;

    // VK_LBUTTON (1) → left mouse button
    if (nVk == kVK_LBUTTON)
        return m_MouseState.bLBDown;

    // VK_RBUTTON (2) → right mouse button
    if (nVk == kVK_RBUTTON)
        return m_MouseState.bRBDown;

    // Regular keys: check m_aKeyState
    return m_aKeyState[nVk] != 0 ? 1 : 0;
}

auto InputSystem::GetSpecialKeyFlag() -> std::uint32_t
{
    // Mirrors original — builds a bitmask from modifier key states.
    // Bit layout:
    //   0x0001 = Shift (generic)
    //   0x0002 = LShift (adds to generic)
    //   0x0004 = RShift (adds to generic)
    //   0x0010 = Ctrl (generic)
    //   0x0020 = LCtrl (adds to generic)
    //   0x0040 = RCtrl (adds to generic)
    //   0x0100 = Alt (generic)
    //   0x0200 = LAlt (adds to generic)
    //   0x0400 = RAlt (adds to generic)
    //   0x1000 = Win (generic pair)
    //   0x2000 = LWin (adds to generic)
    //   0x4000 = RWin (adds to generic)

    std::uint32_t flags = 0;

    const bool bShift    = m_aKeyState[kVK_SHIFT]    != 0;
    const bool bLShift   = m_aKeyState[kVK_LSHIFT]   != 0;
    const bool bRShift   = m_aKeyState[kVK_RSHIFT]   != 0;
    const bool bCtrl     = m_aKeyState[kVK_CONTROL]  != 0;
    const bool bLCtrl    = m_aKeyState[kVK_LCONTROL] != 0;
    const bool bRCtrl    = m_aKeyState[kVK_RCONTROL] != 0;
    const bool bAlt      = m_aKeyState[kVK_MENU]     != 0;
    const bool bLAlt     = m_aKeyState[kVK_LMENU]    != 0;
    const bool bRAlt     = m_aKeyState[kVK_RMENU]    != 0;
    const bool bLWin     = m_aKeyState[kVK_LWIN]     != 0;
    const bool bRWin     = m_aKeyState[kVK_RWIN]     != 0;

    if (bShift)  flags |= 0x0001;
    if (bLShift) flags |= 0x0003;  // includes generic shift
    if (bRShift) flags |= 0x0005;
    if (bCtrl)   flags |= 0x0010;
    if (bLCtrl)  flags |= 0x0030;
    if (bRCtrl)  flags |= 0x0050;
    if (bAlt)    flags |= 0x0100;
    if (bLAlt)   flags |= 0x0300;
    if (bRAlt)   flags |= 0x0500;
    if (bLWin)   flags |= 0x3000;
    if (bRWin)   flags |= 0x5000;

    return flags;
}

auto InputSystem::GenerateAutoKeyDown(ISMSG* pISMsg) -> std::int32_t
{
    // Mirrors original auto-repeat logic.
    // m_nVkLastKeyDown < 0: first press, waiting for initial delay
    // m_nVkLastKeyDown > 0: repeating at keyboard speed

    if (!m_bAcquireKeyboard || m_nVkLastKeyDown == 0)
        return 0;

    const auto tCur = static_cast<std::int32_t>(SDL_GetTicks());
    const auto nVk = m_nVkLastKeyDown;

    // Choose delay: initial delay for first press, repeat speed afterwards
    const auto tThreshold = (nVk < 0) ? m_tKeyboardDelay : m_tKeyboardSpeed;

    if (tCur - m_tLastKeyDown < tThreshold)
        return 0;

    // Switch from initial delay to repeat mode
    if (nVk < 0)
        m_nVkLastKeyDown = -nVk;

    m_tLastKeyDown = tCur;

    pISMsg->message = kWM_KEYDOWN;
    pISMsg->wParam = static_cast<std::uint32_t>(std::abs(nVk));
    pISMsg->lParam = static_cast<std::int32_t>(GetSpecialKeyFlag());

    return 1;
}

// =============================================================================
// Mouse state
// =============================================================================

auto InputSystem::GetCursorPos(Point2D* lpPoint) -> std::int32_t
{
    // Original: if DirectInput mouse present, return m_MouseState.x/y.
    // Otherwise falls back to system GetCursorPos.
    // SDL3: always use our tracked position.
    lpPoint->x = m_MouseState.x;
    lpPoint->y = m_MouseState.y;
    return 1;
}

auto InputSystem::SetCursorPos(std::int32_t x, std::int32_t y) -> std::int32_t
{
    // Clamp to screen bounds
    const auto& gr = get_gr();
    const auto w = static_cast<std::int32_t>(gr.GetWidth());
    const auto h = static_cast<std::int32_t>(gr.GetHeight());

    auto cx = std::clamp(x, 0, w);
    auto cy = std::clamp(y, 0, h);

    m_MouseState.x = cx;
    m_MouseState.y = cy;
    SetCursorVectorPos(cx, cy);

    // Also warp the OS cursor
    auto* window = gr.GetWindow();
    if (window)
        SDL_WarpMouseInWindow(window, static_cast<float>(cx), static_cast<float>(cy));

    return 1;
}

void InputSystem::SetMouseSpeed(std::int32_t nMouseSpeed)
{
    m_nMouseSpeed = nMouseSpeed;
}

// =============================================================================
// Cursor management
// =============================================================================

void InputSystem::ShowCursor(std::int32_t bShow)
{
    // Original: sets cursor layer color to 0xFFFFFFFF (visible) or 0x00FFFFFF (hidden).
    // SDL3: show/hide the system cursor.
    if (bShow)
        SDL_ShowCursor();
    else
        SDL_HideCursor();

    // TODO: when cursor layers are implemented, also set m_pLayerCursor color
}

auto InputSystem::IsCursorShown() -> std::int32_t
{
    // Original: checks cursor layer color == 0xFFFFFFFF.
    // SDL3: check SDL cursor visibility.
    return SDL_CursorVisible() ? 1 : 0;
}

auto InputSystem::GetCursorState() -> std::int32_t
{
    return m_nCursorState;
}

void InputSystem::SetCursorState(std::int32_t nState, bool bForce)
{
    // Mirrors original — certain "sticky" cursor states require bForce to change.
    const auto cur = m_nCursorState;
    if (cur == nState)
        return;

    // Sticky states: these can only be changed with bForce
    if (!bForce)
    {
        if (cur == 17 || cur == 23 || cur == 58 ||
            cur == 18 || cur == 34 || cur == 35 ||
            cur == 42 || cur == 44 || cur == 43)
        {
            return;
        }
    }

    // Release old cursor type layer
    if (cur >= 0 && cur < 64)
    {
        if (m_pCursorType[cur])
            m_pCursorType[cur].reset();
    }

    // Apply new state
    if (nState == -1)
    {
        // Restore last non-sticky state
        m_nCursorState = m_nLastCursorState;
    }
    else
    {
        // Save current as "last" if it was a basic state (0-8)
        if (cur <= 8)
            m_nLastCursorState = cur;
        m_nCursorState = nState;
    }

    // Load the new cursor (requires WZ resources)
    LoadCursorStateWithIndex(m_nCursorState);

    // TODO: if m_pCursorType[m_nCursorState] loaded successfully,
    // assign it to m_pLayerCursor and set up animation/origin.
}

void InputSystem::SetCursor(
    [[maybe_unused]] const std::shared_ptr<WzGr2DLayer>& pLayer)
{
    // Original: sets layer overlay, z-order (0x7FFFFFFE), color (0xFFFFFFFF),
    // and origin (m_pVectorCursor).
    // TODO: implement when WzGr2DLayer overlay/origin interfaces are ready.
}

void InputSystem::SetCursorVectorPos(
    [[maybe_unused]] std::int32_t x, [[maybe_unused]] std::int32_t y)
{
    // Original: applies cursor movement constraints (m_rcCursorMoveable, m_tCantMoveTime),
    // then calls IWzVector2D::RelMove on m_pVectorCursor.

    if (m_tCantMoveTime && SDL_GetTicks() < m_tCantMoveTime)
        return;
    m_tCantMoveTime = 0;

    // Apply movement constraints
    auto cx = x;
    auto cy = y;

    if (!m_rcCursorMoveable.IsEmpty())
    {
        cx = std::clamp(cx, m_rcCursorMoveable.left, m_rcCursorMoveable.right);
        cy = std::clamp(cy, m_rcCursorMoveable.top, m_rcCursorMoveable.bottom);
    }

    if (m_bCantMoveCursorOrigin)
        return;

    // TODO: call m_pVectorCursor->RelMove(cx - screenW/2, cy - screenH/2)
    // when IWzVector2D interface is implemented.

    m_bCursorOriginMoveByMouse = true;
}

void InputSystem::LoadCursorStateWithIndex(
    [[maybe_unused]] std::int32_t nIndex)
{
    // Original: loads cursor sprite from WZ using StringPool format string (0x11E2)
    // and CAnimationDisplayer::LoadLayer.
    // TODO: implement when WZ cursor resources are loaded.
}

void InputSystem::UnLoadCursorStateWithIndex(std::int32_t nIndex)
{
    // Original: releases cursor type layer and flushes cached objects.
    if (nIndex >= 0 && nIndex < 64 && m_pCursorType[nIndex])
        m_pCursorType[nIndex].reset();
}

auto InputSystem::GetCursorOrigin() -> std::shared_ptr<IWzVector2D>
{
    return m_pVectorCursor;
}

// =============================================================================
// Cursor movement constraints
// =============================================================================

void InputSystem::SetCursorMoveableRect(const Rect& rcMoveable)
{
    m_rcCursorMoveable = rcMoveable;
}

void InputSystem::ResetCursorMoveableRect()
{
    m_rcCursorMoveable = {};
}

void InputSystem::SetMouseCantMoveTime(std::uint32_t tTime)
{
    m_tCantMoveTime = tTime;
}

void InputSystem::SetCantMoveCursorOrigin(bool bSet)
{
    m_bCantMoveCursorOrigin = bSet;
}

auto InputSystem::IsCursorOriginMoveByMouse() -> bool
{
    return m_bCursorOriginMoveByMouse;
}

void InputSystem::SetCursorOriginMoveByMouse(bool bSet)
{
    m_bCursorOriginMoveByMouse = bSet;
}

// =============================================================================
// Message queue
// =============================================================================

auto InputSystem::GetISMessage(ISMSG* pISMsg) -> std::int32_t
{
    // Mirrors ZList<ISMSG> pop-front
    if (m_lISMsg.empty())
        return 0;

    *pISMsg = m_lISMsg.front();
    m_lISMsg.erase(m_lISMsg.begin());
    return 1;
}

void InputSystem::AddISMessage(std::uint32_t message, std::uint32_t wParam,
                               std::int32_t lParam)
{
    m_lISMsg.push_back({message, wParam, lParam});
}

// =============================================================================
// Static
// =============================================================================

auto InputSystem::GetFPSCursorIndexByFieldType(
    std::int32_t nIndex, std::int32_t nFieldType) -> std::int32_t
{
    // Mirrors original — maps cursor indices for specific field types
    if (nFieldType == 67 || nFieldType == 69 || nFieldType == 131)
    {
        switch (nIndex)
        {
        case 58:  return 23;   // ':' → 23
        case 59:  return 24;   // ';' → 24
        case 60:  return 25;   // '<' → 25
        default:  break;
        }
    }
    else if (nFieldType == 125)
    {
        switch (nIndex)
        {
        case 23:  return 58;
        case 24:  return 59;
        case 25:  return 60;
        default:  break;
        }
    }
    return nIndex;
}

} // namespace ms
