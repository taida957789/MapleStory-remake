#pragma once

#include "Stage.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ms
{

class WzGr2DLayer;
class WzProperty;
class WzGr2DCanvas;

/**
 * @brief Loading stage - displays loading screen with progress
 *
 * Extracted from CLogo. Shows a random background, repeat animation,
 * and step progress bar while loading WZ files progressively.
 * After loading completes and fade-out finishes, transitions to CLogin.
 *
 * WZ Resources:
 * - UI/Logo.img/Loading/randomBackgrd - Random background images
 * - UI/Logo.img/Loading/repeat - Repeat animations
 * - UI/Logo.img/Loading/step - Step progress indicators
 * - UI/Logo.img/Grade - Grade overlay images
 */
class Loading final : public Stage
{
public:
    Loading();
    ~Loading() override;

    void Init(void* param) override;
    void Update() override;
    void Draw() override;
    void Close() override;

    /**
     * @brief Update loading progress
     * @param step Current loading step (0-based)
     */
    void SetLoadingProgress(std::int32_t step);

private:
    /**
     * @brief Load logo frames from WZ property
     * @param prop Property containing logo frames
     * @return Vector of canvas objects with origin set
     */
    auto LoadLogoFrames(const std::shared_ptr<WzProperty>& prop)
        -> std::vector<std::shared_ptr<WzGr2DCanvas>>;

    /**
     * @brief Initialize loading screen resources
     */
    void InitLoading();

    /**
     * @brief Start loading mode (setup layers and begin animation)
     */
    void StartLoadingMode();

    /**
     * @brief Start fade out effect
     */
    void FadeOutLoading();

    /**
     * @brief Load WZ files progressively during loading screen
     */
    void LoadWzFilesProgressively();

    /**
     * @brief Transition to login stage
     */
    void GoToLogin();

private:
    // Loading mode state
    std::int32_t m_nLoadingStep{0};
    std::int32_t m_nLoadingStepCount{0};
    std::uint8_t m_loadingAlpha{255};
    std::uint64_t m_dwLoadingStartTick{0};

    // WZ loading state
    bool m_wzLoadingComplete{false};
    std::size_t m_currentWzFileIndex{0};

    // Loading layers
    std::shared_ptr<WzGr2DLayer> m_pLayerLoadingBg;
    std::shared_ptr<WzGr2DLayer> m_pLayerLoadingAnim;

    // Step layers (one layer per step, shown cumulatively)
    std::vector<std::shared_ptr<WzGr2DLayer>> m_stepLayers;

    // Loading frames
    std::vector<std::shared_ptr<WzGr2DCanvas>> m_loadingBgCanvases;
    std::vector<std::vector<std::shared_ptr<WzGr2DCanvas>>> m_repeatAnims;
    std::int32_t m_nCurrentRepeat{0};
    std::int32_t m_nCurrentRepeatFrame{0};

    // Grade frames (for overlay, currently TODO)
    std::vector<std::shared_ptr<WzGr2DCanvas>> m_gradeFrames;
};

} // namespace ms
