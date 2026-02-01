# Logo Loading Screen Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add loading screen to Logo stage that displays after logo animation, showing resource loading progress with random background, animated elements, and progress indicators.

**Architecture:** Extend Logo stage with new loading mode that activates after logo animation completes. Uses three-layer rendering (background, animation, progress) with WZ-based resources. Tracks real resource loading through staged progress updates.

**Tech Stack:** C++17, WzGr2D layer system, WZ resource loading (UI/Logo.img/Loading)

---

## Task 1: Preload Logo Resources in Application Init

**Files:**
- Modify: `src/app/Application.cpp` (Init method)
- Modify: `src/app/Application.h` (if needed)

**Step 1: Locate Application::Init() method**

Read `src/app/Application.cpp` and find the `Init()` method where WZ resources are initialized.

**Step 2: Add Logo.img preload**

In `Application::Init()`, after WzResMan initialization, add:

```cpp
// Preload Logo resources (including Loading screen assets)
// This ensures loading screen is ready before Logo stage starts
auto logoProp = resMan.GetProperty("UI/Logo.img");
if (logoProp)
{
    LOG_INFO("Preloaded UI/Logo.img for loading screen");
}
else
{
    LOG_WARN("Failed to preload UI/Logo.img");
}
```

Insert this after existing WZ initialization code but before stage creation.

**Step 3: Build and verify**

Run: `cd build && make -j$(nproc)`
Expected: Clean build with no errors

**Step 4: Test preload**

Run: `./build/MapleStory`
Check logs for: "Preloaded UI/Logo.img for loading screen"

**Step 5: Commit**

```bash
git add src/app/Application.cpp
git commit -m "feat(app): preload UI/Logo.img for loading screen

Preload Logo resources during application initialization to ensure
loading screen assets are ready before Logo stage starts.

Related to loading screen implementation.

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 2: Add Loading Mode Member Variables to Logo Header

**Files:**
- Modify: `src/stage/Logo.h:133-163` (private members section)

**Step 1: Read current Logo.h structure**

Verify the current member variable layout and find the insertion point after existing Logo variables (around line 163).

**Step 2: Add loading mode state variables**

After the existing member variables (after `m_messageFrames`), add:

```cpp
    // Loading mode state
    bool m_bLoadingMode{false};              // Whether in loading mode
    std::int32_t m_nLoadingStep{0};          // Current loading step (0-based)
    std::int32_t m_nLoadingStepCount{0};     // Total number of steps
    std::uint8_t m_loadingAlpha{255};        // Loading screen alpha (for fade out)

    // Loading layers
    std::shared_ptr<WzGr2DLayer> m_pLayerLoadingBg;     // Background layer
    std::shared_ptr<WzGr2DLayer> m_pLayerLoadingAnim;   // Animation layer
    std::shared_ptr<WzGr2DLayer> m_pLayerLoadingStep;   // Progress step layer

    // Loading frames
    std::shared_ptr<WzCanvas> m_loadingBgCanvas;                       // Selected random background
    std::vector<std::vector<std::shared_ptr<WzCanvas>>> m_repeatAnims; // repeat animations [n][frame]
    std::vector<std::shared_ptr<WzCanvas>> m_stepFrames;               // step progress images
    std::int32_t m_nCurrentRepeat{0};       // Current repeat animation index
    std::int32_t m_nCurrentRepeatFrame{0};  // Current frame within repeat animation
```

**Step 3: Build and verify**

Run: `cd build && make -j$(nproc)`
Expected: Clean build (implementation will come next)

**Step 4: Commit**

```bash
git add src/stage/Logo.h
git commit -m "feat(logo): add loading mode member variables

Add member variables for loading screen state:
- Mode flags and progress tracking
- Three rendering layers (bg, anim, step)
- Canvas storage for WZ resources

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 3: Declare Loading Mode Methods in Logo Header

**Files:**
- Modify: `src/stage/Logo.h:79-123` (private methods section)

**Step 1: Find method declaration section**

Locate the private methods section (after `GoToLogin()` around line 123).

**Step 2: Add loading mode method declarations**

After the existing private methods, add:

```cpp
    /**
     * @brief Initialize loading screen resources
     *
     * Loads from UI/Logo.img/Loading:
     * - randomBackgrd: Random background selection
     * - repeat: Animated elements
     * - step: Progress indicators
     */
    void InitLoading();

    /**
     * @brief Start loading mode
     *
     * Switches from Logo/Video mode to loading mode.
     * Hides logo layers, shows loading layers.
     */
    void StartLoadingMode();

    /**
     * @brief Update loading screen
     *
     * Handles repeat animation cycling and fade out effect.
     */
    void UpdateLoading();

    /**
     * @brief Set loading progress
     * @param step Current loading step (0-based)
     *
     * Updates step indicator display.
     * Triggers fade out when final step reached.
     */
    void SetLoadingProgress(std::int32_t step);

    /**
     * @brief Start fade out effect
     *
     * Begins gradual alpha reduction of loading screen.
     * Transitions to Login when complete.
     */
    void FadeOutLoading();
```

**Step 3: Build and verify**

Run: `cd build && make -j$(nproc)`
Expected: Build errors about undefined methods (will implement next)

**Step 4: Commit**

```bash
git add src/stage/Logo.h
git commit -m "feat(logo): declare loading mode methods

Add method declarations for loading screen:
- InitLoading: Load WZ resources
- StartLoadingMode: Transition to loading mode
- UpdateLoading: Animation and fade out
- SetLoadingProgress: Update progress indicator
- FadeOutLoading: Transition to Login

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 4: Implement InitLoading Method

**Files:**
- Modify: `src/stage/Logo.cpp:137-208` (after LoadLogoFrames)

**Step 1: Add InitLoading implementation**

After the `LoadLogoFrames()` method (around line 207), add:

```cpp
void Logo::InitLoading()
{
    auto& resMan = WzResMan::GetInstance();
    auto loadingProp = resMan.GetProperty("UI/Logo.img/Loading");

    if (!loadingProp)
    {
        LOG_WARN("UI/Logo.img/Loading not found - loading screen disabled");
        return;
    }

    LOG_DEBUG("Initializing loading screen resources");

    // 1. Load random background
    auto randomBgProp = loadingProp->GetChild("randomBackgrd");
    if (randomBgProp && randomBgProp->HasChildren())
    {
        auto bgCount = static_cast<std::int32_t>(randomBgProp->GetChildCount());
        auto randomIndex = rand() % bgCount;
        auto bgChild = randomBgProp->GetChild(std::to_string(randomIndex));
        if (bgChild)
        {
            m_loadingBgCanvas = bgChild->GetCanvas();
            LOG_DEBUG("Selected random background: {}/{}", randomIndex, bgCount);
        }
    }
    else
    {
        LOG_DEBUG("No random backgrounds found in Loading/randomBackgrd");
    }

    // 2. Load repeat animations
    auto repeatProp = loadingProp->GetChild("repeat");
    if (repeatProp && repeatProp->HasChildren())
    {
        for (int n = 0; n < 100; ++n)  // Max 100 repeat animations
        {
            auto repeatN = repeatProp->GetChild(std::to_string(n));
            if (!repeatN) break;

            auto frames = LoadLogoFrames(repeatN);
            if (!frames.empty())
            {
                m_repeatAnims.push_back(frames);
            }
        }
        LOG_DEBUG("Loaded {} repeat animations", m_repeatAnims.size());
    }
    else
    {
        LOG_DEBUG("No repeat animations found in Loading/repeat");
    }

    // 3. Load step progress indicators
    auto stepProp = loadingProp->GetChild("step");
    if (stepProp && stepProp->HasChildren())
    {
        m_stepFrames = LoadLogoFrames(stepProp);
        m_nLoadingStepCount = static_cast<std::int32_t>(m_stepFrames.size());
        LOG_DEBUG("Loaded {} loading steps", m_nLoadingStepCount);
    }
    else
    {
        LOG_DEBUG("No step indicators found in Loading/step");
    }

    // 4. Create loading layers (initially hidden)
    auto& gr = get_gr();

    // Background layer (z=10, above logo layers)
    m_pLayerLoadingBg = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 10);
    if (m_pLayerLoadingBg)
    {
        m_pLayerLoadingBg->SetVisible(false);
        m_pLayerLoadingBg->SetScreenSpace(true);
        LOG_DEBUG("Created loading background layer");
    }

    // Animation layer (z=11)
    m_pLayerLoadingAnim = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 11);
    if (m_pLayerLoadingAnim)
    {
        m_pLayerLoadingAnim->SetVisible(false);
        m_pLayerLoadingAnim->SetScreenSpace(true);
        LOG_DEBUG("Created loading animation layer");
    }

    // Progress step layer (z=12)
    m_pLayerLoadingStep = gr.CreateLayer(0, 0, gr.GetWidth(), gr.GetHeight(), 12);
    if (m_pLayerLoadingStep)
    {
        m_pLayerLoadingStep->SetVisible(false);
        m_pLayerLoadingStep->SetScreenSpace(true);
        LOG_DEBUG("Created loading progress layer");
    }

    LOG_INFO("Loading screen initialized (bg={}, anims={}, steps={})",
             m_loadingBgCanvas ? "yes" : "no",
             m_repeatAnims.size(),
             m_nLoadingStepCount);
}
```

**Step 2: Call InitLoading from InitWZLogo**

Modify `InitWZLogo()` method (around line 43-136) to call InitLoading at the end, before `FlushCachedObjects`:

```cpp
void Logo::InitWZLogo()
{
    // ... existing code ...

    // Initialize loading screen
    InitLoading();

    // Flush cached objects after loading
    resMan.FlushCachedObjects(0);
}
```

**Step 3: Build and verify**

Run: `cd build && make -j$(nproc)`
Expected: Clean build

**Step 4: Test loading resource initialization**

Run: `./build/MapleStory`
Check logs for: "Loading screen initialized"

**Step 5: Commit**

```bash
git add src/stage/Logo.cpp
git commit -m "feat(logo): implement InitLoading method

Load loading screen resources from UI/Logo.img/Loading:
- Random background selection from randomBackgrd
- All repeat animation frames
- Progress step indicators

Create three rendering layers (initially hidden).

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 5: Implement StartLoadingMode Method

**Files:**
- Modify: `src/stage/Logo.cpp` (after InitLoading)

**Step 1: Add StartLoadingMode implementation**

After `InitLoading()`, add:

```cpp
void Logo::StartLoadingMode()
{
    LOG_INFO("Starting loading mode");

    m_bLoadingMode = true;
    m_nLoadingStep = 0;
    m_loadingAlpha = 255;
    m_nCurrentRepeat = 0;
    m_nCurrentRepeatFrame = 0;

    // Hide logo layers
    if (m_pLayerBackground) m_pLayerBackground->SetVisible(false);
    if (m_pLayerMain) m_pLayerMain->SetVisible(false);

    // Setup and show background layer
    if (m_pLayerLoadingBg && m_loadingBgCanvas)
    {
        m_pLayerLoadingBg->RemoveAllCanvases();
        m_pLayerLoadingBg->InsertCanvas(m_loadingBgCanvas);

        // Center with origin offset
        auto origin = m_loadingBgCanvas->GetOrigin();
        auto& gr = get_gr();
        auto screenWidth = static_cast<std::int32_t>(gr.GetWidth());
        auto screenHeight = static_cast<std::int32_t>(gr.GetHeight());
        auto layerX = (screenWidth - m_loadingBgCanvas->GetWidth()) / 2 + origin.x;
        auto layerY = (screenHeight - m_loadingBgCanvas->GetHeight()) / 2 + origin.y;
        m_pLayerLoadingBg->SetPosition(layerX, layerY);
        m_pLayerLoadingBg->SetColor(0xFFFFFFFF);
        m_pLayerLoadingBg->SetVisible(true);
    }

    // Setup and show animation layer with first repeat
    if (m_pLayerLoadingAnim && !m_repeatAnims.empty())
    {
        m_pLayerLoadingAnim->RemoveAllCanvases();

        // Load first repeat animation frames
        auto& firstRepeat = m_repeatAnims[0];
        for (auto& frameCanvas : firstRepeat)
        {
            // Use default delay of 100ms (will be read from WZ in future refinement)
            m_pLayerLoadingAnim->InsertCanvas(frameCanvas, 100, 255, 255);
        }

        // Center first frame for positioning
        if (!firstRepeat.empty())
        {
            auto origin = firstRepeat[0]->GetOrigin();
            auto& gr = get_gr();
            auto screenWidth = static_cast<std::int32_t>(gr.GetWidth());
            auto screenHeight = static_cast<std::int32_t>(gr.GetHeight());
            auto layerX = (screenWidth - firstRepeat[0]->GetWidth()) / 2 + origin.x;
            auto layerY = (screenHeight - firstRepeat[0]->GetHeight()) / 2 + origin.y;
            m_pLayerLoadingAnim->SetPosition(layerX, layerY);
        }

        m_pLayerLoadingAnim->SetColor(0xFFFFFFFF);
        m_pLayerLoadingAnim->SetVisible(true);
    }

    // Setup and show progress layer
    if (m_pLayerLoadingStep)
    {
        m_pLayerLoadingStep->SetVisible(true);
        SetLoadingProgress(0);  // Show first step
    }
}
```

**Step 2: Build and verify**

Run: `cd build && make -j$(nproc)`
Expected: Clean build

**Step 3: Commit**

```bash
git add src/stage/Logo.cpp
git commit -m "feat(logo): implement StartLoadingMode method

Transition from logo animation to loading mode:
- Hide logo layers, show loading layers
- Display random background
- Start first repeat animation
- Show initial progress step

Uses WZ origin positioning for all elements.

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 6: Implement SetLoadingProgress Method

**Files:**
- Modify: `src/stage/Logo.cpp` (after StartLoadingMode)

**Step 1: Add SetLoadingProgress implementation**

After `StartLoadingMode()`, add:

```cpp
void Logo::SetLoadingProgress(std::int32_t step)
{
    if (step == m_nLoadingStep)
    {
        return;  // No change
    }

    m_nLoadingStep = step;
    LOG_DEBUG("Loading progress: step {}/{}", step, m_nLoadingStepCount - 1);

    if (!m_pLayerLoadingStep || step >= m_nLoadingStepCount)
    {
        return;
    }

    // Update step indicator
    m_pLayerLoadingStep->RemoveAllCanvases();

    if (step < static_cast<std::int32_t>(m_stepFrames.size()))
    {
        auto& stepCanvas = m_stepFrames[static_cast<std::size_t>(step)];
        m_pLayerLoadingStep->InsertCanvas(stepCanvas);

        // Center with origin offset
        auto origin = stepCanvas->GetOrigin();
        auto& gr = get_gr();
        auto screenWidth = static_cast<std::int32_t>(gr.GetWidth());
        auto screenHeight = static_cast<std::int32_t>(gr.GetHeight());
        auto layerX = (screenWidth - stepCanvas->GetWidth()) / 2 + origin.x;
        auto layerY = (screenHeight - stepCanvas->GetHeight()) / 2 + origin.y;
        m_pLayerLoadingStep->SetPosition(layerX, layerY);
    }

    // If final step reached, start fade out
    if (step >= m_nLoadingStepCount - 1)
    {
        LOG_INFO("Loading complete - starting fade out");
        FadeOutLoading();
    }
}
```

**Step 2: Build and verify**

Run: `cd build && make -j$(nproc)`
Expected: Clean build

**Step 3: Commit**

```bash
git add src/stage/Logo.cpp
git commit -m "feat(logo): implement SetLoadingProgress method

Update loading progress indicator:
- Replace step canvas based on current progress
- Center with WZ origin positioning
- Trigger fade out when final step reached

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 7: Implement FadeOutLoading and UpdateLoading Methods

**Files:**
- Modify: `src/stage/Logo.cpp` (after SetLoadingProgress)

**Step 1: Add FadeOutLoading implementation**

After `SetLoadingProgress()`, add:

```cpp
void Logo::FadeOutLoading()
{
    // Set alpha to trigger fade out (will be handled in UpdateLoading)
    if (m_loadingAlpha == 255)
    {
        m_loadingAlpha = 254;  // Start fade out
        LOG_DEBUG("Starting loading screen fade out");
    }
}
```

**Step 2: Add UpdateLoading implementation**

After `FadeOutLoading()`, add:

```cpp
void Logo::UpdateLoading()
{
    // Handle fade out effect
    if (m_loadingAlpha < 255)
    {
        // Gradually reduce alpha (approximately 5 per frame at 60fps = ~3 second fade)
        auto alphaInt = static_cast<std::int32_t>(m_loadingAlpha);
        alphaInt = std::max(0, alphaInt - 5);
        m_loadingAlpha = static_cast<std::uint8_t>(alphaInt);

        // Apply alpha to all loading layers
        auto color = (static_cast<std::uint32_t>(m_loadingAlpha) << 24) | 0x00FFFFFF;
        if (m_pLayerLoadingBg) m_pLayerLoadingBg->SetColor(color);
        if (m_pLayerLoadingAnim) m_pLayerLoadingAnim->SetColor(color);
        if (m_pLayerLoadingStep) m_pLayerLoadingStep->SetColor(color);

        // When fully faded out, transition to Login
        if (m_loadingAlpha == 0)
        {
            LOG_INFO("Fade out complete - transitioning to Login");
            GoToLogin();
        }

        return;
    }

    // Repeat animation cycling (future enhancement)
    // For now, layer animation system handles frame playback automatically
    // Could add repeat switching logic here if needed
}
```

**Step 3: Build and verify**

Run: `cd build && make -j$(nproc)`
Expected: Clean build

**Step 4: Commit**

```bash
git add src/stage/Logo.cpp
git commit -m "feat(logo): implement fade out and update methods

Add UpdateLoading:
- Handle gradual alpha fade out (~3 seconds)
- Transition to Login when fade complete

Add FadeOutLoading:
- Trigger fade out effect

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 8: Integrate Loading Mode into Update Flow

**Files:**
- Modify: `src/stage/Logo.cpp:209-220` (Update method)
- Modify: `src/stage/Logo.cpp:277-300` (UpdateVideo method)

**Step 1: Modify Update() to handle loading mode**

Find the `Update()` method (around line 209) and modify it:

```cpp
void Logo::Update()
{
    // Based on CLogo::Update @ 0xbc7a90
    if (m_bLoadingMode)
    {
        UpdateLoading();
    }
    else if (m_bVideoMode)
    {
        UpdateVideo();
    }
    else
    {
        UpdateLogo();
    }
}
```

**Step 2: Modify UpdateVideo() to start loading mode**

Find the `UpdateVideo()` method (around line 277) and modify the unavailable case:

```cpp
void Logo::UpdateVideo()
{
    // Based on CLogo::UpdateVideo @ 0xbc5950

    // Video unavailable - start loading mode instead of ending
    if (m_videoState == VideoState::Unavailable)
    {
        StartLoadingMode();
        return;
    }

    // Video ending - fade out and end
    if (m_videoState == VideoState::FadeOut || m_videoState == VideoState::End)
    {
        // FadeOut(0);
        LogoEnd();
        return;
    }

    // In a full implementation, this would check video playback status
    // and handle video state transitions
    // For now, just end the logo since we don't have video support
    LogoEnd();
}
```

**Step 3: Build and verify**

Run: `cd build && make -j$(nproc)`
Expected: Clean build

**Step 4: Test loading mode activation**

Run: `./build/MapleStory`
Expected:
- Logo animation plays
- After logo ends, loading mode starts (check logs: "Starting loading mode")
- Loading screen displays (if WZ resources exist)

**Step 5: Commit**

```bash
git add src/stage/Logo.cpp
git commit -m "feat(logo): integrate loading mode into update flow

Modify Logo::Update to handle loading mode state.
Modify UpdateVideo to start loading mode instead of ending.

Loading mode now activates after logo animation completes.

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 9: Add Loading Layer Cleanup

**Files:**
- Modify: `src/stage/Logo.cpp:418-452` (Close method)

**Step 1: Add loading layer cleanup to Close()**

Find the `Close()` method (around line 418) and add cleanup before the existing frame clear code:

```cpp
void Logo::Close()
{
    // Based on CLogo::Close @ 0xbc7170
    // Stop BGM with 1000ms fade out
    // CSoundMan::PlayBGM(0, 0, 0, 1000, 0, 0);

    Stage::Close();

    // Cleanup logo layers
    auto& gr = get_gr();

    if (m_pLayerBackground)
    {
        gr.RemoveLayer(m_pLayerBackground);
        m_pLayerBackground.reset();
    }

    if (m_pLayerMain)
    {
        gr.RemoveLayer(m_pLayerMain);
        m_pLayerMain.reset();
    }

    // Cleanup loading layers
    if (m_pLayerLoadingBg)
    {
        gr.RemoveLayer(m_pLayerLoadingBg);
        m_pLayerLoadingBg.reset();
    }

    if (m_pLayerLoadingAnim)
    {
        gr.RemoveLayer(m_pLayerLoadingAnim);
        m_pLayerLoadingAnim.reset();
    }

    if (m_pLayerLoadingStep)
    {
        gr.RemoveLayer(m_pLayerLoadingStep);
        m_pLayerLoadingStep.reset();
    }

    // Clear frame references
    m_logoFrames.clear();
    m_gradeFrames.clear();
    m_messageFrames.clear();

    // Clear loading frame references
    m_loadingBgCanvas.reset();
    m_repeatAnims.clear();
    m_stepFrames.clear();

    // Clear property references
    m_pLogoProp.reset();
    m_pGradeProp.reset();
    m_pMessageProp.reset();

    LOG_DEBUG("Logo stage closed");
}
```

**Step 2: Build and verify**

Run: `cd build && make -j$(nproc)`
Expected: Clean build

**Step 3: Test cleanup**

Run: `./build/MapleStory`, let it progress through logo and loading, then exit
Check logs for clean shutdown with no memory leaks or warnings

**Step 4: Commit**

```bash
git add src/stage/Logo.cpp
git commit -m "feat(logo): add loading layer cleanup

Properly remove and reset loading layers in Close():
- Remove layers from graphics system
- Clear canvas references
- Prevent memory leaks

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 10: Add Public Interface for External Progress Updates

**Files:**
- Modify: `src/stage/Logo.h:52-78` (public methods section)

**Step 1: Add SetLoadingProgress to public interface**

In the public methods section (after `ForcedEnd()` around line 77), add:

```cpp
    /**
     * @brief Update loading progress (public interface)
     * @param step Current loading step (0-based)
     *
     * External systems can call this to update loading progress.
     * Safe to call even if not in loading mode.
     */
    void SetLoadingProgress(std::int32_t step);
```

Wait - `SetLoadingProgress` should remain private since it's already declared there.
Instead, we need to make it accessible. Let me reconsider.

**Step 1: Move SetLoadingProgress to public section**

Find the private `SetLoadingProgress` declaration and move it to the public section after `ForcedEnd()`:

In public methods section:
```cpp
    /**
     * @brief Update loading progress
     * @param step Current loading step (0-based)
     *
     * External systems can call this to update loading progress.
     * Automatically triggers fade out when final step reached.
     */
    void SetLoadingProgress(std::int32_t step);
```

Remove the duplicate declaration from private section.

**Step 2: Build and verify**

Run: `cd build && make -j$(nproc)`
Expected: Clean build

**Step 3: Commit**

```bash
git add src/stage/Logo.h
git commit -m "feat(logo): expose SetLoadingProgress as public API

Move SetLoadingProgress to public interface to allow
external systems to update loading progress.

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 11: Add Delay Support for Repeat Animations (Enhancement)

**Files:**
- Modify: `src/stage/Logo.cpp` (StartLoadingMode method)

**Step 1: Read delay from WZ for repeat animations**

Modify the `StartLoadingMode()` method where repeat animations are loaded. Find the section that inserts canvases:

Replace:
```cpp
        // Load first repeat animation frames
        auto& firstRepeat = m_repeatAnims[0];
        for (auto& frameCanvas : firstRepeat)
        {
            // Use default delay of 100ms (will be read from WZ in future refinement)
            m_pLayerLoadingAnim->InsertCanvas(frameCanvas, 100, 255, 255);
        }
```

With:
```cpp
        // Load first repeat animation frames with delay from WZ
        auto& firstRepeat = m_repeatAnims[0];
        auto repeatProp = resMan.GetProperty("UI/Logo.img/Loading/repeat/0");

        for (std::size_t i = 0; i < firstRepeat.size(); ++i)
        {
            auto& frameCanvas = firstRepeat[i];

            // Read delay from WZ (similar to UINewCharRaceSelect pattern)
            int delay = 100;  // Default
            if (repeatProp)
            {
                auto frameProp = repeatProp->GetChild(std::to_string(i));
                if (frameProp)
                {
                    auto delayProp = frameProp->GetChild("delay");
                    if (delayProp)
                    {
                        delay = delayProp->GetInt(100);
                    }
                }
            }

            m_pLayerLoadingAnim->InsertCanvas(frameCanvas, delay, 255, 255);
        }
```

**Step 2: Build and verify**

Run: `cd build && make -j$(nproc)`
Expected: Clean build

**Step 3: Test animation timing**

Run: `./build/MapleStory`
Observe loading animation - should now use WZ-defined delays

**Step 4: Commit**

```bash
git add src/stage/Logo.cpp
git commit -m "feat(logo): read animation delay from WZ resources

Read delay property from Loading/repeat frames to match
original MapleStory animation timing.

Falls back to 100ms default if delay not found.

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 12: Test Basic Loading Screen Flow

**Files:**
- Test manually

**Step 1: Build clean**

Run: `cd build && make clean && make -j$(nproc)`
Expected: Full rebuild succeeds

**Step 2: Test complete flow**

Run: `./build/MapleStory`

Verify:
1. Logo animation plays (Message + Logo phases)
2. Loading mode starts (check logs: "Starting loading mode")
3. Loading screen displays (if WZ has Loading resources)
4. Progress stays at step 0 (no external updates yet)
5. Application remains responsive

**Step 3: Test skip behavior**

Run: `./build/MapleStory`
During logo animation, press Space/Enter/Escape or click
Verify: Loading screen appears after skip

**Step 4: Check logs**

Verify log messages appear:
- "Preloaded UI/Logo.img for loading screen"
- "Loading screen initialized"
- "Starting loading mode"
- "Loading progress: step 0/X"

**Step 5: Document test results**

Create: `docs/testing/logo-loading-screen-manual-test.md`

```markdown
# Logo Loading Screen Manual Test

## Test Date
2026-02-02

## Test Results

### Basic Flow
- [ ] Logo animation plays
- [ ] Loading mode activates after logo
- [ ] Loading screen displays
- [ ] Skip functionality works

### Resource Loading
- [ ] Random background displays
- [ ] Repeat animation plays
- [ ] Step indicator shows step 0

### Performance
- [ ] No visual glitches
- [ ] Smooth transitions
- [ ] No memory leaks

## Notes
[Add any observations or issues]
```

**Step 6: No commit needed**

This is testing only - document results in notes.

---

## Task 13: Add Progress Update Example (Demo/Testing)

**Files:**
- Modify: `src/stage/Logo.cpp` (UpdateLoading method - temporary test code)

**Step 1: Add simulated progress for testing**

Temporarily modify `UpdateLoading()` to simulate progress updates. Add this at the top of the method:

```cpp
void Logo::UpdateLoading()
{
    // TEMPORARY: Simulate progress for testing
    // TODO: Remove this when real resource tracking is implemented
    static std::uint64_t lastStepUpdate = 0;
    auto now = Application::GetTick();

    if (m_loadingAlpha == 255 && m_nLoadingStep < m_nLoadingStepCount - 1)
    {
        if (lastStepUpdate == 0)
        {
            lastStepUpdate = now;
        }
        else if (now - lastStepUpdate > 800)  // Update every 800ms
        {
            SetLoadingProgress(m_nLoadingStep + 1);
            lastStepUpdate = now;
        }
    }

    // Handle fade out effect
    // ... existing code ...
}
```

**Step 2: Build and test**

Run: `cd build && make -j$(nproc) && ./build/MapleStory`

Expected:
- Loading screen appears
- Progress automatically advances every 800ms
- Final step triggers fade out
- Smooth transition to Login

**Step 3: Commit with TODO marker**

```bash
git add src/stage/Logo.cpp
git commit -m "test(logo): add simulated progress updates

TEMPORARY: Auto-advance loading progress for testing.
Progress updates every 800ms until final step.

TODO: Replace with real resource loading tracking.

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Task 14: Final Integration Test and Cleanup

**Files:**
- Test entire flow
- Review code

**Step 1: Full integration test**

Run: `cd build && make -j$(nproc) && ./build/MapleStory`

Test scenarios:
1. **Normal flow**: Let logo play completely, watch loading, verify Login appears
2. **Skip during message**: Press Space during message phase, verify loading shows
3. **Skip during logo**: Press Enter during logo phase, verify loading shows
4. **Mouse skip**: Click during logo, verify loading shows

All should end with Login stage appearing.

**Step 2: Code review checklist**

Verify:
- [ ] All member variables initialized in constructor/header
- [ ] All layers cleaned up in Close()
- [ ] No memory leaks (run with valgrind if available)
- [ ] Consistent code style with existing Logo code
- [ ] All methods have proper documentation comments
- [ ] No TODO markers (except the simulated progress one)

**Step 3: Review commit history**

Run: `git log --oneline -14`

Verify commits follow pattern:
- feat(logo): descriptive message
- All have Co-Authored-By tag
- Logical progression of changes

**Step 4: Create summary commit**

Run:
```bash
git log --oneline -14 > /tmp/logo-loading-commits.txt
```

No commit needed - this is review only.

**Step 5: Update design document**

Modify: `docs/plans/2026-02-02-logo-loading-screen-design.md`

Add section at the end:

```markdown
---

## Implementation Status

**Date Completed**: 2026-02-02
**Status**: ✅ Basic implementation complete

### Completed
- ✅ Resource preloading in Application::Init
- ✅ Loading mode state and layers
- ✅ WZ resource loading (randomBackgrd, repeat, step)
- ✅ Progress tracking and display
- ✅ Fade out transition to Login
- ✅ Integration with Logo stage flow

### Pending
- ⏳ Real resource loading tracking (currently simulated)
- ⏳ Repeat animation cycling between different animations
- ⏳ Login stage resource preloading integration

### Testing
- ✅ Manual testing complete
- ⏳ Automated tests (future)
```

**Step 6: Commit documentation update**

```bash
git add docs/plans/2026-02-02-logo-loading-screen-design.md
git commit -m "docs: update loading screen design with implementation status

Mark basic implementation as complete.
Note pending items for future work.

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Next Steps (Future Work)

### Real Resource Loading Tracking

**Goal**: Replace simulated progress with actual WZ/resource loading events

**Approach**:
1. Add callback/observer system to WzResMan
2. Track critical loading milestones:
   - Step 0: Loading start
   - Step 1: UI.wz loaded
   - Step 2: Character.wz loaded
   - Step 3: Login::Init() complete
3. Remove simulated progress from UpdateLoading()

### Repeat Animation Cycling

**Goal**: Cycle through different repeat animations during loading

**Approach**:
1. Track when current repeat animation completes all frames
2. Switch to next repeat animation (repeat/1, repeat/2, etc.)
3. Handle positioning updates if animations have different sizes

### Login Stage Resource Preloading

**Goal**: Preload Login stage resources during loading screen

**Approach**:
1. Create Login stage in background during loading
2. Call Login::Init() asynchronously
3. Update progress based on Login initialization steps
4. Transition when fully ready

---

## Skills Referenced

This plan follows principles from:
- @superpowers:test-driven-development - TDD workflow (not applicable for graphics work)
- @superpowers:verification-before-completion - Test before claiming complete

## Notes

- This implementation provides the foundation for loading screen
- WZ resources must exist in `UI/Logo.img/Loading/` for display
- Simulated progress is temporary - replace with real tracking
- Code follows existing Logo stage patterns for consistency
