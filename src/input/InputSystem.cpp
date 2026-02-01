#include "InputSystem.h"
#include "util/Logger.h"

namespace ms
{

InputSystem::InputSystem() = default;

InputSystem::~InputSystem()
{
    Shutdown();
}

auto InputSystem::Initialize() -> bool
{
    if (m_bInitialized)
        return true;

    // SDL input is already initialized with SDL_Init
    m_bInitialized = true;

    LOG_INFO("Input system initialized");
    return true;
}

void InputSystem::Shutdown()
{
    if (!m_bInitialized)
        return;

    m_keyDown.clear();
    m_keyPressed.clear();
    m_keyReleased.clear();

    m_bInitialized = false;
}

void InputSystem::ProcessEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_KEY_DOWN:
        if (!event.key.repeat)
        {
            m_keyDown[event.key.key] = true;
            m_keyPressed[event.key.key] = true;
        }
        break;

    case SDL_EVENT_KEY_UP:
        m_keyDown[event.key.key] = false;
        m_keyReleased[event.key.key] = true;
        break;

    case SDL_EVENT_MOUSE_MOTION:
        m_nMouseX = static_cast<int>(event.motion.x);
        m_nMouseY = static_cast<int>(event.motion.y);
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        m_nMouseButtons |= (1u << event.button.button);
        m_nMouseButtonsPressed |= (1u << event.button.button);
        break;

    case SDL_EVENT_MOUSE_BUTTON_UP:
        m_nMouseButtons &= ~(1u << event.button.button);
        m_nMouseButtonsReleased |= (1u << event.button.button);
        break;

    case SDL_EVENT_MOUSE_WHEEL:
        m_nMouseWheelDelta = static_cast<int>(event.wheel.y);
        break;

    default:
        break;
    }
}

void InputSystem::UpdateDevice([[maybe_unused]] int deviceType)
{
    // In original: polls DirectInput devices
    // With SDL, this is handled by ProcessEvent
}

auto InputSystem::IsKeyDown(SDL_Keycode key) const -> bool
{
    const auto it = m_keyDown.find(key);
    return it != m_keyDown.end() && it->second;
}

auto InputSystem::IsKeyPressed(SDL_Keycode key) const -> bool
{
    const auto it = m_keyPressed.find(key);
    return it != m_keyPressed.end() && it->second;
}

auto InputSystem::IsKeyReleased(SDL_Keycode key) const -> bool
{
    const auto it = m_keyReleased.find(key);
    return it != m_keyReleased.end() && it->second;
}

auto InputSystem::IsMouseButtonDown(int button) const -> bool
{
    return (m_nMouseButtons & (1u << static_cast<unsigned>(button))) != 0;
}

auto InputSystem::IsMouseButtonPressed(int button) const -> bool
{
    return (m_nMouseButtonsPressed & (1u << static_cast<unsigned>(button))) != 0;
}

auto InputSystem::IsMouseButtonReleased(int button) const -> bool
{
    return (m_nMouseButtonsReleased & (1u << static_cast<unsigned>(button))) != 0;
}

auto InputSystem::GetKeyState(int vKey) const -> short
{
    // Convert virtual key to SDL keycode (simplified)
    const auto key = static_cast<SDL_Keycode>(vKey);

    if (IsKeyDown(key))
        return static_cast<short>(0x8000);

    return 0;
}

auto InputSystem::GetAsyncKeyState(int vKey) const -> short
{
    return GetKeyState(vKey);
}

void InputSystem::EndFrame()
{
    // Clear pressed/released states
    m_keyPressed.clear();
    m_keyReleased.clear();
    m_nMouseButtonsPressed = 0;
    m_nMouseButtonsReleased = 0;
    m_nMouseWheelDelta = 0;
}

} // namespace ms
