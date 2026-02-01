#pragma once

#include "UIElement.h"
#include "util/Singleton.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ms
{

class Login;
class UIButton;
class WzCanvas;
class WzGr2D;
class WzGr2DLayer;
class WzProperty;
class UIManager;

/**
 * @brief Race/job selection UI for new character creation
 *
 * Based on CUINewCharRaceSelect_Ex from the original MapleStory client (v1029).
 * Singleton class that handles race/job selection for new characters.
 *
 * Original methods (from IDA):
 * - Constructor: CUINewCharRaceSelect_Ex(CLogin*) @ 0xba96f0
 * - Destructor: ~CUINewCharRaceSelect_Ex() @ 0xba9a40
 * - SetSelectedRace(int) @ 0xba6820
 * - SetSelectedSubJob(int) @ 0xba6850
 * - IsEnabledRace(int) @ 0xba6890
 * - LoadButton() @ 0xbac760
 * - Draw() @ 0xbab0b0
 *
 * Base UOL: "UI/Login.img/RaceSelect_new"
 */
class UINewCharRaceSelect final : public UIElement, public Singleton<UINewCharRaceSelect>
{
    friend class Singleton<UINewCharRaceSelect>;

public:
    // Button layout constants from original client (LoadButton @ 0xbac760)
    // X = 126 * i + 92, Y = 427, where i = 0..4
    static constexpr std::int32_t kButtonStartX = 92;
    static constexpr std::int32_t kButtonSpacingX = 126;
    static constexpr std::int32_t kButtonY = 427;
    static constexpr std::int32_t kButtonsPerPage = 5;
    static constexpr std::int32_t kMaxRaceCount = 19;

    // Arrow button IDs from OnButtonClicked @ 0xbb4cd0
    static constexpr std::uint32_t kLeftArrowId = 10001;
    static constexpr std::uint32_t kRightArrowId = 10000;
    static constexpr std::uint32_t kConfirmId = 10002;
    static constexpr std::uint32_t kCancelId = 10003;

    ~UINewCharRaceSelect() override;

    /**
     * @brief Initialize the race select UI
     * @param pLogin Pointer to the Login stage
     * @param gr Graphics engine reference
     * @param uiManager UI manager reference
     */
    void OnCreate(Login* pLogin, WzGr2D& gr, UIManager& uiManager);

    /**
     * @brief Set the selected race
     * @param race Race index
     */
    void SetSelectedRace(std::int32_t race);

    /**
     * @brief Set the selected sub-job
     * @param subJob Sub-job index
     */
    void SetSelectedSubJob(std::int32_t subJob);

    /**
     * @brief Check if a race is enabled (can be selected)
     * @param race Race index
     * @return True if the race can be selected
     */
    [[nodiscard]] auto IsEnabledRace(std::int32_t race) const -> bool;

    /**
     * @brief Get the currently selected race
     */
    [[nodiscard]] auto GetSelectedRace() const noexcept -> std::int32_t { return m_nSelectedRace; }

    /**
     * @brief Destroy the race select UI and cleanup resources
     */
    void Destroy();

    // UIElement interface
    void Update() override;
    void Draw() override;

    void OnMouseMove(std::int32_t x, std::int32_t y) override;
    void OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button) override;
    void OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button) override;
    void OnKeyDown(std::int32_t keyCode) override;

private:
    UINewCharRaceSelect();

    /**
     * @brief Load race buttons for current page (from LoadButton @ 0xbac760)
     * Loads 5 buttons at a time based on m_nFrontOrderBtn offset.
     * Uses WZ paths: "UI/Login.img/RaceSelect_new/button/%d" for enabled,
     *               "UI/Login.img/RaceSelect_new/buttonDisabled/%d" for disabled.
     */
    void LoadButton();

    /**
     * @brief Handle button click (from OnButtonClicked @ 0xbb4cd0)
     * @param nId Button ID (race ID or arrow button ID)
     */
    void OnButtonClicked(std::uint32_t nId);

    /**
     * @brief Select a race button and update UI (from SelectRaceButton)
     * @param nIdx Button index within current page (0-4)
     */
    void SelectRaceButton(std::uint32_t nIdx);

    /**
     * @brief Open race confirmation dialog
     */
    void OpenConfirmRaceDlg();

    /**
     * @brief Load background from WZ
     */
    void LoadBackground();

    /**
     * @brief Load arrow buttons for pagination
     */
    void LoadArrowButtons();

    /**
     * @brief Load character preview for selected race
     * @param raceId The race ID to show preview for
     */
    void LoadCharacterPreview(std::int32_t raceId);

    /**
     * @brief Load race info/description for selected race
     * @param raceId The race ID to show info for
     */
    void LoadRaceInfo(std::int32_t raceId);

    /**
     * @brief Load New/Hot indicator layers for race buttons
     */
    void LoadNewHotIndicators();

    // Reference to Login stage (not owned)
    Login* m_pLogin{nullptr};

    // Reference to graphics engine (not owned)
    WzGr2D* m_pGr{nullptr};

    // Reference to UI manager (not owned)
    UIManager* m_pUIManager{nullptr};

    // Selection state (from constructor @ 0xba96f0)
    std::int32_t m_nSelectedRace{1};        // Default selected race (original uses 1)
    std::int32_t m_nSelectedSubJob{0};
    std::int32_t m_nSelectedBtnIdx{0};      // Current button index (0-4)
    std::int16_t m_nFrontOrderBtn{0};       // Pagination offset
    std::int32_t m_nBtRaceCount{19};        // Total race count
    std::int32_t m_nSelectFirstBtnIdx{-1};  // First selected button index

    // Race ordering array (indices into race list)
    std::int16_t m_anOrderRace[kMaxRaceCount]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};

    // New/Hot race flags
    bool m_abNewRace[kMaxRaceCount]{false};
    bool m_abHotRace[kMaxRaceCount]{false};

    // Race buttons (5 visible at a time, from m_apButton[5] in original)
    std::shared_ptr<UIButton> m_apButton[kButtonsPerPage];

    // Navigation buttons
    std::shared_ptr<UIButton> m_pLeftButton;
    std::shared_ptr<UIButton> m_pRightButton;
    std::shared_ptr<UIButton> m_pCreateButton;  // Make/Confirm button
    std::shared_ptr<UIButton> m_pCancelButton;  // Cancel/Back button

    // Character preview layer
    std::shared_ptr<WzGr2DLayer> m_pLayerCharPreview;

    // Race info text layer
    std::shared_ptr<WzGr2DLayer> m_pLayerRaceInfo;

    // Layers
    std::shared_ptr<WzGr2DLayer> m_pLayerBackGround;
    std::shared_ptr<WzGr2DLayer> m_pLayerBackGround1;
    std::shared_ptr<WzGr2DLayer> m_pLayerBackGround2;

    // Cached WZ property
    std::shared_ptr<WzProperty> m_pRaceSelectProp;

    // New/Hot indicator layers (5 visible at a time, positioned above buttons)
    std::shared_ptr<WzGr2DLayer> m_apNewIndicator[kButtonsPerPage];
    std::shared_ptr<WzGr2DLayer> m_apHotIndicator[kButtonsPerPage];

    // Cached New/Hot indicator canvases from WZ
    std::shared_ptr<WzCanvas> m_pNewCanvas;
    std::shared_ptr<WzCanvas> m_pHotCanvas;
};

} // namespace ms
