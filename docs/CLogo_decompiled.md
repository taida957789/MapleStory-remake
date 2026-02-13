# CLogo Decompiled Functions (MapleStory v1029)

This document contains decompiled code from IDA Pro for the CLogo class in MapleStory v1029.
These serve as a reference for implementing the Logo stage in the SDL3 client.

## Table of Contents

1. [CLogo::CLogo (Constructor)](#clogoclogo-constructor---0xbc5720)
2. [CLogo::Init](#clogoinit---0xbc7120)
3. [CLogo::Update](#clogoupdate---0xbc7a90)
4. [CLogo::UpdateLogo](#clogoupdatelogo---0xbc7a10)
5. [CLogo::UpdateVideo](#clogoupdatevideo---0xbc5950)
6. [CLogo::InitWZLogo](#clogoinitlogo---0xbc5b20)
7. [CLogo::DrawWZLogo](#clogodrawwzlogo---0xbc71a0)
8. [CLogo::LogoEnd](#clogologoend---0xbc5890)
9. [CLogo::Close](#clogoclose---0xbc7170)
10. [CLogo::OnKey](#clogoonkey---0xbc7900)
11. [CLogo::OnMouseButton](#clogoonmousebutton---0xbc7990)
12. [CLogo::CanSkip](#clogocanskip---0xbc55f0)
13. [CLogo::ForcedEnd](#clogoforcedend---0xbc78d0)

---

## CLogo::CLogo (Constructor) - 0xbc5720

```cpp
CLogo *__thiscall CLogo::CLogo(CLogo *this)
{
  CLogo *result; // eax

  result = this; /*0xbc5720*/
  this->CStage::IUIMsgHandler::__vftable = &IUIMsgHandler::`vftable'; /*0xbc5722*/
  this->CStage::INetMsgHandler::__vftable = &INetMsgHandler::`vftable'; /*0xbc5729*/
  this->CStage::ZRefCounted::__vftable = &ZRefCounted::`vftable'; /*0xbc5730*/
  this->_m_nRef = 0; /*0xbc5739*/
  this->_m_pPrev = 0; /*0xbc573c*/
  this->m_bFadeInOut = 1; /*0xbc573f*/
  this->m_bOverlapTransfer = 0; /*0xbc5743*/
  this->CStage::IGObj::__vftable = &CLogo::`vftable'{for `IGObj'}; /*0xbc5746*/
  this->CStage::IUIMsgHandler::__vftable = &CLogo::`vftable'{for `IUIMsgHandler'}; /*0xbc574c*/
  this->CStage::INetMsgHandler::__vftable = &CLogo::`vftable'{for `INetMsgHandler'}; /*0xbc5753*/
  this->CStage::ZRefCounted::__vftable = &CLogo::`vftable'{for `ZRefCounted'}; /*0xbc575a*/
  this->m_pLayerBackground.m_pInterface = 0; /*0xbc5761*/
  this->m_pLayerMain.m_pInterface = 0; /*0xbc5764*/
  this->m_pLogoProp.m_pInterface = 0; /*0xbc5767*/
  this->m_pGradeProp.m_pInterface = 0; /*0xbc576a*/
  this->m_nGradeCount = 0; /*0xbc576d*/
  this->m_pMessageProp.m_pInterface = 0; /*0xbc5770*/
  this->m_bTickInitial = 0; /*0xbc5773*/
  this->m_dwTickInitial = 0; /*0xbc5776*/
  this->m_dwClick = 0; /*0xbc5779*/
  this->m_bLogoSoundPlayed = 0; /*0xbc577c*/
  this->m_bVideoMode = 0; /*0xbc577f*/
  this->m_videoState = VIDEO_STATE_UNAVAILABLE; /*0xbc5782*/
  return result; /*0xbc5785*/
}
```

### Key Member Variables:
- `m_pLayerBackground` - Background layer for logo
- `m_pLayerMain` - Main layer for logo display
- `m_pLogoProp` - WZ property for logo images (UI/Logo.img/Gamania)
- `m_pGradeProp` - WZ property for grade images (UI/Logo.img/Grade)
- `m_nGradeCount` - Number of grade images
- `m_pMessageProp` - WZ property for messages (UI/Logo.img/Message)
- `m_bTickInitial` - Flag for tick initialization
- `m_dwTickInitial` - Initial tick time
- `m_dwClick` - Click time for skipping
- `m_bLogoSoundPlayed` - Flag for logo sound played
- `m_bVideoMode` - Video mode flag
- `m_videoState` - Video state enum

### VIDEO_STATE Enum:
```cpp
enum VIDEO_STATE {
  VIDEO_STATE_UNAVAILABLE = 0,  // No video available
  VIDEO_STATE_PLAYING = 3,      // Video is playing
  VIDEO_STATE_FADEOUT = 4,      // Fading out
  VIDEO_STATE_END = 5           // Video ended
};
```

---

## CLogo::Init - 0xbc7120

```cpp
void __thiscall CLogo::Init(CLogo *this, void *pParam)
{
  CClientLoadingTime *v3; // edi

  CLogo::InitWZLogo(this); /*0xbc7124*/

  // Record launching end time
  v3 = TSingleton<CClientLoadingTime>::ms_pInstance; /*0xbc7129*/
  v3->m_nLaunchingEndTime = ZAPI.timeGetTime(); /*0xbc7135*/

  // Hide cursor during logo
  CInputSystem::ShowCursor(TSingleton<CInputSystem>::ms_pInstance, 0); /*0xbc7140*/

  // Check if auto-skip is enabled (from CWvsApp)
  char *v4 = *(TSingleton<CWvsApp>::ms_pInstance._m_pStr + 41); /*0xbc714a*/
  if (v4 && *v4)  // Skip logo if command line flag set
  {
    CLogo::LogoEnd(this); /*0xbc715b*/
  }
}
```

### Key Points:
- Calls `InitWZLogo()` to load WZ resources
- Records launch end time
- Hides cursor during logo display
- Auto-skips logo if command line flag is set

---

## CLogo::Update - 0xbc7a90

```cpp
void __thiscall CLogo::Update(CLogo *this)
{
  if (this->m_bVideoMode)  /*0xbc7a90*/
    CLogo::UpdateVideo(this);  /*0xbc7a9b*/
  else
    CLogo::UpdateLogo(this);  /*0xbc7a96*/
}
```

### Key Points:
- Simple dispatch between video mode and logo mode
- Video mode plays the intro video
- Logo mode displays WZ-based logos

---

## CLogo::UpdateLogo - 0xbc7a10

```cpp
void __thiscall CLogo::UpdateLogo(CLogo *this)
{
  unsigned int v2; // eax
  unsigned int m_dwClick; // ecx
  __int64 v4; // rax
  int v5; // edi

  // Initialize tick on first call
  if (!this->m_bTickInitial) /*0xbc7a13*/
  {
    this->m_bTickInitial = 1; /*0xbc7a19*/
    this->m_dwTickInitial = ZAPI.timeGetTime(); /*0xbc7a23*/
  }

  v2 = ZAPI.timeGetTime(); /*0xbc7a26*/
  m_dwClick = this->m_dwClick; /*0xbc7a2c*/

  // If clicked, wait 1500ms (0x5DC) before switching to video mode
  if (m_dwClick) /*0xbc7a31*/
  {
    if (v2 - m_dwClick > 0x5DC)  // 1500ms delay after click
    {
      this->m_bTickInitial = 0; /*0xbc7a3c*/
      this->m_bVideoMode = 1; /*0xbc7a40*/
    }
  }
  else if (this->m_pLayerMain.m_pInterface) /*0xbc7a46*/
  {
    // Calculate frame from elapsed time
    // Formula: (elapsed_ms * 2748779070) >> 38 = elapsed / 100 (approximately)
    // This gives ~10 frames per second
    v4 = 2748779070LL * (v2 - this->m_dwTickInitial); /*0xbc7a5a*/
    v5 = HIDWORD(v4) >> 6; /*0xbc7a5e*/  // frame number (10 fps)

    if (v5 >= this->m_nLogoCount)  // All logo frames shown
    {
      CLogo::DrawWZLogo(this, -1); /*0xbc7a73*/

      // Wait additional 10 frames then switch to video
      if (v5 >= this->m_nLogoCount + 10) /*0xbc7a80*/
      {
        this->m_bTickInitial = 0; /*0xbc7a82*/
        this->m_bVideoMode = 1; /*0xbc7a86*/
      }
    }
    else
    {
      CLogo::DrawWZLogo(this, v5); /*0xbc7a69*/
    }
  }
}
```

### Frame Timing:
- Logo runs at approximately 10 frames per second
- Formula: `frame = (elapsed_ms * 2748779070) >> 38` ≈ `elapsed_ms / 100`
- After all logo frames: wait 10 additional frames (~1 second)
- After click: wait 1500ms before switching to video mode

---

## CLogo::UpdateVideo - 0xbc5950

```cpp
void __thiscall CLogo::UpdateVideo(CLogo *this)
{
  CLogo *v1 = this;
  CLogo::VIDEO_STATE m_videoState = this->m_videoState;

  // Video unavailable - end logo immediately
  if (m_videoState == VIDEO_STATE_UNAVAILABLE) /*0xbc5987*/
  {
    CLogo::LogoEnd(this);
    return;
  }

  // Video ending - fade out and end
  if ((m_videoState - 4) <= 1)  // FADEOUT or END state
  {
    CStage::FadeOut(this, 0);
    CLogo::LogoEnd(v1);
    return;
  }

  // Get video status from graphics engine
  int videoStatus = 0;
  g_gr.m_pInterface->get_videoStatus(g_gr.m_pInterface, &videoStatus);

  switch (videoStatus)
  {
    case 1:  // Loading
    case 3:  // Playing
      return;  // Continue waiting

    case 2:  // Ready to play
      // Start video playback
      IWzGr2D::PlayVideo(g_gr.m_pInterface);

      // Set volume based on BGM volume setting
      unsigned int volume = 255 * TSingleton<CSoundMan>::ms_pInstance->m_uBGMVolume / 100;
      if (volume > 255) volume = 255;
      IWzGr2D::PutvideoVolume(g_gr.m_pInterface, volume);

      // Enable video mode
      IWzGr2D::SetVideoMode(g_gr.m_pInterface, 1);

      v1->m_videoState = VIDEO_STATE_PLAYING;
      v1->m_dwTickInitial = timeGetTime();
      v1->m_bTickInitial = 1;
      break;

    case 5:  // Video complete
      v1->m_videoState = VIDEO_STATE_END;
      break;

    default:
      CStage::FadeOut(this, 0);
      CLogo::LogoEnd(v1);
      break;
  }
}
```

### Video States:
| Status | Description |
|--------|-------------|
| 1 | Loading |
| 2 | Ready to play |
| 3 | Playing |
| 5 | Complete |

---

## CLogo::InitWZLogo - 0xbc5b20

This is a very long function. Key operations:

```cpp
void __thiscall CLogo::InitWZLogo(CLogo *this)
{
  // 1. Set video path from StringPool 0x95A
  // StringPool 0x95A = video file path
  g_gr.m_pInterface->raw_SetVideoPath(m_pInterface, videoPath, 0, 1);
  this->m_videoState = (result >= 0);  // Set based on success

  // 2. Load WZ properties
  // StringPool 0x958 = "UI/Logo.img/Gamania"
  this->m_pLogoProp = GetObject("UI/Logo.img/Gamania");
  this->m_nLogoCount = this->m_pLogoProp->Getcount() + 50;

  // 3. Load Grade property
  this->m_pGradeProp = GetObject("UI/Logo.img/Grade");
  this->m_nGradeCount = this->m_pGradeProp->Getcount();

  // 4. Load Message property
  this->m_pMessageProp = GetObject("UI/Logo.img/Message");

  // 5. Create background layer (800x600, z=0)
  // StringPool 0x66D = canvas class name
  this->m_pLayerBackground = CreateLayer(0, 0, 0, 0, z=0);
  InsertCanvas(800x600 black canvas);
  Getalpha()->RelMove(255, 255);  // Full opacity

  // 6. Create main layer (800x600, z=0)
  this->m_pLayerMain = CreateLayer(0, 0, 0, 0, z=0);
  InsertCanvas(800x600 canvas with 0x00FFFFFF color);
  Getalpha()->RelMove(255, 255);  // Full opacity

  // 7. Flush cached objects
  g_rm.m_pInterface->raw_FlushCachedObjects(0);
}
```

### WZ Resource Paths:
| Resource | Path |
|----------|------|
| Logo images | UI/Logo.img/Gamania |
| Grade images | UI/Logo.img/Grade |
| Message images | UI/Logo.img/Message |
| Video file | StringPool 0x95A |

### Layer Structure:
- **Background Layer**: 800x600, black, alpha=255
- **Main Layer**: 800x600, transparent, alpha=255

---

## CLogo::DrawWZLogo - 0xbc71a0

```cpp
void __thiscall CLogo::DrawWZLogo(CLogo *this, int nFrame)
{
  if (!this->m_pLayerMain.m_pInterface)
    return;

  // Clear main layer to black
  Getcanvas(m_pLayerMain, 0)->FillRect(0, 0, 800, 600, -1);  // -1 = black

  if (nFrame < 0)
    return;

  int nGradeAlpha = 255;

  if (nFrame < 50)
  {
    // Phase 1: Message display (frames 0-49)
    // Load message canvas from m_pMessageProp["0"]
    IWzCanvas *messageCanvas = m_pMessageProp->Getitem("0");

    // Calculate alpha based on frame (fade in/out)
    float alpha;
    if (nFrame >= 25)
      alpha = (50 - nFrame);  // Fade out
    else
      alpha = nFrame;  // Fade in
    alpha = alpha / 25.0 * 255.0;

    // Copy message to main layer with alpha
    Getcanvas(m_pLayerMain, 0)->Copy(-cx, -cy, messageCanvas, alpha);
  }
  else
  {
    // Phase 2: Logo display (frames 50+)
    int logoFrame = nFrame - 50;

    // Play logo sound on first logo frame
    if (!this->m_bLogoSoundPlayed)
    {
      // StringPool 0x83E = logo sound path
      CSoundMan::PlayBGM(soundPath, 0, 0, 0, 0, 0);
      this->m_bLogoSoundPlayed = 1;
    }

    // Fade out effect after frame 36
    if (logoFrame >= 36)
    {
      int fadeProgress = logoFrame - 36;
      int fadeTotal = m_pLogoProp->Getcount() - 36;
      nGradeAlpha = 255 - (fadeProgress / fadeTotal * 255);
    }

    // Load logo canvas for current frame
    IWzCanvas *logoCanvas = m_pLogoProp->Getitem(itow(logoFrame));
    if (logoCanvas)
    {
      Getcanvas(m_pLayerMain, 0)->Copy(-cx, -cy, logoCanvas, 255);
    }
  }

  // Draw grade overlays with calculated alpha
  for (int i = 0; i < m_nGradeCount; i++)
  {
    IWzCanvas *gradeCanvas = m_pGradeProp->Getitem(itow(i));
    if (gradeCanvas)
    {
      Getcanvas(m_pLayerMain, 0)->Copy(-cx, -cy, gradeCanvas, nGradeAlpha);
    }
  }
}
```

### Frame Timeline:
| Frame Range | Phase | Description |
|-------------|-------|-------------|
| 0-24 | Message Fade In | Alpha increases from 0 to 255 |
| 25-49 | Message Fade Out | Alpha decreases from 255 to 0 |
| 50+ | Logo Display | Logo frames shown |
| 86+ | Logo Fade Out | Grade alpha fades |

---

## CLogo::LogoEnd - 0xbc5890

```cpp
void __thiscall CLogo::LogoEnd(CLogo *this)
{
  // Stop video playback
  g_gr.m_pInterface->raw_StopVideo();
  g_gr.m_pInterface->raw_SetVideoMode(0);

  // Set program state to Login_DataLoadingState
  ProgramState::SetProgramState(Login_DataLoadingState);

  // Create and set CLogin as the new stage
  CLogin *v2 = new CLogin();
  if (v2)
    CLogin::CLogin(v2);
  set_stage(v2, 0);
}
```

### Key Points:
- Stops video playback
- Sets program state to `Login_DataLoadingState`
- Creates new `CLogin` stage
- Transitions to login stage

---

## CLogo::Close - 0xbc7170

```cpp
void __thiscall CLogo::Close(CLogo *this)
{
  // Stop BGM with 1000ms fade out
  CSoundMan::PlayBGM(0, 0, 0, 0x3E8u, 0, 0);  // 0x3E8 = 1000ms

  // Call base class close
  CStage::Close(this);
}
```

### Key Points:
- Fades out BGM over 1000ms
- Calls base `CStage::Close()`

---

## CLogo::OnKey - 0xbc7900

```cpp
void __thiscall CLogo::OnKey(CLogo *this, unsigned int wParam, signed int lParam)
{
  // Check if skip is allowed
  unsigned int elapsed = ZAPI.timeGetTime() - this->m_dwTickInitial;

  if (this->m_bVideoMode)
  {
    if (elapsed < 0x1388)  // 5000ms minimum in video mode
      return;
  }
  else
  {
    if ((10 * elapsed / 0x3E8) < 50)  // 5 seconds minimum in logo mode
      return;
  }

  // Handle skip keys (Enter=13, Escape=27, Space=32)
  if (lParam >= 0 && (wParam == 13 || wParam == 27 || wParam == 32))
  {
    if (this->m_bVideoMode)
    {
      this->m_videoState = VIDEO_STATE_FADEOUT;
    }
    else if (!this->m_dwClick)
    {
      this->m_dwClick = ZAPI.timeGetTime();
      CLogo::DrawWZLogo(this, 76);  // Draw final frame
    }
  }
}
```

### Skip Keys:
- Enter (13)
- Escape (27)
- Space (32)

### Skip Timing:
- **Logo mode**: Must wait ~5 seconds (frame 50)
- **Video mode**: Must wait 5000ms (0x1388)

---

## CLogo::OnMouseButton - 0xbc7990

```cpp
void __thiscall CLogo::OnMouseButton(CLogo *this, unsigned int msg, unsigned int wParam, int rx, int ry)
{
  // Check if skip is allowed
  unsigned int elapsed = ZAPI.timeGetTime() - this->m_dwTickInitial;

  if (this->m_bVideoMode)
  {
    if (elapsed < 0x1388)  // 5000ms minimum
      return;
  }
  else
  {
    if ((10 * elapsed / 0x3E8) < 50)  // 5 seconds minimum
      return;
  }

  // Handle left mouse button up (WM_LBUTTONUP = 514)
  if (msg == 514)
  {
    if (this->m_bVideoMode)
    {
      this->m_videoState = VIDEO_STATE_FADEOUT;
    }
    else if (!this->m_dwClick)
    {
      this->m_dwClick = ZAPI.timeGetTime();
      CLogo::DrawWZLogo(this, 76);
    }
  }
}
```

### Mouse Skip:
- Left mouse button up (WM_LBUTTONUP = 514)
- Same timing requirements as keyboard

---

## CLogo::CanSkip - 0xbc55f0

```cpp
bool __thiscall CLogo::CanSkip(CLogo *this)
{
  if (!this->m_bTickInitial)
    return false;

  unsigned int elapsed = ZAPI.timeGetTime() - this->m_dwTickInitial;

  if (this->m_bVideoMode)
    return elapsed >= 0x1388;  // 5000ms
  else
    return (10 * elapsed / 0x3E8) >= 50;  // ~5 seconds
}
```

### Skip Conditions:
- **Logo mode**: `(elapsed_ms * 10 / 1000) >= 50` → `elapsed >= 5000ms`
- **Video mode**: `elapsed >= 5000ms`

---

## CLogo::ForcedEnd - 0xbc78d0

```cpp
void __thiscall CLogo::ForcedEnd(CLogo *this)
{
  if (this->m_bVideoMode)
  {
    this->m_videoState = VIDEO_STATE_FADEOUT;
  }
  else if (!this->m_dwClick)
  {
    this->m_dwClick = ZAPI.timeGetTime();
    CLogo::DrawWZLogo(this, 76);
  }
}
```

### Key Points:
- Forces logo/video to end
- In video mode: sets state to FADEOUT
- In logo mode: triggers click skip sequence

---

## Logo Flow Summary

```
CLogo::Init()
    ↓
CLogo::InitWZLogo()  ← Load WZ resources, create layers
    ↓
CLogo::Update() [called every frame]
    ↓
┌───────────────────────────────────────┐
│  m_bVideoMode == false?               │
│  → CLogo::UpdateLogo()                │
│    └─ CLogo::DrawWZLogo(frame)        │
│       └─ Frame 0-49: Message          │
│       └─ Frame 50+: Logo + Grade      │
│                                       │
│  After all frames or click:           │
│  → m_bVideoMode = true                │
└───────────────────────────────────────┘
    ↓
┌───────────────────────────────────────┐
│  m_bVideoMode == true?                │
│  → CLogo::UpdateVideo()               │
│    └─ Play video if available         │
│    └─ Handle video states             │
│                                       │
│  After video ends or skip:            │
│  → CLogo::LogoEnd()                   │
└───────────────────────────────────────┘
    ↓
CLogo::LogoEnd()
    ↓
set_stage(new CLogin(), 0)  ← Transition to Login
```

### Timing Constants:
| Constant | Value | Description |
|----------|-------|-------------|
| 0x3E8 | 1000 | Milliseconds per second (used in frame calc) |
| 0x5DC | 1500 | Click to video mode delay |
| 0x1388 | 5000 | Minimum video display time |
| 50 | 50 | Minimum logo frames before skip |
| 76 | 76 | Final frame for skip display |
