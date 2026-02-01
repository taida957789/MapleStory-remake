#pragma once

#include "util/Singleton.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include <unordered_map>

namespace ms
{

/**
 * @brief Input handling system
 *
 * Based on CInputSystem from the original MapleStory client.
 * Original uses DirectInput8 for keyboard and mouse input.
 * This implementation uses SDL3 events.
 */
class InputSystem final : public Singleton<InputSystem>
{
    friend class Singleton<InputSystem>;

public:
    [[nodiscard]] auto Initialize() -> bool;
    void Shutdown();

    /**
     * @brief Process SDL event
     * @param event The SDL event to process
     */
    void ProcessEvent(const SDL_Event& event);

    /**
     * @brief Update input devices
     *
     * Based on CInputSystem::UpdateDevice
     * @param deviceType Type of device to update
     */
    void UpdateDevice(int deviceType);

    // Keyboard state queries
    [[nodiscard]] auto IsKeyDown(SDL_Keycode key) const -> bool;
    [[nodiscard]] auto IsKeyPressed(SDL_Keycode key) const -> bool;
    [[nodiscard]] auto IsKeyReleased(SDL_Keycode key) const -> bool;

    // Mouse position
    [[nodiscard]] auto GetMouseX() const noexcept -> int { return m_nMouseX; }
    [[nodiscard]] auto GetMouseY() const noexcept -> int { return m_nMouseY; }

    // Mouse button state
    [[nodiscard]] auto IsMouseButtonDown(int button) const -> bool;
    [[nodiscard]] auto IsMouseButtonPressed(int button) const -> bool;
    [[nodiscard]] auto IsMouseButtonReleased(int button) const -> bool;

    /**
     * @brief Get key state (for compatibility)
     *
     * Based on ZAPI.GetKeyState
     */
    [[nodiscard]] auto GetKeyState(int vKey) const -> short;

    /**
     * @brief Get async key state
     *
     * Based on ZAPI.GetAsyncKeyState
     */
    [[nodiscard]] auto GetAsyncKeyState(int vKey) const -> short;

    /**
     * @brief End of frame - clear pressed/released states
     */
    void EndFrame();

private:
    InputSystem();
    ~InputSystem() override;

    // Key states
    std::unordered_map<SDL_Keycode, bool> m_keyDown;
    std::unordered_map<SDL_Keycode, bool> m_keyPressed;
    std::unordered_map<SDL_Keycode, bool> m_keyReleased;

    // Mouse state
    int m_nMouseX = 0;
    int m_nMouseY = 0;
    std::uint32_t m_nMouseButtons = 0;
    std::uint32_t m_nMouseButtonsPressed = 0;
    std::uint32_t m_nMouseButtonsReleased = 0;

    // Mouse wheel
    int m_nMouseWheelDelta = 0;

    bool m_bInitialized = false;
};

} // namespace ms
