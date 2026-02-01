# MapleStory Client Architecture

This document describes the architecture of the MapleStory client recreation, based on reverse engineering of the Korean MapleStoryT v1029 executable.

## Overview

The original MapleStory client is a Windows application written in C++ using:
- **DirectX 9** for 2D rendering (through WzGr2D COM interface)
- **DirectInput 8** for input handling
- **Miles Sound System (MSS32)** for audio
- **WinSock 2** for networking
- **Awesomium** for embedded web browser (cash shop, etc.)

This recreation uses:
- **SDL2** for rendering, input, and audio
- **C++17** standard
- **CMake** build system

## Main Components

### 1. Application Layer (src/app/)

#### Application (CWvsApp)
The main application class, singleton pattern. Responsible for:
- Window creation and management
- Main game loop
- Subsystem initialization
- Stage management

Original WinMain flow:
```cpp
// CWvsContext singleton (global game state)
TSingleton<CWvsContext>::CreateInstance();

// Create app
CWvsApp::CWvsApp(sCmdLine);

// Initialize all subsystems
CWvsApp::SetUp();

// Main loop
CWvsApp::Run(&msg, &bTerminate);

// Cleanup
CWvsApp::~CWvsApp();
TSingleton<CWvsContext>::DestroyInstance();
```

#### WvsContext (CWvsContext)
Global game context singleton. Holds:
- Character data
- Basic/Secondary stats
- World/Channel information
- Screen resolution management
- UI state management

#### Configuration (CConfig)
Configuration singleton. Manages:
- Screen settings
- Audio settings
- Key bindings
- UI positions

### 2. Stage System (src/stage/)

Stages represent different game screens/states.

#### Base Class: Stage (CStage)
All stages inherit from CStage which implements:
- `IGObj` - Game object interface
- `IUIMsgHandler` - UI message handler
- `INetMsgHandler` - Network packet handler
- `ZRefCounted` - Reference counting

Key virtual methods:
- `Init(void* param)` - Initialize stage
- `Update()` - Update logic
- `Close()` - Cleanup
- `OnSetFocus(int)` - Focus handling

#### Stage Transitions
Global function `set_stage(CStage* pStage, void* pParam)`:
1. Close old stage (calls `Close()`)
2. Handle camera transitions
3. Update screen resolution based on stage type
4. Initialize new stage (calls `Init()`)
5. Reset camera rotation

#### Known Stages:
| Class | Purpose |
|-------|---------|
| CLogo | Company logos, game rating |
| CLogin | Login, world/channel/character selection |
| CField | Main game map |
| CCashShop | Cash shop |
| CMonsterFarm | Monster farm mini-game |
| CInterStage | Transition between stages |

### 3. WZ Resource System (src/wz/)

MapleStory uses WZ files for resources (UI, maps, characters, etc.).

#### WzResMan (IWzResMan)
Resource manager COM interface:
- Loads and caches WZ file contents
- `GetProperty(path)` - Get property node
- `FlushCachedObjects()` - Clear cache

#### WzProperty (IWzProperty)
Property node in WZ file tree:
- Can contain: int, long, float, double, string, canvas, vector, sound
- Has children (sub-nodes)
- Path format: `"UI.wz/Login.img/Title/0"`

#### WzCanvas (IWzCanvas)
Image data from WZ files:
- Compressed bitmap data (various formats)
- Origin point for sprites
- Z-ordering value

### 4. Input System (src/input/)

#### InputSystem (CInputSystem)
Input handling using DirectInput (original) / SDL2 (recreation):
- Keyboard state tracking
- Mouse position and button state
- Input message queue (ISMSG)

Original uses three DirectInput devices:
- Keyboard
- Mouse
- Joystick (optional)

### 5. Audio System (src/audio/)

#### SoundSystem
Based on Miles Sound System (MSS32):
- `AIL_quick_startup/shutdown` - Initialize/shutdown
- `AIL_quick_load_mem` - Load sound from memory
- `AIL_quick_play` - Play sound
- `AIL_quick_set_volume` - Set volume
- `AIL_quick_set_ms_position` - Seek

### 6. Network System (src/network/)

#### ClientSocket (CClientSocket)
TCP socket for server communication:
- Connect to login/game servers
- Send/receive encrypted packets
- Packet queue management

#### Packet Classes
- `InPacket (CInPacket)` - Read incoming packets
- `OutPacket (COutPacket)` - Build outgoing packets

Packet format (little-endian):
```
[2 bytes: length] [2 bytes: opcode] [payload...]
```

Encryption: AES with rolling IV.

### 7. UI System (src/ui/)

#### LayoutMan

UI 自動化構建和管理系統。負責：
- 從 WZ 資源自動創建 UI 元素
- 按名稱管理按鈕和圖層
- 提供批量操作接口

使用映射表 (std::map) 管理控件，支持 `type:name` 格式的 WZ 屬性解析。

詳見：`docs/LayoutMan.md`

## Class Hierarchy

```
ZRefCounted
└── CStage
    ├── CLogo
    ├── CLogin
    ├── CField
    ├── CCashShop
    ├── CMonsterFarm
    └── CInterStage

TSingleton<T>
├── CWvsApp
├── CWvsContext
├── CConfig
├── CClientSocket
├── CChatSocket
├── CInputSystem
├── CActionMan
├── CQuestMan
├── CFieldStatMan
└── ... (many more)
```

## Memory Management

Original uses custom allocators:
- `ZAllocEx<ZAllocAnonSelector>` - General allocation
- `ZAllocEx<ZAllocStrSelector<char>>` - String allocation
- Reference counting with `ZRefCounted` / `ZRef<T>`

Recreation uses:
- `std::shared_ptr` / `std::unique_ptr`
- Standard allocators

## Threading Model

Original uses:
- Main thread: UI, input, rendering
- `CLoadClientDataThread` - Background data loading
- `CAddRenderThread` - Additional render thread
- `CLoadMobActThread` - Mob action loading

## File Structure

```
src/
├── app/           # Application core
│   ├── Application.h/cpp
│   ├── Configuration.h/cpp
│   └── WvsContext.h/cpp
├── stage/         # Game stages
│   ├── Stage.h/cpp
│   ├── Logo.h/cpp
│   ├── Login.h/cpp
│   └── Field.h/cpp (TODO)
├── wz/            # WZ resource system
│   ├── WzResMan.h/cpp
│   ├── WzProperty.h/cpp
│   └── WzCanvas.h/cpp
├── input/         # Input handling
│   └── InputSystem.h/cpp
├── audio/         # Audio system
│   └── SoundSystem.h/cpp
├── network/       # Networking
│   ├── ClientSocket.h/cpp
│   ├── InPacket.h/cpp
│   └── OutPacket.h/cpp
├── field/         # Game field (TODO)
├── user/          # Player character (TODO)
├── mob/           # Monsters (TODO)
├── npc/           # NPCs (TODO)
├── life/          # Life objects (TODO)
├── ui/            # UI system (TODO)
└── util/          # Utilities
    ├── Singleton.h
    ├── Point.h
    └── ZXString.h/cpp
```

## TODO

- [ ] Implement WZ file parsing
- [ ] Implement CField (main game map)
- [ ] Implement CUser/CUserLocal (player)
- [ ] Implement CMob/CMobPool (monsters)
- [ ] Implement UI system (CWnd, CWndMan)
- [ ] Implement skill system
- [ ] Implement item system
- [ ] Network protocol implementation
- [ ] Packet encryption/decryption
