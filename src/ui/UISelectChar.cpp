#include "UISelectChar.h"
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

UISelectChar::UISelectChar() = default;

UISelectChar::~UISelectChar()
{
    Destroy();
}

auto UISelectChar::OnCreate(std::any params) -> Result<void>
{
    // 1. Extract and validate parameters
    auto* createParams = std::any_cast<CreateParams>(&params);
    if (!createParams)
    {
        return Result<void>::Error("Invalid params type for UISelectChar");
    }

    if (!createParams->IsValid())
    {
        return Result<void>::Error("UISelectChar CreateParams validation failed");
    }

    // 2. Store references
    m_pLogin = createParams->login;
    m_pGr = createParams->gr;
    m_pUIManager = createParams->uiManager;

    // 3. Initialize state
    m_nSelectedIndex = -1;
    m_nPageIndex = 0;

    // 4. Get character count and slot count from Login
    if (m_pLogin)
    {
        m_nCharCount = m_pLogin->GetCharCount();
        m_nSlotCount = m_pLogin->GetSlotCount();
    }

    // 5. Load CharSelect WZ property
    auto& resMan = WzResMan::GetInstance();
    auto loginImgProp = resMan.GetProperty("UI/Login.img");
    if (loginImgProp)
    {
        m_pCharSelectProp = loginImgProp->GetChild("CharSelect");
        if (m_pCharSelectProp)
        {
            LOG_DEBUG("UISelectChar: CharSelect property loaded");
        }
    }

    // 6. Create character slots
    CreateCharacterSlots();

    LOG_DEBUG("UISelectChar::OnCreate completed (charCount={}, slotCount={})",
              m_nCharCount, m_nSlotCount);

    return Result<void>::Success();
}

void UISelectChar::OnDestroy() noexcept
{
    try
    {
        // 1. Clear world button vectors
        m_vBtCharacter.clear();

        // 2. Clear button shared_ptrs
        m_pBtnSelect.reset();
        m_pBtnNew.reset();
        m_pBtnDelete.reset();
        m_pBtnPageL.reset();
        m_pBtnPageR.reset();

        // 3. Clear name tag layers
        if (m_pGr)
        {
            for (auto& layer : m_vLayerNameTag)
            {
                if (layer)
                {
                    m_pGr->RemoveLayer(layer);
                }
            }
        }
        m_vLayerNameTag.clear();

        // 4. Clear background layers
        if (m_pLayerBg && m_pGr)
        {
            m_pGr->RemoveLayer(m_pLayerBg);
        }
        m_pLayerBg.reset();

        if (m_pLayerSelectedWorld && m_pGr)
        {
            m_pGr->RemoveLayer(m_pLayerSelectedWorld);
        }
        m_pLayerSelectedWorld.reset();

        // 5. Clear cached WZ property
        m_pCharSelectProp.reset();

        // 6. Clear references (non-owning)
        m_pLogin = nullptr;
        m_pGr = nullptr;
        m_pUIManager = nullptr;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in UISelectChar::OnDestroy: {}", e.what());
    }
}

void UISelectChar::CreateCharacterSlots()
{
    if (!m_pGr || !m_pUIManager)
    {
        return;
    }

    // CUISelectChar window: (30, 30) with size (750, 600)
    constexpr int kWindowX = 30;
    constexpr int kWindowY = 30;

    // Character slot positions - 8 slots in 2 rows of 4
    constexpr int kSlotWidth = 150;
    constexpr int kSlotHeight = 200;
    constexpr int kSlotsPerRow = 4;
    constexpr int kStartX = kWindowX + 50;
    constexpr int kStartY = kWindowY + 50;
    constexpr int kSpacingX = 160;
    constexpr int kSpacingY = 220;

    // Clear existing slots
    for (auto& btn : m_vBtCharacter)
    {
        if (btn && btn->GetLayer())
        {
            m_pGr->RemoveLayer(btn->GetLayer());
        }
    }
    m_vBtCharacter.clear();

    // Clear name tag layers
    for (auto& layer : m_vLayerNameTag)
    {
        if (layer)
        {
            m_pGr->RemoveLayer(layer);
        }
    }
    m_vLayerNameTag.clear();

    // Try to load character slot canvases from WZ
    // string.csv: UI/Login.img/CharSelect/character/0 and character/1
    std::shared_ptr<WzGr2DCanvas> slotCanvas;
    std::shared_ptr<WzGr2DCanvas> emptySlotCanvas;

    if (m_pCharSelectProp)
    {
        auto characterProp = m_pCharSelectProp->GetChild("character");
        if (characterProp)
        {
            auto char0Prop = characterProp->GetChild("0");
            if (char0Prop)
            {
                auto tmpWzCanvas_180 = char0Prop->GetCanvas();
                slotCanvas = tmpWzCanvas_180 ? std::make_shared<WzGr2DCanvas>(tmpWzCanvas_180) : nullptr;
            }
            auto char1Prop = characterProp->GetChild("1");
            if (char1Prop)
            {
                auto tmpWzCanvas_185 = char1Prop->GetCanvas();
                emptySlotCanvas = tmpWzCanvas_185 ? std::make_shared<WzGr2DCanvas>(tmpWzCanvas_185) : nullptr;
            }
        }
    }

    // Create character slots (up to 8)
    int maxSlots = std::min(m_nSlotCount, 8);
    for (int i = 0; i < maxSlots; ++i)
    {
        int row = i / kSlotsPerRow;
        int col = i % kSlotsPerRow;
        int slotX = kStartX + col * kSpacingX;
        int slotY = kStartY + row * kSpacingY;

        auto btn = std::make_shared<UIButton>();

        // Use WZ canvas if available, otherwise create placeholder
        std::shared_ptr<WzGr2DCanvas> canvas = (i < m_nCharCount) ? slotCanvas : emptySlotCanvas;
        if (canvas)
        {
            btn->SetStateCanvas(UIState::Normal, canvas);
            btn->SetSize(canvas->GetWidth(), canvas->GetHeight());
        }
        else
        {
            // Create placeholder slot
            auto wzPlaceholder = std::make_shared<WzCanvas>(kSlotWidth, kSlotHeight);
            std::vector<std::uint8_t> pixels(static_cast<size_t>(kSlotWidth * kSlotHeight * 4));
            for (size_t p = 0; p < pixels.size(); p += 4)
            {
                bool isEmpty = (i >= m_nCharCount);
                pixels[p + 0] = isEmpty ? 100 : 150;  // R
                pixels[p + 1] = isEmpty ? 100 : 150;  // G
                pixels[p + 2] = isEmpty ? 100 : 180;  // B
                pixels[p + 3] = 200;                   // A
            }
            wzPlaceholder->SetPixelData(std::move(pixels));
            auto placeholder = std::make_shared<WzGr2DCanvas>(wzPlaceholder);
            btn->SetStateCanvas(UIState::Normal, placeholder);
            btn->SetSize(kSlotWidth, kSlotHeight);
        }

        btn->SetPosition(slotX, slotY);
        btn->CreateLayer(*m_pGr, 150);

        const std::int32_t slotIndex = i;
        btn->SetClickCallback([this, slotIndex]() {
            SelectCharacter(slotIndex);
        });

        m_pUIManager->AddElement("charSlot" + std::to_string(i), btn);
        m_vBtCharacter.push_back(btn);

        LOG_DEBUG("Created character slot {} at ({}, {})", i, slotX, slotY);
    }

    // Create buttons: BtSelect, BtNew, BtDelete
    constexpr int kButtonY = kWindowY + 520;
    constexpr int kButtonSpacing = 120;
    int buttonX = kWindowX + 200;

    // BtSelect - Enter game
    bool btSelectLoaded = false;
    if (m_pCharSelectProp)
    {
        auto btSelectProp = m_pCharSelectProp->GetChild("BtSelect");
        if (btSelectProp)
        {
            m_pBtnSelect = std::make_shared<UIButton>();
            if (m_pBtnSelect->LoadFromProperty(btSelectProp))
            {
                m_pBtnSelect->SetPosition(buttonX, kButtonY);
                m_pBtnSelect->CreateLayer(*m_pGr, 160);
                btSelectLoaded = true;
            }
        }
    }

    if (!btSelectLoaded)
    {
        m_pBtnSelect = CreatePlaceholderButton("Select", buttonX, kButtonY, 100, 35);
    }

    m_pBtnSelect->SetClickCallback([this]() {
        if (m_nSelectedIndex >= 0 && m_nSelectedIndex < m_nCharCount)
        {
            EnterCharacter();
        }
        else
        {
            LOG_DEBUG("No character selected");
        }
    });
    m_pUIManager->AddElement("btnSelect", m_pBtnSelect);
    buttonX += kButtonSpacing;

    // BtNew - Create new character
    bool btNewLoaded = false;
    if (m_pCharSelectProp)
    {
        auto btNewProp = m_pCharSelectProp->GetChild("BtNew");
        if (btNewProp)
        {
            m_pBtnNew = std::make_shared<UIButton>();
            if (m_pBtnNew->LoadFromProperty(btNewProp))
            {
                m_pBtnNew->SetPosition(buttonX, kButtonY);
                m_pBtnNew->CreateLayer(*m_pGr, 160);
                btNewLoaded = true;
            }
        }
    }

    if (!btNewLoaded)
    {
        m_pBtnNew = CreatePlaceholderButton("New", buttonX, kButtonY, 100, 35);
    }

    m_pBtnNew->SetClickCallback([this]() {
        if (m_pLogin)
        {
            LOG_DEBUG("Creating new character");
            m_pLogin->ChangeStep(3);  // Go to race selection
        }
    });
    m_pUIManager->AddElement("btnNew", m_pBtnNew);
    buttonX += kButtonSpacing;

    // BtDelete - Delete character
    bool btDeleteLoaded = false;
    if (m_pCharSelectProp)
    {
        auto btDeleteProp = m_pCharSelectProp->GetChild("BtDelete");
        if (btDeleteProp)
        {
            m_pBtnDelete = std::make_shared<UIButton>();
            if (m_pBtnDelete->LoadFromProperty(btDeleteProp))
            {
                m_pBtnDelete->SetPosition(buttonX, kButtonY);
                m_pBtnDelete->CreateLayer(*m_pGr, 160);
                btDeleteLoaded = true;
            }
        }
    }

    if (!btDeleteLoaded)
    {
        m_pBtnDelete = CreatePlaceholderButton("Delete", buttonX, kButtonY, 100, 35);
    }

    m_pBtnDelete->SetClickCallback([this]() {
        if (m_nSelectedIndex >= 0 && m_nSelectedIndex < m_nCharCount)
        {
            DeleteChar();
        }
        else
        {
            LOG_DEBUG("No character selected for deletion");
        }
    });
    m_pUIManager->AddElement("btnDelete", m_pBtnDelete);
}

std::shared_ptr<UIButton> UISelectChar::CreatePlaceholderButton(
    const std::string& /*name*/, int x, int y, int width, int height)
{
    auto btn = std::make_shared<UIButton>();
    auto wzCanvas = std::make_shared<WzCanvas>(width, height);

    std::vector<std::uint8_t> pixels(static_cast<size_t>(width * height * 4));
    for (int py = 0; py < height; ++py)
    {
        for (int px = 0; px < width; ++px)
        {
            auto idx = static_cast<size_t>((py * width + px) * 4);
            float t = static_cast<float>(py) / static_cast<float>(height);
            pixels[idx + 0] = static_cast<std::uint8_t>(200 - 40 * t);
            pixels[idx + 1] = static_cast<std::uint8_t>(200 - 40 * t);
            pixels[idx + 2] = static_cast<std::uint8_t>(220 - 30 * t);
            pixels[idx + 3] = 255;
        }
    }
    wzCanvas->SetPixelData(std::move(pixels));
    auto canvas = std::make_shared<WzGr2DCanvas>(wzCanvas);

    btn->SetStateCanvas(UIState::Normal, canvas);
    btn->SetSize(width, height);
    btn->SetPosition(x, y);
    btn->CreateLayer(*m_pGr, 160);

    return btn;
}

void UISelectChar::SelectCharacter(std::int32_t charIndex)
{
    // Based on CUISelectChar::SelectCharacter (0xb795e0)
    if (charIndex < 0 || charIndex >= m_nCharCount)
    {
        LOG_DEBUG("Invalid character index: {}", charIndex);
        return;
    }

    m_nSelectedIndex = charIndex;

    if (m_pLogin)
    {
        m_pLogin->SetCharSelected(charIndex);
    }

    LOG_DEBUG("Character {} selected", charIndex);

    // Update visual feedback - highlight selected slot
    UpdateCharacterButtonStates();
}

void UISelectChar::EnterCharacter()
{
    // Based on CUISelectChar::EnterCharacter (0xb70c10)
    if (!m_pLogin)
    {
        return;
    }

    if (m_nSelectedIndex < 0 || m_nSelectedIndex >= m_nCharCount)
    {
        LOG_DEBUG("No valid character selected");
        return;
    }

    LOG_DEBUG("Entering game with character {}", m_nSelectedIndex);

    // In original: CLogin::SendCheckSPWExistPacket(m_pLogin, 1)
    // For now, just log the action
    // The actual game entry would involve network communication
}

void UISelectChar::DeleteChar()
{
    // Based on CUISelectChar::DeleteChar (0xb72290)
    if (m_nSelectedIndex < 0 || m_nSelectedIndex >= m_nCharCount)
    {
        LOG_DEBUG("No valid character selected for deletion");
        return;
    }

    LOG_DEBUG("Delete character {} requested", m_nSelectedIndex);
    // In original: Shows confirmation dialog, then sends delete packet
}

void UISelectChar::Refresh()
{
    // Based on CUISelectChar::Refresh (0xb79510)
    LOG_DEBUG("Refreshing character list");
    CreateCharacterSlots();
}

void UISelectChar::OnButtonClicked(std::uint32_t nId)
{
    // Based on CUISelectChar::OnButtonClicked (0xb79ff0)
    LOG_DEBUG("Button {} clicked", nId);
}

void UISelectChar::Destroy()
{
    // Call base class Destroy which calls OnDestroy
    UIElement::Destroy();
}

void UISelectChar::Update()
{
    // Based on CUISelectChar::Update (0xb73350)
    // Update character animations, burning effects, etc.
}

void UISelectChar::Draw()
{
    // Drawing is handled by the layer system
}

void UISelectChar::OnMouseMove(std::int32_t x, std::int32_t y)
{
    (void)x;
    (void)y;
}

void UISelectChar::OnMouseDown(std::int32_t x, std::int32_t y, std::int32_t button)
{
    (void)x;
    (void)y;
    (void)button;
}

void UISelectChar::OnMouseUp(std::int32_t x, std::int32_t y, std::int32_t button)
{
    (void)x;
    (void)y;
    (void)button;
}

void UISelectChar::OnKeyDown(std::int32_t keyCode)
{
    // Based on CUISelectChar::OnKey (0xb7cb50)
    // ESC - go back to world select
    if (keyCode == 27 && m_pLogin)  // SDLK_ESCAPE
    {
        m_pLogin->ChangeStep(1);
    }
}

void UISelectChar::CreateLayer(WzGr2D& gr, std::int32_t z)
{
    (void)gr;
    (void)z;
}

void UISelectChar::CreatePlaceholderUI()
{
    // Placeholder implementation
}

void UISelectChar::UpdateNameTags()
{
    // Update name tags for visible characters
}

void UISelectChar::UpdateCharacterButtonStates()
{
    // Update visual state of character slot buttons
    // Selected character gets Pressed state, others get Normal state
    for (size_t i = 0; i < m_vBtCharacter.size(); ++i)
    {
        if (m_vBtCharacter[i])
        {
            if (static_cast<std::int32_t>(i) == m_nSelectedIndex)
            {
                m_vBtCharacter[i]->SetState(UIState::Pressed);
            }
            else
            {
                m_vBtCharacter[i]->SetState(UIState::Normal);
            }
        }
    }
}

} // namespace ms
