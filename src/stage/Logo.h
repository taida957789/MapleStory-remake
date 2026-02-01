#pragma once

#include "Stage.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

class WzGr2DLayer;
class WzProperty;
class WzCanvas;

/**
 * @brief Logo stage - displays company logos and intro
 *
 * Based on CLogo from the original MapleStory client (v1029).
 * Constructor: 0xBC5720
 *
 * This stage is the first thing shown after initialization.
 * It displays:
 * - Message (frames 0-49): Fade in (0-24), fade out (25-49)
 * - Logo (frames 50+): Company logos with grade overlays
 * - Optional intro video
 *
 * Frame timing: ~10 fps (frame = elapsed_ms / 100)
 *
 * After completion, transitions to CLogin stage.
 *
 * WZ Resources:
 * - UI/Logo.img/Logo - Logo animation frames
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
     * - Logo mode: 5 seconds minimum (frame >= 50)
     * - Video mode: 5000ms minimum
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
     * Loads UI/Logo.img/Logo, Grade, Message and creates display layers
     */
    void InitWZLogo();

    /**
     * @brief Update logo display mode
     *
     * Based on CLogo::UpdateLogo @ 0xbc7a10
     * Frame timing: frame = (elapsed_ms * 2748779070) >> 38 ≈ elapsed_ms / 100
     */
    void UpdateLogo();

    /**
     * @brief Update video playback mode
     *
     * Based on CLogo::UpdateVideo @ 0xbc5950
     */
    void UpdateVideo();

    /**
     * @brief Draw logo for specific frame
     *
     * Based on CLogo::DrawWZLogo @ 0xbc71a0
     * @param nFrame Frame number (-1 = clear only)
     *        0-49: Message display
     *        50+: Logo display with grade overlays
     */
    void DrawWZLogo(std::int32_t nFrame);

    /**
     * @brief End logo stage and transition
     *
     * Based on CLogo::LogoEnd @ 0xbc5890
     */
    void LogoEnd();

    /**
     * @brief Transition to login stage
     */
    void GoToLogin();

    /**
     * @brief Load logo frames from WZ property
     * @param prop Property containing logo frames
     * @return Vector of canvas objects
     */
    auto LoadLogoFrames(const std::shared_ptr<WzProperty>& prop)
        -> std::vector<std::shared_ptr<WzCanvas>>;

private:
    // Logo properties (from UI/Logo.img)
    std::shared_ptr<WzProperty> m_pLogoProp;      // UI/Logo.img/Logo
    std::shared_ptr<WzProperty> m_pGradeProp;     // UI/Logo.img/Grade
    std::shared_ptr<WzProperty> m_pMessageProp;   // UI/Logo.img/Message

    // Logo/Grade counts
    std::int32_t m_nLogoCount{};    // Number of logo frames
    std::int32_t m_nGradeCount{};   // Number of grade overlays

    // Timing (from CLogo constructor @ 0xbc5773-0xbc5779)
    bool m_bTickInitial{false};         // Tick initialized flag
    std::uint64_t m_dwTickInitial{};    // Initial tick time
    std::uint64_t m_dwClick{};          // Click time for skip

    // Sound
    bool m_bLogoSoundPlayed{false};

    // Video mode (from CLogo @ 0xbc577f-0xbc5782)
    bool m_bVideoMode{false};
    VideoState m_videoState{VideoState::Unavailable};

    // Rendering layers (matching CLogo member names)
    std::shared_ptr<WzGr2DLayer> m_pLayerBackground;  // Background layer (800x600 black)
    std::shared_ptr<WzGr2DLayer> m_pLayerMain;        // Main logo layer

    // Loaded logo frames
    std::vector<std::shared_ptr<WzCanvas>> m_logoFrames;
    std::vector<std::shared_ptr<WzCanvas>> m_gradeFrames;
    std::vector<std::shared_ptr<WzCanvas>> m_messageFrames;
};

} // namespace ms
