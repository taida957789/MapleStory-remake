#pragma once

#include "Stage.h"

#include <cstdint>
#include <memory>

namespace ms
{

class WzGr2DLayer;
class WzProperty;

/**
 * @brief Logo stage - displays company logos and intro
 *
 * Based on CLogo from the original MapleStory client (v1029).
 * Constructor: 0xBC5720
 *
 * This stage is the first thing shown after initialization.
 * It displays:
 * - Message phase: Fade in/out using Gr2D alpha interpolation
 * - Logo phase: Wizet animation using Gr2D frame animation with WZ delay values
 * - Optional intro video
 *
 * After completion, transitions to Loading stage.
 *
 * WZ Resources:
 * - UI/Logo.img/Wizet - Logo animation frames
 * - UI/Logo.img/Grade - Grade overlay images
 * - UI/Logo.img/Message - Message frames (rating info)
 */
class Logo final : public Stage
{
public:
    /**
     * @brief Video state enum (from CLogo @ 0xbc5782)
     */
    enum class VideoState
    {
        Unavailable = 0,  // VIDEO_STATE_UNAVAILABLE
        Playing = 3,      // VIDEO_STATE_PLAYING
        FadeOut = 4,      // VIDEO_STATE_FADEOUT
        End = 5           // VIDEO_STATE_END
    };

    /**
     * @brief Logo display phase
     */
    enum class LogoPhase
    {
        Message,  // Showing message with fade in/out
        Logo,     // Showing Wizet logo animation
        Done      // Animation complete, waiting to transition
    };

    Logo();
    ~Logo() override;

    void Init(void* param) override;
    void Update() override;
    void Draw() override;
    void Close() override;

    // Input handling for skip
    void OnKeyDown(std::int32_t keyCode) override;
    void OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button) override;

    /**
     * @brief Check if logo can be skipped
     *
     * Based on CLogo::CanSkip @ 0xbc55f0
     */
    [[nodiscard]] auto CanSkip() const -> bool;

    /**
     * @brief Force end the logo display
     *
     * Based on CLogo::ForcedEnd @ 0xbc78d0
     */
    void ForcedEnd();

private:
    /**
     * @brief Initialize WZ logo resources
     *
     * Based on CLogo::InitWZLogo @ 0xbc5b20
     * Sets up layers with all frames and starts message animation.
     */
    void InitWZLogo();

    /**
     * @brief Update logo display mode
     *
     * Monitors Gr2D animation state for phase transitions.
     */
    void UpdateLogo();

    /**
     * @brief Update video playback mode
     */
    void UpdateVideo();

    /**
     * @brief End logo stage and transition
     */
    void LogoEnd();

    /**
     * @brief Transition to loading stage
     */
    void GoToLoading();

private:
    // Logo properties (from UI/Logo.img)
    std::shared_ptr<WzProperty> m_pLogoProp;      // UI/Logo.img/Wizet
    std::shared_ptr<WzProperty> m_pGradeProp;     // UI/Logo.img/Grade
    std::shared_ptr<WzProperty> m_pMessageProp;   // UI/Logo.img/Message

    // Logo phase tracking
    LogoPhase m_logoPhase{LogoPhase::Message};

    // Timing (from CLogo constructor @ 0xbc5773-0xbc5779)
    bool m_bTickInitial{false};
    std::uint64_t m_dwTickInitial{};
    std::uint64_t m_dwClick{};
    std::uint64_t m_dwDoneTick{};  // Tick when logo animation finished

    // Sound
    bool m_bLogoSoundPlayed{false};

    // Video mode (from CLogo @ 0xbc577f-0xbc5782)
    bool m_bVideoMode{false};
    VideoState m_videoState{VideoState::Unavailable};

    // Rendering layers
    std::shared_ptr<WzGr2DLayer> m_pLayerBackground;  // Background layer (black, z=0)
    std::shared_ptr<WzGr2DLayer> m_pLayerMessage;      // Message layer (z=1)
    std::shared_ptr<WzGr2DLayer> m_pLayerLogo;         // Logo animation layer (z=2)

};

} // namespace ms
