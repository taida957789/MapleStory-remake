#include "UIWorldSelect.h"
#include "UIButton.h"
#include "UIChannelSelect.h"
#include "UIManager.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "stage/Login.h"
#include "util/Logger.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "wz/WzResMan.h"

namespace ms
{

UIWorldSelect::UIWorldSelect() = default;

UIWorldSelect::~UIWorldSelect()
{
    Destroy();
}

void UIWorldSelect::OnCreate(Login* pLogin, WzGr2D& gr, UIManager& uiManager)
{
    // Based on CUIWorldSelect::OnCreate (0xbc54f0)
    m_pLogin = pLogin;
    m_pGr = &gr;
    m_pUIManager = &uiManager;
    m_nBalloonCount = 0;
    m_nKeyFocus = -1;

    // Clear existing balloon layers
    for (auto& layer : m_apLayerBalloon)
    {
        if (layer)
        {
            gr.RemoveLayer(layer);
        }
    }
    m_apLayerBalloon.clear();

    // Load WorldSelect WZ property for future use
    auto& resMan = WzResMan::GetInstance();
    auto loginImgProp = resMan.GetProperty("UI/Login.img");
    if (loginImgProp)
    {
        m_pWorldSelectProp = loginImgProp->GetChild("WorldSelect");
        if (m_pWorldSelectProp)
        {
            LOG_DEBUG("UIWorldSelect: WorldSelect property loaded");
        }
    }

    // Draw world items
    DrawWorldItems();

    LOG_DEBUG("UIWorldSelect::OnCreate completed");
}

void UIWorldSelect::SetRet(std::int32_t ret)
{
    m_nRet = ret;
}

void UIWorldSelect::InitWorldButtons()
{
    // Based on CUIWorldSelect::InitWorldButtons (0xbbcb80)
    // Hide all buttons initially, show bg layer

    if (m_pLayerBg)
    {
        m_pLayerBg->SetVisible(true);
    }

    // Clear button names
    m_aButtonName.clear();
}

void UIWorldSelect::DrawWorldItems()
{
    // Based on CUIWorldSelect::DrawWorldItems (0xbc4b00)
    if (!m_pLogin || !m_pGr || !m_pUIManager)
    {
        LOG_WARN("UIWorldSelect::DrawWorldItems - missing references");
        return;
    }

    InitWorldButtons();

    // Get balloon count from Login
    m_nBalloonCount = m_pLogin->GetBalloonCount();

    // Get world items from Login
    const auto& worldItems = m_pLogin->GetWorldItemFinal();
    m_nWorld = static_cast<std::int32_t>(worldItems.size());

    LOG_DEBUG("UIWorldSelect::DrawWorldItems - {} worlds", m_nWorld);

    // Clear existing world state layers
    for (auto& layer : m_apLayerWorldState)
    {
        if (layer)
        {
            m_pGr->RemoveLayer(layer);
        }
    }
    m_apLayerWorldState.clear();

    // Clear existing world buttons
    for (size_t i = 0; i < m_vBtWorld.size(); ++i)
    {
        if (m_vBtWorld[i])
        {
            if (i < m_aButtonName.size())
            {
                m_pUIManager->RemoveElement("world" + m_aButtonName[i]);
            }
            if (m_vBtWorld[i]->GetLayer())
            {
                m_pGr->RemoveLayer(m_vBtWorld[i]->GetLayer());
            }
        }
    }
    m_vBtWorld.clear();

    // If no worlds from server, create placeholder UI
    if (worldItems.empty())
    {
        CreatePlaceholderUI();
        return;
    }

    // CUIWorldSelect dialog position from IDA: (652, 37) with Origin_LT
    // World buttons are positioned relative to this dialog
    constexpr int kDialogX = 652;
    constexpr int kDialogY = 37;

    // World buttons start at approximately (10, 50) relative to dialog
    const int startX = kDialogX + 10;
    int worldY = kDialogY + 50;

    // Try to load BtWorld from WZ
    // string.csv idx 2364: UI/Login.img/WorldSelect/BtWorld/%d where %d is the world ID
    // string.csv idx 2365: UI/Login.img/WorldSelect/BtWorld/t%d for button text
    std::shared_ptr<WzProperty> btWorldProp;
    if (m_pWorldSelectProp)
    {
        btWorldProp = m_pWorldSelectProp->GetChild("BtWorld");
        if (btWorldProp)
        {
            LOG_DEBUG("UIWorldSelect: BtWorld property found");
        }
        else
        {
            LOG_DEBUG("UIWorldSelect: BtWorld property not found");
        }
    }

    // Create button for each world
    for (std::int32_t i = 0; i < static_cast<std::int32_t>(worldItems.size()); ++i)
    {
        const auto& world = worldItems[static_cast<size_t>(i)];

        // Skip Star Planet worlds (handled separately)
        if (world.nWorldID >= 100)
        {
            continue;
        }

        std::string worldIdStr = std::to_string(world.nWorldID);

        // Try to load button from WZ: UI/Login.img/WorldSelect/BtWorld/{worldId}
        std::shared_ptr<UIButton> btn;
        if (btWorldProp)
        {
            auto worldBtnProp = btWorldProp->GetChild(worldIdStr);
            if (worldBtnProp)
            {
                LOG_DEBUG("UIWorldSelect: Found BtWorld/{} property", worldIdStr);
                btn = std::make_shared<UIButton>();
                if (!btn->LoadFromProperty(worldBtnProp))
                {
                    LOG_DEBUG("UIWorldSelect: Failed to load button from BtWorld/{}", worldIdStr);
                    btn.reset();
                }
                else
                {
                    LOG_DEBUG("UIWorldSelect: Successfully loaded button from BtWorld/{}", worldIdStr);
                }
            }
            else
            {
                LOG_DEBUG("UIWorldSelect: BtWorld/{} property not found", worldIdStr);
            }
        }

        // Create placeholder button if WZ not found
        if (!btn)
        {
            btn = CreateWorldButton(world.sName, startX, worldY);
        }
        else
        {
            btn->SetPosition(startX, worldY);
            btn->CreateLayer(*m_pGr, 150);
        }

        if (btn)
        {
            // Set click callback with world index
            const std::int32_t worldIndex = i;
            btn->SetClickCallback([this, worldIndex]() {
                OnButtonClicked(static_cast<std::uint32_t>(worldIndex));
            });

            m_pUIManager->AddElement("world" + worldIdStr, btn);
            m_vBtWorld.push_back(btn);
            m_aButtonName.push_back(worldIdStr);

            LOG_DEBUG("Created world button: {} at ({}, {})", world.sName, startX, worldY);
        }

        // Load world state icon if nWorldState != 0 (Hot/New/Event)
        // string.csv idx 2380: UI/Login.img/WorldSelect/world/%d
        // string.csv idx 2381: UI/Login.img/WorldSelect/world/t%d (xref: 0xbbe36c)
        if (world.nWorldState != 0 && m_pWorldSelectProp)
        {
            auto worldStateProp = m_pWorldSelectProp->GetChild("world");
            if (worldStateProp)
            {
                auto stateProp = worldStateProp->GetChild(std::to_string(world.nWorldState));
                if (stateProp)
                {
                    auto canvas = stateProp->GetCanvas();
                    if (canvas)
                    {
                        // Position icon next to button
                        int iconX = startX + (btn ? btn->GetWidth() : 200) + 5;
                        auto stateLayer = m_pGr->CreateLayer(
                            iconX, worldY,
                            static_cast<std::uint32_t>(canvas->GetWidth()),
                            static_cast<std::uint32_t>(canvas->GetHeight()),
                            151
                        );
                        if (stateLayer)
                        {
                            stateLayer->SetScreenSpace(true);
                            stateLayer->InsertCanvas(canvas, 0, 255, 255);
                            m_apLayerWorldState.push_back(stateLayer);
                        }
                    }
                }
            }
        }

        worldY += 35;
    }

    // Load BtGoworld from WZ - string.csv idx 2369: UI/Login.img/WorldSelect/BtGoworld
    const int goBtnX = kDialogX + 50;
    const int goBtnY = worldY + 20;

    bool btGoworldLoaded = false;
    if (m_pWorldSelectProp)
    {
        auto btGoworldProp = m_pWorldSelectProp->GetChild("BtGoworld");
        if (btGoworldProp)
        {
            m_pBtnGoWorld = std::make_shared<UIButton>();
            if (m_pBtnGoWorld->LoadFromProperty(btGoworldProp))
            {
                m_pBtnGoWorld->SetPosition(goBtnX, goBtnY);
                m_pBtnGoWorld->CreateLayer(*m_pGr, 150);
                btGoworldLoaded = true;
                LOG_DEBUG("UIWorldSelect: BtGoworld loaded from WZ at ({}, {})", goBtnX, goBtnY);
            }
            else
            {
                LOG_DEBUG("UIWorldSelect: Failed to load BtGoworld from WZ");
                m_pBtnGoWorld.reset();
            }
        }
        else
        {
            LOG_DEBUG("UIWorldSelect: BtGoworld property not found");
        }
    }

    // Create placeholder if WZ not loaded
    if (!btGoworldLoaded)
    {
        const int btnWidth = 100;
        const int btnHeight = 35;

        m_pBtnGoWorld = std::make_shared<UIButton>();

        // Create orange gradient button
        auto canvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
        std::vector<std::uint8_t> pixels(static_cast<size_t>(btnWidth * btnHeight * 4));
        for (int y = 0; y < btnHeight; ++y)
        {
            for (int x = 0; x < btnWidth; ++x)
            {
                auto idx = static_cast<size_t>((y * btnWidth + x) * 4);
                float t = static_cast<float>(y) / static_cast<float>(btnHeight);
                pixels[idx + 0] = static_cast<std::uint8_t>(255 - 80 * t);
                pixels[idx + 1] = static_cast<std::uint8_t>(180 - 60 * t);
                pixels[idx + 2] = static_cast<std::uint8_t>(80 - 30 * t);
                pixels[idx + 3] = 255;
            }
        }
        canvas->SetPixelData(std::move(pixels));
        m_pBtnGoWorld->SetStateCanvas(UIState::Normal, canvas);
        m_pBtnGoWorld->SetSize(btnWidth, btnHeight);
        m_pBtnGoWorld->SetPosition(goBtnX, goBtnY);
        m_pBtnGoWorld->CreateLayer(*m_pGr, 150);
        LOG_DEBUG("UIWorldSelect: Placeholder BtGoworld created at ({}, {})", goBtnX, goBtnY);
    }

    m_pBtnGoWorld->SetClickCallback([this]() {
        if (m_pLogin && m_nKeyFocus >= 0)
        {
            LOG_DEBUG("Entering world, changing to step 2");
            m_pLogin->ChangeStep(2);
        }
        else
        {
            LOG_DEBUG("No world selected");
        }
    });
    m_pUIManager->AddElement("btnGoWorld", m_pBtnGoWorld);
}

void UIWorldSelect::OnButtonClicked(std::uint32_t nId)
{
    // Based on CUIWorldSelect::OnButtonClicked (0xbc3750)
    if (!m_pLogin || !m_pGr || !m_pUIManager)
    {
        return;
    }

    // Check if request already sent
    if (m_pLogin->IsRequestSent() || m_pLogin->GetLoginStep() != 1)
    {
        return;
    }

    // Validate world index
    const auto& worldItems = m_pLogin->GetWorldItemFinal();
    if (nId >= worldItems.size())
    {
        return;
    }

    // Find world index in button names
    std::int32_t worldIndex = static_cast<std::int32_t>(nId);

    m_nKeyFocus = worldIndex;

    EnableButtons(static_cast<std::int32_t>(nId));
    SetKeyFocus(-1);  // Reset key focus to update visual state

    LOG_DEBUG("World selected: {} (ID={})",
              worldItems[static_cast<size_t>(worldIndex)].sName,
              worldItems[static_cast<size_t>(worldIndex)].nWorldID);

    // Show channel selection for the selected world
    auto& channelSelect = UIChannelSelect::GetInstance();
    channelSelect.OnCreate(m_pLogin, *m_pGr, *m_pUIManager, worldIndex);
}

void UIWorldSelect::EnableButtons(std::int32_t nId)
{
    // Based on CUIWorldSelect::EnableButtons (0xbbbfb0)
    // Update button states - highlight selected world

    (void)nId;  // Selection tracked via m_nKeyFocus

    // Update visual feedback for buttons
    UpdateWorldButtonStates();
}

void UIWorldSelect::SetFocusWorld(std::int32_t worldIndex)
{
    // Based on CUIWorldSelect::SetFocusWorld (0xbbc350)
    if (worldIndex < 0 || worldIndex >= static_cast<std::int32_t>(m_vBtWorld.size()))
    {
        return;
    }

    m_nKeyFocus = worldIndex;
    EnableButtons(worldIndex);
}

void UIWorldSelect::SetKeyFocus(std::int32_t nFocus)
{
    // Based on CUIWorldSelect::SetKeyFocus (0xbbc120)
    // Internal method to update keyboard focus state
    (void)nFocus;  // Not fully implemented yet
}

auto UIWorldSelect::IsRequestValid() const -> bool
{
    // Based on CUIWorldSelect::IsRequestValid (0xbbb8a0)
    if (!m_pLogin)
    {
        return false;
    }
    return !m_pLogin->IsRequestSent();
}

void UIWorldSelect::MakeWSBalloon(const std::string& message, std::int32_t x, std::int32_t y)
{
    // Based on CUIWorldSelect::MakeWSBalloon (0xbc3950)
    // Create balloon message popup to display world status/event info
    if (!m_pGr || message.empty())
    {
        return;
    }

    // Try to load balloon background from WZ
    auto& resMan = WzResMan::GetInstance();
    std::shared_ptr<WzCanvas> balloonCanvas;

    if (m_pWorldSelectProp)
    {
        auto balloonProp = m_pWorldSelectProp->GetChild("balloon");
        if (balloonProp)
        {
            balloonCanvas = balloonProp->GetCanvas();
        }
    }

    // Determine balloon dimensions
    std::uint32_t balloonWidth = 200;
    std::uint32_t balloonHeight = 50;

    if (balloonCanvas)
    {
        balloonWidth = balloonCanvas->GetWidth();
        balloonHeight = balloonCanvas->GetHeight();
    }

    // Create balloon layer (z=200 to render above world buttons)
    auto layer = m_pGr->CreateLayer(x, y, balloonWidth, balloonHeight, 200);
    if (layer)
    {
        layer->SetScreenSpace(true);
        layer->SetCenterBased(true);

        if (balloonCanvas)
        {
            layer->InsertCanvas(balloonCanvas, 0, 255, 255);
        }

        m_apLayerBalloon.push_back(layer);
        ++m_nBalloonCount;

        LOG_DEBUG("UIWorldSelect: Created balloon {} at ({}, {}): {}",
                  m_nBalloonCount, x, y, message);
    }
}

auto UIWorldSelect::OnSetFocus(bool bFocus) -> bool
{
    // Based on CUIWorldSelect::OnSetFocus (0xbbc760)
    return UIElement::OnSetFocus(bFocus);
}

auto UIWorldSelect::HitTest(std::int32_t x, std::int32_t y) const -> bool
{
    // Based on CUIWorldSelect::HitTest (0xbbda20)
    return UIElement::HitTest(x, y);
}

void UIWorldSelect::Destroy()
{
    if (!m_pGr)
    {
        return;
    }

    // Destroy channel select first
    UIChannelSelect::GetInstance().Destroy();

    // Clean up enter button
    if (m_pBtnGoWorld)
    {
        if (m_pUIManager)
        {
            m_pUIManager->RemoveElement("btnGoWorld");
        }
        if (m_pBtnGoWorld->GetLayer())
        {
            m_pGr->RemoveLayer(m_pBtnGoWorld->GetLayer());
        }
        m_pBtnGoWorld.reset();
    }

    // Clean up world buttons
    for (size_t i = 0; i < m_vBtWorld.size(); ++i)
    {
        if (m_vBtWorld[i])
        {
            if (m_pUIManager && i < m_aButtonName.size())
            {
                m_pUIManager->RemoveElement("world" + m_aButtonName[i]);
            }
            if (m_vBtWorld[i]->GetLayer())
            {
                m_pGr->RemoveLayer(m_vBtWorld[i]->GetLayer());
            }
        }
    }
    m_vBtWorld.clear();
    m_aButtonName.clear();

    // Clean up world state layers
    for (auto& layer : m_apLayerWorldState)
    {
        if (layer)
        {
            m_pGr->RemoveLayer(layer);
        }
    }
    m_apLayerWorldState.clear();

    // Clean up balloon layers
    for (auto& layer : m_apLayerBalloon)
    {
        if (layer)
        {
            m_pGr->RemoveLayer(layer);
        }
    }
    m_apLayerBalloon.clear();

    // Clean up background layer
    if (m_pLayerBg)
    {
        m_pGr->RemoveLayer(m_pLayerBg);
        m_pLayerBg.reset();
    }

    m_pLogin = nullptr;
    m_pGr = nullptr;
    m_pUIManager = nullptr;
    m_pWorldSelectProp.reset();

    LOG_DEBUG("UIWorldSelect destroyed");
}

void UIWorldSelect::Update()
{
    // Update world buttons
    for (auto& btn : m_vBtWorld)
    {
        if (btn)
        {
            btn->Update();
        }
    }

    if (m_pBtnGoWorld)
    {
        m_pBtnGoWorld->Update();
    }

    // Update channel select if a world is selected
    if (m_nKeyFocus >= 0)
    {
        UIChannelSelect::GetInstance().Update();
    }
}

void UIWorldSelect::Draw()
{
    // Drawing is handled by layers
}

void UIWorldSelect::OnMouseMove(std::int32_t x, std::int32_t y)
{
    for (auto& btn : m_vBtWorld)
    {
        if (btn)
        {
            btn->OnMouseMove(x, y);
        }
    }

    if (m_pBtnGoWorld)
    {
        m_pBtnGoWorld->OnMouseMove(x, y);
    }

    // Forward to channel select if active
    if (m_nKeyFocus >= 0)
    {
        UIChannelSelect::GetInstance().OnMouseMove(x, y);
    }
}

void UIWorldSelect::OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button)
{
    for (auto& btn : m_vBtWorld)
    {
        if (btn)
        {
            btn->OnMouseDown(x, y, button);
        }
    }

    if (m_pBtnGoWorld)
    {
        m_pBtnGoWorld->OnMouseDown(x, y, button);
    }

    // Forward to channel select if active
    if (m_nKeyFocus >= 0)
    {
        UIChannelSelect::GetInstance().OnMouseDown(x, y, button);
    }
}

void UIWorldSelect::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button)
{
    for (auto& btn : m_vBtWorld)
    {
        if (btn)
        {
            btn->OnMouseUp(x, y, button);
        }
    }

    if (m_pBtnGoWorld)
    {
        m_pBtnGoWorld->OnMouseUp(x, y, button);
    }

    // Forward to channel select if active
    if (m_nKeyFocus >= 0)
    {
        UIChannelSelect::GetInstance().OnMouseUp(x, y, button);
    }
}

void UIWorldSelect::OnKeyDown(std::int32_t keyCode)
{
    // Based on CUIWorldSelect::OnKey (0xbbc450)
    // Handle keyboard navigation for world selection

    if (!m_pLogin)
    {
        return;
    }

    // If channel select is active, forward key events to it
    if (m_nKeyFocus >= 0)
    {
        UIChannelSelect::GetInstance().OnKeyDown(keyCode);
        return;
    }

    const auto& worldItems = m_pLogin->GetWorldItemFinal();
    if (worldItems.empty())
    {
        return;
    }

    const int worldCount = static_cast<int>(worldItems.size());

    switch (keyCode)
    {
    case 0x0D:  // VK_RETURN (Enter)
        // Select currently hovered world
        if (m_nKeyFocus >= 0 && m_nKeyFocus < worldCount)
        {
            OnButtonClicked(static_cast<std::uint32_t>(m_nKeyFocus));
        }
        else if (!m_vBtWorld.empty())
        {
            // If no world focused, select first world
            OnButtonClicked(0);
        }
        break;

    case 0x1B:  // VK_ESCAPE
        // Go back to title screen
        if (m_pLogin->GetLoginStep() == 1)
        {
            LOG_DEBUG("UIWorldSelect: Escape pressed, returning to title");
            m_pLogin->ChangeStep(0);
        }
        break;

    case 0x25:  // VK_LEFT
    case 0x26:  // VK_UP
        // Navigate to previous world
        NavigateWorld(-1);
        break;

    case 0x27:  // VK_RIGHT
    case 0x28:  // VK_DOWN
        // Navigate to next world
        NavigateWorld(1);
        break;

    default:
        break;
    }
}

void UIWorldSelect::NavigateWorld(std::int32_t delta)
{
    const auto& worldItems = m_pLogin->GetWorldItemFinal();
    if (worldItems.empty())
    {
        return;
    }

    const int worldCount = static_cast<int>(worldItems.size());

    // If no world currently focused, start from first/last based on direction
    int currentFocus = m_nKeyFocus;
    if (currentFocus < 0)
    {
        currentFocus = (delta > 0) ? -1 : worldCount;
    }

    int newFocus = currentFocus + delta;

    // Wrap around
    if (newFocus < 0)
    {
        newFocus = worldCount - 1;
    }
    else if (newFocus >= worldCount)
    {
        newFocus = 0;
    }

    if (m_nKeyFocus != newFocus)
    {
        LOG_DEBUG("World navigation: {} -> {} ({})",
                  m_nKeyFocus >= 0 ? worldItems[static_cast<size_t>(m_nKeyFocus)].sName : "none",
                  worldItems[static_cast<size_t>(newFocus)].sName,
                  newFocus);
        m_nKeyFocus = newFocus;

        // Update visual feedback for buttons
        UpdateWorldButtonStates();
    }
}

void UIWorldSelect::UpdateWorldButtonStates()
{
    // Update visual feedback for selected world
    for (size_t i = 0; i < m_vBtWorld.size(); ++i)
    {
        if (m_vBtWorld[i])
        {
            if (static_cast<std::int32_t>(i) == m_nKeyFocus)
            {
                m_vBtWorld[i]->SetState(UIState::Pressed);
            }
            else
            {
                m_vBtWorld[i]->SetState(UIState::Normal);
            }
        }
    }
}

void UIWorldSelect::CreateLayer(WzGr2D& gr, std::int32_t z)
{
    (void)gr;
    (void)z;
    // Layer creation is handled in DrawWorldItems
}

void UIWorldSelect::CreatePlaceholderUI()
{
    if (!m_pGr || !m_pUIManager)
    {
        return;
    }

    LOG_DEBUG("Creating placeholder world select UI");

    // CUIWorldSelect dialog position from IDA: (652, 37) with Origin_LT
    constexpr int kDialogX = 652;
    constexpr int kDialogY = 37;

    const int startX = kDialogX + 10;
    int worldY = kDialogY + 50;

    // Create sample world buttons
    std::vector<std::string> sampleWorlds = {"Scania", "Bera", "Broa", "Windia"};

    for (size_t i = 0; i < sampleWorlds.size(); ++i)
    {
        auto btn = CreateWorldButton(sampleWorlds[i], startX, worldY);
        if (btn)
        {
            const size_t worldIndex = i;
            btn->SetClickCallback([this, worldIndex]() {
                // For placeholder UI, just set focus (no server data)
                m_nKeyFocus = static_cast<std::int32_t>(worldIndex);
                LOG_DEBUG("Placeholder world selected: {}", worldIndex);
            });

            m_pUIManager->AddElement("world" + std::to_string(i), btn);
            m_vBtWorld.push_back(btn);
            m_aButtonName.push_back(std::to_string(i));
        }
        worldY += 35;
    }

    // Create enter button for placeholder UI
    // Position below the world list within the dialog area
    const int btnWidth = 100;
    const int btnHeight = 35;
    const int btnX = kDialogX + 50;  // Relative to dialog
    const int btnY = worldY + 20;    // Below the world buttons

    m_pBtnGoWorld = std::make_shared<UIButton>();

    // Create orange gradient button
    auto canvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
    std::vector<std::uint8_t> pixels(static_cast<size_t>(btnWidth * btnHeight * 4));
    for (int y = 0; y < btnHeight; ++y)
    {
        for (int x = 0; x < btnWidth; ++x)
        {
            auto idx = static_cast<size_t>((y * btnWidth + x) * 4);
            float t = static_cast<float>(y) / static_cast<float>(btnHeight);
            pixels[idx + 0] = static_cast<std::uint8_t>(255 - 80 * t);
            pixels[idx + 1] = static_cast<std::uint8_t>(180 - 60 * t);
            pixels[idx + 2] = static_cast<std::uint8_t>(80 - 30 * t);
            pixels[idx + 3] = 255;
        }
    }
    canvas->SetPixelData(std::move(pixels));
    m_pBtnGoWorld->SetStateCanvas(UIState::Normal, canvas);
    m_pBtnGoWorld->SetSize(btnWidth, btnHeight);
    m_pBtnGoWorld->SetPosition(btnX, btnY);
    m_pBtnGoWorld->CreateLayer(*m_pGr, 150);
    m_pBtnGoWorld->SetClickCallback([this]() {
        if (m_pLogin && m_nKeyFocus >= 0)
        {
            LOG_DEBUG("Entering placeholder world, changing to step 2");
            m_pLogin->ChangeStep(2);
        }
        else
        {
            LOG_DEBUG("No world selected");
        }
    });
    m_pUIManager->AddElement("btnGoWorld", m_pBtnGoWorld);
    LOG_DEBUG("Placeholder enter button created at ({}, {})", btnX, btnY);
}

auto UIWorldSelect::CreateWorldButton(const std::string& name, std::int32_t x, std::int32_t y)
    -> std::shared_ptr<UIButton>
{
    if (!m_pGr)
    {
        return nullptr;
    }

    const int btnWidth = 200;
    const int btnHeight = 30;

    auto btn = std::make_shared<UIButton>();

    // Create gradient button canvas
    auto canvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
    std::vector<std::uint8_t> pixels(static_cast<size_t>(btnWidth * btnHeight * 4));

    for (int py = 0; py < btnHeight; ++py)
    {
        for (int px = 0; px < btnWidth; ++px)
        {
            auto idx = static_cast<size_t>((py * btnWidth + px) * 4);
            float t = static_cast<float>(py) / static_cast<float>(btnHeight);
            // Blue gradient
            pixels[idx + 0] = static_cast<std::uint8_t>(60 + 20 * t);
            pixels[idx + 1] = static_cast<std::uint8_t>(80 + 30 * t);
            pixels[idx + 2] = static_cast<std::uint8_t>(140 + 40 * t);
            pixels[idx + 3] = 230;
        }
    }
    canvas->SetPixelData(std::move(pixels));
    btn->SetStateCanvas(UIState::Normal, canvas);

    // Create hover state
    auto hoverCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
    std::vector<std::uint8_t> hoverPixels(static_cast<size_t>(btnWidth * btnHeight * 4));
    for (int py = 0; py < btnHeight; ++py)
    {
        for (int px = 0; px < btnWidth; ++px)
        {
            auto idx = static_cast<size_t>((py * btnWidth + px) * 4);
            float t = static_cast<float>(py) / static_cast<float>(btnHeight);
            hoverPixels[idx + 0] = static_cast<std::uint8_t>(80 + 20 * t);
            hoverPixels[idx + 1] = static_cast<std::uint8_t>(100 + 30 * t);
            hoverPixels[idx + 2] = static_cast<std::uint8_t>(180 + 40 * t);
            hoverPixels[idx + 3] = 255;
        }
    }
    hoverCanvas->SetPixelData(std::move(hoverPixels));
    btn->SetStateCanvas(UIState::MouseOver, hoverCanvas);

    // Create pressed state
    auto pressedCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
    std::vector<std::uint8_t> pressedPixels(static_cast<size_t>(btnWidth * btnHeight * 4));
    for (int py = 0; py < btnHeight; ++py)
    {
        for (int px = 0; px < btnWidth; ++px)
        {
            auto idx = static_cast<size_t>((py * btnWidth + px) * 4);
            float t = static_cast<float>(py) / static_cast<float>(btnHeight);
            // Brighter when selected
            pressedPixels[idx + 0] = static_cast<std::uint8_t>(100 + 30 * t);
            pressedPixels[idx + 1] = static_cast<std::uint8_t>(140 + 40 * t);
            pressedPixels[idx + 2] = static_cast<std::uint8_t>(220 + 30 * t);
            pressedPixels[idx + 3] = 255;
        }
    }
    pressedCanvas->SetPixelData(std::move(pressedPixels));
    btn->SetStateCanvas(UIState::Pressed, pressedCanvas);

    btn->SetSize(btnWidth, btnHeight);
    btn->SetPosition(x, y);
    btn->CreateLayer(*m_pGr, 150);

    LOG_DEBUG("Created world button '{}' at ({}, {})", name, x, y);

    return btn;
}

} // namespace ms
