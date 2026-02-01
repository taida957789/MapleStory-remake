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
 * @brief Character selection UI
 *
 * Based on CUISelectChar from the original MapleStory client (v1029).
 * Singleton class that handles character selection.
 *
 * CUISelectChar inherits from CWnd.
 *
 * Original methods (from IDA):
 * - Constructor: CUISelectChar(CLogin*, ZArray<AvatarData>&, int, ZMap&) @ 0xb72740
 * - Destructor: ~CUISelectChar() @ 0xb72360
 * - OnCreate(void*) @ 0xb7a4a0 - Virtual, called after construction
 * - OnButtonClicked(uint) @ 0xb79ff0 - Virtual, button click handler
 * - OnKey(uint, uint) @ 0xb7cb50 - Virtual, keyboard handler
 * - OnMouseMove(int, int) @ 0xb71b20 - Virtual, mouse handler
 * - OnMouseButton(uint, uint, int, int) @ 0xb7cec0 - Virtual, mouse button handler
 * - Update() @ 0xb73350 - Virtual, update handler
 * - SelectCharacter(int) @ 0xb795e0 - Select a character
 * - EnterCharacter() @ 0xb70c10 - Enter with selected character
 * - DeleteChar() @ 0xb72290 - Delete selected character
 * - Refresh() @ 0xb79510 - Refresh character list
 *
 * Window: (30, 30) with size (750, 600)
 * Base UOL: "UI/Login.img/CharSelect"
 */
class UISelectChar final : public UIElement, public Singleton<UISelectChar>
{
    friend class Singleton<UISelectChar>;

public:
    ~UISelectChar() override;

    /**
     * @brief Initialize the character select UI
     * @param pLogin Pointer to the Login stage
     * @param gr Graphics engine reference
     * @param uiManager UI manager reference
     */
    void OnCreate(Login* pLogin, WzGr2D& gr, UIManager& uiManager);

    /**
     * @brief Select a character by index
     * @param charIndex Character index (0-based)
     */
    void SelectCharacter(std::int32_t charIndex);

    /**
     * @brief Enter the game with selected character
     */
    void EnterCharacter();

    /**
     * @brief Delete the selected character
     */
    void DeleteChar();

    /**
     * @brief Refresh character list
     */
    void Refresh();

    /**
     * @brief Handle button click
     * @param nId Button ID
     */
    virtual void OnButtonClicked(std::uint32_t nId);

    /**
     * @brief Get the currently selected character index
     */
    [[nodiscard]] auto GetSelectedIndex() const noexcept -> std::int32_t { return m_nSelectedIndex; }

    /**
     * @brief Destroy the character select UI and cleanup resources
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

private:
    UISelectChar();

    /**
     * @brief Create character slot buttons
     */
    void CreateCharacterSlots();

    /**
     * @brief Create placeholder UI when WZ resources not available
     */
    void CreatePlaceholderUI();

    /**
     * @brief Create a placeholder button
     */
    [[nodiscard]] auto CreatePlaceholderButton(const std::string& name, int x, int y, int width, int height)
        -> std::shared_ptr<UIButton>;

    /**
     * @brief Update character name tags
     */
    void UpdateNameTags();

    /**
     * @brief Update character button visual states based on selection
     */
    void UpdateCharacterButtonStates();

    // Reference to Login stage (not owned)
    Login* m_pLogin{nullptr};

    // Reference to graphics engine (not owned)
    WzGr2D* m_pGr{nullptr};

    // Reference to UI manager (not owned)
    UIManager* m_pUIManager{nullptr};

    // Base UOL for resources
    std::string m_sBaseUOL{"UI/Login.img/CharSelect"};

    // Selection state
    std::int32_t m_nSelectedIndex{-1};  // Currently selected character
    std::int32_t m_nCharCount{0};        // Number of characters
    std::int32_t m_nSlotCount{8};        // Maximum slots per page
    std::int32_t m_nPageIndex{0};        // Current page

    // UI elements
    std::shared_ptr<UIButton> m_pBtnSelect;   // BtSelect - Enter game
    std::shared_ptr<UIButton> m_pBtnNew;      // BtNew - Create new character
    std::shared_ptr<UIButton> m_pBtnDelete;   // BtDelete - Delete character
    std::shared_ptr<UIButton> m_pBtnPageL;    // pageL - Previous page
    std::shared_ptr<UIButton> m_pBtnPageR;    // pageR - Next page

    // Character slot buttons
    std::vector<std::shared_ptr<UIButton>> m_vBtCharacter;

    // Layers
    std::shared_ptr<WzGr2DLayer> m_pLayerBg;           // Background layer
    std::shared_ptr<WzGr2DLayer> m_pLayerSelectedWorld; // Selected world info layer
    std::vector<std::shared_ptr<WzGr2DLayer>> m_vLayerNameTag;  // Character name tags

    // Cached WZ property
    std::shared_ptr<WzProperty> m_pCharSelectProp;
};

} // namespace ms
