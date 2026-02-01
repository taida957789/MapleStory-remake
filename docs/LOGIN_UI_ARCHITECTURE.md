# Login UI Architecture (Based on Reverse Engineering v1029)

## CUITitle Structure (Login Dialog)

**Dialog Size:** 244 x 158 pixels
**Origin:** Origin_LT (Left-Top based positioning)

### Buttons (CCtrlButton)

| Name | ID | Position (x, y) | UOL String Pool ID |
|------|-----|-----------------|-------------------|
| m_pBtLogin | 1000 | (178, 41) | 0x917 |
| m_pBtEmailSave | 1001 | (27, 97) | 0x918 |
| m_pBtEmailLost | 1002 | (99, 97) | 0x919 |
| m_pBtPasswdLost | 1003 | (171, 97) | 0x91A |
| m_pBtNew | 1004 | (15, 117) | 0x91B |
| m_pBtHomePage | 1005 | (87, 117) | 0x91C |
| m_pBtQuit | 1006 | (159, 117) | 0x91D |

### Input Fields (CCtrlEdit)

| Name | ID | Position | Size | Properties |
|------|-----|----------|------|------------|
| m_pEditID | 1007 | (14, 43) | (163, 24) | nHorzMax=64, sEmptyImageUOL=0x920/0x921 |
| m_pEditPasswd | 1008 | (14, 69) | (163, 24) | nHorzMax=12/16, bPasswd=1, sEmptyImageUOL=0x922 |

### Tab Control (CCtrlTab)

| Name | ID | Position | Size |
|------|-----|----------|------|
| m_pTab | 1009 | (8, 1) | (243, 30) |

Tab items loaded from:
- Normal state: StringPool 0x931
- Selected state: StringPool 0x930

---

## CCtrlEdit::CREATEPARAM Structure

```cpp
struct CREATEPARAM {
    ZXString<char> sText;           // Initial text
    ZXString<unsigned short> sEmptyImageUOL;  // UOL for empty placeholder image
    POINT ptText;                   // Text offset within control (default: 6, 6)
    int nFontColor;                 // Font color (e.g., 0xFF5D7E3D = -10666979)
    int nHorzMax;                   // Max characters
    bool bPasswd;                   // Password mode (show asterisks)
};
```

## CCtrlButton::CREATEPARAM Structure

```cpp
struct CREATEPARAM {
    ZXString<unsigned short> sUOL;  // UOL path to button resource
    // Button states are loaded from: sUOL/normal, sUOL/mouseOver, sUOL/pressed, sUOL/disabled
};
```

---

## UI Control Hierarchy

```
CWnd (Base window class)
├── CChildWnd (Child window with origin support)
│   └── CFadeWnd (Fade effect support)
│       └── CUITitle (Login dialog)
│
└── CCtrlWnd (Control base class)
    ├── CCtrlButton (Button control)
    ├── CCtrlEdit (Text input control)
    └── CCtrlTab (Tab control)
```

---

## Key Methods

### CCtrlButton
- `CreateCtrl(parent, id, x, y, z, createParam, alpha)` - Create and add to parent
- `SetEnable(bool)` - Enable/disable button
- `SetCheck(bool)` - Toggle state
- `ChangeDisplayState(state)` - Update visual state (0=normal, 1=mouseOver, 2=pressed, 4=checked)

### CCtrlEdit
- `CreateCtrl(parent, id, x, y, width, height, createParam)` - Create input field
- `SetText(string)` - Set text content
- `GetText()` - Get current text
- `SelectAll()` - Select all text
- `MoveCaret(direction)` - Move cursor (2 = end)
- `SetGuideText(string)` - Set placeholder text

### CWnd
- `SetFocusChild(ctrl)` - Set keyboard focus to child control
- `NotifyToParent(msg, param)` - Send notification to parent

---

## Resource Paths (StringPool IDs)

| ID | Description | Typical Path |
|----|-------------|--------------|
| 0x917 | BtLogin | UI/Login.img/Title_new/BtLogin |
| 0x918 | BtEmailSave | UI/Login.img/Title_new/BtEmailSave |
| 0x919 | BtEmailLost | UI/Login.img/Title_new/BtEmailLost |
| 0x91A | BtPasswdLost | UI/Login.img/Title_new/BtPasswdLost |
| 0x91B | BtNew | UI/Login.img/Title_new/BtNew |
| 0x91C | BtHomePage | UI/Login.img/Title_new/BtHomePage |
| 0x91D | BtQuit | UI/Login.img/Title_new/BtQuit |
| 0x91E | RMA canvas 0 | (Remember Mail Address checkbox) |
| 0x91F | RMA canvas 1 | (Remember Mail Address checkbox) |
| 0x920 | EditID empty (Nexon) | UI/Login.img/Title_new/ID (placeholder) |
| 0x921 | EditID empty (Maple) | UI/Login.img/Title_new/ID (placeholder) |
| 0x922 | EditPasswd empty | UI/Login.img/Title_new/PW (placeholder) |
| 0x930 | Tab selected | UI/Login.img/Title_new/Tab/selected |
| 0x931 | Tab normal | UI/Login.img/Title_new/Tab/normal |

---

## Implementation Notes

1. **Coordinate System**: All child control positions are relative to the parent dialog's top-left corner.

2. **Focus Management**: When dialog opens, focus is set to either m_pEditID or m_pEditPasswd based on whether a saved ID exists.

3. **Tab Switching**: Tab control toggles between m_bNexonIDTab modes, which affects:
   - Empty placeholder images for ID field
   - Password max length (12 vs 16)
   - "New account" button visibility

4. **Button States**: Each button loads 4 canvas states from WZ:
   - `{UOL}/normal`
   - `{UOL}/mouseOver`
   - `{UOL}/pressed`
   - `{UOL}/disabled`

5. **Event IDs**:
   - 1000-1006: Button click events
   - 1007: ID field change
   - 1008: Password field change
   - 1009: Tab change
