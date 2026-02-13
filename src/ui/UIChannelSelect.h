#pragma once

#include "UIElement.h"
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
class WzGr2DCanvas;
class WzGr2DLayer;
class WzProperty;
class UIManager;

/**
 * @brief Channel selection UI dialog
 *
 * Based on CUIChannelSelect from the original MapleStory client (v1029).
 * Singleton class that handles channel selection after a world is selected.
 *
 * CUIChannelSelect inherits from CUniqueModeless (which inherits from CDialog/CWnd).
 *
 * Original methods (from IDA):
 * - Constructor: CUIChannelSelect(void*) @ 0xbbdc00
 * - Destructor: ~CUIChannelSelect() @ 0xbbdf60
 * - OnCreate(void*) @ 0xbc4780 - Virtual, called after construction
 * - OnButtonClicked(uint) @ 0xbbc880 - Virtual, button click handler
 * - OnKey(uint, uint) @ 0xbbbd10 - Virtual, keyboard handler
 * - OnSetFocus(int) @ 0xbbb8d0 - Virtual, focus change handler
 * - ResetInfo(long, int) @ 0xbc3150 - Reset channel info for world
 * - EnterChannel() @ 0xbbb950 - Enter selected channel
 * - DrawNoticeConnecting() @ 0xbbbe50 - Draw connecting notice
 * - GetSelectWorldStatus() @ 0xb531d0 - Get select world status
 * - SetSelectWorldStatus(int) @ 0xb531e0 - Set select world status
 *
 * Dialog position: (203, 194) with Origin_LT
 * Base UOL: "UI/Login.img/WorldSelect/BtChannel/test"
 */
class UIChannelSelect final : public UIElement
{
public:
    UIChannelSelect();
    ~UIChannelSelect() override;

    /**
     * @brief Creation parameters for UIChannelSelect
     */
    struct CreateParams
    {
        Login* login{nullptr};
        WzGr2D* gr{nullptr};
        UIManager* uiManager{nullptr};
        std::int32_t worldIndex{-1};

        [[nodiscard]] auto IsValid() const noexcept -> bool
        {
            return login != nullptr && gr != nullptr && uiManager != nullptr && worldIndex >= 0;
        }
    };

    /**
     * @brief Reset channel info for a world (matches CUIChannelSelect::ResetInfo)
     * @param worldIndex World index
     * @param bRedraw Whether to redraw
     */
    void ResetInfo(std::int32_t worldIndex, bool bRedraw = false);

    /**
     * @brief Enter the selected channel (matches CUIChannelSelect::EnterChannel)
     */
    void EnterChannel();

    /**
     * @brief Handle button click (matches CUIChannelSelect::OnButtonClicked)
     * @param nId Button ID (channel index)
     */
    virtual void OnButtonClicked(std::uint32_t nId);

    /**
     * @brief Handle focus change (matches CUIChannelSelect::OnSetFocus)
     * @param bFocus True if gaining focus
     * @return True to accept focus
     */
    auto OnSetFocus(bool bFocus) -> bool override;

    /**
     * @brief Get the currently selected channel
     */
    [[nodiscard]] auto GetSelectedChannel() const noexcept -> std::int32_t { return m_nSelect; }

    /**
     * @brief Get select world status
     */
    [[nodiscard]] auto GetSelectWorldStatus() const noexcept -> bool { return m_bSelectWorld; }

    /**
     * @brief Set select world status
     */
    void SetSelectWorldStatus(bool status) noexcept { m_bSelectWorld = status; }

    /**
     * @brief Destroy the channel select UI and cleanup resources
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

#ifdef MS_DEBUG_CANVAS
    [[nodiscard]] auto GetDebugTypeName() const -> std::string override { return "UIChannelSelect"; }
#endif

protected:
    auto OnCreate(std::any params) -> Result<void> override;
    void OnDestroy() noexcept override;

private:

    /**
     * @brief Check if request is valid (matches CUIChannelSelect::IsRequestValid)
     */
    [[nodiscard]] auto IsRequestValid() const -> bool;

    /**
     * @brief Navigate to a different channel (from OnKey logic)
     * @param delta Number of channels to move (+1 right, -1 left, +5 down, -5 up)
     */
    void NavigateChannel(std::int32_t delta);

    /**
     * @brief Update visual feedback for channel buttons
     */
    void UpdateChannelButtonStates();

    /**
     * @brief Draw connecting notice (matches CUIChannelSelect::DrawNoticeConnecting)
     */
    void DrawNoticeConnecting();

    /**
     * @brief Remove connecting notice (matches CUIChannelSelect::RemoveNoticeConnecting)
     */
    void RemoveNoticeConnecting();

    /**
     * @brief Create placeholder UI when WZ resources not available
     */
    void CreatePlaceholderUI();

    /**
     * @brief Create placeholder background when chBackgrn WZ resource not available
     * @param gr Graphics engine reference
     * @param x X position of background
     * @param y Y position of background
     */
    void CreatePlaceholderBackground(WzGr2D& gr, std::int32_t x, std::int32_t y);

    // CreateChannelButton 已移除 - 現在由 LayoutMan::AutoBuild 處理

    // Reference to Login stage (not owned) - from original m_pLogin
    Login* m_pLogin{nullptr};

    // Reference to graphics engine (not owned)
    WzGr2D* m_pGr{nullptr};

    // Reference to UI manager (not owned)
    UIManager* m_pUIManager{nullptr};

    // Base UOL for resources (from original m_sBaseUOL)
    std::string m_sBaseUOL{"UI/Login.img/WorldSelect/BtChannel/test"};

    // World index this channel select is showing
    std::int32_t m_nWorldIndex{-1};

    // Channel selection state (from original)
    std::int32_t m_nSelect{0};           // Currently selected channel
    bool m_bSelectWorld{false};          // World selected flag (ready to enter)

    // UI elements
    std::shared_ptr<UIButton> m_pBtnGoWorld;              // GoWorld button (enter channel)
    std::vector<std::shared_ptr<UIButton>> m_vBtChannel;  // Channel buttons

    // Layers
    std::shared_ptr<WzGr2DLayer> m_pLayerBg;              // Background layer
    std::shared_ptr<WzGr2DLayer> m_pLayerGauge;           // Gauge layer
    std::shared_ptr<WzGr2DLayer> m_pLayerEventDesc;       // Event description layer

    // Gauge canvas
    std::shared_ptr<WzGr2DCanvas> m_pCanvasGauge;

    // Cached WZ property
    std::shared_ptr<WzProperty> m_pChannelSelectProp;

    // LayoutMan for automated UI building
    std::unique_ptr<LayoutMan> m_pLayoutMan;
};

} // namespace ms
