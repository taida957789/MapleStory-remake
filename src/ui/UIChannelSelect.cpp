#include "UIChannelSelect.h"
#include "UIButton.h"
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

UIChannelSelect::UIChannelSelect() = default;

UIChannelSelect::~UIChannelSelect()
{
    Destroy();
}

auto UIChannelSelect::OnCreate(std::any params) -> Result<void>
{
    // 1. Extract and validate parameters
    auto* createParams = std::any_cast<CreateParams>(&params);
    if (!createParams)
    {
        return Result<void>::Error("Invalid params type for UIChannelSelect");
    }

    if (!createParams->IsValid())
    {
        return Result<void>::Error("UIChannelSelect CreateParams validation failed");
    }

    // 2. Store references
    m_pLogin = createParams->login;
    m_pGr = createParams->gr;
    m_pUIManager = createParams->uiManager;
    m_nWorldIndex = createParams->worldIndex;
    m_bSelectWorld = false;
    m_nSelect = 0;

    // 3. Set UIChannelSelect position
    // CUIChannelSelect dialog position from IDA: (203, 194) with Origin_LT
    constexpr int kDialogX = 203;
    constexpr int kDialogY = 194;
    SetPosition(kDialogX, kDialogY);

    // 4. Load WorldSelect WZ properties
    // Based on docs: Base UOL is UI/Login.img/WorldSelect/BtChannel/test
    auto& resMan = WzResMan::GetInstance();
    auto loginImgProp = resMan.GetProperty("UI/Login.img");
    if (loginImgProp)
    {
        auto worldSelectProp = loginImgProp->GetChild("WorldSelect");
        if (worldSelectProp)
        {
            // Load base channel select property (CLayoutMan AutoBuild path)
            // Original: UI/Login.img/WorldSelect/BtChannel/test
            auto btChannelProp = worldSelectProp->GetChild("BtChannel");
            if (btChannelProp)
            {
                m_pChannelSelectProp = btChannelProp->GetChild("release");
                if (m_pChannelSelectProp)
                {
                    LOG_DEBUG("UIChannelSelect: BtChannel/release property loaded");
                }
            }

            // Load chBackgrn (channel background)
            // From docs: UI/Login.img/WorldSelect/BtChannel/layer:bg
            auto chBackgrnProp = btChannelProp ? btChannelProp->GetChild("layer:bg") : nullptr;
            if (chBackgrnProp)
            {
                auto wzCanvas = chBackgrnProp->GetCanvas();
                auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas) : nullptr;
                if (canvas)
                {
                    LOG_DEBUG("UIChannelSelect: chBackgrn canvas loaded ({}x{})",
                              canvas->GetWidth(), canvas->GetHeight());

                    // For dialog backgrounds (Origin_LT style):
                    // - kDialogX/kDialogY is the desired TOP-LEFT position
                    // - render formula: renderPos = layerPos - origin
                    // - To get renderPos = dialogPos, we need: layerPos = dialogPos + origin
                    auto origin = canvas->GetOrigin();
                    int layerX = kDialogX + origin.x;
                    int layerY = kDialogY + origin.y;

                    m_pLayerBg = m_pGr->CreateLayer(layerX, layerY,
                                                 static_cast<std::uint32_t>(canvas->GetWidth()),
                                                 static_cast<std::uint32_t>(canvas->GetHeight()), 140);
                    if (m_pLayerBg)
                    {
                        m_pLayerBg->InsertCanvas(canvas, 0, 255, 255);
                        LOG_DEBUG("UIChannelSelect: chBackgrn layer at ({}, {}), renders at ({}, {})",
                                  layerX, layerY, kDialogX, kDialogY);
                    }
                }
                else
                {
                    LOG_DEBUG("UIChannelSelect: chBackgrn has no canvas, trying child '0'");
                    // Some WZ properties store canvas in child "0"
                    auto child0 = chBackgrnProp->GetChild("0");
                    if (child0)
                    {
                        auto tmpWzCanvas = child0->GetCanvas();
                        canvas = tmpWzCanvas ? std::make_shared<WzGr2DCanvas>(tmpWzCanvas) : nullptr;
                        if (canvas)
                        {
                            // Origin_LT style: layerPos = targetTopLeft + origin
                            auto origin = canvas->GetOrigin();
                            int layerX = kDialogX + origin.x;
                            int layerY = kDialogY + origin.y;

                            m_pLayerBg = m_pGr->CreateLayer(layerX, layerY,
                                                         static_cast<std::uint32_t>(canvas->GetWidth()),
                                                         static_cast<std::uint32_t>(canvas->GetHeight()), 140);
                            if (m_pLayerBg)
                            {
                                m_pLayerBg->InsertCanvas(canvas, 0, 255, 255);
                                LOG_DEBUG("UIChannelSelect: chBackgrn/0 layer at ({}, {})", layerX, layerY);
                            }
                        }
                    }
                }
            }
            else
            {
                LOG_DEBUG("UIChannelSelect: chBackgrn property not found");
            }
        }
    }

    // 5. Create placeholder background if WZ chBackgrn not loaded
    if (!m_pLayerBg)
    {
        CreatePlaceholderBackground(*m_pGr, kDialogX, kDialogY);
    }

    // 6. Reset info for the selected world
    ResetInfo(m_nWorldIndex, false);

    LOG_DEBUG("UIChannelSelect::OnCreate completed for world {}", m_nWorldIndex);
    return Result<void>::Success();
}

void UIChannelSelect::ResetInfo(std::int32_t worldIndex, bool bRedraw)
{
    // Based on CUIChannelSelect::ResetInfo (0xbc3150)
    if (!m_pLogin || !m_pGr || !m_pUIManager)
    {
        LOG_WARN("UIChannelSelect::ResetInfo - missing references");
        return;
    }

    m_nWorldIndex = worldIndex;

    const auto& worldItems = m_pLogin->GetWorldItemFinal();
    if (worldIndex < 0 || worldIndex >= static_cast<std::int32_t>(worldItems.size()))
    {
        LOG_WARN("UIChannelSelect::ResetInfo - invalid world index {}", worldIndex);
        CreatePlaceholderUI();
        return;
    }

    const auto& world = worldItems[static_cast<size_t>(worldIndex)];

    LOG_DEBUG("UIChannelSelect::ResetInfo - world {} ({}) with {} channels",
              world.sName, world.nWorldID, world.aChannelLoad.size());

    // 創建 LayoutMan
    if (!m_pLayoutMan)
    {
        m_pLayoutMan = std::make_unique<LayoutMan>();
        auto initResult = m_pLayoutMan->Init(this, 0, 0);
        if (!initResult)
        {
            LOG_ERROR("UIChannelSelect: Failed to initialize LayoutMan: {}", initResult.error());
            CreatePlaceholderUI();
            return;
        }

        // 基於 IDA 分析 (CUIChannelSelect::OnCreate @ 0xbc4780)
        // 原始實作呼叫 AutoBuild 兩次:

        // 第一次 AutoBuild: 使用 base UOL (0xbc4823)
        std::string sBaseUOL = "UI/Login.img/WorldSelect/BtChannel/test";
        auto buildResult1 = m_pLayoutMan->AutoBuild(sBaseUOL);
        if (!buildResult1)
        {
            LOG_ERROR("UIChannelSelect: First AutoBuild failed: {}", buildResult1.error());
        }
        else
        {
            LOG_DEBUG("UIChannelSelect: First AutoBuild completed (base UOL)");
        }

        // 第二次 AutoBuild: 使用 base UOL + "/test" (0xbc48dc)
        // 原始碼: ZXString<char>::operator+(&this->m_sBaseUOL, &sWorldUOL, "/test");
        std::string sWorldUOL = "UI/Login.img/WorldSelect/BtChannel/test/test";
        auto buildResult2 = m_pLayoutMan->AutoBuild(sWorldUOL);
        if (!buildResult2)
        {
            LOG_ERROR("UIChannelSelect: Second AutoBuild failed: {}", buildResult2.error());
        }
        else
        {
            LOG_DEBUG("UIChannelSelect: Second AutoBuild completed (base UOL + /test)");
        }

        // 測試查找按鈕
        auto pGoWorldBtn = m_pLayoutMan->ABGetButton(L"GoWorld");
        if (pGoWorldBtn)
        {
            LOG_INFO("Found GoWorld button via LayoutMan");
        }
        else
        {
            LOG_INFO("GoWorld button not found via LayoutMan (may need to be created manually)");
        }
    }

    // Clear existing channel buttons
    for (size_t i = 0; i < m_vBtChannel.size(); ++i)
    {
        if (m_vBtChannel[i])
        {
            m_pUIManager->RemoveElement("channel" + std::to_string(i));
            if (m_vBtChannel[i]->GetLayer())
            {
                m_pGr->RemoveLayer(m_vBtChannel[i]->GetLayer());
            }
        }
    }
    m_vBtChannel.clear();

    // Clear GoWorld button
    if (m_pBtnGoWorld)
    {
        m_pUIManager->RemoveElement("btnGoWorld_channel");
        if (m_pBtnGoWorld->GetLayer())
        {
            m_pGr->RemoveLayer(m_pBtnGoWorld->GetLayer());
        }
        m_pBtnGoWorld.reset();
    }

    // Channel buttons are now created by LayoutMan::AutoBuild

    // Load gauge canvas for channel load indicator
    // From docs: UI/Login.img/WorldSelect/BtChannel/test/gauge
    if (m_pChannelSelectProp && !m_pCanvasGauge)
    {
        auto chgaugeProp = m_pChannelSelectProp->GetChild("gauge");
        if (chgaugeProp)
        {
            auto wzCanvas_260 = chgaugeProp->GetCanvas();
            m_pCanvasGauge = wzCanvas_260 ? std::make_shared<WzGr2DCanvas>(wzCanvas_260) : nullptr;
            if (m_pCanvasGauge)
            {
                LOG_DEBUG("UIChannelSelect: chgauge canvas loaded ({}x{})",
                          m_pCanvasGauge->GetWidth(), m_pCanvasGauge->GetHeight());
            }
            else
            {
                // Try child "0"
                auto child0 = chgaugeProp->GetChild("0");
                if (child0)
                {
                    auto wzCanvas_272 = child0->GetCanvas();
                    m_pCanvasGauge = wzCanvas_272 ? std::make_shared<WzGr2DCanvas>(wzCanvas_272) : nullptr;
                    if (m_pCanvasGauge)
                    {
                        LOG_DEBUG("UIChannelSelect: chgauge/0 canvas loaded");
                    }
                }
            }
        }
    }

    // Load selection indicator (if exists in WZ)
    // Note: Original uses CLayoutMan AutoBuild, may not have separate chSelect
    std::shared_ptr<WzGr2DCanvas> chSelectCanvas;
    if (m_pChannelSelectProp)
    {
        auto chSelectProp = m_pChannelSelectProp->GetChild("chSelect");
        if (!chSelectProp)
        {
            // Try test sub-path
            auto testProp = m_pChannelSelectProp->GetChild("test");
            if (testProp)
            {
                chSelectProp = testProp->GetChild("chSelect");
            }
        }
        if (chSelectProp)
        {
            auto tmpWzCanvas_301 = chSelectProp->GetCanvas();
            chSelectCanvas = tmpWzCanvas_301 ? std::make_shared<WzGr2DCanvas>(tmpWzCanvas_301) : nullptr;
            if (chSelectCanvas)
            {
                LOG_DEBUG("UIChannelSelect: chSelect canvas loaded ({}x{})",
                          chSelectCanvas->GetWidth(), chSelectCanvas->GetHeight());
            }
            else
            {
                // Try child "0"
                auto child0 = chSelectProp->GetChild("0");
                if (child0)
                {
                    auto tmpWzCanvas_313 = child0->GetCanvas();
                    chSelectCanvas = tmpWzCanvas_313 ? std::make_shared<WzGr2DCanvas>(tmpWzCanvas_313) : nullptr;
                    if (chSelectCanvas)
                    {
                        LOG_DEBUG("UIChannelSelect: chSelect/0 canvas loaded");
                    }
                }
            }
        }
    }

    // 從 LayoutMan 獲取按鈕並設置回調
    // AutoBuild 已經創建了按鈕，現在設置它們的點擊回調
    bool bSelected = false;
    for (size_t i = 0; i < world.aChannelLoad.size(); ++i)
    {
        const int load = world.aChannelLoad[i];

        // 從 LayoutMan 獲取按鈕（按鈕名稱是 "0", "1", "2" 等）
        if (m_pLayoutMan)
        {
            auto btn = m_pLayoutMan->ABGetButton(std::to_wstring(i));
            if (btn)
            {
                const size_t channelIndex = i;
                btn->SetClickCallback([this, channelIndex]() {
                    OnButtonClicked(static_cast<std::uint32_t>(channelIndex));
                });
                m_vBtChannel.push_back(btn);
                LOG_DEBUG("UIChannelSelect: Set callback for channel {} button", i);
            }
            else
            {
                LOG_WARN("UIChannelSelect: Channel {} button not found in LayoutMan", i);
            }
        }

        // Auto-select first channel with load < 73% (from original)
        if (!bSelected && load < 73)
        {
            m_nSelect = static_cast<std::int32_t>(i);
            bSelected = true;
            LOG_DEBUG("Auto-selected channel {} with load {}%", i + 1, load);
        }
    }

    // If no channel was auto-selected, select channel 0
    if (!bSelected && !world.aChannelLoad.empty())
    {
        m_nSelect = 0;
        LOG_DEBUG("Default selected channel 1");
    }

    // 從 LayoutMan 獲取 GoWorld 按鈕
    // AutoBuild 已經從 WZ 創建了 GoWorld 按鈕
    if (m_pLayoutMan)
    {
        m_pBtnGoWorld = m_pLayoutMan->ABGetButton(L"GoWorld");
        if (m_pBtnGoWorld)
        {
            m_pBtnGoWorld->SetClickCallback([this]() {
                EnterChannel();
            });
            m_pUIManager->AddElement("btnGoWorld_channel", m_pBtnGoWorld);
            LOG_DEBUG("UIChannelSelect: GoWorld button retrieved from LayoutMan");
        }
        else
        {
            LOG_WARN("UIChannelSelect: GoWorld button not found in LayoutMan");
        }
    }

    if (bRedraw)
    {
        // Force redraw - layers will handle this
    }
}

void UIChannelSelect::EnterChannel()
{
    // Based on CUIChannelSelect::EnterChannel (0xbbb950)
    if (!m_pLogin)
    {
        return;
    }

    if (!IsRequestValid())
    {
        LOG_DEBUG("UIChannelSelect::EnterChannel - request not valid");
        return;
    }

    m_bSelectWorld = true;

    const auto& worldItems = m_pLogin->GetWorldItemFinal();
    if (m_nWorldIndex >= 0 && m_nWorldIndex < static_cast<std::int32_t>(worldItems.size()))
    {
        const auto& world = worldItems[static_cast<size_t>(m_nWorldIndex)];
        LOG_DEBUG("Entering world {} channel {}", world.sName, m_nSelect + 1);
    }

    // Transition to character selection
    m_pLogin->ChangeStep(2);
}

void UIChannelSelect::OnButtonClicked(std::uint32_t nId)
{
    // Based on CUIChannelSelect::OnButtonClicked (0xbbc880)
    if (!m_pLogin)
    {
        return;
    }

    // Check if request already sent
    if (m_pLogin->IsRequestSent() || m_pLogin->GetLoginStep() != 1)
    {
        return;
    }

    // If clicking the same channel, enter it
    if (nId == static_cast<std::uint32_t>(m_nSelect))
    {
        m_bSelectWorld = true;
        DrawNoticeConnecting();
        EnterChannel();
        return;
    }

    // Otherwise, just select the channel
    LOG_DEBUG("Channel {} selected", nId + 1);
    m_nSelect = static_cast<std::int32_t>(nId);

    // Update visual feedback for selected channel
    // In original, this plays UI sound (StringPool 0x946) and updates button focus
    UpdateChannelButtonStates();
}

auto UIChannelSelect::OnSetFocus(bool bFocus) -> bool
{
    // Based on CUIChannelSelect::OnSetFocus (0xbbb8d0)
    return UIElement::OnSetFocus(bFocus);
}

auto UIChannelSelect::IsRequestValid() const -> bool
{
    // Based on CUIChannelSelect::IsRequestValid (0xbbb9b0)
    if (!m_pLogin)
    {
        return false;
    }
    return !m_pLogin->IsRequestSent();
}

void UIChannelSelect::DrawNoticeConnecting()
{
    // Based on CUIChannelSelect::DrawNoticeConnecting (0xbbbe50)
    // In original: Creates CConnectionNoticeDlg and shows it as modal dialog
    // For now, just log the connection attempt
    LOG_INFO("Connecting to channel {}...", m_nSelect + 1);

    // Disable all buttons during connection
    for (auto& btn : m_vBtChannel)
    {
        if (btn)
        {
            btn->SetEnabled(false);
        }
    }
    if (m_pBtnGoWorld)
    {
        m_pBtnGoWorld->SetEnabled(false);
    }
}

void UIChannelSelect::RemoveNoticeConnecting()
{
    // Based on CUIChannelSelect::RemoveNoticeConnecting (0xbbbc50)
    // Re-enable buttons after connection attempt
    for (auto& btn : m_vBtChannel)
    {
        if (btn)
        {
            btn->SetEnabled(true);
        }
    }
    if (m_pBtnGoWorld)
    {
        m_pBtnGoWorld->SetEnabled(true);
    }
}

void UIChannelSelect::OnDestroy() noexcept
{
    try
    {
        // 1. Clear GoWorld button (shared_ptr RAII)
        if (m_pBtnGoWorld && m_pUIManager)
        {
            m_pUIManager->RemoveElement("btnGoWorld_channel");
        }
        m_pBtnGoWorld.reset();

        // 2. Clear channel buttons
        for (size_t i = 0; i < m_vBtChannel.size(); ++i)
        {
            if (m_vBtChannel[i] && m_pUIManager)
            {
                m_pUIManager->RemoveElement("channel" + std::to_string(i));
            }
        }
        m_vBtChannel.clear();

        // 3. Clear layers
        if (m_pLayerBg && m_pGr)
        {
            m_pGr->RemoveLayer(m_pLayerBg);
        }
        m_pLayerBg.reset();

        if (m_pLayerGauge && m_pGr)
        {
            m_pGr->RemoveLayer(m_pLayerGauge);
        }
        m_pLayerGauge.reset();

        if (m_pLayerEventDesc && m_pGr)
        {
            m_pGr->RemoveLayer(m_pLayerEventDesc);
        }
        m_pLayerEventDesc.reset();

        // 4. Clear LayoutMan
        m_pLayoutMan.reset();

        // 5. Clear cached WZ property
        m_pChannelSelectProp.reset();
        m_pCanvasGauge.reset();

        // 6. Clear references (non-owning)
        m_pLogin = nullptr;
        m_pGr = nullptr;
        m_pUIManager = nullptr;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in UIChannelSelect::OnDestroy: {}", e.what());
    }
}

void UIChannelSelect::Destroy()
{
    UIElement::Destroy();
}

void UIChannelSelect::Update()
{
    // Update channel buttons
    for (auto& btn : m_vBtChannel)
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
}

void UIChannelSelect::Draw()
{
    // Drawing is handled by layers
}

void UIChannelSelect::OnMouseMove(std::int32_t x, std::int32_t y)
{
    for (auto& btn : m_vBtChannel)
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
}

void UIChannelSelect::OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button)
{
    for (auto& btn : m_vBtChannel)
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
}

void UIChannelSelect::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button)
{
    for (auto& btn : m_vBtChannel)
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
}

void UIChannelSelect::OnKeyDown(std::int32_t keyCode)
{
    // Based on CUIChannelSelect::OnKey (0xbbbd10)
    // Navigation uses 5-channel grid pattern

    if (!m_pLogin)
    {
        return;
    }

    constexpr int kChannelsPerRow = 5;

    switch (keyCode)
    {
    case 0x09:  // VK_TAB
        // Shift+Tab goes left, Tab goes right
        // For now, treat Tab as right navigation
        NavigateChannel(1);
        break;

    case 0x0D:  // VK_RETURN (Enter)
        // Enter selected channel
        OnButtonClicked(static_cast<std::uint32_t>(m_nSelect));
        break;

    case 0x1B:  // VK_ESCAPE
        // Close channel select and return to world select
        if (m_pLogin->GetLoginStep() == 1)
        {
            LOG_DEBUG("UIChannelSelect: Escape pressed, closing channel select");
            Destroy();
        }
        break;

    case 0x25:  // VK_LEFT
        NavigateChannel(-1);
        break;

    case 0x26:  // VK_UP
        NavigateChannel(-kChannelsPerRow);
        break;

    case 0x27:  // VK_RIGHT
        NavigateChannel(1);
        break;

    case 0x28:  // VK_DOWN
        NavigateChannel(kChannelsPerRow);
        break;

    default:
        break;
    }
}

void UIChannelSelect::NavigateChannel(std::int32_t delta)
{
    // Based on CUIChannelSelect::OnKey navigation logic
    const auto& worldItems = m_pLogin->GetWorldItemFinal();
    if (m_nWorldIndex < 0 || m_nWorldIndex >= static_cast<std::int32_t>(worldItems.size()))
    {
        return;
    }

    const auto& world = worldItems[static_cast<size_t>(m_nWorldIndex)];
    const int channelCount = static_cast<int>(world.aChannelLoad.size());
    if (channelCount == 0)
    {
        return;
    }

    int newSelect = m_nSelect + delta;

    // Wrap around
    if (newSelect < 0)
    {
        newSelect = newSelect + channelCount;
    }
    else if (newSelect >= channelCount)
    {
        newSelect = newSelect - channelCount;
    }

    if (m_nSelect != newSelect)
    {
        LOG_DEBUG("Channel navigation: {} -> {}", m_nSelect + 1, newSelect + 1);
        m_nSelect = newSelect;

        // Update visual feedback for buttons
        UpdateChannelButtonStates();
    }
}

void UIChannelSelect::UpdateChannelButtonStates()
{
    // Update visual feedback for selected channel
    for (size_t i = 0; i < m_vBtChannel.size(); ++i)
    {
        if (m_vBtChannel[i])
        {
            if (static_cast<std::int32_t>(i) == m_nSelect)
            {
                m_vBtChannel[i]->SetState(UIState::Pressed);
            }
            else
            {
                m_vBtChannel[i]->SetState(UIState::Normal);
            }
        }
    }
}

void UIChannelSelect::CreateLayer(WzGr2D& gr, std::int32_t z)
{
    (void)gr;
    (void)z;
    // Layer creation is handled in ResetInfo
}

void UIChannelSelect::CreatePlaceholderUI()
{
    if (!m_pGr || !m_pUIManager)
    {
        return;
    }

    LOG_DEBUG("Creating placeholder channel select UI");

    // CUIChannelSelect dialog position from IDA: (203, 194) with Origin_LT
    constexpr int kDialogX = 203;
    constexpr int kDialogY = 194;

    // Set UIChannelSelect position
    SetPosition(kDialogX, kDialogY);

    // TODO: Placeholder UI - 應該由 LayoutMan::AutoBuild 處理
    // Select first channel
    m_nSelect = 0;

    // Create GoWorld button
    {
        const int goBtnWidth = 100;
        const int goBtnHeight = 35;
        const int goBtnX = 50;  // Relative to dialog

        m_pBtnGoWorld = std::make_shared<UIButton>();

        auto canvas = std::make_shared<WzCanvas>(goBtnWidth, goBtnHeight);
        std::vector<std::uint8_t> pixels(static_cast<size_t>(goBtnWidth * goBtnHeight * 4));
        for (int y = 0; y < goBtnHeight; ++y)
        {
            for (int x = 0; x < goBtnWidth; ++x)
            {
                auto idx = static_cast<size_t>((y * goBtnWidth + x) * 4);
                float t = static_cast<float>(y) / static_cast<float>(goBtnHeight);
                pixels[idx + 0] = static_cast<std::uint8_t>(60 + 20 * t);
                pixels[idx + 1] = static_cast<std::uint8_t>(180 - 60 * t);
                pixels[idx + 2] = static_cast<std::uint8_t>(80 + 20 * t);
                pixels[idx + 3] = 255;
            }
        }
        canvas->SetPixelData(std::move(pixels));
        m_pBtnGoWorld->SetStateCanvas(UIState::Normal, std::make_shared<WzGr2DCanvas>(canvas));
        m_pBtnGoWorld->SetSize(goBtnWidth, goBtnHeight);
        m_pBtnGoWorld->SetParent(this);  // Set parent-child relationship
        const int goWorldY = 180;  // Placeholder position
        m_pBtnGoWorld->SetPosition(goBtnX, goWorldY);
        m_pBtnGoWorld->CreateLayer(*m_pGr, 160);
        m_pBtnGoWorld->SetClickCallback([this]() {
            if (m_pLogin)
            {
                LOG_DEBUG("Entering placeholder world, channel {}", m_nSelect + 1);
                m_pLogin->ChangeStep(2);
            }
        });
        m_pUIManager->AddElement("btnGoWorld_channel", m_pBtnGoWorld);
    }
}

// CreateChannelButton 函數已移除 - 現在由 LayoutMan::AutoBuild 處理按鈕創建

void UIChannelSelect::CreatePlaceholderBackground(WzGr2D& gr, std::int32_t x, std::int32_t y)
{
    // Create placeholder background when WZ chBackgrn not available
    // Based on typical MapleStory channel select dialog size
    constexpr std::uint32_t bgWidth = 350;
    constexpr std::uint32_t bgHeight = 200;

    auto canvas = std::make_shared<WzCanvas>(bgWidth, bgHeight);
    std::vector<std::uint8_t> pixels(bgWidth * bgHeight * 4);

    // Create a semi-transparent dark blue gradient background
    for (std::uint32_t py = 0; py < bgHeight; ++py)
    {
        for (std::uint32_t px = 0; px < bgWidth; ++px)
        {
            auto idx = (py * bgWidth + px) * 4;
            float t = static_cast<float>(py) / static_cast<float>(bgHeight);

            // Dark blue gradient with slight transparency
            pixels[idx + 0] = static_cast<std::uint8_t>(30 + 10 * t);   // R
            pixels[idx + 1] = static_cast<std::uint8_t>(40 + 15 * t);   // G
            pixels[idx + 2] = static_cast<std::uint8_t>(70 + 30 * t);   // B
            pixels[idx + 3] = 220;                                       // A

            // Add border (2px)
            if (px < 2 || px >= bgWidth - 2 || py < 2 || py >= bgHeight - 2)
            {
                pixels[idx + 0] = 80;
                pixels[idx + 1] = 100;
                pixels[idx + 2] = 140;
                pixels[idx + 3] = 255;
            }
        }
    }

    canvas->SetPixelData(std::move(pixels));

    // Create layer at z-order 140 (below channel buttons which are at 160)
    m_pLayerBg = gr.CreateLayer(x, y, bgWidth, bgHeight, 140);
    if (m_pLayerBg)
    {
        m_pLayerBg->InsertCanvas(std::make_shared<WzGr2DCanvas>(canvas), 0, 255, 255);
        LOG_DEBUG("UIChannelSelect: Placeholder background created at ({}, {}) size {}x{}",
                  x, y, bgWidth, bgHeight);
    }
}

} // namespace ms
