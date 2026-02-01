# CUIChannelSelect Decompiled Analysis

Based on reverse engineering of MapleStory v1029 client using IDA Pro.

## Overview

CUIChannelSelect is a dialog that appears when a world is selected in the world selection UI. It displays available channels and their load status, allowing the player to select a channel to enter.

## Class Hierarchy

```
CUIChannelSelect : CUniqueModeless : CDialog : CWnd : IGObj, IUIMsgHandler, ZRefCounted
```

## Key Constants

```cpp
// Button IDs
constexpr int kChannelButtonBase = 1100;  // Channel 0 = 1100, Channel 1 = 1101, etc.
constexpr int kGoWorldButtonId = 1120;    // Enter world button

// Channel layout
constexpr int kChannelsPerRow = 5;        // Channels per row for navigation

// Gauge threshold
constexpr int kFullGaugeThreshold = 73;   // Channels with gauge >= 73 are considered full
```

## WZ Resource Paths

```
UI/Login.img/WorldSelect/BtChannel/test       # Base UOL for channel select dialog
UI/Login.img/WorldSelect/BtChannel/test/layer:bg  # Background canvas
UI/Login.img/WorldSelect/BtChannel/test/gauge     # Channel load gauge canvas
UI/Login.img/WorldSelect/BtChannel/test/test      # Additional layouts
```

## Member Variables

```cpp
class CUIChannelSelect
{
    CLogin* m_pLogin;                           // Parent login stage
    WORLDITEM* m_pWorldItem;                    // Current world item

    ZXString<char> m_sBaseUOL;                  // "UI/Login.img/WorldSelect/BtChannel/test"
    ZRef<CLayoutMan> m_pLm;                     // Layout manager

    _com_ptr_t<IWzCanvas> m_pCanvasGauge;       // Gauge canvas template
    _com_ptr_t<IWzGr2DLayer> m_pLayerGauge;     // Gauge layer
    _com_ptr_t<IWzGr2DLayer> m_pLayerEventDesc; // Event description balloon layer

    int m_nSelect;                              // Currently selected channel (button ID)
    bool m_bSelectWorld;                        // Flag: ready to enter world
};
```

---

## Constructor (0xbbdc00)

```cpp
CUIChannelSelect::CUIChannelSelect(void* pData)
{
    CUniqueModeless::CUniqueModeless(this);

    // Initialize member variables
    m_pCanvasGauge = nullptr;
    m_pLayerGauge = nullptr;
    m_pLayerEventDesc = nullptr;
    m_sBaseUOL = "";
    m_pLm = nullptr;

    m_bSelectWorld = false;
    m_nSelect = 0;

    // Set base UOL
    m_sBaseUOL = "UI/Login.img/WorldSelect/BtChannel/test";

    // Load background canvas
    ZXString<char> sBack = m_sBaseUOL + "/layer:bg";
    IWzCanvas* pCanvasBack = g_rm->GetObject(sBack);

    // Get canvas dimensions
    int w, h;
    pCanvasBack->GetWidth(&w);
    pCanvasBack->GetHeight(&h);

    // Create dialog with canvas dimensions
    // Position: (203, 194), Origin_LT
    CDialog::CreateDlg(this, 203, 194, w, h, 10, 1, pData, Origin_LT);
}
```

---

## OnCreate (0xbc4780)

```cpp
void CUIChannelSelect::OnCreate(void* pData)
{
    // pData[0] = CLogin*, pData[1] = nWorldIdx
    m_pLogin = pData[0];
    int nWorldIdx = pData[1];

    // Initialize layout manager
    ZRef<CLayoutMan>::_Alloc(&m_pLm);
    CLayoutMan::Init(m_pLm, this, 0, 0);

    // Build UI from base UOL
    CLayoutMan::AutoBuild(m_pLm, m_sBaseUOL, 0, 0, 0, 1, 0);

    // Copy gauge layer from layer overlay (z=110)
    IWzGr2DLayer* pLayer;
    CWnd::GetLayer(this, &pLayer);
    m_pLayerGauge = CLayoutMan::CopyToEmptyLayerOverlay(pLayer, 110);

    // Build additional layout from test sub-path
    ZXString<char> sWorldUOL = m_sBaseUOL + "/test";
    CLayoutMan::AutoBuild(m_pLm, sWorldUOL, 0, 0, 0, 1, 0);

    // Load gauge canvas
    ZXString<char> sGauge = m_sBaseUOL + "/gauge";
    m_pCanvasGauge = g_rm->GetObject(sGauge);

    // Initialize with world info
    CUIChannelSelect::ResetInfo(this, nWorldIdx, 0);
}
```

---

## ResetInfo (0xbc3150)

```cpp
void CUIChannelSelect::ResetInfo(int nWorldIdx, int bRedraw)
{
    // Get world item
    m_pWorldItem = &m_pLogin->m_WorldItem[nWorldIdx];

    // Build event description
    ZXString<char> sWorldEventDesc = m_pWorldItem->sWorldEventDesc;

    // Add EXP/Drop bonus info if not 100%
    if (m_pWorldItem->nWorldEventEXP_WSE != 100 || m_pWorldItem->nWorldEventDrop_WSE != 100)
    {
        ZXString<char> sFormat = StringPool::GetString(0x1560);  // EXP bonus format
        ZXString<char> sBonus;
        sBonus.Format(sFormat, m_pWorldItem->nWorldEventEXP_WSE / 100);
        sWorldEventDesc += sBonus;
    }

    // Create event description balloon
    CUIChannelSelect::CreateEventDescBalloon(this, sWorldEventDesc);

    // Hide all layers and buttons initially
    CLayoutMan::ABSetLayerVisibleAll(m_pLm, 0);
    CLayoutMan::ABSetButtonShowAll(m_pLm, 0);
    CLayoutMan::ABSetButtonEnableAll(m_pLm, 0);

    // Show background
    CLayoutMan::ABSetLayerVisible(m_pLm, L"bg", 1);

    // Show world-specific layer (by world ID)
    wchar_t wszWorldId[32];
    _itow(m_pWorldItem->nWorldID, wszWorldId, 10);
    CLayoutMan::ABSetLayerVisible(m_pLm, wszWorldId, 1);

    // Show and enable GoWorld button
    CCtrlOriginButton* pBtnGoWorld = CLayoutMan::ABGetButton(m_pLm, L"GoWorld");
    pBtnGoWorld->SetShow(1, 0);
    CLayoutMan::ABSetButtonEnable(m_pLm, L"GoWorld", 1);

    // Get gauge layer canvas
    IWzCanvas* pCanvas;
    m_pLayerGauge->GetCanvas(&pCanvas, 0);

    // Get gauge dimensions
    int gaugeWidth, gaugeHeight;
    pCanvas->GetWidth(&gaugeWidth);
    pCanvas->GetHeight(&gaugeHeight);

    // Clear gauge canvas
    pCanvas->FillRect(0, 0, gaugeWidth, gaugeHeight, 0xFF);

    bool bSelected = false;

    // Set up channel buttons
    for (int i = 0; i < m_pWorldItem->ci.count; i++)
    {
        CHANNELITEM& channel = m_pWorldItem->ci[i];

        // Get button by channel index (as string "0", "1", etc.)
        wchar_t wszIdx[32];
        _itow(i, wszIdx, 10);
        CCtrlOriginButton* pBtn = CLayoutMan::ABGetButton(m_pLm, wszIdx);

        // Show and enable button
        pBtn->SetShow(1, 0);
        pBtn->SetEnable(1);
        pBtn->SetKeyFocus(0);

        int nGaugePx = channel.nGaugePx;

        // Draw gauge on canvas at button position
        int x = pBtn->GetX() + 6;
        int y = pBtn->GetY() + 16;
        pCanvas->DrawImage(x, y, m_pCanvasGauge, 255,
                          nGaugePx, gaugeHeight,  // source size
                          0, 0,                   // source offset
                          nGaugePx, gaugeHeight); // dest size

        // Auto-select first non-full channel
        if (!bSelected && nGaugePx < kFullGaugeThreshold)
        {
            pBtn->SetKeyFocus(1);
            m_nSelect = pBtn->m_nCtrlId;
            bSelected = true;
        }
    }

    // If no channel was auto-selected, select first channel
    if (!bSelected && m_pWorldItem->ci.count > 0)
    {
        CCtrlOriginButton* pBtn = CLayoutMan::ABGetButton(m_pLm, L"0");
        pBtn->SetKeyFocus(1);
        m_nSelect = pBtn->m_nCtrlId;
    }

    if (bRedraw)
    {
        InvalidateRect(nullptr);
    }
}
```

---

## OnButtonClicked (0xbbc880)

```cpp
void CUIChannelSelect::OnButtonClicked(unsigned int nId)
{
    // Check if request allowed
    if (m_pLogin->m_bRequestSent || m_pLogin->m_nLoginStep != 1)
        return;

    // Get button and verify it's enabled and shown
    CCtrlOriginButton* pBtn = CLayoutMan::ABGetButtonByID(m_pLm, nId);
    if (!pBtn || !pBtn->IsEnabled() || !pBtn->IsShown())
        return;

    m_pLogin->m_bGoToStarPlanetForUpdate = false;

    // Check if clicking same channel or GoWorld button
    if (nId == m_nSelect || nId == kGoWorldButtonId)
    {
        // Ready to enter world
        m_bSelectWorld = true;
        CUIChannelSelect::DrawNoticeConnecting(this);
    }
    else
    {
        // Play UI sound (StringPool 0x946)
        ZXString<wchar_t> sSoundPath = StringPool::GetStringW(0x946);
        play_ui_sound(sSoundPath);

        // Clear focus from previous selection
        CCtrlOriginButton* pOldBtn = CLayoutMan::ABGetButtonByID(m_pLm, m_nSelect);
        if (pOldBtn)
        {
            pOldBtn->SetKeyFocus(0);
        }

        // Set focus to new selection
        CCtrlOriginButton* pNewBtn = CLayoutMan::ABGetButtonByID(m_pLm, nId);
        pNewBtn->SetKeyFocus(1);

        m_nSelect = nId;
    }
}
```

---

## OnKey (0xbbbd10)

```cpp
void CUIChannelSelect::OnKey(unsigned int wParam, int lParam)
{
    if (lParam < 0)  // Key up events ignored
        return;

    switch (wParam)
    {
    case VK_TAB:  // 0x09
        // Shift+Tab: go left, Tab: go right
        if (CInputSystem::IsKeyPressed(VK_SHIFT) || CInputSystem::IsKeyPressed(VK_RSHIFT))
            goto NavigateLeft;
        goto NavigateRight;

    case VK_RETURN:  // 0x0D (Enter)
        // Click the currently selected button
        OnButtonClicked(m_nSelect);
        return;

    case VK_ESCAPE:  // 0x1B
        // Close channel select dialog
        m_pLogin->CloseChannelSelect();
        return;

    case VK_LEFT:  // 0x25
    NavigateLeft:
        NavigateChannel(-1);
        break;

    case VK_UP:  // 0x26
        NavigateChannel(-kChannelsPerRow);  // -5
        break;

    case VK_RIGHT:  // 0x27
    NavigateRight:
        NavigateChannel(1);
        break;

    case VK_DOWN:  // 0x28
        NavigateChannel(kChannelsPerRow);  // +5
        break;

    default:
        CDialog::OnKey(this, wParam, lParam);
        break;
    }
}

// Helper function extracted from OnKey logic
void CUIChannelSelect::NavigateChannel(int delta)
{
    int channelCount = m_pWorldItem->ci.count;
    int maxButtonId = kChannelButtonBase + channelCount - 1;  // 1099 + count

    int newSelect = m_nSelect + delta;

    // Wrap around
    if (newSelect < kChannelButtonBase)  // < 1100
    {
        newSelect = newSelect + maxButtonId - kChannelButtonBase + 1;
    }
    else if (newSelect > maxButtonId)
    {
        newSelect = newSelect - (maxButtonId - kChannelButtonBase + 1);
    }

    if (m_nSelect != newSelect)
    {
        OnButtonClicked(newSelect);
    }
}
```

---

## EnterChannel (0xbbb950)

```cpp
void CUIChannelSelect::EnterChannel()
{
    if (!m_pLogin->m_tStepChanging)
    {
        int nWorldID = m_pWorldItem->nWorldID;
        int nChannel = m_nSelect % 100;  // Convert button ID to channel index

        CLogin::SendLoginPacket(m_pLogin, nWorldID, nChannel, 0);
    }
}
```

---

## DrawNoticeConnecting (0xbbbe50)

```cpp
void CUIChannelSelect::DrawNoticeConnecting()
{
    if (!TSingleton<CConnectionNoticeDlg>::ms_pInstance)
    {
        // Create connection notice dialog
        CConnectionNoticeDlg* pDlg = new CConnectionNoticeDlg(m_pLogin, 1);

        // Show as modal dialog
        CDialog::DoModal(pDlg);

        // Clean up
        delete pDlg;
    }
}
```

---

## Destructor (0xbbdf60)

```cpp
CUIChannelSelect::~CUIChannelSelect()
{
    // Release layout manager
    if (m_pLm)
    {
        m_pLm->Release();
        m_pLm = nullptr;
    }

    // Release base UOL string
    // (handled by ZXString destructor)

    // Release layers
    if (m_pLayerEventDesc)
    {
        m_pLayerEventDesc->Release();
    }

    if (m_pLayerGauge)
    {
        m_pLayerGauge->Release();
    }

    // Release gauge canvas
    if (m_pCanvasGauge)
    {
        m_pCanvasGauge->Release();
    }

    // Call base destructor
    CUniqueModeless::~CUniqueModeless(this);
}
```

---

## Data Structures

### CHANNELITEM

```cpp
struct CHANNELITEM
{
    int nGaugePx;        // Channel load gauge in pixels (0-73+ range)
    bool bAdultChannel;  // Adult-only channel flag
    // ... other fields
};
```

### WORLDITEM (partial)

```cpp
struct WORLDITEM
{
    int nWorldID;
    ZXString<char> sWorldEventDesc;
    int nWorldEventEXP_WSE;   // Event EXP multiplier (100 = normal)
    int nWorldEventDrop_WSE;  // Event drop multiplier (100 = normal)
    ZArray<CHANNELITEM> ci;   // Channel info array
    // ... other fields
};
```

---

## UI Flow

1. **World Selection** → User clicks world button in CUIWorldSelect
2. **Channel Dialog Opens** → CUIChannelSelect created with world index
3. **ResetInfo()** → Populates channel buttons and gauge display
4. **User Navigation** → Arrow keys, Tab, or mouse clicks to select channel
5. **Channel Selection** → Double-click or Enter to confirm
6. **EnterChannel()** → Sends login packet to server
7. **DrawNoticeConnecting()** → Shows "Connecting..." dialog

---

## Key Behaviors

1. **Auto-Select**: First channel with gauge < 73px is auto-selected
2. **Double-Click**: Clicking same channel twice triggers enter
3. **GoWorld Button**: Clicking GoWorld button (ID 1120) triggers enter
4. **Keyboard Navigation**: Arrow keys navigate in 5-channel grid pattern
5. **Wrap-Around**: Navigation wraps from last to first channel and vice versa
6. **Event Info**: Shows EXP/Drop bonuses if different from 100%

---

## String Pool References

| ID | Usage |
|----|-------|
| 0x946 | UI click sound path |
| 0x1560 | EXP bonus format string |
