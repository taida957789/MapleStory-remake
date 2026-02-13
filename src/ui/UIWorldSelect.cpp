#include "UIWorldSelect.h"
#include "UIButton.h"
#include "UIChannelSelect.h"
#include "UIManager.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "stage/Login.h"
#include "util/Logger.h"
#include "graphics/WzGr2DCanvas.h"
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

auto UIWorldSelect::OnCreate(std::any params) -> Result<void>
{
    // 1. Extract and validate parameters
    auto* createParams = std::any_cast<CreateParams>(&params);
    if (!createParams)
    {
        return Result<void>::Error("Invalid params type for UIWorldSelect");
    }

    if (!createParams->IsValid())
    {
        return Result<void>::Error("UIWorldSelect CreateParams validation failed");
    }

    // 2. Store references
    m_pLogin = createParams->login;
    m_pGr = createParams->gr;
    m_pUIManager = createParams->uiManager;

    // 3. Create LayoutMan and initialize
    m_pLayoutMan = std::make_unique<LayoutMan>();
    auto initResult = m_pLayoutMan->Init(this, 0, 0);
    if (!initResult)
    {
        return Result<void>::Error("Failed to initialize LayoutMan: {}", initResult.error());
    }

    // 4. Build UI from WZ
    std::string sLayoutUOL = "UI/Login.img/WorldSelect/BtWorld/release";
    auto buildResult = m_pLayoutMan->AutoBuild(sLayoutUOL);
    if (!buildResult)
    {
        return Result<void>::Error("Failed to build UI from WZ: {}", buildResult.error());
    }
    m_pLayoutMan->CreateLayers(*m_pGr, 140, true);

    // 5. Create background layer
    if (m_pGr)
    {
        m_pLayerBg = m_pGr->CreateLayer(652, 37,
                                        m_pGr->GetWidth(), m_pGr->GetHeight(),
                                        10);
        if (!m_pLayerBg)
        {
            return Result<void>::Error("Failed to create background layer");
        }
        m_pLayerBg->SetVisible(true);
    }

    // 6. Initialize world buttons
    InitWorldButtons();

    return Result<void>::Success();
}

void UIWorldSelect::OnDestroy() noexcept
{
    try
    {
        // 1. Clear world buttons (shared_ptr RAII)
        m_vBtWorld.clear();
        m_pBtnGoWorld.reset();

        // 2. Clear world state layers
        m_apLayerWorldState.clear();

        // 3. Clear balloon layers
        m_apLayerBalloon.clear();

        // 4. Clear background layer
        if (m_pLayerBg && m_pGr)
        {
            m_pGr->RemoveLayer(m_pLayerBg);
        }
        m_pLayerBg.reset();

        // 5. Clean up UIChannelSelect
        if (m_channelSelectUI)
        {
            if (m_channelSelectUI->IsCreated())
            {
                m_channelSelectUI->Destroy();
            }
            m_channelSelectUI.reset();
        }

        // 6. Clear LayoutMan
        m_pLayoutMan.reset();

        // 7. Clear cached WZ property
        m_pWorldSelectProp.reset();

        // 8. Clear references (non-owning)
        m_pLogin = nullptr;
        m_pGr = nullptr;
        m_pUIManager = nullptr;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in UIWorldSelect::OnDestroy: {}", e.what());
    }
}

void UIWorldSelect::SetRet(std::int32_t ret)
{
    m_nRet = ret;
}

void UIWorldSelect::InitWorldButtons()
{
    // 這個版本不接受參數，用於重置
    InitWorldButtons(10, nullptr);
}

void UIWorldSelect::InitWorldButtons(int nDisplayCount, std::shared_ptr<WzProperty> layoutProp)
{
    if (!m_pLogin || !m_pLayoutMan)
    {
        LOG_WARN("UIWorldSelect::InitWorldButtons - missing references");
        return;
    }

    // 清空現有按鈕
    for (auto& btn : m_vBtWorld)
    {
        if (btn && btn->GetLayer())
        {
            m_pGr->RemoveLayer(btn->GetLayer());
        }
    }
    m_vBtWorld.clear();

    // 清空按鈕名稱陣列
    m_aButtonName.clear();

    const auto& worldItems = m_pLogin->GetWorldItems();

    // 讀取 origin 配置（按鈕位置）
    std::shared_ptr<WzProperty> originsProp;
    if (layoutProp)
    {
        originsProp = layoutProp->GetChild("origin");
        if (!originsProp)
        {
            LOG_WARN("UIWorldSelect::InitWorldButtons - origin property not found");
        }
    }

    // 從 LayoutMan 獲取 AutoBuild 創建的按鈕並設置位置
    for (int displayIndex = 0; displayIndex < nDisplayCount; ++displayIndex)
    {
        // 按鈕名稱是 "0", "1", "2" 等（由 AutoBuild 從 WZ "button:0" 創建）
        auto btn = m_pLayoutMan->ABGetButton(std::to_wstring(displayIndex));
        if (!btn)
        {
            LOG_DEBUG("UIWorldSelect: Button {} not found in LayoutMan", displayIndex);
            continue;
        }

        // 讀取按鈕位置 (origin/{displayIndex})
        if (originsProp)
        {
            auto posVecProp = originsProp->GetChild(std::to_string(displayIndex));
            if (posVecProp)
            {
                auto posVec = posVecProp->GetVector();
                btn->SetPosition(posVec.x, posVec.y);
                LOG_DEBUG("UIWorldSelect: Set button {} position to ({}, {})",
                          displayIndex, posVec.x, posVec.y);
            }
        }

        // 讀取這個顯示位置對應的 worldID
        int worldID = -1;
        if (layoutProp)
        {
            auto worldIdProp = layoutProp->GetChild(std::to_string(displayIndex));
            if (worldIdProp)
            {
                worldID = worldIdProp->GetInt(-1);
            }
        }

        // 檢查 worldItems 中是否有這個 worldID
        bool hasWorld = false;
        if (worldID >= 0)
        {
            for (const auto& world : worldItems)
            {
                if (world.nWorldID == worldID)
                {
                    hasWorld = true;
                    break;
                }
            }
        }

        // 設置按鈕屬性
        btn->SetID(static_cast<std::uint32_t>(worldID >= 0 ? worldID : displayIndex));
        btn->SetVisible(hasWorld);

        // 設置點擊回調
        btn->SetClickCallback([this, displayIndex, worldID]() {
            OnButtonClicked(static_cast<std::uint32_t>(worldID >= 0 ? worldID : displayIndex));
        });

        m_vBtWorld.push_back(btn);
        LOG_DEBUG("UIWorldSelect: Initialized button {} (worldID={}, visible={})",
                  displayIndex, worldID, hasWorld);
    }

    // 設置初始焦點
    if (!m_vBtWorld.empty())
    {
        m_nKeyFocus = 0;
        LOG_DEBUG("UIWorldSelect: Set initial key focus to 0");
    }

    LOG_DEBUG("UIWorldSelect::InitWorldButtons completed - {} buttons", m_vBtWorld.size());
}

void UIWorldSelect::DrawWorldItems()
{
    // Based on CUIWorldSelect::DrawWorldItems (0xbc4b00)
    // Note: 按鈕創建已在 LoadWorldButtons 中完成
    if (!m_pLogin || !m_pGr || !m_pUIManager)
    {
        LOG_WARN("UIWorldSelect::DrawWorldItems - missing references");
        return;
    }

    // 1. 處理 Balloon (氣球消息)
    m_nBalloonCount = m_pLogin->GetBalloonCount();
    if (m_nBalloonCount > 0)
    {
        // TODO: 實作 balloon 創建
        // for (int i = 0; i < m_nBalloonCount; ++i)
        // {
        //     auto& balloon = balloons[i];
        //     MakeWSBalloon(balloon.sMessage, balloon.nX, balloon.nY);
        // }
        LOG_DEBUG("UIWorldSelect::DrawWorldItems - {} balloons (not yet implemented)", m_nBalloonCount);
    }

    // 2. 獲取世界數量
    const auto& worldItems = m_pLogin->GetWorldItemFinal();
    m_nWorld = static_cast<std::int32_t>(worldItems.size());
    LOG_DEBUG("UIWorldSelect::DrawWorldItems - {} worlds", m_nWorld);

    // 3. TODO: 處理世界狀態圖標 (worldState icons)
    // 這些會在 LoadWorldButtons 之後根據按鈕位置創建

    LOG_DEBUG("UIWorldSelect::DrawWorldItems - completed");
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

    // Create UIChannelSelect and initialize with new lifecycle pattern
    m_channelSelectUI = std::make_unique<UIChannelSelect>();

    // Create params
    UIChannelSelect::CreateParams params;
    params.login = m_pLogin;
    params.gr = m_pGr;
    params.uiManager = m_pUIManager;
    params.worldIndex = worldIndex;

    // Call Create() and check result
    auto result = m_channelSelectUI->Create(params);
    if (!result)
    {
        LOG_ERROR("Failed to create UIChannelSelect: {}", result.error());
        m_channelSelectUI.reset();
        return;
    }

    LOG_DEBUG("UIChannelSelect created successfully for world {}", worldIndex);
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
    std::shared_ptr<WzGr2DCanvas> balloonCanvas;

    if (m_pWorldSelectProp)
    {
        auto balloonProp = m_pWorldSelectProp->GetChild("balloon");
        if (balloonProp)
        {
            auto tmpWzCanvas_391 = balloonProp->GetCanvas();
            balloonCanvas = tmpWzCanvas_391 ? std::make_shared<WzGr2DCanvas>(tmpWzCanvas_391) : nullptr;
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
    // Call base class Destroy which calls OnDestroy
    UIElement::Destroy();
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

    // Update channel select if created
    if (m_channelSelectUI && m_channelSelectUI->IsCreated())
    {
        m_channelSelectUI->Update();
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

    // Forward to channel select if created
    if (m_channelSelectUI && m_channelSelectUI->IsCreated())
    {
        m_channelSelectUI->OnMouseMove(x, y);
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

    // Forward to channel select if created
    if (m_channelSelectUI && m_channelSelectUI->IsCreated())
    {
        m_channelSelectUI->OnMouseDown(x, y, button);
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

    // Forward to channel select if created
    if (m_channelSelectUI && m_channelSelectUI->IsCreated())
    {
        m_channelSelectUI->OnMouseUp(x, y, button);
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

    // If channel select is created, forward key events to it
    if (m_channelSelectUI && m_channelSelectUI->IsCreated())
    {
        m_channelSelectUI->OnKeyDown(keyCode);
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
    auto wzCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
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
    wzCanvas->SetPixelData(std::move(pixels));
    auto canvas = std::make_shared<WzGr2DCanvas>(wzCanvas);
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
    auto wzCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
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
    wzCanvas->SetPixelData(std::move(pixels));
    auto canvas = std::make_shared<WzGr2DCanvas>(wzCanvas);
    btn->SetStateCanvas(UIState::Normal, canvas);

    // Create hover state
    auto wzHoverCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
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
    wzHoverCanvas->SetPixelData(std::move(hoverPixels));
    auto hoverCanvas = std::make_shared<WzGr2DCanvas>(wzHoverCanvas);
    btn->SetStateCanvas(UIState::MouseOver, hoverCanvas);

    // Create pressed state
    auto wzPressedCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
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
    wzPressedCanvas->SetPixelData(std::move(pressedPixels));
    auto pressedCanvas = std::make_shared<WzGr2DCanvas>(wzPressedCanvas);
    btn->SetStateCanvas(UIState::Pressed, pressedCanvas);

    btn->SetSize(btnWidth, btnHeight);
    btn->SetPosition(x, y);
    btn->CreateLayer(*m_pGr, 150);

    LOG_DEBUG("Created world button '{}' at ({}, {})", name, x, y);

    return btn;
}

} // namespace ms
