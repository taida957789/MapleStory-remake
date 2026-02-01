# MapleStory Client Recreation Project

## Project Overview

This is a recreation of the MapleStory client based on reverse engineering of Korean MapleStoryT v1029 with PDB symbols. The goal is to create a clean, cross-platform implementation using SDL3 and modern C++17 while maintaining the original architecture.

## Tech Stack

- **Language**: C++17
- **Graphics/Audio/Input**: SDL3 (via git submodule in `3rdparty/SDL`)
- **Build System**: CMake 3.16+
- **Testing**: Google Test (FetchContent)
- **Reference**: https://github.com/PShocker/sdlMS (similar project)

## Source Binary

- File: `MapleStoryT.exe` (Korean MapleStoryT v1029)
- Some strings using StringPool is located ./reources/strings.csv
- Contains PDB symbols for accurate class/function names
- Available via IDA Pro MCP server for decompilation

## Key Original Classes

### StringPool
- Some strings using StringPool is located ./reources/strings.csv

### Core
- `CWvsApp` - Main application (singleton)
- `CWvsContext` - Global game context (singleton)
- `CConfig` - Configuration manager (singleton)

### Stages (Scenes)
- `CStage` - Base stage class
- `CLogo` - Logo/intro screen
- `CLogin` - Login and character selection
- `CField` - Main game field/map
- `CCashShop` - Cash shop
- `CMonsterFarm` - Monster farm

### Systems
- `CClientSocket` - Network communication
- `CInputSystem` - DirectInput wrapper
- `IWzResMan` - WZ resource manager (COM interface)
- `IWzGr2D` - 2D graphics (COM interface)

### Utility
- `TSingleton<T>` - Singleton template
- `ZXString<char>` - Reference-counted string
- `ZRef<T>` - Smart pointer with reference counting
- `ZList<T>`, `ZArray<T>`, `ZMap<K,V>` - Container classes

## MCP IDA Tools Usage

Use these MCP tools to analyze the original binary:

```
mcp__sse-server__get_decompiled_func(ea)  - Get pseudocode
mcp__sse-server__get_function_name(ea)     - Get function name
mcp__sse-server__get_functions()           - List all functions
mcp__sse-server__get_imports()             - Get imported functions
mcp__sse-server__get_strings()             - Get string table
mcp__sse-server__get_xrefs_to(ea)          - Get cross-references
```

## Important Addresses

- Entry Point: `0x1BF7039` (_WinMainCRTStartup)
- WinMain: `0x1A88E80`
- CWvsApp::CWvsApp: `0x1A95390`
- CWvsApp::SetUp: `0x1A98B50`
- CWvsApp::Run: `0x1A94A60`
- CLogo::CLogo: `0xBC5420`
- CLogo::Update: `0xBC7A90`
- CLogin::Update: `0xB6B4D0`
- set_stage: `0xFC3B10`

## Development Guidelines

1. **Naming Convention**: Keep original class/function names when possible
2. **Architecture**: Mirror the original singleton pattern and class hierarchy
3. **Comments**: Document original addresses and behaviors
4. **Testing**: Write unit tests for utility classes and packet handling
5. **Cross-Platform**: Use SDL3 abstractions instead of Windows-specific APIs

## Modern C++ Coding Style

### General Principles

- Use **C++17** features consistently
- Prefer **compile-time safety** over runtime checks
- Follow the **Rule of Zero/Five** for resource management
- Minimize raw pointer usage; prefer smart pointers and RAII

### Trailing Return Types

Use trailing return type syntax (`auto ... -> Type`) for all function declarations:

```cpp
// Good
[[nodiscard]] auto GetWidth() const noexcept -> int;
auto Initialize() -> bool;
static auto GetInstance() noexcept -> T&;

// Avoid
int GetWidth() const noexcept;
bool Initialize();
```

### Attributes

Use C++17 attributes consistently:

```cpp
// [[nodiscard]] - for functions whose return value should not be ignored
[[nodiscard]] auto GetInstance() noexcept -> T&;
[[nodiscard]] auto IsEmpty() const noexcept -> bool;

// [[maybe_unused]] - for intentionally unused parameters
void ProcessEvent([[maybe_unused]] int eventType);

// noexcept - mark functions that don't throw
auto GetWidth() const noexcept -> int { return m_nWidth; }
```

### Const Correctness

- Mark all non-modifying methods as `const`
- Use `const` for variables that don't change
- Prefer `const auto&` for read-only references

```cpp
[[nodiscard]] auto GetName() const noexcept -> const std::string&;
const auto& data = GetPixelData();
```

### Modern Initialization

Use brace initialization and `auto`:

```cpp
// Good
int m_nWidth{};           // Value-initialized to 0
auto count = static_cast<std::size_t>(length);
auto ptr = std::make_unique<Widget>();

// Avoid
int m_nWidth = 0;
size_t count = (size_t)length;
Widget* ptr = new Widget();
```

### Smart Pointers & RAII

```cpp
// Unique ownership
std::unique_ptr<Resource> m_resource;

// Shared ownership (use sparingly)
std::shared_ptr<Stage> m_pStage;

// Non-owning reference (prefer raw pointer or reference)
SDL_Renderer* m_pRenderer = nullptr;  // SDL owns this
```

### Templates & Type Traits

Use `if constexpr` and type traits for compile-time branching:

```cpp
template <typename CharT>
[[nodiscard]] constexpr auto GetEmptyStr() noexcept -> const CharT*
{
    if constexpr (std::is_same_v<CharT, char>)
        return "";
    else if constexpr (std::is_same_v<CharT, wchar_t>)
        return L"";
}
```

### Class Design

```cpp
class MyClass final : public BaseClass
{
public:
    // Constructors
    MyClass();
    explicit MyClass(int value);

    // Rule of Five (if needed)
    ~MyClass() override;
    MyClass(const MyClass&) = delete;
    auto operator=(const MyClass&) -> MyClass& = delete;
    MyClass(MyClass&&) noexcept;
    auto operator=(MyClass&&) noexcept -> MyClass&;

    // Public interface with [[nodiscard]] for getters
    [[nodiscard]] auto GetValue() const noexcept -> int;
    void SetValue(int value) noexcept;

private:
    int m_nValue{};
};
```

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Classes | PascalCase | `WzCanvas`, `InputSystem` |
| Functions | PascalCase | `GetInstance()`, `Initialize()` |
| Member variables | m_ prefix + camelCase | `m_nWidth`, `m_pTexture` |
| Local variables | camelCase | `pixelData`, `currentTime` |
| Constants | ALL_CAPS or k prefix | `MAX_SIZE`, `kDefaultValue` |
| Namespaces | lowercase | `ms`, `detail` |
| Template params | PascalCase | `typename CharT` |

### Hungarian Notation (Legacy Compatible)

For consistency with original MapleStory code:

| Prefix | Type | Example |
|--------|------|---------|
| `n` | int/numeric | `m_nWidth` |
| `b` | bool | `m_bInitialized` |
| `s` | string | `m_sPath` |
| `p` | pointer | `m_pTexture` |
| `t` | time/tick | `m_tLastUpdate` |
| `dw` | DWORD/uint32 | `m_dwFlags` |

### Standard Library Usage

```cpp
// Prefer standard algorithms
#include <algorithm>
m_nVolume = std::clamp(volume, 0, 100);

// Use string_view for non-owning strings
void ProcessPath(std::string_view path);

// Use optional for nullable values
[[nodiscard]] auto FindChild(const std::string& name) -> std::optional<WzProperty>;
```

### Error Handling

- Use return values (`bool`, `std::optional`, `std::expected` in C++23)
- Avoid exceptions in performance-critical code
- Use `assert` for programmer errors (debug only)

```cpp
[[nodiscard]] auto Initialize() -> bool
{
    if (!InitializeGraphics())
    {
        std::cerr << "Failed to initialize graphics\n";
        return false;
    }
    return true;
}
```

### Include Order

```cpp
#include "LocalHeader.h"        // 1. Related header

#include "project/OtherLocal.h" // 2. Project headers

#include <SDL3/SDL.h>           // 3. Third-party headers

#include <algorithm>            // 4. Standard library
#include <memory>
#include <string>
```

### File Organization

```cpp
// Header file (.h)
#pragma once

#include <dependencies>

namespace ms
{

class MyClass
{
    // ...
};

} // namespace ms

// Source file (.cpp)
#include "MyClass.h"

namespace ms
{

// Implementation

} // namespace ms
```

## Build Instructions

```bash
# Clone with submodules
git clone --recursive <repo-url>
# Or if already cloned:
git submodule update --init --recursive

# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
./MapleStory
```

## Runtime Configuration

### WZ Path

The WZ resource files location can be configured:

- **Default path**: `resources/old`
- **Command line**: `./MapleStory --wz-path <path>` or `./MapleStory -w <path>`

```bash
# Use default path (resources/old)
./MapleStory

# Use custom WZ path
./MapleStory --wz-path /path/to/wz/files
./MapleStory -w ../my-wz-files
```

If the configured path doesn't exist, the application will search common locations:
- `./UI.wz`
- `../UI.wz`
- `./data/UI.wz`
- `./resources/old/UI.wz`

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Build type (Debug/Release) |
| `BUILD_TESTS` | ON | Build test suite |
| `SDL_X11_XTEST` | OFF | Disable X11 XTEST (may need install) |

## Testing

Tests use **Google Test** framework (automatically fetched via CMake FetchContent).

```bash
cd build
ctest --verbose

# Or run directly with more details
./tests/maplestory_tests
```

Test coverage:
- `test_packet.cpp` - Network packet encode/decode
- `test_point.cpp` - Point2D, Point2DF, Rect structures
- `test_wz.cpp` - WzProperty tree, WzCanvas

## Current Progress

- [x] Project structure created
- [x] Application class (CWvsApp equivalent)
- [x] WvsContext class (CWvsContext equivalent)
- [x] Stage base class
- [x] Logo stage
- [x] Login stage (basic)
- [x] Input system
- [x] Audio system
- [x] Network packet classes
- [x] WZ resource system (stubs)
- [ ] WZ file parsing
- [ ] Field stage (main game)
- [ ] Character system
- [ ] UI system
- [ ] Complete networking

## File Layout

```
/home/t4si/Desktop/repos/maplestory/
├── CMakeLists.txt
├── claude.md (this file)
├── 3rdparty/
│   └── SDL/              # SDL3 git submodule
├── docs/
│   └── ARCHITECTURE.md
├── src/
│   ├── main.cpp
│   ├── app/              # Application, Configuration, WvsContext
│   ├── stage/            # Stage base, Logo, Login, Field
│   ├── wz/               # WZ resource system
│   ├── input/            # Input handling
│   ├── audio/            # Sound system
│   ├── network/          # Packets, ClientSocket
│   └── util/             # Singleton, ZXString, Point
├── tests/
│   ├── CMakeLists.txt
│   ├── test_packet.cpp
│   ├── test_point.cpp
│   └── test_wz.cpp
├── build/                # CMake build directory (gitignored)
└── resources/
```

## Notes for Future Development

1. The original uses COM interfaces (IWzGr2D, IWzResMan) for graphics and resources
2. Packet encryption uses AES with rolling IVs
3. WZ files use various compression formats (zlib, lz4)
4. Character animations are complex state machines
5. The field system has many sub-components (portals, footholds, ladders, etc.)
6. SDL3_mixer may be needed for full audio support (currently stub implementation)
7. Consider SDL3_image and SDL3_ttf submodules for image/font loading
