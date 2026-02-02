#include "UINewCharRaceSelect.h"
#include "UIButton.h"
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

UINewCharRaceSelect::UINewCharRaceSelect() = default;

UINewCharRaceSelect::~UINewCharRaceSelect()
{
    Destroy();
}

auto UINewCharRaceSelect::OnCreate(std::any params) -> Result<void>
{
    // 1. Extract and validate parameters
    auto* createParams = std::any_cast<CreateParams>(&params);
    if (!createParams)
    {
        return Result<void>::Error("Invalid params type for UINewCharRaceSelect");
    }

    if (!createParams->IsValid())
    {
        return Result<void>::Error("UINewCharRaceSelect CreateParams validation failed");
    }

    // 2. Store references
    m_pLogin = createParams->login;
    m_pGr = createParams->gr;
    m_pUIManager = createParams->uiManager;

    // 3. Initialize state (from constructor @ 0xba96f0)
    m_nSelectedRace = 1;  // Default to race 1 (original uses 1)
    m_nSelectedSubJob = 0;
    m_nSelectedBtnIdx = 0;
    m_nFrontOrderBtn = 0;
    m_nBtRaceCount = 19;
    m_nSelectFirstBtnIdx = -1;

    // 4. Initialize race ordering (default sequential)
    for (int i = 0; i < kMaxRaceCount; ++i)
    {
        m_anOrderRace[i] = static_cast<std::int16_t>(i);
    }

    // 5. Load RaceSelect_new WZ property
    auto& resMan = WzResMan::GetInstance();
    auto loginImgProp = resMan.GetProperty("UI/Login.img");
    if (loginImgProp)
    {
        m_pRaceSelectProp = loginImgProp->GetChild("RaceSelect_new");
        if (m_pRaceSelectProp)
        {
            LOG_DEBUG("UINewCharRaceSelect: RaceSelect_new property loaded");
        }
        else
        {
            LOG_DEBUG("UINewCharRaceSelect: RaceSelect_new NOT found in Login.img");
        }
    }
    else
    {
        LOG_DEBUG("UINewCharRaceSelect: Login.img NOT found");
    }

    // 6. Load New/Hot indicator canvases from WZ
    auto newProp = resMan.GetProperty("UI/Login.img/RaceSelect_new/new");
    if (newProp)
    {
        m_pNewCanvas = newProp->GetCanvas();
        if (!m_pNewCanvas)
        {
            auto frame0 = newProp->GetChild("0");
            if (frame0)
            {
                m_pNewCanvas = frame0->GetCanvas();
            }
        }
        if (m_pNewCanvas)
        {
            LOG_DEBUG("UINewCharRaceSelect: 'new' indicator canvas loaded");
        }
    }

    auto hotProp = resMan.GetProperty("UI/Login.img/RaceSelect_new/hot");
    if (hotProp)
    {
        m_pHotCanvas = hotProp->GetCanvas();
        if (!m_pHotCanvas)
        {
            auto frame0 = hotProp->GetChild("0");
            if (frame0)
            {
                m_pHotCanvas = frame0->GetCanvas();
            }
        }
        if (m_pHotCanvas)
        {
            LOG_DEBUG("UINewCharRaceSelect: 'hot' indicator canvas loaded");
        }
    }

    // 7. Initialize test flags for demonstration (races 0, 5, 10 are "new", races 2, 7 are "hot")
    // In actual implementation, these would come from server or WZ data
    m_abNewRace[0] = true;
    m_abNewRace[5] = true;
    m_abNewRace[10] = true;
    m_abHotRace[2] = true;
    m_abHotRace[7] = true;

    // 8. Load race buttons (5 at a time with pagination)
    LoadButton();

    LOG_DEBUG("UINewCharRaceSelect::OnCreate completed");

    return Result<void>::Success();
}

void UINewCharRaceSelect::LoadButton()
{
    // Based on CUINewCharRaceSelect_Ex::LoadButton @ 0xbac760
    // Loads 5 buttons at a time based on m_nFrontOrderBtn pagination offset
    // Button positioning: X = 126 * i + 92, Y = 427

    if (!m_pGr || !m_pUIManager)
    {
        return;
    }

    if (m_nFrontOrderBtn > 14)  // Max valid offset (19 - 5 = 14)
    {
        return;
    }

    auto& resMan = WzResMan::GetInstance();

    for (int i = 0; i < kButtonsPerPage; ++i)
    {
        // Calculate race ID from order array
        const std::int32_t raceId = m_anOrderRace[i + m_nFrontOrderBtn];

        // Calculate button position (from original: 126 * i + 92, 427)
        const int btnX = kButtonStartX + i * kButtonSpacingX;
        const int btnY = kButtonY;

        // Remove existing button layer if any
        if (m_apButton[i] && m_apButton[i]->GetLayer())
        {
            m_pGr->RemoveLayer(m_apButton[i]->GetLayer());
        }

        // Create new button
        auto btn = std::make_shared<UIButton>();
        bool wzLoaded = false;

        // Check if race is enabled and load appropriate WZ path
        // Original: "UI/Login.img/RaceSelect_new/button/%d" for enabled
        //          "UI/Login.img/RaceSelect_new/buttonDisabled/%d" for disabled
        const bool isEnabled = IsEnabledRace(raceId);
        std::string wzPath;
        if (isEnabled)
        {
            wzPath = "UI/Login.img/RaceSelect_new/button/" + std::to_string(raceId);
        }
        else
        {
            wzPath = "UI/Login.img/RaceSelect_new/buttonDisabled/" + std::to_string(raceId);
        }

        // Try to load button from WZ
        auto btnProp = resMan.GetProperty(wzPath);
        if (btnProp)
        {
            if (btn->LoadFromProperty(btnProp))
            {
                wzLoaded = true;
                LOG_DEBUG("UINewCharRaceSelect: Loaded button {} from WZ (race {})",
                          wzPath, raceId);
            }
            else
            {
                LOG_DEBUG("UINewCharRaceSelect: LoadFromProperty failed for {}", wzPath);
            }
        }
        else
        {
            LOG_DEBUG("UINewCharRaceSelect: Button property not found: {}", wzPath);
        }

        // Create placeholder if WZ not loaded
        if (!wzLoaded)
        {
            constexpr int kButtonWidth = 100;
            constexpr int kButtonHeight = 80;

            auto canvas = std::make_shared<WzCanvas>(kButtonWidth, kButtonHeight);
            std::vector<std::uint8_t> pixels(static_cast<size_t>(kButtonWidth * kButtonHeight * 4));

            // Color based on enabled state
            std::uint8_t r = isEnabled ? 100 : 80;
            std::uint8_t g = isEnabled ? 150 : 80;
            std::uint8_t b = isEnabled ? 200 : 80;

            for (size_t p = 0; p < pixels.size(); p += 4)
            {
                pixels[p + 0] = r;
                pixels[p + 1] = g;
                pixels[p + 2] = b;
                pixels[p + 3] = isEnabled ? 220 : 150;
            }
            canvas->SetPixelData(std::move(pixels));

            btn->SetStateCanvas(UIState::Normal, canvas);
            btn->SetSize(kButtonWidth, kButtonHeight);
            LOG_DEBUG("UINewCharRaceSelect: Using placeholder for race {}", raceId);
        }

        btn->SetPosition(btnX, btnY);
        btn->CreateLayer(*m_pGr, 150);
        btn->SetEnabled(isEnabled);

        // Set click callback with race ID
        const std::uint32_t buttonId = static_cast<std::uint32_t>(raceId);
        btn->SetClickCallback([this, buttonId]() {
            OnButtonClicked(buttonId);
        });

        m_pUIManager->AddElement("raceBtn" + std::to_string(i), btn);
        m_apButton[i] = btn;

        LOG_DEBUG("Created race button at slot {} for race {} at ({}, {})",
                  i, raceId, btnX, btnY);
    }

    // Load arrow buttons for pagination
    LoadArrowButtons();

    // Load background
    LoadBackground();

    // Load New/Hot indicators
    LoadNewHotIndicators();
}

void UINewCharRaceSelect::LoadArrowButtons()
{
    if (!m_pGr || !m_pUIManager)
    {
        return;
    }

    auto& resMan = WzResMan::GetInstance();

    // Load left arrow button
    if (m_pLeftButton && m_pLeftButton->GetLayer())
    {
        m_pGr->RemoveLayer(m_pLeftButton->GetLayer());
    }
    m_pLeftButton = std::make_shared<UIButton>();

    auto leftArrowProp = resMan.GetProperty("UI/Login.img/RaceSelect_new/leftArrow");
    if (leftArrowProp && m_pLeftButton->LoadFromProperty(leftArrowProp))
    {
        LOG_DEBUG("UINewCharRaceSelect: Left arrow loaded from WZ");
    }
    else
    {
        // Create placeholder
        constexpr int kArrowSize = 30;
        auto canvas = std::make_shared<WzCanvas>(kArrowSize, kArrowSize);
        std::vector<std::uint8_t> pixels(static_cast<size_t>(kArrowSize * kArrowSize * 4));
        for (size_t p = 0; p < pixels.size(); p += 4)
        {
            pixels[p + 0] = 150;
            pixels[p + 1] = 150;
            pixels[p + 2] = 200;
            pixels[p + 3] = 220;
        }
        canvas->SetPixelData(std::move(pixels));
        m_pLeftButton->SetStateCanvas(UIState::Normal, canvas);
        m_pLeftButton->SetSize(kArrowSize, kArrowSize);
    }

    m_pLeftButton->SetPosition(kButtonStartX - 50, kButtonY + 20);
    m_pLeftButton->CreateLayer(*m_pGr, 155);
    m_pLeftButton->SetClickCallback([this]() { OnButtonClicked(kLeftArrowId); });
    m_pLeftButton->SetEnabled(m_nFrontOrderBtn > 0 || m_nSelectedBtnIdx > 0);
    m_pUIManager->AddElement("leftArrow", m_pLeftButton);

    // Load right arrow button
    if (m_pRightButton && m_pRightButton->GetLayer())
    {
        m_pGr->RemoveLayer(m_pRightButton->GetLayer());
    }
    m_pRightButton = std::make_shared<UIButton>();

    auto rightArrowProp = resMan.GetProperty("UI/Login.img/RaceSelect_new/rightArrow");
    if (rightArrowProp && m_pRightButton->LoadFromProperty(rightArrowProp))
    {
        LOG_DEBUG("UINewCharRaceSelect: Right arrow loaded from WZ");
    }
    else
    {
        constexpr int kArrowSize = 30;
        auto canvas = std::make_shared<WzCanvas>(kArrowSize, kArrowSize);
        std::vector<std::uint8_t> pixels(static_cast<size_t>(kArrowSize * kArrowSize * 4));
        for (size_t p = 0; p < pixels.size(); p += 4)
        {
            pixels[p + 0] = 200;
            pixels[p + 1] = 150;
            pixels[p + 2] = 150;
            pixels[p + 3] = 220;
        }
        canvas->SetPixelData(std::move(pixels));
        m_pRightButton->SetStateCanvas(UIState::Normal, canvas);
        m_pRightButton->SetSize(kArrowSize, kArrowSize);
    }

    m_pRightButton->SetPosition(kButtonStartX + kButtonsPerPage * kButtonSpacingX, kButtonY + 20);
    m_pRightButton->CreateLayer(*m_pGr, 155);
    m_pRightButton->SetClickCallback([this]() { OnButtonClicked(kRightArrowId); });
    m_pRightButton->SetEnabled(m_nSelectedBtnIdx < 4 || m_nFrontOrderBtn < m_nBtRaceCount - kButtonsPerPage);
    m_pUIManager->AddElement("rightArrow", m_pRightButton);

    // Load create/make button
    if (m_pCreateButton && m_pCreateButton->GetLayer())
    {
        m_pGr->RemoveLayer(m_pCreateButton->GetLayer());
    }
    m_pCreateButton = std::make_shared<UIButton>();

    auto makeProp = resMan.GetProperty("UI/Login.img/RaceSelect_new/make");
    if (makeProp && m_pCreateButton->LoadFromProperty(makeProp))
    {
        LOG_DEBUG("UINewCharRaceSelect: Make button loaded from WZ");
    }
    else
    {
        constexpr int kMakeWidth = 100;
        constexpr int kMakeHeight = 35;
        auto canvas = std::make_shared<WzCanvas>(kMakeWidth, kMakeHeight);
        std::vector<std::uint8_t> pixels(static_cast<size_t>(kMakeWidth * kMakeHeight * 4));
        for (size_t p = 0; p < pixels.size(); p += 4)
        {
            pixels[p + 0] = 100;
            pixels[p + 1] = 200;
            pixels[p + 2] = 100;
            pixels[p + 3] = 255;
        }
        canvas->SetPixelData(std::move(pixels));
        m_pCreateButton->SetStateCanvas(UIState::Normal, canvas);
        m_pCreateButton->SetSize(kMakeWidth, kMakeHeight);
    }

    m_pCreateButton->SetPosition(400, 520);
    m_pCreateButton->CreateLayer(*m_pGr, 160);
    m_pCreateButton->SetClickCallback([this]() { OnButtonClicked(kConfirmId); });
    m_pUIManager->AddElement("makeButton", m_pCreateButton);

    // Load cancel button
    if (m_pCancelButton && m_pCancelButton->GetLayer())
    {
        m_pGr->RemoveLayer(m_pCancelButton->GetLayer());
    }
    m_pCancelButton = std::make_shared<UIButton>();

    auto cancelProp = resMan.GetProperty("UI/Login.img/RaceSelect_new/cancel");
    if (cancelProp && m_pCancelButton->LoadFromProperty(cancelProp))
    {
        LOG_DEBUG("UINewCharRaceSelect: Cancel button loaded from WZ");
    }
    else
    {
        // Create placeholder cancel button
        constexpr int kCancelWidth = 100;
        constexpr int kCancelHeight = 35;
        auto canvas = std::make_shared<WzCanvas>(kCancelWidth, kCancelHeight);
        std::vector<std::uint8_t> pixels(static_cast<size_t>(kCancelWidth * kCancelHeight * 4));
        for (size_t p = 0; p < pixels.size(); p += 4)
        {
            pixels[p + 0] = 200;  // R - reddish
            pixels[p + 1] = 100;  // G
            pixels[p + 2] = 100;  // B
            pixels[p + 3] = 255;  // A
        }
        canvas->SetPixelData(std::move(pixels));
        m_pCancelButton->SetStateCanvas(UIState::Normal, canvas);
        m_pCancelButton->SetSize(kCancelWidth, kCancelHeight);
    }

    // Position cancel button to the left of make button
    m_pCancelButton->SetPosition(280, 520);
    m_pCancelButton->CreateLayer(*m_pGr, 160);
    m_pCancelButton->SetClickCallback([this]() { OnButtonClicked(kCancelId); });
    m_pUIManager->AddElement("cancelButton", m_pCancelButton);
}

void UINewCharRaceSelect::OnButtonClicked(std::uint32_t nId)
{
    // Based on CUINewCharRaceSelect_Ex::OnButtonClicked @ 0xbb4cd0
    // Handles race button clicks (0-18) and arrow buttons (10000-10003)

    if (!m_pLogin)
    {
        return;
    }

    LOG_DEBUG("UINewCharRaceSelect::OnButtonClicked nId={}", nId);

    // Handle arrow button clicks
    if (nId == kLeftArrowId)
    {
        // Left arrow - move selection left or scroll page
        if (m_nSelectedBtnIdx > 0)
        {
            m_nSelectedBtnIdx--;
        }
        else if (m_nFrontOrderBtn > 0)
        {
            m_nFrontOrderBtn--;
            LoadButton();
        }
        SelectRaceButton(m_nSelectedBtnIdx);
        return;
    }

    if (nId == kRightArrowId)
    {
        // Right arrow - move selection right or scroll page
        if (m_nSelectedBtnIdx < kButtonsPerPage - 1)
        {
            m_nSelectedBtnIdx++;
        }
        else if (m_nFrontOrderBtn < m_nBtRaceCount - kButtonsPerPage)
        {
            m_nFrontOrderBtn++;
            LoadButton();
        }
        SelectRaceButton(m_nSelectedBtnIdx);
        return;
    }

    if (nId == kConfirmId)
    {
        // Confirm button - open race selection confirmation
        OpenConfirmRaceDlg();
        return;
    }

    if (nId == kCancelId)
    {
        // Cancel button - go back
        if (m_pLogin)
        {
            m_pLogin->ChangeStep(2);  // Go back to character select
        }
        return;
    }

    // Handle race button click (nId = race ID)
    if (nId <= 18)
    {
        // Find the button index for this race ID
        int btnIdx = -1;
        for (int i = 0; i < kMaxRaceCount; ++i)
        {
            if (m_anOrderRace[i] == static_cast<std::int16_t>(nId))
            {
                btnIdx = i - m_nFrontOrderBtn;
                break;
            }
        }

        if (btnIdx >= 0 && btnIdx < kButtonsPerPage)
        {
            m_nSelectedBtnIdx = btnIdx;
            m_nSelectFirstBtnIdx = nId;
            SelectRaceButton(btnIdx);
            SetSelectedRace(m_nSelectedRace);
            SetSelectedSubJob(m_nSelectedSubJob);
        }
    }

    // Update arrow button states
    if (m_pLeftButton)
    {
        m_pLeftButton->SetEnabled(m_nSelectedBtnIdx > 0 || m_nFrontOrderBtn > 0);
    }
    if (m_pRightButton)
    {
        m_pRightButton->SetEnabled(
            m_nSelectedBtnIdx < kButtonsPerPage - 1 ||
            m_nFrontOrderBtn < m_nBtRaceCount - kButtonsPerPage);
    }
}

void UINewCharRaceSelect::SelectRaceButton(std::uint32_t nIdx)
{
    if (nIdx >= kButtonsPerPage)
    {
        return;
    }

    // Update selected race from order array
    m_nSelectedRace = m_anOrderRace[nIdx + m_nFrontOrderBtn];

    // Update button visual states
    for (int i = 0; i < kButtonsPerPage; ++i)
    {
        if (m_apButton[i])
        {
            if (i == static_cast<int>(nIdx))
            {
                m_apButton[i]->SetState(UIState::Pressed);
            }
            else
            {
                m_apButton[i]->SetState(UIState::Normal);
            }
        }
    }

    // Load character preview and race info for selected race
    LoadCharacterPreview(m_nSelectedRace);
    LoadRaceInfo(m_nSelectedRace);

    LOG_DEBUG("UINewCharRaceSelect: Selected race {} at button index {}", m_nSelectedRace, nIdx);
}

void UINewCharRaceSelect::OpenConfirmRaceDlg()
{
    if (m_nSelectedRace < 0)
    {
        LOG_DEBUG("UINewCharRaceSelect: No race selected");
        return;
    }

    LOG_DEBUG("UINewCharRaceSelect: Confirming race selection: {}", m_nSelectedRace);

    if (m_pLogin)
    {
        m_pLogin->SetSelectedRace(m_nSelectedRace);
        m_pLogin->ChangeStep(4);  // Go to avatar customization
    }
}

void UINewCharRaceSelect::LoadBackground()
{
    if (!m_pGr)
    {
        return;
    }

    auto& resMan = WzResMan::GetInstance();
    auto& gr = *m_pGr;

    // Try to load main background: UI/Login.img/RaceSelect_new/Back/0
    auto backProp = resMan.GetProperty("UI/Login.img/RaceSelect_new/Back/0");
    if (backProp)
    {
        auto canvas = backProp->GetCanvas();
        if (canvas)
        {
            auto origin = canvas->GetOrigin();
            int screenWidth = static_cast<int>(gr.GetWidth());
            int screenHeight = static_cast<int>(gr.GetHeight());
            int canvasWidth = canvas->GetWidth();
            int canvasHeight = canvas->GetHeight();

            int layerX = (screenWidth - canvasWidth) / 2 + origin.x;
            int layerY = (screenHeight - canvasHeight) / 2 + origin.y;

            m_pLayerBackGround = gr.CreateLayer(layerX, layerY,
                                                static_cast<std::uint32_t>(canvasWidth),
                                                static_cast<std::uint32_t>(canvasHeight), 100);
            if (m_pLayerBackGround)
            {
                m_pLayerBackGround->SetScreenSpace(true);
                m_pLayerBackGround->InsertCanvas(canvas, 0, 255, 255);
                LOG_DEBUG("UINewCharRaceSelect: Background loaded at ({}, {})", layerX, layerY);
            }
        }
    }
    else
    {
        LOG_DEBUG("UINewCharRaceSelect: No background property found");
    }

    // Load Back1/0 (second layer)
    auto back1Prop = resMan.GetProperty("UI/Login.img/RaceSelect_new/Back1/0");
    if (back1Prop)
    {
        auto canvas = back1Prop->GetCanvas();
        if (canvas)
        {
            auto origin = canvas->GetOrigin();
            int screenWidth = static_cast<int>(gr.GetWidth());
            int screenHeight = static_cast<int>(gr.GetHeight());
            int canvasWidth = canvas->GetWidth();
            int canvasHeight = canvas->GetHeight();

            int layerX = (screenWidth - canvasWidth) / 2 + origin.x;
            int layerY = (screenHeight - canvasHeight) / 2 + origin.y;

            m_pLayerBackGround1 = gr.CreateLayer(layerX, layerY,
                                                 static_cast<std::uint32_t>(canvasWidth),
                                                 static_cast<std::uint32_t>(canvasHeight), 105);
            if (m_pLayerBackGround1)
            {
                m_pLayerBackGround1->SetScreenSpace(true);
                m_pLayerBackGround1->InsertCanvas(canvas, 0, 255, 255);
                LOG_DEBUG("UINewCharRaceSelect: Background1 loaded");
            }
        }
    }
}

void UINewCharRaceSelect::LoadCharacterPreview(std::int32_t raceId)
{
    if (!m_pGr)
    {
        return;
    }

    auto& gr = *m_pGr;
    auto& resMan = WzResMan::GetInstance();

    // Remove existing preview layer
    if (m_pLayerCharPreview)
    {
        gr.RemoveLayer(m_pLayerCharPreview);
        m_pLayerCharPreview.reset();
    }

    // Try to load character preview from WZ
    // Path: UI/Login.img/RaceSelect_new/race/{raceId}/0 or charImg/{raceId}
    std::string previewPath = "UI/Login.img/RaceSelect_new/race/" + std::to_string(raceId);
    auto previewProp = resMan.GetProperty(previewPath);

    if (!previewProp)
    {
        // Try alternative path
        previewPath = "UI/Login.img/RaceSelect_new/charImg/" + std::to_string(raceId);
        previewProp = resMan.GetProperty(previewPath);
    }

    if (previewProp)
    {
        // Check if it's animated (has numbered children) or static
        auto frame0 = previewProp->GetChild("0");
        std::shared_ptr<WzCanvas> canvas;

        if (frame0)
        {
            canvas = frame0->GetCanvas();
        }
        else
        {
            canvas = previewProp->GetCanvas();
        }

        if (canvas)
        {
            // Position preview in the center-top area of the screen
            int screenWidth = static_cast<int>(gr.GetWidth());
            int canvasWidth = canvas->GetWidth();
            int canvasHeight = canvas->GetHeight();
            auto origin = canvas->GetOrigin();

            int layerX = (screenWidth - canvasWidth) / 2 + origin.x;
            int layerY = 150 + origin.y;  // Fixed Y position for preview

            m_pLayerCharPreview = gr.CreateLayer(
                layerX, layerY,
                static_cast<std::uint32_t>(canvasWidth),
                static_cast<std::uint32_t>(canvasHeight),
                120  // Above background, below buttons
            );

            if (m_pLayerCharPreview)
            {
                m_pLayerCharPreview->SetScreenSpace(true);
                m_pLayerCharPreview->InsertCanvas(canvas, 0, 255, 255);

                // If animated, load more frames
                if (frame0)
                {
                    for (int i = 1; ; ++i)
                    {
                        auto frameN = previewProp->GetChild(std::to_string(i));
                        if (!frameN)
                        {
                            break;
                        }
                        auto frameCanvas = frameN->GetCanvas();
                        if (frameCanvas)
                        {
                            auto delayProp = frameN->GetChild("delay");
                            int delay = delayProp ? delayProp->GetInt(100) : 100;
                            m_pLayerCharPreview->InsertCanvas(frameCanvas, delay, 255, 255);
                        }
                    }

                    // Start animation if multiple frames
                    if (m_pLayerCharPreview->GetCanvasCount() > 1)
                    {
                        m_pLayerCharPreview->Animate(Gr2DAnimationType::Loop);
                    }
                }

                LOG_DEBUG("UINewCharRaceSelect: Character preview loaded for race {}", raceId);
            }
        }
    }
    else
    {
        LOG_DEBUG("UINewCharRaceSelect: No character preview found for race {}", raceId);
    }
}

void UINewCharRaceSelect::LoadRaceInfo(std::int32_t raceId)
{
    if (!m_pGr)
    {
        return;
    }

    auto& gr = *m_pGr;
    auto& resMan = WzResMan::GetInstance();

    // Remove existing info layer
    if (m_pLayerRaceInfo)
    {
        gr.RemoveLayer(m_pLayerRaceInfo);
        m_pLayerRaceInfo.reset();
    }

    // Try to load race info/description from WZ
    // Path: UI/Login.img/RaceSelect_new/name/{raceId} or desc/{raceId}
    std::string infoPath = "UI/Login.img/RaceSelect_new/name/" + std::to_string(raceId);
    auto infoProp = resMan.GetProperty(infoPath);

    if (infoProp)
    {
        auto canvas = infoProp->GetCanvas();
        if (!canvas)
        {
            auto frame0 = infoProp->GetChild("0");
            if (frame0)
            {
                canvas = frame0->GetCanvas();
            }
        }

        if (canvas)
        {
            int screenWidth = static_cast<int>(gr.GetWidth());
            int canvasWidth = canvas->GetWidth();
            int canvasHeight = canvas->GetHeight();
            auto origin = canvas->GetOrigin();

            // Position race name/info below the preview
            int layerX = (screenWidth - canvasWidth) / 2 + origin.x;
            int layerY = 380 + origin.y;

            m_pLayerRaceInfo = gr.CreateLayer(
                layerX, layerY,
                static_cast<std::uint32_t>(canvasWidth),
                static_cast<std::uint32_t>(canvasHeight),
                125  // Above preview
            );

            if (m_pLayerRaceInfo)
            {
                m_pLayerRaceInfo->SetScreenSpace(true);
                m_pLayerRaceInfo->InsertCanvas(canvas, 0, 255, 255);
                LOG_DEBUG("UINewCharRaceSelect: Race info loaded for race {}", raceId);
            }
        }
    }
    else
    {
        LOG_DEBUG("UINewCharRaceSelect: No race info found for race {}", raceId);
    }
}

void UINewCharRaceSelect::LoadNewHotIndicators()
{
    if (!m_pGr)
    {
        return;
    }

    auto& gr = *m_pGr;

    // Remove existing indicator layers
    for (int i = 0; i < kButtonsPerPage; ++i)
    {
        if (m_apNewIndicator[i])
        {
            gr.RemoveLayer(m_apNewIndicator[i]);
            m_apNewIndicator[i].reset();
        }
        if (m_apHotIndicator[i])
        {
            gr.RemoveLayer(m_apHotIndicator[i]);
            m_apHotIndicator[i].reset();
        }
    }

    // Create indicator layers for each visible button
    for (int i = 0; i < kButtonsPerPage; ++i)
    {
        const std::int32_t raceId = m_anOrderRace[i + m_nFrontOrderBtn];
        const int btnX = kButtonStartX + i * kButtonSpacingX;
        const int btnY = kButtonY;

        // Create "New" indicator if this race is flagged as new
        if (raceId >= 0 && raceId < kMaxRaceCount && m_abNewRace[raceId] && m_pNewCanvas)
        {
            auto origin = m_pNewCanvas->GetOrigin();
            int canvasWidth = m_pNewCanvas->GetWidth();
            int canvasHeight = m_pNewCanvas->GetHeight();

            // Position indicator at top-right corner of button
            int layerX = btnX + 60 + origin.x;  // Offset to right side of button
            int layerY = btnY - 10 + origin.y;  // Offset above button

            m_apNewIndicator[i] = gr.CreateLayer(
                layerX, layerY,
                static_cast<std::uint32_t>(canvasWidth),
                static_cast<std::uint32_t>(canvasHeight),
                165  // Above buttons
            );

            if (m_apNewIndicator[i])
            {
                m_apNewIndicator[i]->SetScreenSpace(true);
                m_apNewIndicator[i]->InsertCanvas(m_pNewCanvas, 0, 255, 255);
                LOG_DEBUG("UINewCharRaceSelect: 'new' indicator created for race {} at slot {}",
                          raceId, i);
            }
        }

        // Create "Hot" indicator if this race is flagged as hot
        if (raceId >= 0 && raceId < kMaxRaceCount && m_abHotRace[raceId] && m_pHotCanvas)
        {
            auto origin = m_pHotCanvas->GetOrigin();
            int canvasWidth = m_pHotCanvas->GetWidth();
            int canvasHeight = m_pHotCanvas->GetHeight();

            // Position indicator at top-left corner of button
            int layerX = btnX - 10 + origin.x;  // Offset to left side of button
            int layerY = btnY - 10 + origin.y;  // Offset above button

            m_apHotIndicator[i] = gr.CreateLayer(
                layerX, layerY,
                static_cast<std::uint32_t>(canvasWidth),
                static_cast<std::uint32_t>(canvasHeight),
                165  // Above buttons
            );

            if (m_apHotIndicator[i])
            {
                m_apHotIndicator[i]->SetScreenSpace(true);
                m_apHotIndicator[i]->InsertCanvas(m_pHotCanvas, 0, 255, 255);
                LOG_DEBUG("UINewCharRaceSelect: 'hot' indicator created for race {} at slot {}",
                          raceId, i);
            }
        }
    }
}

void UINewCharRaceSelect::SetSelectedRace(std::int32_t race)
{
    // Based on CUINewCharRaceSelect_Ex::SetSelectedRace @ 0xba6820
    // Updates m_pLogin->m_nCurSelectedRace

    if (race < 0 || race >= kMaxRaceCount)
    {
        return;
    }

    if (!IsEnabledRace(race))
    {
        LOG_DEBUG("Race {} is not enabled", race);
        return;
    }

    m_nSelectedRace = race;

    if (m_pLogin)
    {
        m_pLogin->SetSelectedRace(race);
    }

    LOG_DEBUG("Race {} selected", race);
}

void UINewCharRaceSelect::SetSelectedSubJob(std::int32_t subJob)
{
    m_nSelectedSubJob = subJob;

    if (m_pLogin)
    {
        m_pLogin->SetSelectedSubJob(static_cast<std::int16_t>(subJob));
    }
}

auto UINewCharRaceSelect::IsEnabledRace(std::int32_t race) const -> bool
{
    // Check if the race can be selected
    // This could be based on account level, events, etc.
    if (race < 0 || race >= kMaxRaceCount)
    {
        return false;
    }

    // For now, all races are enabled
    // Original client checks various conditions (account level, events, etc.)
    return true;
}

void UINewCharRaceSelect::OnDestroy() noexcept
{
    try
    {
        // 1. Clear race button arrays
        for (int i = 0; i < kButtonsPerPage; ++i)
        {
            m_apButton[i].reset();
        }

        // 2. Clear navigation buttons
        m_pLeftButton.reset();
        m_pRightButton.reset();
        m_pCreateButton.reset();
        m_pCancelButton.reset();

        // 3. Remove preview and info layers
        if (m_pGr)
        {
            if (m_pLayerCharPreview)
            {
                m_pGr->RemoveLayer(m_pLayerCharPreview);
            }
            if (m_pLayerRaceInfo)
            {
                m_pGr->RemoveLayer(m_pLayerRaceInfo);
            }

            // 4. Remove New/Hot indicator layers
            for (int i = 0; i < kButtonsPerPage; ++i)
            {
                if (m_apNewIndicator[i])
                {
                    m_pGr->RemoveLayer(m_apNewIndicator[i]);
                }
                if (m_apHotIndicator[i])
                {
                    m_pGr->RemoveLayer(m_apHotIndicator[i]);
                }
            }

            // 5. Remove background layers
            if (m_pLayerBackGround)
            {
                m_pGr->RemoveLayer(m_pLayerBackGround);
            }
            if (m_pLayerBackGround1)
            {
                m_pGr->RemoveLayer(m_pLayerBackGround1);
            }
            if (m_pLayerBackGround2)
            {
                m_pGr->RemoveLayer(m_pLayerBackGround2);
            }
        }

        // 6. Clear New/Hot indicator arrays
        for (int i = 0; i < kButtonsPerPage; ++i)
        {
            m_apNewIndicator[i].reset();
            m_apHotIndicator[i].reset();
        }

        // 7. Clear layer pointers
        m_pLayerBackGround.reset();
        m_pLayerBackGround1.reset();
        m_pLayerBackGround2.reset();
        m_pLayerCharPreview.reset();
        m_pLayerRaceInfo.reset();

        // 8. Clear WZ property pointers
        m_pRaceSelectProp.reset();
        m_pNewCanvas.reset();
        m_pHotCanvas.reset();

        // 9. Clear UI manager elements
        if (m_pUIManager)
        {
            for (int i = 0; i < kButtonsPerPage; ++i)
            {
                m_pUIManager->RemoveElement("raceBtn" + std::to_string(i));
            }
            m_pUIManager->RemoveElement("leftArrow");
            m_pUIManager->RemoveElement("rightArrow");
            m_pUIManager->RemoveElement("makeButton");
            m_pUIManager->RemoveElement("cancelButton");
        }

        // 10. Clear references (nullify pointers)
        m_pLogin = nullptr;
        m_pGr = nullptr;
        m_pUIManager = nullptr;

        LOG_DEBUG("UINewCharRaceSelect destroyed");
    }
    catch (...)
    {
        // Suppress all exceptions in destructor
    }
}

void UINewCharRaceSelect::Destroy()
{
    UIElement::Destroy();
}

void UINewCharRaceSelect::Update()
{
    // Update logic
}

void UINewCharRaceSelect::Draw()
{
    // Drawing is handled by the layer system
}

void UINewCharRaceSelect::OnMouseMove(std::int32_t x, std::int32_t y)
{
    (void)x;
    (void)y;
}

void UINewCharRaceSelect::OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button)
{
    (void)x;
    (void)y;
    (void)button;
}

void UINewCharRaceSelect::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button)
{
    (void)x;
    (void)y;
    (void)button;
}

void UINewCharRaceSelect::OnKeyDown(std::int32_t keyCode)
{
    // ESC - go back to character select (SDLK_ESCAPE = 27)
    if (keyCode == 27 && m_pLogin)
    {
        m_pLogin->ChangeStep(2);  // Go back to character select
        return;
    }

    // Left arrow key (SDLK_LEFT = 1073741904 in SDL3)
    if (keyCode == 1073741904)
    {
        OnButtonClicked(kLeftArrowId);
        return;
    }

    // Right arrow key (SDLK_RIGHT = 1073741903 in SDL3)
    if (keyCode == 1073741903)
    {
        OnButtonClicked(kRightArrowId);
        return;
    }

    // Enter key (SDLK_RETURN = 13)
    if (keyCode == 13)
    {
        OnButtonClicked(kConfirmId);
        return;
    }
}

} // namespace ms
