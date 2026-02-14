#include "Login.h"
#include "app/Application.h"
#include "app/Configuration.h"
#include "app/WvsContext.h"
#include "graphics/WzGr2D.h"
#include "graphics/WzGr2DLayer.h"
#include "ui/UIButton.h"
#include "ui/UIChannelSelect.h"
#include "ui/UIEdit.h"
#include "ui/UINewCharRaceSelect.h"
#include "ui/UISelectChar.h"
#include "ui/UIWorldSelect.h"
#include "util/Logger.h"
#include "graphics/WzGr2DCanvas.h"
#include "wz/WzCanvas.h"
#include "wz/WzProperty.h"
#include "wz/WzImage.h"
#include "wz/WzResMan.h"

#include <random>

namespace ms
{

// Static member initialization
std::int32_t Login::m_nBaseStep = 0;
bool Login::m_bOpenedNotActiveAccountDlg = false;

Login::Login() = default;

Login::~Login() = default;

void Login::Init(void* param)
{
    // Call base class init (handles graphics engine setup)
    MapLoadable::Init(param);

    // Reset state
    m_nMagLevel_Obj = 0;
    m_nMagLevel_Back = 0;
    m_nMagLevel_SkillEffect = 0;

    // Load login resources (UI/LoginBack.img and UI/Login.img)
    LoadLoginResources();

    // Load login background from LoginBack.img
    // Note: LoginBack.img is a simple background image, not a map format
    // Structure: LoginBack.img/Title or LoginBack.img/WorldSelect/[variant]
    LoadLoginBackground();

    // Create placeholder background if loading failed
    if (m_pLayerBackground == nullptr)
    {
        LOG_DEBUG("No background loaded from WZ, creating placeholder...");
        CreatePlaceholderBackground();
    }
    else
    {
        LOG_INFO("Login background loaded successfully");
    }

    // Create login-specific layers
    // Book layer at z=110 (login book animation)
    // Position uses center-based coordinate system: (0,0) = screen center
    // Based on CLogin::Init from original client: RelMove(v72, -400, -300 - adjust_cy, ...)
    m_pLayerBook = CreateLayer(110);
    if (m_pLayerBook)
    {
        // Fixed position relative to screen center (matching original MS client)
        // -400, -300 places the 800x600 content area's top-left at screen center minus (400, 300)
        m_pLayerBook->SetPosition(-400, -300);
        m_pLayerBook->SetColor(0xFFFFFFFF);  // Full opacity
    }

    // Initialize state
    m_nLoginStep = 0;
    m_nSubStep = 0;
    m_nCharSelected = -1;
    m_bCanOpenUI = true;

    // Initialize character card saved time
    m_tCharCardSaved = Application::GetTick() - 5000;

    // Setup initial UI based on login mode (based on decompiled CLogin::Init)
    auto& context = WvsContext::GetInstance();

    // Check for relogin cookie first (quick relogin to previous world/channel)
    const auto& reloginCookie = context.GetReloginCookie();
    if (!reloginCookie.empty())
    {
        // Has relogin cookie - skip title and go directly to world selection
        LOG_DEBUG("Relogin cookie found, skipping to step 1");
        ChangeStep(1);
    }
    else if (context.GetLoginBaseStep() == 1)
    {
        // Web login mode (nLoginBaseStep == 1)
        // Go directly to world selection, send world info request
        LOG_DEBUG("Web login mode, starting at step 1");
        m_nLoginStep = 1;
        m_nBaseStep = 1;

        // Create world select UI
        SetupStep1UI();

        // Send world info request packet (opcode 104)
        SendWorldInfoRequestPacket();
    }
    else
    {
        // Normal login - show title screen
        SetupStep0UI();
    }

    // Set initial camera position based on starting step
    // Based on CLogin::Init: IWzVector2D::RelMove(*v126, 28, v125, ...)
    // X = 28, Y = -8 - 600 * step
    auto& gr = get_gr();
    // Load gender and frame choosable options for character creation
    LoadGenderAndFrameChoosable();
    // Load new character info
    LoadNewCharInfo(0);
    LoadNewDummyCharInfo();

    // Play background music from map info
    // PlayBGMFromMapInfo(); // Implemented in MapLoadable

    // Initialize world item final list
    InitWorldItemFinal();

    LOG_INFO("Login stage initialized (step={})", m_nLoginStep);
}

void Login::LoadLoginResources()
{
    auto& resMan = WzResMan::GetInstance();
    m_pLoginImgProp = resMan.GetImage("UI/Login.img");
}

void Login::LoadLoginBackground()
{
    auto& resMan = WzResMan::GetInstance();
    auto& gr = get_gr();

    auto canvasProp = resMan.GetProperty("UI/LoginBack.img/Title/0");
    if (!canvasProp)
    {
        LOG_WARN("LoadLoginBackground: UI/LoginBack.img/Title/0 not found");
        return;
    }

    auto wzCanvas = canvasProp->GetCanvas();
    auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas) : nullptr;
    if (!canvas)
    {
        LOG_WARN("LoadLoginBackground: No canvas found in Title");
        return;
    }
    auto originProp = canvasProp->GetChild("origin");
    if (originProp)
    {
        auto vec = originProp->GetVector();
        canvas->SetOrigin({vec.x, vec.y});
    }

    m_pLayerBackground = gr.CreateLayer(
        0, 0,  // Position at screen center
        canvas->GetWidth(),
        canvas->GetHeight(),
        0  // z=0, behind everything
    );
    if (m_pLayerBackground)
    {
        m_pLayerBackground->InsertCanvas(canvas, 0, 255, 255);
    }
}

void Login::CreatePlaceholderBackground()
{
    auto& gr = get_gr();
    auto bgLayer = GetObjectLayer("background");
    if (!bgLayer)
    {
        bgLayer = CreateObjectLayer("background", 0);
    }
    if (!bgLayer)
    {
        return;
    }

    // Create a gradient background canvas
    const auto width = static_cast<int>(gr.GetWidth());
    const auto height = static_cast<int>(gr.GetHeight());

    auto wzCanvas = std::make_shared<WzCanvas>(width, height);
    auto canvas = std::make_shared<WzGr2DCanvas>(wzCanvas);
    if (!canvas->HasPixelData())
    {
        return;
    }

    // Classic MapleStory login gradient colors (dark blue to slightly lighter blue)
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);

    const std::uint8_t topR = 20, topG = 30, topB = 80;
    const std::uint8_t botR = 40, botG = 50, botB = 120;

    for (int y = 0; y < height; ++y)
    {
        const float t = static_cast<float>(y) / static_cast<float>(height);
        const auto r = static_cast<std::uint8_t>(static_cast<float>(topR) * (1.0F - t) + static_cast<float>(botR) * t);
        const auto g = static_cast<std::uint8_t>(static_cast<float>(topG) * (1.0F - t) + static_cast<float>(botG) * t);
        const auto b = static_cast<std::uint8_t>(static_cast<float>(topB) * (1.0F - t) + static_cast<float>(botB) * t);

        for (int x = 0; x < width; ++x)
        {
            const auto idx = static_cast<std::size_t>((y * width + x) * 4);
            pixels[idx + 0] = r;     // R
            pixels[idx + 1] = g;     // G
            pixels[idx + 2] = b;     // B
            pixels[idx + 3] = 255;   // A (fully opaque)
        }
    }

    wzCanvas->SetPixelData(std::move(pixels));
    canvas->SetOrigin({0, 0});

    bgLayer->InsertCanvas(canvas, 0, 255, 255);

    LOG_DEBUG("Placeholder background created: {}x{}", width, height);
}

void Login::SetupStep0UI()
{
    ClearStepUI();

    auto& gr = get_gr();

    // Try to load buttons from UI.wz
    bool hasWzButtons = false;

    if (m_pLoginImgProp)
    {
        // v1029 uses title, not Title (based on CUITitle::OnCreate decompilation)
        // Try title first (KMS v1029), fallback to Title (older versions)
        auto newTitleProp = (*m_pLoginImgProp)["Title_new"];
        if (newTitleProp)
        {
            auto backgrdProp = newTitleProp->GetChild("backgrd");
            if (backgrdProp)
            {
                LOG_DEBUG("Found backgrd property");
                auto wzCanvas = backgrdProp->GetCanvas();
                auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas) : nullptr;
                if (canvas)
                {
                    LOG_DEBUG("backgrd canvas: {}x{}, origin=({},{})", canvas->GetWidth(), canvas->GetHeight(),
                              canvas->GetOrigin().x, canvas->GetOrigin().y);
                }
                auto titleBgLayer = CreateObjectLayer("titleDialogBg", 100);
                if (titleBgLayer)
                {
                }
            }
            else
            {
                LOG_DEBUG("backgrd property NOT found in Title");
            }

            // Load Login button - original position: (178, 41) relative to dialog
            auto btnLoginProp = newTitleProp->GetChild("BtLogin");
            if (btnLoginProp)
            {
                LOG_DEBUG("Found BtLogin property");

                m_pBtnLogin = std::make_shared<UIButton>();
                if (m_pBtnLogin->LoadFromProperty(btnLoginProp))
                {
                    m_pBtnLogin->CreateLayer(gr, 150);
                    m_pBtnLogin->SetClickCallback([this]() { OnLoginButtonClick(); });
                    m_uiManager.AddElement("btnLogin", m_pBtnLogin);
                    hasWzButtons = true;
                }
                else
                {
                    LOG_WARN("Failed to load BtLogin from property");
                }
            }
            else
            {
                LOG_DEBUG("BtLogin property NOT found");
            }

            // Load Quit button - original position: (159, 117) relative to dialog
            auto btnQuitProp = newTitleProp->GetChild("BtQuit");
            if (btnQuitProp)
            {
                m_pBtnQuit = std::make_shared<UIButton>();
                if (m_pBtnQuit->LoadFromProperty(btnQuitProp))
                {
                    m_pBtnQuit->CreateLayer(gr, 150);
                    m_pBtnQuit->SetClickCallback([this]() { OnQuitButtonClick(); });
                    m_uiManager.AddElement("btnQuit", m_pBtnQuit);
                }
            }

            // Load additional buttons based on CUITitle::OnCreate
            // BtEmailSave - (27, 97) - Save email checkbox (based on CCtrlButton checkbox mode)
            auto btnEmailSaveProp = newTitleProp->GetChild("BtEmailSave");
            if (btnEmailSaveProp)
            {
                m_pBtnEmailSave = std::make_shared<UIButton>();
                if (m_pBtnEmailSave->LoadFromProperty(btnEmailSaveProp))
                {
                    m_pBtnEmailSave->SetCheckMode(true);  // Enable checkbox behavior
                    m_pBtnEmailSave->SetChecked(false);   // Default unchecked
                    m_pBtnEmailSave->CreateLayer(gr, 150);

                    // Load checkmark canvases from Title_new/check
                    auto checkProp = newTitleProp->GetChild("check");
                    if (checkProp)
                    {
                        auto check0Prop = checkProp->GetChild("0");
                        auto check1Prop = checkProp->GetChild("1");
                        if (check0Prop)
                        {
                            auto tmpWzCanvas_433 = check0Prop->GetCanvas();
                            m_pCanvasCheck0 = tmpWzCanvas_433 ? std::make_shared<WzGr2DCanvas>(tmpWzCanvas_433) : nullptr;
                        }
                        if (check1Prop)
                        {
                            auto tmpWzCanvas_437 = check1Prop->GetCanvas();
                            m_pCanvasCheck1 = tmpWzCanvas_437 ? std::make_shared<WzGr2DCanvas>(tmpWzCanvas_437) : nullptr;
                        }

                        // Create checkmark layer (position relative to button)
                        // The checkmark is typically displayed to the left of the button text
                        if (m_pCanvasCheck0 || m_pCanvasCheck1)
                        {
                            auto checkCanvas = m_pCanvasCheck0 ? m_pCanvasCheck0 : m_pCanvasCheck1;
                            m_pLayerEmailCheck = gr.CreateLayer(
                                0, 0,
                                checkCanvas->GetWidth(),
                                checkCanvas->GetHeight(),
                                151  // Slightly above button
                            );
                            if (m_pLayerEmailCheck)
                            {
                                // Start with unchecked state (check/0)
                                if (m_pCanvasCheck0)
                                {
                                    m_pLayerEmailCheck->InsertCanvas(m_pCanvasCheck0, 0, 255, 255);
                                }
                            }
                        }
                    }

                    m_pBtnEmailSave->SetClickCallback([this]() {
                        // Toggle remember email address (based on CUITitle::ToggleRememberMailAddr)
                        LOG_DEBUG("Email save checkbox toggled: {}", m_pBtnEmailSave->IsChecked());

                        // Update checkmark layer
                        if (m_pLayerEmailCheck)
                        {
                            m_pLayerEmailCheck->RemoveAllCanvases();
                            if (m_pBtnEmailSave->IsChecked() && m_pCanvasCheck1)
                            {
                                m_pLayerEmailCheck->InsertCanvas(m_pCanvasCheck1, 0, 255, 255);
                            }
                            else if (m_pCanvasCheck0)
                            {
                                m_pLayerEmailCheck->InsertCanvas(m_pCanvasCheck0, 0, 255, 255);
                            }
                        }
                    });
                    m_uiManager.AddElement("btnEmailSave", m_pBtnEmailSave);
                }
            }

            // BtEmailLost - (99, 97) - Lost email button
            auto btnEmailLostProp = newTitleProp->GetChild("BtEmailLost");
            if (btnEmailLostProp)
            {
                m_pBtnEmailLost = std::make_shared<UIButton>();
                if (m_pBtnEmailLost->LoadFromProperty(btnEmailLostProp))
                {
                    m_pBtnEmailLost->CreateLayer(gr, 150);
                    m_uiManager.AddElement("btnEmailLost", m_pBtnEmailLost);
                }
            }

            // BtPasswdLost - (171, 97) - Lost password button
            auto btnPasswdLostProp = newTitleProp->GetChild("BtPasswdLost");
            if (btnPasswdLostProp)
            {
                m_pBtnPasswdLost = std::make_shared<UIButton>();
                if (m_pBtnPasswdLost->LoadFromProperty(btnPasswdLostProp))
                {
                    m_pBtnPasswdLost->CreateLayer(gr, 150);
                    m_uiManager.AddElement("btnPasswdLost", m_pBtnPasswdLost);
                }
            }

            // BtNew - (15, 117) - New account button
            auto btnNewProp = newTitleProp->GetChild("BtNew");
            if (btnNewProp)
            {
                m_pBtnNew = std::make_shared<UIButton>();
                if (m_pBtnNew->LoadFromProperty(btnNewProp))
                {
                    m_pBtnNew->CreateLayer(gr, 150);
                    m_uiManager.AddElement("btnNew", m_pBtnNew);
                }
            }

            // BtHomePage - (87, 117) - Homepage button
            auto btnHomePageProp = newTitleProp->GetChild("BtHomePage");
            if (btnHomePageProp)
            {
                m_pBtnHomePage = std::make_shared<UIButton>();
                if (m_pBtnHomePage->LoadFromProperty(btnHomePageProp))
                {
                    m_pBtnHomePage->CreateLayer(gr, 150);
                    m_uiManager.AddElement("btnHomePage", m_pBtnHomePage);
                }
            }

            // Create Edit fields for ID and Password
            // EditID - (14, 43) - size (163, 24) - nHorzMax=64
            m_pEditID = std::make_shared<UIEdit>();
            m_pEditID->SetSize(163, 24);
            m_pEditID->SetMaxLength(64);
            m_pEditID->SetTextOffset(6, 6);
            m_pEditID->SetFontColor(0xFF5D7E3D);  // From CCtrlEdit::CREATEPARAM

            // Load placeholder canvas for ID field (UI/Login.img/title/ID)
            auto idPlaceholderProp = newTitleProp->GetChild("ID");
            if (idPlaceholderProp)
            {
                auto wzCanvas = idPlaceholderProp->GetCanvas();
        auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas) : nullptr;
                if (!canvas)
                {
                    auto childProp = idPlaceholderProp->GetChild("0");
                    if (childProp)
                    {
                        auto tmpWzCanvas = childProp->GetCanvas();
                        canvas = tmpWzCanvas ? std::make_shared<WzGr2DCanvas>(tmpWzCanvas) : nullptr;
                    }
                }
                if (canvas)
                {
                    m_pEditID->SetPlaceholderCanvas(canvas);
                    LOG_DEBUG("ID placeholder loaded: {}x{}", canvas->GetWidth(), canvas->GetHeight());
                }
            }

            m_pEditID->CreateLayer(gr, 150);
            m_pEditID->SetEnterPressedCallback([this](const std::string&) {
                // Move focus to password field when Enter is pressed in ID field
                if (m_pEditPasswd)
                {
                    m_uiManager.SetFocusedElement(m_pEditPasswd);
                }
            });
            m_uiManager.AddElement("editID", m_pEditID);

            // EditPasswd - (14, 69) - size (163, 24) - nHorzMax=12, bPasswd=1
            m_pEditPasswd = std::make_shared<UIEdit>(); 
            m_pEditPasswd->SetSize(163, 24);
            m_pEditPasswd->SetMaxLength(12);  // 12 for MapleID, 16 for NexonID
            m_pEditPasswd->SetPasswordMode(true);
            m_pEditPasswd->SetTextOffset(6, 6);
            m_pEditPasswd->SetFontColor(0xFF5D7E3D);

            // Load placeholder canvas for Password field (UI/Login.img/title/PW)
            auto pwPlaceholderProp = newTitleProp->GetChild("PW");
            if (pwPlaceholderProp)
            {
                auto wzCanvas = pwPlaceholderProp->GetCanvas();
        auto canvas = wzCanvas ? std::make_shared<WzGr2DCanvas>(wzCanvas) : nullptr;
                if (!canvas)
                {
                    auto childProp = pwPlaceholderProp->GetChild("0");
                    if (childProp)
                    {
                        auto tmpWzCanvas = childProp->GetCanvas();
                        canvas = tmpWzCanvas ? std::make_shared<WzGr2DCanvas>(tmpWzCanvas) : nullptr;
                    }
                }
                if (canvas)
                {
                    m_pEditPasswd->SetPlaceholderCanvas(canvas);
                    LOG_DEBUG("PW placeholder loaded: {}x{}", canvas->GetWidth(), canvas->GetHeight());
                }
            }

            m_pEditPasswd->CreateLayer(gr, 150);
            m_pEditPasswd->SetEnterPressedCallback([this](const std::string&) {
                // Trigger login when Enter is pressed in password field
                OnLoginButtonClick();
            });
            m_uiManager.AddElement("editPasswd", m_pEditPasswd);

            // Set initial focus to ID field
            m_uiManager.SetFocusedElement(m_pEditID);
        }
    }

    // Create placeholder buttons if WZ loading failed
    if (!hasWzButtons)
    {
        LOG_DEBUG("Creating placeholder buttons...");

        const int btnWidth = 100;
        const int btnHeight = 40;

        // Login button
        m_pBtnLogin = std::make_shared<UIButton>();
        auto loginWzCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
    auto loginCanvas = std::make_shared<WzGr2DCanvas>(loginWzCanvas);
        {
            std::vector<std::uint8_t> pixels(static_cast<std::size_t>(btnWidth * btnHeight * 4));
            for (int y = 0; y < btnHeight; ++y)
            {
                for (int x = 0; x < btnWidth; ++x)
                {
                    auto idx = static_cast<std::size_t>((y * btnWidth + x) * 4);
                    float t = static_cast<float>(y) / static_cast<float>(btnHeight);
                    pixels[idx + 0] = static_cast<std::uint8_t>(255 - 80 * t);  // R
                    pixels[idx + 1] = static_cast<std::uint8_t>(180 - 60 * t);  // G
                    pixels[idx + 2] = static_cast<std::uint8_t>(80 - 30 * t);   // B
                    pixels[idx + 3] = 255;
                }
            }
            loginWzCanvas->SetPixelData(std::move(pixels));
        }
        m_pBtnLogin->SetStateCanvas(UIState::Normal, loginCanvas);
        m_pBtnLogin->SetSize(btnWidth, btnHeight);

        // MouseOver state (brighter)
        auto loginOverWzCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
    auto loginOverCanvas = std::make_shared<WzGr2DCanvas>(loginOverWzCanvas);
        {
            std::vector<std::uint8_t> pixels(static_cast<std::size_t>(btnWidth * btnHeight * 4));
            for (int y = 0; y < btnHeight; ++y)
            {
                for (int x = 0; x < btnWidth; ++x)
                {
                    auto idx = static_cast<std::size_t>((y * btnWidth + x) * 4);
                    float t = static_cast<float>(y) / static_cast<float>(btnHeight);
                    pixels[idx + 0] = 255;
                    pixels[idx + 1] = static_cast<std::uint8_t>(200 - 40 * t);
                    pixels[idx + 2] = static_cast<std::uint8_t>(100 - 20 * t);
                    pixels[idx + 3] = 255;
                }
            }
            loginOverWzCanvas->SetPixelData(std::move(pixels));
        }
        m_pBtnLogin->SetStateCanvas(UIState::MouseOver, loginOverCanvas);

        // Pressed state (darker)
        auto loginPressWzCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
    auto loginPressCanvas = std::make_shared<WzGr2DCanvas>(loginPressWzCanvas);
        {
            std::vector<std::uint8_t> pixels(static_cast<std::size_t>(btnWidth * btnHeight * 4));
            for (int y = 0; y < btnHeight; ++y)
            {
                for (int x = 0; x < btnWidth; ++x)
                {
                    auto idx = static_cast<std::size_t>((y * btnWidth + x) * 4);
                    float t = static_cast<float>(y) / static_cast<float>(btnHeight);
                    pixels[idx + 0] = static_cast<std::uint8_t>(200 - 60 * t);
                    pixels[idx + 1] = static_cast<std::uint8_t>(140 - 40 * t);
                    pixels[idx + 2] = static_cast<std::uint8_t>(60 - 20 * t);
                    pixels[idx + 3] = 255;
                }
            }
            loginPressWzCanvas->SetPixelData(std::move(pixels));
        }
        m_pBtnLogin->SetStateCanvas(UIState::Pressed, loginPressCanvas);

        m_pBtnLogin->CreateLayer(gr, 150);
        m_pBtnLogin->SetClickCallback([this]() { OnLoginButtonClick(); });
        m_uiManager.AddElement("btnLogin", m_pBtnLogin);

        // Quit button (gray)
        m_pBtnQuit = std::make_shared<UIButton>();
        auto quitWzCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
    auto quitCanvas = std::make_shared<WzGr2DCanvas>(quitWzCanvas);
        {
            std::vector<std::uint8_t> pixels(static_cast<std::size_t>(btnWidth * btnHeight * 4));
            for (int y = 0; y < btnHeight; ++y)
            {
                for (int x = 0; x < btnWidth; ++x)
                {
                    auto idx = static_cast<std::size_t>((y * btnWidth + x) * 4);
                    float t = static_cast<float>(y) / static_cast<float>(btnHeight);
                    auto gray = static_cast<std::uint8_t>(140 - 40 * t);
                    pixels[idx + 0] = gray;
                    pixels[idx + 1] = gray;
                    pixels[idx + 2] = gray;
                    pixels[idx + 3] = 255;
                }
            }
            quitWzCanvas->SetPixelData(std::move(pixels));
        }
        m_pBtnQuit->SetStateCanvas(UIState::Normal, quitCanvas);
        m_pBtnQuit->SetSize(btnWidth, btnHeight);

        // MouseOver state
        auto quitOverWzCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
    auto quitOverCanvas = std::make_shared<WzGr2DCanvas>(quitOverWzCanvas);
        {
            std::vector<std::uint8_t> pixels(static_cast<std::size_t>(btnWidth * btnHeight * 4));
            for (int y = 0; y < btnHeight; ++y)
            {
                for (int x = 0; x < btnWidth; ++x)
                {
                    auto idx = static_cast<std::size_t>((y * btnWidth + x) * 4);
                    float t = static_cast<float>(y) / static_cast<float>(btnHeight);
                    auto gray = static_cast<std::uint8_t>(170 - 30 * t);
                    pixels[idx + 0] = gray;
                    pixels[idx + 1] = gray;
                    pixels[idx + 2] = gray;
                    pixels[idx + 3] = 255;
                }
            }
            quitOverWzCanvas->SetPixelData(std::move(pixels));
        }
        m_pBtnQuit->SetStateCanvas(UIState::MouseOver, quitOverCanvas);

        // Pressed state
        auto quitPressWzCanvas = std::make_shared<WzCanvas>(btnWidth, btnHeight);
    auto quitPressCanvas = std::make_shared<WzGr2DCanvas>(quitPressWzCanvas);
        {
            std::vector<std::uint8_t> pixels(static_cast<std::size_t>(btnWidth * btnHeight * 4));
            for (int y = 0; y < btnHeight; ++y)
            {
                for (int x = 0; x < btnWidth; ++x)
                {
                    auto idx = static_cast<std::size_t>((y * btnWidth + x) * 4);
                    float t = static_cast<float>(y) / static_cast<float>(btnHeight);
                    auto gray = static_cast<std::uint8_t>(100 - 20 * t);
                    pixels[idx + 0] = gray;
                    pixels[idx + 1] = gray;
                    pixels[idx + 2] = gray;
                    pixels[idx + 3] = 255;
                }
            }
            quitPressWzCanvas->SetPixelData(std::move(pixels));
        }
        m_pBtnQuit->SetStateCanvas(UIState::Pressed, quitPressCanvas);

        m_pBtnQuit->CreateLayer(gr, 150);
        m_pBtnQuit->SetClickCallback([this]() { OnQuitButtonClick(); });
        m_uiManager.AddElement("btnQuit", m_pBtnQuit);

        LOG_DEBUG("Placeholder buttons created");
    }
}

void Login::SetupStep1UI()
{
    ClearStepUI();
    auto& gr = get_gr();

    LOG_DEBUG("Step 1 UI (World Selection) - Creating UIWorldSelect");

    // Reset state
    m_nCharSelected = -1;
    m_nCharCount = 0;
    m_sSPW.clear();
    m_sGoToStarPlanetSPW.clear();

    // Create UIWorldSelect and initialize with new lifecycle pattern
    m_worldSelectUI = std::make_unique<UIWorldSelect>();

    // Create params
    UIWorldSelect::CreateParams params;
    params.login = this;
    params.gr = &gr;
    params.uiManager = &m_uiManager;

    // Call Create() and check result
    auto result = m_worldSelectUI->Create(params);
    if (!result)
    {
        LOG_ERROR("Failed to create UIWorldSelect: {}", result.error());
        m_worldSelectUI.reset();
        return;
    }

    LOG_DEBUG("UIWorldSelect created successfully");
}

void Login::SetupStep2UI()
{
    ClearStepUI();
    auto& gr = get_gr();

    LOG_DEBUG("Step 2 UI (Character Selection) - Creating UISelectChar");

    // Reset race selection
    m_nCurSelectedRace = 0;
    m_nCurSelectedSubJob = 0;

    // Create UISelectChar and initialize with new lifecycle pattern
    m_selectCharUI = std::make_unique<UISelectChar>();

    // Create params
    UISelectChar::CreateParams params;
    params.login = this;
    params.gr = &gr;
    params.uiManager = &m_uiManager;

    // Call Create() and check result
    auto result = m_selectCharUI->Create(params);
    if (!result)
    {
        LOG_ERROR("Failed to create UISelectChar: {}", result.error());
        m_selectCharUI.reset();
        return;
    }

    LOG_DEBUG("UISelectChar created successfully");
}

void Login::SetupStep3UI()
{
    ClearStepUI();
    auto& gr = get_gr();

    LOG_DEBUG("Step 3 UI (Race Selection) - Creating UINewCharRaceSelect");

    // Reset race selection
    m_nCurSelectedRace = 0;
    m_nCurSelectedSubJob = 0;

    // Create UINewCharRaceSelect and initialize with new lifecycle pattern
    m_raceSelectUI = std::make_unique<UINewCharRaceSelect>();

    // Create params
    UINewCharRaceSelect::CreateParams params;
    params.login = this;
    params.gr = &gr;
    params.uiManager = &m_uiManager;

    // Call Create() and check result
    auto result = m_raceSelectUI->Create(params);
    if (!result)
    {
        LOG_ERROR("Failed to create UINewCharRaceSelect: {}", result.error());
        m_raceSelectUI.reset();
        return;
    }

    LOG_DEBUG("UINewCharRaceSelect created successfully");
}

void Login::SetupStep4UI()
{
    ClearStepUI();
    // Character creation appearance UI
    LOG_DEBUG("Step 4 UI (Character Creation) - Creating appearance UI");

    // Clear checked name
    m_sCheckedName.clear();

    // Initialize new avatar with current gender and frame
    InitNewAvatar(m_nAccountGender, 0);

    // Set sub-step based on whether gender is choosable
    if (!m_bCharSale)
    {
        m_nSubStep = m_bChoosableGender ? 1 : 3;
    }
    else
    {
        m_nSubStep = 0;
    }
    m_bSubStepChanged = true;
}

void Login::ClearStepUI()
{
    m_uiManager.Clear();

    auto& gr = get_gr();

    // Remove button layers from graphics engine
    if (m_pBtnLogin && m_pBtnLogin->GetLayer())
    {
        gr.RemoveLayer(m_pBtnLogin->GetLayer());
    }
    if (m_pBtnQuit && m_pBtnQuit->GetLayer())
    {
        gr.RemoveLayer(m_pBtnQuit->GetLayer());
    }
    if (m_pBtnEmailSave && m_pBtnEmailSave->GetLayer())
    {
        gr.RemoveLayer(m_pBtnEmailSave->GetLayer());
    }
    if (m_pBtnEmailLost && m_pBtnEmailLost->GetLayer())
    {
        gr.RemoveLayer(m_pBtnEmailLost->GetLayer());
    }
    if (m_pBtnPasswdLost && m_pBtnPasswdLost->GetLayer())
    {
        gr.RemoveLayer(m_pBtnPasswdLost->GetLayer());
    }
    if (m_pBtnNew && m_pBtnNew->GetLayer())
    {
        gr.RemoveLayer(m_pBtnNew->GetLayer());
    }
    if (m_pBtnHomePage && m_pBtnHomePage->GetLayer())
    {
        gr.RemoveLayer(m_pBtnHomePage->GetLayer());
    }

    // Remove edit field layers
    if (m_pEditID && m_pEditID->GetLayer())
    {
        gr.RemoveLayer(m_pEditID->GetLayer());
    }
    if (m_pEditPasswd && m_pEditPasswd->GetLayer())
    {
        gr.RemoveLayer(m_pEditPasswd->GetLayer());
    }

    // Remove checkbox layer
    if (m_pLayerEmailCheck)
    {
        gr.RemoveLayer(m_pLayerEmailCheck);
        m_pLayerEmailCheck.reset();
    }

    // Remove step 0 title dialog background layer (Title_new/backgrd)
    auto titleBgLayer = GetObjectLayer("titleDialogBg");
    if (titleBgLayer)
    {
        gr.RemoveLayer(titleBgLayer);
        // Remove from the object layer map
        m_mpLayerObj.erase("titleDialogBg");
    }

    // Reset all UI element pointers
    m_pBtnLogin.reset();
    m_pBtnQuit.reset();
    m_pBtnEmailSave.reset();
    m_pBtnEmailLost.reset();
    m_pBtnPasswdLost.reset();
    m_pBtnNew.reset();
    m_pBtnHomePage.reset();
    m_pEditID.reset();
    m_pEditPasswd.reset();
    m_pCanvasCheck0.reset();
    m_pCanvasCheck1.reset();

    // Clean up Step 1 UI (UIWorldSelect)
    if (m_worldSelectUI)
    {
        if (m_worldSelectUI->IsCreated())
        {
            m_worldSelectUI->Destroy();
        }
        m_worldSelectUI.reset();
    }

    // Clean up Step 2 UI (UISelectChar)
    if (m_selectCharUI)
    {
        if (m_selectCharUI->IsCreated())
        {
            m_selectCharUI->Destroy();
        }
        m_selectCharUI.reset();
    }

    // Clean up Step 3 UI (UINewCharRaceSelect)
    if (m_raceSelectUI)
    {
        if (m_raceSelectUI->IsCreated())
        {
            m_raceSelectUI->Destroy();
        }
        m_raceSelectUI.reset();
    }
}

void Login::DestroyUICharNameSelectAll()
{
    // Destroy all character name selection UIs
    // This includes CUINewCharNameSelect and all its variants
}

void Login::CloseLoginDescWnd(FadeWnd* pExcept)
{
    // Close login description windows (except specified one)
    if (m_pLoginDesc0 && m_pLoginDesc0.get() != pExcept)
    {
        // Close the window
        m_pLoginDesc0.reset();
    }
    if (m_pLoginDesc1 && m_pLoginDesc1.get() != pExcept)
    {
        m_pLoginDesc1.reset();
    }
}

void Login::OnLoginButtonClick()
{
    LOG_DEBUG("Login button clicked!");

    // Transition to step 1 (world selection)
    ChangeStep(1);
}

void Login::OnQuitButtonClick()
{
    LOG_DEBUG("Quit button clicked!");
    m_bTerminate = true;
}

void Login::OnServerButtonClick()
{
    LOG_DEBUG("Server button clicked!");
}

void Login::OnMouseMove(std::int32_t x, std::int32_t y)
{
    m_uiManager.OnMouseMove(x, y);
}

void Login::OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button)
{
    m_uiManager.OnMouseDown(x, y, button);
}

void Login::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button)
{
    m_uiManager.OnMouseUp(x, y, button);
}

void Login::OnKeyDown(std::int32_t keyCode)
{
    m_uiManager.OnKeyDown(keyCode);

    // ESC to quit or go back
    if (keyCode == 27) // SDLK_ESCAPE
    {
        if (m_nLoginStep == 0)
        {
            m_bTerminate = true;
        }
        else if (m_nLoginStep > 0 && m_nLoginStep <= 2)
        {
            // Go back to previous step
            ChangeStep(m_nLoginStep - 1);
        }
    }
}

void Login::OnTextInput(const std::string& text)
{
    m_uiManager.OnTextInput(text);
}

void Login::Update()
{
    // Call base class update (handles layer updates)
    MapLoadable::Update();

    // Check termination
    if (m_bTerminate)
    {
        // Post quit message - in real implementation this signals the app to quit
        // ZAPI.PostQuitMessage(0);
        return;
    }

    const auto tNow = Application::GetTick();

    // Handle world info request (for login step 1)
    if (m_nLoginStep == 1 && !m_bWorldInfoRequest)
    {
        const auto tWorldInfoRequest = m_tWorldInfoRequest;
        // Request world info every 3 seconds if we don't have shining star count
        if ((tWorldInfoRequest == 0 || (tNow - tWorldInfoRequest > 3000)) && m_nShiningStarCount < 0)
        {
            m_bWorldInfoRequest = true;
            m_tWorldInfoRequest = tNow;
            SendWorldInfoForShiningRequest();
        }
    }

    // Handle character selection and login
    if (m_nCharSelected >= 0 && m_nLoginStep == 2)
    {
        if (!m_sSPW.empty() && !m_bRequestSent)
        {
            // Send select character packet
            LOG_DEBUG("Sending character select request...");
            m_bRequestSent = true;
            m_sSPW.clear();
            m_sGoToStarPlanetSPW.clear();
            m_bOfflineMode = false;
        }
    }

    // Handle fade out transition
    if (m_tStartFadeOut != 0)
    {
        auto elapsed = static_cast<std::int64_t>(tNow - m_tStartFadeOut);
        if (elapsed > 0)
        {
            ChangeStepImmediate();
            m_nFadeOutLoginStep = -1;
            m_tStartFadeOut = 0;
        }
    }

    // Handle step changing completion
    if (m_tStepChanging != 0)
    {
        auto elapsed = static_cast<std::int64_t>(tNow - m_tStepChanging);
        if (elapsed > 0)
        {
            OnStepChanged();
            m_tStepChanging = 0;
        }
    }

    // Handle sub-step changes for character creation
    if (m_bSubStepChanged)
    {
        LOG_DEBUG("Sub-step changed to: {}", m_nSubStep);
        switch (m_nSubStep)
        {
        case 0: // Job selection
            // Create CUINewCharJobSelect
            break;
        case 1: // Gender selection
            // Create CUINewCharGenderSelect
            break;
        case 2: // Frame/equipment selection
            // Create CUINewCharEquipFrameSelect
            break;
        case 3: // Avatar customization
            // Create CUINewCharAvatarSelectCommon
            break;
        case 4: // Name input
            // Create CUINewCharNameSelect
            break;
        default:
            break;
        }
        m_bSubStepChanged = false;
    }

    // Clean up light/dust layers when not in character selection
    if (m_nLoginStep != 2)
    {
        if (m_pLayerLight)
        {
            get_gr().RemoveLayer(m_pLayerLight);
            m_pLayerLight.reset();
        }
        if (m_pLayerDust)
        {
            get_gr().RemoveLayer(m_pLayerDust);
            m_pLayerDust.reset();
        }
    }

    // Clean up new avatar when not in character creation
    if (m_pNewAvatar && m_nLoginStep != 4)
    {
        m_pNewAvatar.reset();
    }
    if (m_pNewDummyAvatar && m_nLoginStep != 4)
    {
        m_pNewDummyAvatar.reset();
    }

    // Check go to star planet
    CheckGoToStarPlanet();

    // Update banner
    UpdateBanner();

    // Update UI manager
    m_uiManager.Update();
}

void Login::Draw()
{
    // All rendering removed - layers and UI are handled by the base class
}

void Login::Close()
{
    // Destroy all UI elements
    ClearStepUI();
    DestroyUICharNameSelectAll();
    CloseLoginDescWnd(nullptr);

    // Clear login start
    if (m_pLoginStart)
    {
        m_pLoginStart.reset();
    }

    // Clear login-specific layer pointers
    if (m_pLayerBackground)
    {
        get_gr().RemoveLayer(m_pLayerBackground);
        m_pLayerBackground.reset();
    }
    if (m_pLayerBook)
    {
        get_gr().RemoveLayer(m_pLayerBook);
        m_pLayerBook.reset();
    }
    if (m_pLayerLight)
    {
        get_gr().RemoveLayer(m_pLayerLight);
        m_pLayerLight.reset();
    }
    if (m_pLayerDust)
    {
        get_gr().RemoveLayer(m_pLayerDust);
        m_pLayerDust.reset();
    }

    // Clear WZ properties
    m_pLoginImgProp.reset();
    m_pPropChangeStepBGM.reset();

    // Flush cached objects
    auto& resMan = WzResMan::GetInstance();
    resMan.FlushCachedObjects(0);

    // Call base class close (cleans up all layers)
    MapLoadable::Close();

    LOG_DEBUG("Login stage closed");
}

void Login::ChangeStep(std::int32_t nStep)
{
    // If already in fade out, complete it immediately
    if (m_tStartFadeOut != 0)
    {
        ChangeStepImmediate();
    }

    const auto nPrevStep = m_nLoginStep;
    m_nFadeOutLoginStep = nPrevStep;

    // Handle step value
    auto nNewStep = nStep;
    if (nStep < 0)
    {
        nNewStep = (nPrevStep + 1) % 5;
    }
    m_nLoginStep = nNewStep;

    // Special handling for step 3 (race selection)
    if (nNewStep == 3)
    {
        if (m_bEventNewChar)
        {
            // Event new character - skip to step 4 with predetermined race
            m_nCurSelectedRace = m_nEventNewCharJob;  // Simplified
            m_nCurSelectedSubJob = 0;
            m_nLoginStep = 4;
        }
        else if (m_nMakeShiningStar == 2)
        {
            m_nCurSelectedRace = -1;
            m_nCurSelectedSubJob = 0;
            m_nLoginStep = 4;
        }
    }

    // Reset world info on world select (based on decompiled code at 0xb65f7a)
    if (m_nLoginStep <= 1)
    {
        WvsContext::GetInstance().ResetWorldInfoOnWorldSelect();
    }

    // Schedule step change with fade animation if step actually changed
    if (m_nFadeOutLoginStep != m_nLoginStep)
    {
        const auto tNow = Application::GetTick();

        // Register fade animation
        // CAnimationDisplayer::RegisterFadeInOutAnimation(200, 0, 200, 22, 255, 0xFF000000);

        auto tChange = tNow + 200;
        if (tNow == static_cast<std::uint64_t>(-200))
        {
            tChange = 1;
        }
        m_tStepChanging = tChange;

        auto tFadeOut = tNow + 200;
        if (tFadeOut == 0)
        {
            tFadeOut = 1;
        }
        m_tStartFadeOut = tFadeOut;
    }

}

void Login::ChangeStepImmediate()
{
    const auto nFadeOutStep = m_nFadeOutLoginStep;
    const auto nCurStep = m_nLoginStep;

    if (nFadeOutStep == nCurStep)
    {
        return;
    }

    // Update button visibility based on current step
    // ...

    // Close login description window if transitioning between certain steps
    if (m_pLoginDesc0)
    {
        bool shouldClose = true;
        if ((nFadeOutStep == 3 && nCurStep == 4) || (nFadeOutStep == 4 && nCurStep == 3))
        {
            shouldClose = false;
        }
        if (shouldClose)
        {
            m_pLoginDesc0.reset();
        }
    }

    // Handle step-specific transitions
    switch (nCurStep)
    {
    case 0:
        // Title screen
        m_bRequestSent = false;
        if (m_pLoginDesc1)
        {
            m_pLoginDesc1.reset();
        }
        m_nCurSelectedRace = 0;
        m_nCurSelectedSubJob = 0;
        m_bOpenedNotActiveAccountDlg = false;
        break;

    case 1:
        // World selection
        m_nCharSelected = -1;
        m_nCharCount = 0;
        m_sGoToStarPlanetSPW.clear();
        m_sSPW.clear();
        if (m_pLoginDesc1)
        {
            m_pLoginDesc1.reset();
        }
        m_bGotoWorldSelect = false;
        m_nCurSelectedRace = 0;
        m_nCurSelectedSubJob = 0;
        break;

    case 2:
        // Character selection
        m_nCurSelectedRace = 0;
        m_nCurSelectedSubJob = 0;
        break;

    case 3:
        // Race selection
        if (m_pLoginDesc1 && nFadeOutStep <= 2)
        {
            m_pLoginDesc1.reset();
        }
        // Create race selection UI
        m_nCurSelectedRace = 0;
        m_nCurSelectedSubJob = 0;
        break;

    case 4:
        // Character creation
        if (m_pLoginDesc1)
        {
            m_pLoginDesc1.reset();
        }
        m_sCheckedName.clear();
        InitNewAvatar(m_nAccountGender, 0);
        break;

    default:
        break;
    }

    // Update camera/view position based on step
    // Based on CLogin::Init: X = 28, Y = -8 - 600 * (step + race_ui_offset)
    // Camera looks at this world coordinate (renderer adds screen center offset)
    auto& gr = get_gr();
    auto raceUIOffset = (m_nLoginStep == 4) ? ConvertSelectedRaceToUIRace() : 0;
    auto stepY = -8 - 600 * (m_nLoginStep + raceUIOffset);

    gr.SetCameraPosition(28, stepY);

    LOG_DEBUG("Camera set to ({}, {}) for step {}", 28, stepY, m_nLoginStep);

    // Play step change sound
    // play_ui_sound("BtMouseClick");
}

void Login::OnStepChanged()
{
    LOG_INFO("Login step changed to: {}", m_nLoginStep);

    // Handle UI transitions when login step changes
    switch (m_nLoginStep)
    {
    case 0:
        // Destroy all other UIs
        SetupStep0UI();
        m_nCurSelectedRace = 0;
        m_nCurSelectedSubJob = 0;
        break;

    case 1:
        // Create world selection UI
        SetupStep1UI();
        m_nCurSelectedRace = 0;
        m_nCurSelectedSubJob = 0;
        break;

    case 2:
        // Create character selection UI
        SetupStep2UI();
        m_nCurSelectedRace = 0;
        m_nCurSelectedSubJob = 0;
        break;

    case 3:
        // Create race selection UI
        SetupStep3UI();
        m_nCurSelectedRace = 0;
        m_nCurSelectedSubJob = 0;
        break;

    case 4:
        // Create character creation UI
        SetupStep4UI();
        break;

    default:
        break;
    }
}

void Login::SendWorldInfoRequest()
{
    m_bWorldInfoRequest = false;
    m_tWorldInfoRequest = 0;
}

void Login::SendWorldInfoForShiningRequest()
{
    LOG_DEBUG("Sending world info for shining request...");
    // Send packet to request world info for shining star
}

void Login::OnWorldInfoReceived()
{
    m_bWorldInfoRequest = true;
    // Update world list UI
}

void Login::GotoWorldSelect()
{
    // Based on CLogin::GotoWorldSelect @ 0xb66a10
    // Only allow going back to world select from step 2+ and not during step change
    if (m_nLoginStep > 1 && m_tStepChanging == 0)
    {
        // Clear relogin cookie (in online mode, this would clear WvsContext::m_sReloginCookie)
        // For now, just log the action
        LOG_DEBUG("GotoWorldSelect: clearing relogin cookie and world items");

        // Clear world items to force refresh
        m_vWorldItem.clear();

        // In online mode: Send world info request packet (opcode 117)
        // COutPacket::COutPacket(&oPacket, 117);
        // CClientSocket::SendPacket(...);
        LOG_DEBUG("GotoWorldSelect: would send world info request packet (opcode 117)");

        m_bGotoWorldSelect = true;
        m_bWorldInfoRequest = false;

        // Change to step 1 (world selection)
        ChangeStep(1);
    }
}

void Login::InitWorldItemFinal()
{
    // Initialize final world item list (from CLogin::InitWorldItemFinal @ 0xb6ef30)
    // Uses hardcoded world IDs matching original MapleStory v1029 client
    m_vWorldItemFinal.clear();
    m_vWorldItemFinalReboot.clear();

    // Check if offline mode is enabled
    auto& config = Configuration::GetInstance();
    m_bOfflineMode = config.IsOfflineMode();

    if (m_bOfflineMode)
    {
        // Generate worlds with original world IDs (from decompiled code)
        LOG_DEBUG("Initializing offline worlds with original IDs");

        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> worldStateDist(0, 3);  // 0=normal, 1=event, 2=new, 3=hot
        std::uniform_int_distribution<> channelCountDist(10, 20);
        std::uniform_int_distribution<> channelLoadDist(10, 90);

        // Original world IDs from CLogin::InitWorldItemFinal @ 0xb6ef30
        // Format: {worldID, name}
        const std::vector<std::pair<std::int32_t, std::string>> kDefaultWorlds = {
            {0, "Scania"},
            {1, "Bera"},
            {3, "Broa"},      // Note: ID 2 is skipped
            {4, "Windia"},
            {5, "Khaini"},
            {10, "Demethos"}, // Note: IDs 6-9 are skipped
            {16, "Galicia"},  // Note: IDs 11-15 are skipped
            {29, "Renegades"},
            {43, "Arcania"},
            {44, "Zenith"},
            {99, "Nova"},
        };

        // Reboot world goes to separate list (ID 45)
        const std::pair<std::int32_t, std::string> kRebootWorld = {45, "Reboot"};

        // Add main worlds
        for (const auto& [worldId, worldName] : kDefaultWorlds)
        {
            WorldItem world;
            world.nWorldID = worldId;
            world.sName = worldName;
            world.nWorldState = worldStateDist(gen);

            // Generate event description for event worlds
            if (world.nWorldState == 1)
            {
                world.sEventDesc = "2x EXP Event!";
                world.nEventEXP = 200;
                world.nEventDrop = 150;
            }

            world.nBlockCharCreation = 0;

            // Generate random channel loads
            const int numChannels = channelCountDist(gen);
            for (int c = 0; c < numChannels; ++c)
            {
                world.aChannelLoad.push_back(channelLoadDist(gen));
            }

            m_vWorldItemFinal.push_back(world);
            LOG_DEBUG("Added world: {} (ID={}, state={}, channels={})",
                      world.sName, world.nWorldID, world.nWorldState, numChannels);
        }

        // Add Reboot world to separate list
        {
            WorldItem world;
            world.nWorldID = kRebootWorld.first;
            world.sName = kRebootWorld.second;
            world.nWorldState = 0;  // Reboot is always normal state
            world.nBlockCharCreation = 0;

            const int numChannels = channelCountDist(gen);
            for (int c = 0; c < numChannels; ++c)
            {
                world.aChannelLoad.push_back(channelLoadDist(gen));
            }

            m_vWorldItemFinalReboot.push_back(world);
            LOG_DEBUG("Added Reboot world: {} (ID={}, channels={})",
                      world.sName, world.nWorldID, numChannels);
        }

        return;
    }

    // Copy from m_vWorldItem with filtering (normal online mode)
    for (const auto& item : m_vWorldItem)
    {
        // Separate Reboot worlds (ID 45) to separate list
        if (item.nWorldID == 45)
        {
            m_vWorldItemFinalReboot.push_back(item);
        }
        else
        {
            m_vWorldItemFinal.push_back(item);
        }
    }
}

void Login::UpdateBanner()
{
    // Update any animated banner elements
}

void Login::CheckGoToStarPlanet()
{
    // Check if we should transition to Star Planet
}

auto Login::ConvertSelectedRaceToUIRace() const -> std::int32_t
{
    // Convert selected race to UI race index for camera Y offset calculation
    // Based on CLogin::ConvertSelectedRaceToUIRace @ 0xb536b0
    // The mapping is non-sequential to match the original client's UI layout
    switch (m_nCurSelectedRace)
    {
    case -1:  // Shining Star (special case)
        return 17;
    case 0:   // Explorer
        return 4;
    case 1:   // Cygnus Knights
        return 1;
    case 2:   // Aran
        return 0;
    case 3:   // Evan
        return 3;
    case 4:   // Mercedes
        return 2;
    case 5:   // Demon
        return 5;
    case 6:   // Phantom
        return 6;
    case 7:   // Luminous
        return 7;
    case 8:   // Dual Blade
        return 8;
    case 9:   // Mihile
        return 9;
    case 10:  // Kaiser
        return 10;
    case 11:  // Angelic Buster
        return 11;
    case 12:  // Xenon
        return 12;
    case 13:  // Zero
        return 13;
    case 14:  // Beast Tamer
        return 14;
    case 15:  // Kinesis
        return 15;
    case 16:  // Cadena
        return 16;
    case 17:  // Illium
        return 18;
    case 18:  // Ark
        return 19;
    default:
        return m_nCurSelectedRace;
    }
}

void Login::SetSubStep(std::int32_t nSubStep)
{
    m_nSubStep = nSubStep;
    m_bSubStepChanged = true;
}

void Login::LoadGenderAndFrameChoosable()
{
    // Load gender and frame choosable options from WZ
    // This determines which races allow gender/frame selection
    m_mGenderChoosable.clear();
    m_mFrameChoosable.clear();
    m_mBasicAvatar.clear();

    // Default values for common jobs
    // 0 = gender fixed by account
    // 1 = gender choosable
    // 2 = male only
    // 3 = female only
    m_mGenderChoosable[0] = 1;     // Explorer - choosable
    m_mGenderChoosable[1000] = 1;  // Cygnus - choosable
    m_mGenderChoosable[2000] = 0;  // Aran - fixed
    m_mGenderChoosable[2001] = 0;  // Evan - fixed
}

void Login::LoadNewCharInfo(std::int32_t nRace)
{
    // Load new character equipment info for the given race
    // Based on original CLogin::LoadNewCharInfo
    m_lNewEquip.clear();

    auto& resMan = WzResMan::GetInstance();
    auto makeCharProp = resMan.GetProperty("Etc/MakeCharInfo.img");

    if (!makeCharProp)
    {
        LOG_WARN("MakeCharInfo.img not found, using defaults");
        return;
    }

    // Get race-specific equipment from MakeCharInfo.img/<race>
    auto raceProp = makeCharProp->GetChild(std::to_string(nRace));
    if (!raceProp)
    {
        // Try "Info" child for the race
        auto infoProp = makeCharProp->GetChild("Info");
        if (infoProp)
        {
            raceProp = infoProp->GetChild(std::to_string(nRace));
        }
    }

    if (!raceProp)
    {
        LOG_WARN("Race {} not found in MakeCharInfo, using defaults", nRace);
        return;
    }

    // Load equipment parts from race property
    // Structure: MakeCharInfo.img/<race>/<gender>/<partType>/id, part, frame
    // Or: MakeCharInfo.img/<race>/<partType>/id, part, frame
    for (const auto& child : raceProp->GetChildren())
    {
        const auto& [childName, childProp] = child;
        if (!childProp)
            continue;

        // Check if this is a direct equipment entry or a gender container
        auto idProp = childProp->GetChild("id");
        if (idProp)
        {
            // Direct equipment entry
            NewEquip equip;
            equip.nItemID = idProp->GetInt(0);

            auto partProp = childProp->GetChild("part");
            equip.nPart = partProp ? partProp->GetInt(0) : 0;

            auto frameProp = childProp->GetChild("frame");
            equip.nFrame = frameProp ? frameProp->GetInt(0) : 0;

            if (equip.nItemID != 0)
            {
                m_lNewEquip.push_back(equip);
                LOG_DEBUG("LoadNewCharInfo: Loaded equip id={}, part={}, frame={}",
                          equip.nItemID, equip.nPart, equip.nFrame);
            }
        }
        else
        {
            // This might be a container (gender or part list)
            // Try to iterate its children
            for (const auto& subChild : childProp->GetChildren())
            {
                const auto& [subName, subProp] = subChild;
                if (!subProp)
                    continue;

                auto subIdProp = subProp->GetChild("id");
                if (subIdProp)
                {
                    NewEquip equip;
                    equip.nItemID = subIdProp->GetInt(0);

                    auto subPartProp = subProp->GetChild("part");
                    equip.nPart = subPartProp ? subPartProp->GetInt(0) : 0;

                    auto subFrameProp = subProp->GetChild("frame");
                    equip.nFrame = subFrameProp ? subFrameProp->GetInt(0) : 0;

                    if (equip.nItemID != 0)
                    {
                        m_lNewEquip.push_back(equip);
                        LOG_DEBUG("LoadNewCharInfo: Loaded equip id={}, part={}, frame={}",
                                  equip.nItemID, equip.nPart, equip.nFrame);
                    }
                }
            }
        }
    }

    LOG_INFO("LoadNewCharInfo: Loaded {} equipment items for race {}", m_lNewEquip.size(), nRace);
}

void Login::LoadNewDummyCharInfo()
{
    // Load dummy character info for preview
    m_lNewDummyEquip.clear();
}

void Login::LoadSkinList()
{
    // Load available skin colors
    m_aSkin.clear();
    // Default skin list
    for (int i = 0; i <= 12; ++i)
    {
        m_aSkin.push_back(i);
    }
}

void Login::InitNewAvatar(std::int32_t nGender, std::int32_t nFrame)
{
    // Initialize new character avatar with given gender and frame
    LOG_DEBUG("Initializing new avatar: gender={}, frame={}", nGender, nFrame);

    // Create avatar with default equipment
    // Load from m_lNewEquip based on race/gender/frame
}

void Login::InitNewCharEquip(std::int32_t nRace)
{
    // Initialize equipment options for new character creation
    LoadNewCharInfo(nRace);
}

void Login::ShiftNewCharEquip(std::int32_t nPart,
                              std::int32_t nDirection,
                              [[maybe_unused]] bool bIgnoreFrame)
{
    // Shift equipment selection for the given part
    // Based on original CLogin::ShiftNewCharEquip

    // Find all equipment items matching the given part
    std::vector<std::size_t> partIndices;
    for (std::size_t i = 0; i < m_lNewEquip.size(); ++i)
    {
        if (m_lNewEquip[i].nPart == nPart)
        {
            partIndices.push_back(i);
        }
    }

    if (partIndices.empty())
    {
        LOG_DEBUG("ShiftNewCharEquip: No equipment for part {}", nPart);
        return;
    }

    // Get current selection index for this part (stored in m_mEquipSelIdx)
    auto it = m_mEquipSelIdx.find(nPart);
    std::int32_t curIdx = 0;
    if (it != m_mEquipSelIdx.end())
    {
        curIdx = it->second;
    }

    // Shift the index
    curIdx += nDirection;

    // Wrap around
    auto count = static_cast<std::int32_t>(partIndices.size());
    if (curIdx < 0)
    {
        curIdx = count - 1;
    }
    else if (curIdx >= count)
    {
        curIdx = 0;
    }

    // Store the new selection
    m_mEquipSelIdx[nPart] = curIdx;

    // Get the selected equipment info
    auto& equip = m_lNewEquip[partIndices[static_cast<std::size_t>(curIdx)]];
    LOG_DEBUG("ShiftNewCharEquip: Part {} shifted to idx {} (itemID={})",
              nPart, curIdx, equip.nItemID);
}

void Login::ShiftNewCharSkin(std::int32_t nDirection)
{
    // Shift skin selection
    if (m_aSkin.empty())
    {
        return;
    }

    m_nCurSelectedSkinIdx += nDirection;
    if (m_nCurSelectedSkinIdx < 0)
    {
        m_nCurSelectedSkinIdx = static_cast<std::int32_t>(m_aSkin.size()) - 1;
    }
    else if (m_nCurSelectedSkinIdx >= static_cast<std::int32_t>(m_aSkin.size()))
    {
        m_nCurSelectedSkinIdx = 0;
    }
}

void Login::OnNewCharJobSel()
{
    SetSubStep(1);  // Go to gender selection
}

void Login::OnNewCharGenderSel()
{
    SetSubStep(2);  // Go to frame selection
}

void Login::OnNewCharFrameSel()
{
    SetSubStep(3);  // Go to avatar customization
}

void Login::OnNewCharAvatarSel()
{
    SetSubStep(4);  // Go to name input
}

void Login::OnNewCharNameSel(const std::string& sName)
{
    // Handle character name selection
    LOG_DEBUG("Character name selected: {}", sName);
    m_sCheckedName = sName;
    // Send character creation packet
}

void Login::OnNewCharCanceled()
{
    // Handle character creation canceled
    ChangeStep(2);  // Go back to character selection
}

// =============================================================================
// Network Packet Methods (stubs for offline mode)
// Based on decompiled CLogin packet handling from v1029
// =============================================================================

void Login::SendWorldInfoRequestPacket()
{
    // Opcode 104: World info request
    // Based on CLogin::Init sending at 0xb6f3xx
    // Format: opcode(104) + isWebLogin(1 byte) + [webCookie(string) if isWebLogin]

    LOG_DEBUG("SendWorldInfoRequestPacket: opcode 104, baseStep={}", m_nBaseStep);

    if (m_bOfflineMode)
    {
        // In offline mode, directly call OnWorldInfoReceived to simulate server response
        OnWorldInfoReceived();
        return;
    }

    // Online mode would send:
    // OutPacket packet(104);
    // packet.Encode1(m_nBaseStep == 1 ? 1 : 0);  // isWebLogin
    // if (m_nBaseStep == 1) {
    //     packet.EncodeStr(GetWebCookie());
    // }
    // ClientSocket::GetInstance().SendPacket(packet);
}

void Login::SendSelectCharacterPacket(std::uint32_t characterId)
{
    // Opcode 107: Select character
    // Based on CLogin::Update at 0xb6adxx
    // Format: opcode(107) + SPW(string) + characterID(4 bytes) + offlineMode(1 byte)

    LOG_DEBUG("SendSelectCharacterPacket: opcode 107, charId={}, offline={}", characterId, m_bOfflineMode);

    if (m_bOfflineMode)
    {
        // In offline mode, directly proceed to character login
        LOG_DEBUG("Offline mode: simulating character selection success");
        return;
    }

    // Online mode would send:
    // OutPacket packet(107);
    // packet.EncodeStr(m_sSPW);
    // packet.Encode4(static_cast<std::int32_t>(characterId));
    // packet.Encode1(m_bOfflineMode ? 1 : 0);
    // ClientSocket::GetInstance().SendPacket(packet);
}

void Login::SendWorldInfoRequestForGotoPacket()
{
    // Opcode 117: Simple world info request (for GotoWorldSelect)
    // Based on CLogin::GotoWorldSelect at 0xb66a10
    // Format: opcode(117) only

    LOG_DEBUG("SendWorldInfoRequestForGotoPacket: opcode 117");

    if (m_bOfflineMode)
    {
        // In offline mode, just reinitialize world list
        InitWorldItemFinal();
        OnWorldInfoReceived();
        return;
    }

    // Online mode would send:
    // OutPacket packet(117);
    // ClientSocket::GetInstance().SendPacket(packet);
}

void Login::SendClientLoadingTimePacket()
{
    // Opcode 108: Client loading time report
    // Based on CLogin::Update at 0xb6aexx
    // Reports how long the client took to load

    LOG_DEBUG("SendClientLoadingTimePacket: opcode 108");

    // This packet is purely informational for the server
    // In offline mode, we just skip it

    // Online mode would send loading metrics:
    // OutPacket packet(108);
    // packet.Encode4(loadTimeMs);
    // ClientSocket::GetInstance().SendPacket(packet);
}

void Login::SendSPWVerificationPacket(const std::string& spw)
{
    // Opcode 937: SPW verification for new character
    // Based on CLogin::ProgressNewCharStep at 0xb66010
    // Format: opcode(937) + SPW(string)

    LOG_DEBUG("SendSPWVerificationPacket: opcode 937, spw length={}", spw.length());

    if (m_bOfflineMode)
    {
        // In offline mode, always accept the SPW
        LOG_DEBUG("Offline mode: simulating SPW verification success");
        ChangeStep(-1);  // Proceed to next step
        return;
    }

    // Online mode would send:
    // OutPacket packet(937);
    // packet.EncodeStr(spw);
    // ClientSocket::GetInstance().SendPacket(packet);
}

} // namespace ms
