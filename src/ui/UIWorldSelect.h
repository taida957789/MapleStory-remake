#pragma once

#include "UIElement.h"
#include "util/Singleton.h"
#include "LayoutMan.h"

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
 * @brief World selection UI dialog
 *
 * Based on CUIWorldSelect from the original MapleStory client (v1029).
 * Singleton class that handles world/channel selection.
 *
 * CUIWorldSelect inherits from CDialog (which inherits from CWnd).
 *
 * Original methods (from IDA):
 * - Constructor: CUIWorldSelect(CLogin*) @ 0xbbd6f0
 * - Destructor: ~CUIWorldSelect() @ 0xbbda60
 * - OnCreate(void*) @ 0xbc54f0 - Virtual, called after construction
 * - OnButtonClicked(uint) @ 0xbc3750 - Virtual, button click handler
 * - OnKey(uint, uint) @ 0xbbc450 - Virtual, keyboard handler
 * - OnSetFocus(int) @ 0xbbc760 - Virtual, focus change handler
 * - OnMouseButton(uint, uint, long, long) @ 0xbbbc10 - Virtual, mouse handler
 * - HitTest(long, long, CCtrlWnd**) @ 0xbbda20 - Virtual, hit testing
 * - InitWorldButtons() @ 0xbbcb80 - Virtual, initialize world buttons
 * - SetRet(long) @ 0xbbb890 - Virtual, set return value
 * - DrawWorldItems() @ 0xbc4b00 - Draw/create world item buttons
 * - EnableButtons(long) @ 0xbbbfb0 - Enable/disable buttons
 * - SetFocusWorld(long) @ 0xbbc350 - Set focus to specific world
 * - SetKeyFocus(long) @ 0xbbc120 - Private, set keyboard focus
 * - IsRequestValid() @ 0xbbb8a0 - Private, check if request is valid
 * - MakeWSBalloon(Ztl_bstr_t, long, long) @ 0xbc3950 - Private, create balloon
 * - CreateCanvas(...) @ 0xbc07c0 - Private, create canvas
 * - TestInit() @ 0xbbcf30 - Private, test initialization
 *
 * Dialog position: (652, 37) with Origin_LT
 * Base UOL: "UI/Login.img/WorldSelect/BtWorld/test"
 */
class UIWorldSelect final : public UIElement
{
public:
    UIWorldSelect();
    ~UIWorldSelect() override;

    /**
     * @brief Creation parameters for UIWorldSelect
     */
    struct CreateParams
    {
        Login* login{nullptr};
        WzGr2D* gr{nullptr};
        UIManager* uiManager{nullptr};

        [[nodiscard]] auto IsValid() const noexcept -> bool
        {
            return login != nullptr && gr != nullptr && uiManager != nullptr;
        }
    };

    /**
     * @brief Set return value (matches CUIWorldSelect::SetRet)
     */
    void SetRet(std::int32_t ret);

    /**
     * @brief Initialize world buttons (matches CUIWorldSelect::InitWorldButtons)
     */
    virtual void InitWorldButtons();

    /**
     * @brief Initialize world buttons from LayoutMan
     * @param nDisplayCount Number of display slots
     * @param layoutProp Layout property for worldID mapping
     */
    void InitWorldButtons(int nDisplayCount, std::shared_ptr<WzProperty> layoutProp);

    /**
     * @brief Draw world items - creates buttons for each world
     * Based on CUIWorldSelect::DrawWorldItems (0xbc4b00)
     */
    void DrawWorldItems();

    /**
     * @brief Handle button click (matches CUIWorldSelect::OnButtonClicked)
     * @param nId Button ID
     */
    virtual void OnButtonClicked(std::uint32_t nId);

    /**
     * @brief Enable/disable buttons (matches CUIWorldSelect::EnableButtons)
     * @param nId Button ID
     */
    void EnableButtons(std::int32_t nId);

    /**
     * @brief Set focus to a world (matches CUIWorldSelect::SetFocusWorld)
     * @param worldIndex World index to focus
     */
    void SetFocusWorld(std::int32_t worldIndex);

    /**
     * @brief Handle focus change (matches CUIWorldSelect::OnSetFocus)
     * @param bFocus True if gaining focus
     * @return True to accept focus
     */
    auto OnSetFocus(bool bFocus) -> bool override;

    /**
     * @brief Hit test (matches CUIWorldSelect::HitTest)
     */
    [[nodiscard]] auto HitTest(std::int32_t x, std::int32_t y) const -> bool override;

    /**
     * @brief Get the currently focused world index
     */
    [[nodiscard]] auto GetKeyFocus() const noexcept -> std::int32_t { return m_nKeyFocus; }

    /**
     * @brief Get the selected channel
     */
    [[nodiscard]] auto GetSelectedChannel() const noexcept -> std::int32_t { return m_nSelectedChannel; }
    void SetSelectedChannel(std::int32_t channel) noexcept { m_nSelectedChannel = channel; }

    /**
     * @brief Destroy the world select UI and cleanup resources
     */
    void Destroy();

    // UIElement interface
    void Update() override;
    void Draw() override;

    void OnMouseMove(std::int32_t x, std::int32_t y) override;
    void OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button) override;
    void OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button) override;
    void OnKeyDown(std::int32_t keyCode) override;

    /**
     * @brief Create the rendering layer
     */
    void CreateLayer(WzGr2D& gr, std::int32_t z);

protected:
    auto OnCreate(std::any params) -> Result<void> override;
    void OnDestroy() noexcept override;

private:
    /**
     * @brief Set keyboard focus (matches CUIWorldSelect::SetKeyFocus)
     */
    void SetKeyFocus(std::int32_t nFocus);

    /**
     * @brief Navigate to a different world
     * @param delta Number of worlds to move (+1 down, -1 up)
     */
    void NavigateWorld(std::int32_t delta);

    /**
     * @brief Update visual feedback for world buttons
     */
    void UpdateWorldButtonStates();

    /**
     * @brief Check if request is valid (matches CUIWorldSelect::IsRequestValid)
     */
    [[nodiscard]] auto IsRequestValid() const -> bool;

    /**
     * @brief Create balloon message (matches CUIWorldSelect::MakeWSBalloon)
     */
    void MakeWSBalloon(const std::string& message, std::int32_t x, std::int32_t y);

    /**
     * @brief Create placeholder UI when WZ resources not available
     */
    void CreatePlaceholderUI();

    /**
     * @brief Create a world button
     */
    [[nodiscard]] auto CreateWorldButton(const std::string& name, std::int32_t x, std::int32_t y)
        -> std::shared_ptr<UIButton>;

    // Reference to Login stage (not owned) - from original m_pLogin
    Login* m_pLogin{nullptr};

    // Reference to graphics engine (not owned)
    WzGr2D* m_pGr{nullptr};

    // Reference to UI manager (not owned)
    UIManager* m_pUIManager{nullptr};

    // Base UOL for resources (from original m_sBaseUOL)
    std::string m_sBaseUOL{"UI/Login.img/WorldSelect/BtWorld/test"};

    // World selection state (from original)
    std::int32_t m_nKeyFocus{-1};        // Currently focused world index
    std::int32_t m_nSelectedChannel{0};  // Selected channel
    std::int32_t m_nWorld{0};            // Number of worlds
    std::int32_t m_nRet{0};              // Return value

    // UI elements
    std::shared_ptr<UIButton> m_pBtnGoWorld;      // Enter world button (BtGoworld)
    std::vector<std::shared_ptr<UIButton>> m_vBtWorld;   // World buttons

    // Button names array (from original m_aButtonName)
    std::vector<std::string> m_aButtonName;

    // Layers
    std::shared_ptr<WzGr2DLayer> m_pLayerBg;             // Background layer

    // World state layers (from original m_apLayerWorldState)
    std::vector<std::shared_ptr<WzGr2DLayer>> m_apLayerWorldState;

    // Balloon layers (from original m_apLayerBalloon)
    std::vector<std::shared_ptr<WzGr2DLayer>> m_apLayerBalloon;

    // Balloon count (from original nBalloonCount)
    std::int32_t m_nBalloonCount{0};

    // Cached WZ property
    std::shared_ptr<WzProperty> m_pWorldSelectProp;

    // LayoutMan for automated UI building (from original m_pLm)
    std::unique_ptr<LayoutMan> m_pLayoutMan;
};

} // namespace ms
