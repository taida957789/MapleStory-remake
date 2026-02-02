# Modern C++ UI Lifecycle Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Implement modern C++ lifecycle management for UIElement and Stage classes with RAII, smart pointers, and proper Create/Destroy pattern.

**Architecture:**
- Add two-phase construction (Constructor → OnCreate → OnDestroy → Destructor) to UIElement
- Replace singleton pattern in UI classes with proper ownership via std::unique_ptr
- Implement RAII wrappers for graphics resources (LayerHandle)
- Add Result<T> type for robust error handling
- Refactor Login::SetupStep1UI to use Create/Destroy pattern instead of GetInstance

**Tech Stack:** C++17, std::optional, std::any, std::unique_ptr, std::shared_ptr, std::reference_wrapper

---

## Phase 1: Foundation Infrastructure

### Task 1: Create Result<T> Error Handling Type

**Files:**
- Create: `src/util/Result.h`
- Test: Manual verification (no formal test framework yet)

**Step 1: Write Result<T> header**

Create `src/util/Result.h`:

```cpp
#pragma once

#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace ms
{

/**
 * @brief Modern C++ error handling type
 *
 * Result<T> represents either a successful value of type T or an error message.
 * Forces explicit error checking via [[nodiscard]] attribute.
 *
 * Usage:
 *   auto result = SomeFunction();
 *   if (!result) {
 *       LOG_ERROR("Failed: {}", result.error());
 *       return;
 *   }
 *   auto value = result.value();
 */
template<typename T>
class [[nodiscard]] Result
{
public:
    // Success construction
    static auto Success(T value = T{}) -> Result
    {
        return Result(std::move(value));
    }

    // Error construction with formatting
    template<typename... Args>
    static auto Error(std::format_string<Args...> fmt, Args&&... args) -> Result
    {
        return Result(std::format(fmt, std::forward<Args>(args)...));
    }

    // Check if successful
    [[nodiscard]] explicit operator bool() const noexcept
    {
        return m_value.has_value();
    }

    // Get value (throws if error)
    [[nodiscard]] auto value() const -> const T&
    {
        if (!m_value)
        {
            throw std::runtime_error(m_error);
        }
        return *m_value;
    }

    // Get value (throws if error)
    [[nodiscard]] auto value() -> T&
    {
        if (!m_value)
        {
            throw std::runtime_error(m_error);
        }
        return *m_value;
    }

    // Get error message
    [[nodiscard]] auto error() const noexcept -> std::string_view
    {
        return m_error;
    }

private:
    explicit Result(T value) : m_value(std::move(value)) {}
    explicit Result(std::string error) : m_error(std::move(error)) {}

    std::optional<T> m_value;
    std::string m_error;
};

// Specialization for void (success/failure only)
template<>
class [[nodiscard]] Result<void>
{
public:
    static auto Success() -> Result { return Result(true); }

    template<typename... Args>
    static auto Error(std::format_string<Args...> fmt, Args&&... args) -> Result
    {
        return Result(std::format(fmt, std::forward<Args>(args)...));
    }

    [[nodiscard]] explicit operator bool() const noexcept { return m_success; }
    [[nodiscard]] auto error() const noexcept -> std::string_view { return m_error; }

private:
    explicit Result(bool success) : m_success(success) {}
    explicit Result(std::string error) : m_success(false), m_error(std::move(error)) {}

    bool m_success{false};
    std::string m_error;
};

} // namespace ms
```

**Step 2: Verify compilation**

Run: `make` or build system command
Expected: Clean compilation with no errors

**Step 3: Commit**

```bash
git add src/util/Result.h
git commit -m "feat: add Result<T> error handling type

- Modern C++ alternative to error codes
- [[nodiscard]] forces explicit error checking
- Template specialization for void case
- Supports std::format for error messages"
```

---

### Task 2: Create LifecycleState Enum

**Files:**
- Modify: `src/ui/UIElement.h` (add before UIElement class)

**Step 1: Add LifecycleState enum class**

Add to `src/ui/UIElement.h` after includes, before UIState enum:

```cpp
/**
 * @brief UI lifecycle states for two-phase construction pattern
 *
 * State machine:
 *   Constructed -> Created -> Destroyed
 *
 * Prevents calling methods in invalid states (e.g., Update before OnCreate).
 */
enum class LifecycleState : std::uint8_t
{
    Constructed,  // Object created, OnCreate not yet called
    Created,      // OnCreate complete, ready to use
    Destroyed     // OnDestroy called, resources released
};

/**
 * @brief Convert lifecycle state to string for debugging
 */
constexpr auto to_string(LifecycleState state) noexcept -> std::string_view
{
    using enum LifecycleState;
    switch (state)
    {
        case Constructed: return "Constructed";
        case Created: return "Created";
        case Destroyed: return "Destroyed";
    }
    return "Unknown";
}

/**
 * @brief Validate state transition
 */
constexpr bool is_valid_transition(LifecycleState from, LifecycleState to) noexcept
{
    using enum LifecycleState;
    switch (from)
    {
        case Constructed: return to == Created;
        case Created: return to == Destroyed;
        case Destroyed: return false;  // Terminal state
    }
    return false;
}
```

**Step 2: Verify compilation**

Run: `make`
Expected: Clean compilation

**Step 3: Commit**

```bash
git add src/ui/UIElement.h
git commit -m "feat: add LifecycleState enum for UI lifecycle tracking

- Strong typing with enum class
- State machine: Constructed -> Created -> Destroyed
- Compile-time state validation with constexpr"
```

---

### Task 3: Add Lifecycle Methods to UIElement

**Files:**
- Modify: `src/ui/UIElement.h` (add lifecycle methods)
- Modify: `src/ui/UIElement.cpp` (implement lifecycle methods)

**Step 1: Add includes to UIElement.h**

Add to top of `src/ui/UIElement.h`:

```cpp
#include "util/Result.h"

#include <any>
```

**Step 2: Add lifecycle members to UIElement class**

Add to private section of UIElement class in `src/ui/UIElement.h`:

```cpp
    // Lifecycle state
    LifecycleState m_lifecycleState{LifecycleState::Constructed};
```

**Step 3: Add lifecycle methods to UIElement public section**

Add to public section after destructor:

```cpp
    // ========== Lifecycle Management ==========

    /**
     * @brief Create UI with two-phase initialization
     *
     * Pattern from CWnd::OnCreate. Call after construction to initialize resources.
     *
     * @param params Creation parameters (type-safe via std::any)
     * @return Result<void> indicating success or error message
     */
    [[nodiscard]] auto Create(std::any params) -> Result<void>;

    /**
     * @brief Destroy UI and cleanup resources
     *
     * Pattern from CWnd::Destroy (0x1a67630 in v1029).
     * Calls OnDestroy, recursively destroys children, releases layers.
     * Safe to call multiple times (idempotent).
     */
    void Destroy() noexcept;

    /**
     * @brief Check if lifecycle is in Created state
     */
    [[nodiscard]] auto IsCreated() const noexcept -> bool
    {
        return m_lifecycleState == LifecycleState::Created;
    }

    /**
     * @brief Check if lifecycle is in Destroyed state
     */
    [[nodiscard]] auto IsDestroyed() const noexcept -> bool
    {
        return m_lifecycleState == LifecycleState::Destroyed;
    }

    /**
     * @brief Get current lifecycle state
     */
    [[nodiscard]] auto GetLifecycleState() const noexcept -> LifecycleState
    {
        return m_lifecycleState;
    }
```

**Step 4: Add virtual lifecycle hooks to UIElement protected section**

Add to protected section:

```cpp
    /**
     * @brief Virtual hook: create UI resources
     *
     * Override in derived classes to load WZ data, create layers, etc.
     * Base implementation does nothing and returns success.
     *
     * @param params Type-safe creation parameters via std::any
     * @return Result indicating success or error
     */
    virtual auto OnCreate(std::any params) -> Result<void>;

    /**
     * @brief Virtual hook: destroy UI resources
     *
     * Override in derived classes to cleanup specific resources.
     * Base implementation does nothing.
     * Must be noexcept.
     */
    virtual void OnDestroy() noexcept;
```

**Step 5: Implement lifecycle methods in UIElement.cpp**

Add to `src/ui/UIElement.cpp`:

```cpp
#include "util/Logger.h"
#include <stdexcept>

// ... existing code ...

// Add at end of file:

// ========== Lifecycle Management ==========

auto UIElement::Create(std::any params) -> Result<void>
{
    // Validate state transition
    if (!is_valid_transition(m_lifecycleState, LifecycleState::Created))
    {
        return Result<void>::Error("Invalid state transition: {} -> Created",
                                   to_string(m_lifecycleState));
    }

    // Call virtual OnCreate hook
    if (auto result = OnCreate(std::move(params)); !result)
    {
        return result;
    }

    // Update state
    m_lifecycleState = LifecycleState::Created;
    return Result<void>::Success();
}

void UIElement::Destroy() noexcept
{
    // Prevent double-destroy
    if (m_lifecycleState == LifecycleState::Destroyed)
    {
        return;
    }

    try
    {
        // 1. Call virtual OnDestroy (derived class cleanup)
        OnDestroy();

        // 2. Recursively destroy all children (reverse order, bottom-up)
        for (auto it = m_aChildren.rbegin(); it != m_aChildren.rend(); ++it)
        {
            if (*it)
            {
                (*it)->Destroy();
            }
        }
        m_aChildren.clear();

        // 3. Clear layer (RAII via shared_ptr)
        m_pLayer.reset();

        // 4. Clear parent/focus pointers
        m_pParent = nullptr;
        m_pFocusChild = nullptr;

        // 5. Update state
        m_lifecycleState = LifecycleState::Destroyed;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in UIElement::Destroy: {}", e.what());
        // Continue cleanup, don't rethrow (noexcept)
    }
}

auto UIElement::OnCreate(std::any /*params*/) -> Result<void>
{
    // Base implementation: do nothing
    return Result<void>::Success();
}

void UIElement::OnDestroy() noexcept
{
    // Base implementation: do nothing
}
```

**Step 6: Update UIElement destructor to check lifecycle**

Modify destructor in `src/ui/UIElement.cpp`:

```cpp
UIElement::~UIElement()
{
    // Defensive: ensure Destroy was called
    if (m_lifecycleState != LifecycleState::Destroyed)
    {
        LOG_WARN("UIElement destroyed without calling Destroy() - state: {}",
                 to_string(m_lifecycleState));
    }

#ifdef MS_DEBUG_CANVAS
    DebugOverlay::GetInstance().UnregisterUIElement(this);
#endif
}
```

**Step 7: Verify compilation**

Run: `make`
Expected: Clean compilation

**Step 8: Commit**

```bash
git add src/ui/UIElement.h src/ui/UIElement.cpp
git commit -m "feat: add lifecycle management to UIElement

- Implement Create/Destroy pattern from CWnd
- Add OnCreate/OnDestroy virtual hooks
- State tracking with LifecycleState enum
- Recursive child destruction in Destroy()
- Defensive check in destructor for missing Destroy call"
```

---

## Phase 2: Refactor UIWorldSelect to Use Lifecycle

### Task 4: Update UIWorldSelect to Use OnCreate/OnDestroy

**Files:**
- Modify: `src/ui/UIWorldSelect.h`
- Modify: `src/ui/UIWorldSelect.cpp`

**Step 1: Remove Singleton pattern from UIWorldSelect.h**

Remove in `src/ui/UIWorldSelect.h`:

```cpp
class UIWorldSelect final : public UIElement, public Singleton<UIWorldSelect>
{
    friend class Singleton<UIWorldSelect>;
```

Replace with:

```cpp
class UIWorldSelect final : public UIElement
{
```

**Step 2: Change constructor from private to public**

In `src/ui/UIWorldSelect.h`, move constructor from private to public:

```cpp
public:
    UIWorldSelect();
    ~UIWorldSelect() override;
```

Remove `private:` section that had `UIWorldSelect();`.

**Step 3: Add CreateParams struct to UIWorldSelect**

Add to public section in `src/ui/UIWorldSelect.h` after destructor:

```cpp
    /**
     * @brief Creation parameters for UIWorldSelect
     */
    struct CreateParams
    {
        Login* login{nullptr};
        WzGr2D* gr{nullptr};
        UIManager* uiManager{nullptr};

        [[nodiscard]] auto IsValid() const noexcept -> bool
        {
            return login != nullptr && gr != nullptr && uiManager != nullptr;
        }
    };
```

**Step 4: Replace OnCreate method signature in UIWorldSelect.h**

Replace:

```cpp
    void OnCreate(Login* pLogin, WzGr2D& gr, UIManager& uiManager);
```

With:

```cpp
protected:
    auto OnCreate(std::any params) -> Result<void> override;
    void OnDestroy() noexcept override;
```

**Step 5: Implement OnCreate in UIWorldSelect.cpp**

Find `UIWorldSelect::OnCreate` implementation and replace with:

```cpp
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

    // 3. Create LayoutMan (RAII)
    m_pLayoutMan = std::make_unique<LayoutMan>();
    auto lmResult = m_pLayoutMan->Init(this, 0, 0);
    if (!lmResult)
    {
        return Result<void>::Error("LayoutMan init failed: {}", lmResult.error());
    }

    // 4. Build UI from WZ
    auto buildResult = m_pLayoutMan->AutoBuild(m_sBaseUOL);
    if (!buildResult)
    {
        return Result<void>::Error("AutoBuild failed: {}", buildResult.error());
    }

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
```

**Step 6: Implement OnDestroy in UIWorldSelect.cpp**

Add to `src/ui/UIWorldSelect.cpp`:

```cpp
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

        // 5. Clear LayoutMan
        m_pLayoutMan.reset();

        // 6. Clear cached WZ property
        m_pWorldSelectProp.reset();

        // 7. Clear references (non-owning)
        m_pLogin = nullptr;
        m_pGr = nullptr;
        m_pUIManager = nullptr;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in UIWorldSelect::OnDestroy: {}", e.what());
    }
}
```

**Step 7: Update Destroy() method**

Replace `UIWorldSelect::Destroy()` implementation with:

```cpp
void UIWorldSelect::Destroy()
{
    // Call base class Destroy which calls OnDestroy
    UIElement::Destroy();
}
```

**Step 8: Verify compilation**

Run: `make`
Expected: Clean compilation with warnings about unused GetInstance calls in Login.cpp

**Step 9: Commit**

```bash
git add src/ui/UIWorldSelect.h src/ui/UIWorldSelect.cpp
git commit -m "refactor: remove singleton pattern from UIWorldSelect

- Replace GetInstance with normal construction
- Add CreateParams struct for type-safe initialization
- Implement OnCreate/OnDestroy lifecycle hooks
- Proper resource cleanup in OnDestroy
- Move constructor from private to public"
```

---

## Phase 3: Refactor Login to Own UIWorldSelect

### Task 5: Update Login to Create/Destroy UIWorldSelect

**Files:**
- Modify: `src/stage/Login.h`
- Modify: `src/stage/Login.cpp`

**Step 1: Add UIWorldSelect member to Login.h**

Add to private section of Login class:

```cpp
    // UI instances (owned)
    std::unique_ptr<UIWorldSelect> m_worldSelectUI;
```

**Step 2: Update SetupStep1UI in Login.cpp**

Find `Login::SetupStep1UI()` and replace with:

```cpp
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

    // Create UIWorldSelect with modern lifecycle
    m_worldSelectUI = std::make_unique<UIWorldSelect>();

    // Create with parameters
    auto params = UIWorldSelect::CreateParams{
        .login = this,
        .gr = &gr,
        .uiManager = &m_uiManager
    };

    auto result = m_worldSelectUI->Create(params);
    if (!result)
    {
        LOG_ERROR("Failed to create UIWorldSelect: {}", result.error());
        m_worldSelectUI.reset();
        return;
    }

    // Add to UI manager
    m_uiManager.AddElement("worldSelect", m_worldSelectUI);

    LOG_DEBUG("UIWorldSelect created successfully");
}
```

**Step 3: Update ClearStepUI in Login.cpp**

Find `Login::ClearStepUI()` and add:

```cpp
void Login::ClearStepUI()
{
    // Clear UIManager
    m_uiManager.Clear();

    // Destroy UIWorldSelect if exists
    if (m_worldSelectUI)
    {
        if (!m_worldSelectUI->IsDestroyed())
        {
            m_worldSelectUI->Destroy();
        }
        m_worldSelectUI.reset();
    }

    // ... existing cleanup code ...
}
```

**Step 4: Fix any remaining GetInstance() calls**

Search for `UIWorldSelect::GetInstance()` in Login.cpp and replace with `m_worldSelectUI`:

Example:
```cpp
// Before:
auto& worldSelect = UIWorldSelect::GetInstance();
worldSelect.SetFocusWorld(0);

// After:
if (m_worldSelectUI && m_worldSelectUI->IsCreated())
{
    m_worldSelectUI->SetFocusWorld(0);
}
```

**Step 5: Verify compilation**

Run: `make`
Expected: Clean compilation, no more GetInstance warnings

**Step 6: Test the changes**

Run: `./maplestory` or test executable
Expected: World select UI appears correctly, no crashes

**Step 7: Commit**

```bash
git add src/stage/Login.h src/stage/Login.cpp
git commit -m "refactor: replace singleton with owned UIWorldSelect

- Add std::unique_ptr<UIWorldSelect> m_worldSelectUI
- Update SetupStep1UI to use Create() pattern
- Update ClearStepUI to call Destroy() before reset
- Replace GetInstance calls with direct member access
- Proper RAII ownership via unique_ptr"
```

---

## Phase 4: Add LayoutMan Lifecycle Integration

### Task 6: Update LayoutMan to Return Result<T>

**Files:**
- Modify: `src/ui/LayoutMan.h`
- Modify: `src/ui/LayoutMan.cpp`

**Step 1: Add Result include to LayoutMan.h**

Add to top of `src/ui/LayoutMan.h`:

```cpp
#include "util/Result.h"
```

**Step 2: Update Init method signature**

Replace:

```cpp
    void Init(UIElement* pParent, std::int32_t x, std::int32_t y);
```

With:

```cpp
    [[nodiscard]] auto Init(UIElement* pParent, std::int32_t x, std::int32_t y) -> Result<void>;
```

**Step 3: Update AutoBuild method signature**

Replace:

```cpp
    void AutoBuild(const std::string& sBaseUOL);
```

With:

```cpp
    [[nodiscard]] auto AutoBuild(const std::string& sBaseUOL) -> Result<void>;
```

**Step 4: Update Init implementation**

In `src/ui/LayoutMan.cpp`, update Init:

```cpp
auto LayoutMan::Init(UIElement* pParent, std::int32_t x, std::int32_t y) -> Result<void>
{
    if (!pParent)
    {
        return Result<void>::Error("LayoutMan::Init: pParent is null");
    }

    m_pParent = pParent;
    m_nX = x;
    m_nY = y;

    return Result<void>::Success();
}
```

**Step 5: Update AutoBuild implementation**

Update to return Result:

```cpp
auto LayoutMan::AutoBuild(const std::string& sBaseUOL) -> Result<void>
{
    if (!m_pParent)
    {
        return Result<void>::Error("LayoutMan not initialized");
    }

    // ... existing AutoBuild logic ...

    // If successful:
    return Result<void>::Success();

    // If failed at any point:
    // return Result<void>::Error("Failed to load WZ resource: {}", sBaseUOL);
}
```

**Step 6: Verify compilation**

Run: `make`
Expected: Clean compilation

**Step 7: Commit**

```bash
git add src/ui/LayoutMan.h src/ui/LayoutMan.cpp
git commit -m "refactor: add Result<T> return types to LayoutMan

- Update Init to return Result<void>
- Update AutoBuild to return Result<void>
- Add error messages for null checks
- Enable proper error propagation in UIElement::OnCreate"
```

---

## Phase 5: Testing and Verification

### Task 7: Manual Testing

**Files:**
- Test: Run application and verify lifecycle

**Step 1: Build the application**

Run: `make` or `cmake --build build`
Expected: Clean compilation

**Step 2: Run the application**

Run: `./maplestory` or equivalent
Expected: Application starts without crashes

**Step 3: Test world select UI**

Actions:
1. Progress to world selection screen
2. Click world buttons
3. Navigate with keyboard
4. Return to title screen

Expected:
- UI appears correctly
- No crashes
- No memory leaks (check with valgrind if available)
- Proper cleanup when switching screens

**Step 4: Check logs**

Review logs for:
- "UIWorldSelect created successfully" message
- No "destroyed without calling Destroy()" warnings
- No "Invalid state transition" errors

**Step 5: Document test results**

Create test notes in commit message or separate doc.

---

## Phase 6: Documentation

### Task 8: Update Architecture Documentation

**Files:**
- Modify: `docs/plans/2026-02-02-modern-ui-lifecycle-design.md`

**Step 1: Add implementation status section**

Add to end of design document:

```markdown
---

## Implementation Status

### ✅ Completed
- [x] Result<T> error handling type
- [x] LifecycleState enum and state machine
- [x] UIElement Create/Destroy lifecycle methods
- [x] UIWorldSelect OnCreate/OnDestroy implementation
- [x] Removed singleton pattern from UIWorldSelect
- [x] Login owns UIWorldSelect via std::unique_ptr
- [x] LayoutMan returns Result<T>

### 🚧 In Progress
- [ ] LayerHandle RAII wrapper (deferred)
- [ ] Other UI classes lifecycle refactoring (UISelectChar, etc.)

### 📋 Planned
- [ ] Stage lifecycle management
- [ ] Concepts for UIComponent validation (C++20)
- [ ] Lifecycle observer pattern
- [ ] Object pool optimization

---

## Migration Notes

### Breaking Changes
- UIWorldSelect no longer uses GetInstance()
- Must call Create() after construction
- Must call Destroy() before destruction

### Migration Example

**Before:**
```cpp
auto& worldSelect = UIWorldSelect::GetInstance();
worldSelect.OnCreate(this, gr, m_uiManager);
```

**After:**
```cpp
m_worldSelectUI = std::make_unique<UIWorldSelect>();
auto params = UIWorldSelect::CreateParams{this, &gr, &m_uiManager};
auto result = m_worldSelectUI->Create(params);
if (!result) {
    LOG_ERROR("Failed: {}", result.error());
}
```
```

**Step 2: Commit**

```bash
git add docs/plans/2026-02-02-modern-ui-lifecycle-design.md
git commit -m "docs: add implementation status to lifecycle design

- Mark completed tasks
- Document breaking changes
- Add migration examples"
```

---

## Summary

This implementation plan follows TDD principles and breaks the refactoring into small, verifiable steps:

1. **Phase 1**: Foundation (Result<T>, LifecycleState, UIElement lifecycle)
2. **Phase 2**: UIWorldSelect refactoring (remove singleton, add OnCreate/OnDestroy)
3. **Phase 3**: Login refactoring (own UIWorldSelect, proper Create/Destroy)
4. **Phase 4**: LayoutMan integration (Result<T> return types)
5. **Phase 5**: Testing and verification
6. **Phase 6**: Documentation updates

Each step includes:
- Exact file paths
- Complete code snippets
- Compilation verification
- Commit messages

**Next Steps:**
- Apply this pattern to UISelectChar, UINewCharRaceSelect
- Add RAII LayerHandle wrapper
- Implement Stage lifecycle management
- Add C++20 concepts for compile-time validation
