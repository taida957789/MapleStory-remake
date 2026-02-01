#pragma once

#include <cstdint>
#include <string>

namespace ms
{

/**
 * @brief Base class for game stages (scenes)
 *
 * Based on CStage from the original MapleStory client.
 *
 * CStage inherits from:
 * - IGObj (game object interface)
 * - IUIMsgHandler (UI message handler)
 * - INetMsgHandler (network message handler)
 * - ZRefCounted (reference counting)
 *
 * Known derived classes:
 * - CLogo - Logo/intro screen
 * - CLogin - Login/character select screen
 * - CField - Main game field/map
 * - CCashShop - Cash shop
 * - CMonsterFarm - Monster farm
 * - CInterStage - Transition stage
 */
class Stage
{
public:
    Stage();
    virtual ~Stage();

    // Non-copyable
    Stage(const Stage&) = delete;
    auto operator=(const Stage&) -> Stage& = delete;

    /**
     * @brief Initialize the stage with optional parameters
     *
     * Based on CStage::Init (virtual)
     */
    virtual void Init(void* param);

    /**
     * @brief Update the stage logic
     *
     * Based on pure virtual Update in derived classes
     */
    virtual void Update() = 0;

    /**
     * @brief Draw the stage
     */
    virtual void Draw() = 0;

    /**
     * @brief Close the stage
     *
     * Based on CStage::Close (virtual)
     */
    virtual void Close();

    /**
     * @brief Handle focus change
     *
     * Based on CStage::OnSetFocus
     */
    virtual auto OnSetFocus(int focused) -> bool;

    /**
     * @brief Handle mouse movement
     */
    virtual void OnMouseMove(std::int32_t x, std::int32_t y);

    /**
     * @brief Handle mouse button down
     */
    virtual void OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button);

    /**
     * @brief Handle mouse button up
     */
    virtual void OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button);

    /**
     * @brief Handle key down
     */
    virtual void OnKeyDown(std::int32_t keyCode);

    /**
     * @brief Handle key up
     */
    virtual void OnKeyUp(std::int32_t keyCode);

    /**
     * @brief Handle text input (for edit fields)
     */
    virtual void OnTextInput(const std::string& text);

    // Properties
    [[nodiscard]] auto IsFadeInOut() const noexcept -> bool { return m_bFadeInOut; }
    void SetFadeInOut(bool fade) noexcept { m_bFadeInOut = fade; }

    [[nodiscard]] auto IsOverlapTransfer() const noexcept -> bool { return m_bOverlapTransfer; }
    void SetOverlapTransfer(bool overlap) noexcept { m_bOverlapTransfer = overlap; }

protected:
    // Fade in/out effect enabled
    bool m_bFadeInOut{true};

    // Allow overlap during stage transfer
    bool m_bOverlapTransfer{false};
};

} // namespace ms
