# Logo Loading Screen - Manual Test Plan

**Date**: 2026-02-02
**Feature**: Logo Loading Screen Implementation
**Status**: Ready for Testing

---

## Test Environment

**Build Requirements**:
- Clean build completed: `cd build && make clean && make -j$(nproc)`
- Binary location: `build/maplestory`
- WZ files required: `UI/Logo.img` with Loading sub-nodes

**Display Requirements**:
- Runtime testing requires display (X11/Wayland)
- Note: Headless environments can verify build success only

---

## Test Cases

### Test 1: Random Background Selection

**Objective**: Verify that loading screen displays a random background from available options

**Steps**:
1. Launch application multiple times
2. Observe the loading screen background each time
3. Note which background is displayed

**Expected Results**:
- ✅ Background should vary across multiple launches
- ✅ Background from `UI/Logo.img/Loading/randomBackgrd/{0,1,2,...}` is displayed
- ✅ Background remains stable throughout entire loading session
- ✅ No visual glitches or missing textures

**Pass Criteria**: Different backgrounds appear across 5+ launches

---

### Test 2: Repeat Animation Loop

**Objective**: Verify that repeat animations play continuously and correctly

**Steps**:
1. Launch application and observe loading screen
2. Watch the repeat animation cycle
3. Observe multiple complete cycles
4. Note frame timing and transitions

**Expected Results**:
- ✅ Animation plays smoothly from `UI/Logo.img/Loading/repeat/{0,1,2,...}`
- ✅ Each repeat animation completes before transitioning to next
- ✅ Animation loops continuously without freezing
- ✅ Frame delays are respected (as defined in WZ `delay` property)
- ✅ No frame skipping or visual stuttering

**Pass Criteria**: At least 3 complete animation cycles observed

---

### Test 3: Progress Step Updates

**Objective**: Verify that loading progress steps update correctly

**Steps**:
1. Launch application with verbose logging enabled
2. Observe loading screen progress indicator
3. Monitor log output for step changes
4. Verify visual step indicator matches logged steps

**Expected Results**:
- ✅ Progress starts at step 0
- ✅ Step indicator updates as loading progresses
- ✅ Each step from `UI/Logo.img/Loading/step/{0,1,2,...}` displays correctly
- ✅ Step indicator is positioned correctly (centered with origin offset)
- ✅ Log messages show: `Loading progress: step X/Y`

**Pass Criteria**: All progress steps display sequentially and match log output

---

### Test 4: Fade-Out Effect

**Objective**: Verify smooth transition from loading screen to login stage

**Steps**:
1. Launch application and wait for loading to complete
2. Observe the final progress step
3. Watch the fade-out transition
4. Verify login stage appears after fade completes

**Expected Results**:
- ✅ Fade-out begins when final step is reached
- ✅ All layers (background, animation, progress) fade together
- ✅ Alpha transition is smooth (no sudden changes)
- ✅ Fade duration is appropriate (~1-2 seconds)
- ✅ Login stage appears after fade completes
- ✅ No visual artifacts during transition

**Pass Criteria**: Smooth fade-out observed, login stage loads successfully

---

### Test 5: Resource Cleanup

**Objective**: Verify proper resource management and cleanup

**Steps**:
1. Launch application with memory profiling enabled
2. Allow loading screen to complete
3. Proceed to login stage
4. Return to logo/loading flow (if applicable)
5. Monitor memory usage

**Expected Results**:
- ✅ No memory leaks detected
- ✅ Loading layers properly removed from graphics system
- ✅ Canvas resources released after stage transition
- ✅ Log shows: "Logo stage closed" with no errors
- ✅ Subsequent launches show consistent memory usage

**Pass Criteria**: No memory growth across multiple stage transitions

---

## Build Verification (Headless)

**For environments without display**:

### Build Success Check

```bash
cd /home/t4si/Desktop/repos/maplestory/.worktrees/logo-loading-screen
cd build
make clean
make -j$(nproc)
echo "Build exit code: $?"
```

**Expected Output**:
- Build completes without errors
- Exit code: 0
- Binary created: `build/maplestory`

### Code Review Checklist

- ✅ `Logo.h` contains loading mode member variables
- ✅ `Logo.cpp` implements all loading mode methods
- ✅ `Application.cpp` preloads `UI/Logo.img`
- ✅ Loading layers use correct z-order (10, 11, 12)
- ✅ All canvases use origin-based centering
- ✅ Cleanup code in `Logo::Close()` removes all layers

---

## Known Limitations

1. **Display Required**: Full runtime testing requires X11/Wayland display
2. **Progress Updates**: Current implementation uses simulated progress (demo mode)
3. **WZ Dependencies**: Requires properly formatted `UI/Logo.img/Loading` structure

---

## Test Results Template

**Test Date**: ___________
**Tester**: ___________
**Build**: ___________

| Test Case | Status | Notes |
|-----------|--------|-------|
| Test 1: Random Background | ⬜ Pass ⬜ Fail | |
| Test 2: Repeat Animation | ⬜ Pass ⬜ Fail | |
| Test 3: Progress Steps | ⬜ Pass ⬜ Fail | |
| Test 4: Fade-Out Effect | ⬜ Pass ⬜ Fail | |
| Test 5: Resource Cleanup | ⬜ Pass ⬜ Fail | |

**Overall Assessment**: ⬜ Pass ⬜ Fail

**Additional Notes**:
___________________________________________________________________________
___________________________________________________________________________
___________________________________________________________________________
