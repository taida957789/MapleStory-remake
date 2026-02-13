#include <gtest/gtest.h>

#include "graphics/Gr2DVector.h"

using namespace ms;

// Test fixture for Gr2DVector tests
class Gr2DVectorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Reset global time before each test
        Gr2DTime::SetCurrentTime(0);
    }

    void TearDown() override
    {
        // Cleanup if needed
    }
};

// =============================================================================
// Basic Operations
// =============================================================================

TEST_F(Gr2DVectorTest, Constructor)
{
    Gr2DVector vec(100, 200);

    EXPECT_EQ(vec.GetX(), 100);
    EXPECT_EQ(vec.GetY(), 200);
    EXPECT_EQ(vec.GetRX(), 100);
    EXPECT_EQ(vec.GetRY(), 200);
}

TEST_F(Gr2DVectorTest, DefaultConstructor)
{
    Gr2DVector vec;

    EXPECT_EQ(vec.GetX(), 0);
    EXPECT_EQ(vec.GetY(), 0);
}

TEST_F(Gr2DVectorTest, Move)
{
    Gr2DVector vec(100, 200);

    vec.Move(300, 400);

    EXPECT_EQ(vec.GetX(), 300);
    EXPECT_EQ(vec.GetY(), 400);
}

TEST_F(Gr2DVectorTest, Offset)
{
    Gr2DVector vec(100, 200);

    vec.Offset(50, -30);

    EXPECT_EQ(vec.GetX(), 150);
    EXPECT_EQ(vec.GetY(), 170);
}

TEST_F(Gr2DVectorTest, OffsetWithChain)
{
    Gr2DVector vec(100, 200);

    // Create a chain by doing a RelMove, then offset
    vec.RelMove(100, 200, 0, 1000);

    // Offset goes to chain's offset_x/offset_y (world offset)
    vec.Offset(10, 20);

    Gr2DTime::SetCurrentTime(500);
    // world = parent(0) + local(100 + dx*0.5) + offset(10)
    // Actually at t=500, easing dx=0 (target=100, cur_rx=100, so dx=0)
    // Wait: RelMove(100, 200, 0, 1000) from (100, 200) → dx = 100-100 = 0, dy = 200-200 = 0
    // So no animation. World = base(100) + offset(10) = 110
    EXPECT_EQ(vec.GetX(), 110);
    EXPECT_EQ(vec.GetY(), 220);
}

TEST_F(Gr2DVectorTest, RawAccess)
{
    Gr2DVector vec(42, 99);
    EXPECT_EQ(vec.RawX(), 42);
    EXPECT_EQ(vec.RawY(), 99);
    EXPECT_EQ(vec.Chain(), nullptr);  // No chain until animation is used
}

// =============================================================================
// Parent-Child Hierarchy
// =============================================================================

TEST_F(Gr2DVectorTest, ParentChildOrigin)
{
    Gr2DVector parent(100, 100);
    Gr2DVector child(50, 0);

    child.PutOrigin(&parent);

    EXPECT_EQ(child.GetX(), 150);  // 100 + 50
    EXPECT_EQ(child.GetY(), 100);  // 100 + 0
    EXPECT_EQ(child.GetRX(), 50);  // Local coordinate
    EXPECT_EQ(child.GetRY(), 0);
}

TEST_F(Gr2DVectorTest, MultiLevelHierarchy)
{
    Gr2DVector grandparent(100, 100);
    Gr2DVector parent(50, 50);
    Gr2DVector child(25, 25);

    parent.PutOrigin(&grandparent);
    child.PutOrigin(&parent);

    EXPECT_EQ(parent.GetX(), 150);   // 100 + 50
    EXPECT_EQ(parent.GetY(), 150);   // 100 + 50
    EXPECT_EQ(child.GetX(), 175);    // 150 + 25
    EXPECT_EQ(child.GetY(), 175);    // 150 + 25
}

TEST_F(Gr2DVectorTest, ParentMovement)
{
    Gr2DVector parent(100, 100);
    Gr2DVector child(50, 50);

    child.PutOrigin(&parent);

    EXPECT_EQ(child.GetX(), 150);
    EXPECT_EQ(child.GetY(), 150);

    // Move parent (advance time to invalidate evaluation cache)
    parent.Move(200, 200);
    Gr2DTime::SetCurrentTime(1);

    EXPECT_EQ(child.GetX(), 250);  // 200 + 50
    EXPECT_EQ(child.GetY(), 250);  // 200 + 50
}

TEST_F(Gr2DVectorTest, GetOriginReturnsParent)
{
    Gr2DVector parent(100, 100);
    Gr2DVector child(50, 50);

    EXPECT_EQ(child.GetOrigin(), nullptr);

    child.PutOrigin(&parent);
    EXPECT_EQ(child.GetOrigin(), &parent);
}

TEST_F(Gr2DVectorTest, SelfParentPrevented)
{
    Gr2DVector vec(100, 200);
    vec.PutOrigin(&vec);  // Should be ignored

    EXPECT_EQ(vec.GetX(), 100);
    EXPECT_EQ(vec.GetY(), 200);
}

// =============================================================================
// RelMove Animation
// =============================================================================

TEST_F(Gr2DVectorTest, BasicRelMove)
{
    Gr2DVector vec(0, 0);

    // RelMove target is absolute local position
    vec.RelMove(100, 0, 0, 1000);

    Gr2DTime::SetCurrentTime(0);
    EXPECT_EQ(vec.GetX(), 0);

    Gr2DTime::SetCurrentTime(500);  // Midpoint
    EXPECT_NEAR(vec.GetX(), 50, 1);

    Gr2DTime::SetCurrentTime(1000);  // End
    EXPECT_EQ(vec.GetX(), 100);
}

TEST_F(Gr2DVectorTest, RelMoveVertical)
{
    Gr2DVector vec(0, 0);

    vec.RelMove(0, 200, 0, 1000);

    Gr2DTime::SetCurrentTime(250);
    EXPECT_NEAR(vec.GetY(), 50, 1);  // 25% of 200

    Gr2DTime::SetCurrentTime(750);
    EXPECT_NEAR(vec.GetY(), 150, 1);  // 75% of 200

    Gr2DTime::SetCurrentTime(1000);
    EXPECT_EQ(vec.GetY(), 200);
}

TEST_F(Gr2DVectorTest, RelMoveDiagonal)
{
    Gr2DVector vec(100, 100);

    // RelMove target is absolute: move from (100,100) to (200,200)
    vec.RelMove(200, 200, 0, 1000);

    Gr2DTime::SetCurrentTime(500);
    EXPECT_NEAR(vec.GetX(), 150, 1);
    EXPECT_NEAR(vec.GetY(), 150, 1);

    Gr2DTime::SetCurrentTime(1000);
    EXPECT_EQ(vec.GetX(), 200);
    EXPECT_EQ(vec.GetY(), 200);
}

TEST_F(Gr2DVectorTest, PingPongLoop)
{
    Gr2DVector vec(0, 0);

    // bounce=true, pingpong=true for repeating back-and-forth
    vec.RelMove(100, 0, 0, 1000, true, true);

    // Forward (use non-boundary times to avoid edge cases)
    Gr2DTime::SetCurrentTime(500);
    EXPECT_NEAR(vec.GetX(), 50, 1);

    // At end of first cycle
    Gr2DTime::SetCurrentTime(999);
    EXPECT_NEAR(vec.GetX(), 99, 2);

    // Backward cycle (past first period)
    Gr2DTime::SetCurrentTime(1500);
    EXPECT_NEAR(vec.GetX(), 50, 2);

    // Near start of third cycle
    Gr2DTime::SetCurrentTime(2001);
    EXPECT_NEAR(vec.GetX(), 0, 2);

    // Forward again
    Gr2DTime::SetCurrentTime(2500);
    EXPECT_NEAR(vec.GetX(), 50, 2);
}

TEST_F(Gr2DVectorTest, BounceRepeat)
{
    Gr2DVector vec(0, 0);

    // bounce=true, pingpong=false — accumulates each cycle
    vec.RelMove(100, 0, 0, 1000, true, false);

    Gr2DTime::SetCurrentTime(500);
    EXPECT_NEAR(vec.GetX(), 50, 1);

    // After first cycle, accumulates dx
    Gr2DTime::SetCurrentTime(1500);
    EXPECT_NEAR(vec.GetX(), 150, 2);  // 100 + 50

    Gr2DTime::SetCurrentTime(2500);
    EXPECT_NEAR(vec.GetX(), 250, 2);  // 200 + 50
}

// =============================================================================
// RelMove with Replace
// =============================================================================

TEST_F(Gr2DVectorTest, RelMoveReplace)
{
    Gr2DVector vec(0, 0);

    vec.RelMove(100, 0, 0, 2000);

    Gr2DTime::SetCurrentTime(500);
    EXPECT_NEAR(vec.GetX(), 25, 1);  // 25% of 100

    // Replace with new animation targeting (50, 0)
    vec.RelMove(50, 0, 500, 1500, false, false, true);

    Gr2DTime::SetCurrentTime(1000);
    // Midpoint of new animation: should be around 50% from current to 50
    auto x = vec.GetX();
    EXPECT_NEAR(x, 37, 5);  // Interpolating toward 50 from ~25

    Gr2DTime::SetCurrentTime(1500);
    EXPECT_EQ(vec.GetX(), 50);
}

// =============================================================================
// RelOffset Animation
// =============================================================================

TEST_F(Gr2DVectorTest, RelOffsetBasic)
{
    Gr2DVector vec(100, 100);

    vec.RelOffset(50, 50, 0, 1000);

    Gr2DTime::SetCurrentTime(500);
    EXPECT_NEAR(vec.GetX(), 125, 1);  // 100 + 50*0.5
    EXPECT_NEAR(vec.GetY(), 125, 1);

    Gr2DTime::SetCurrentTime(1000);
    EXPECT_EQ(vec.GetX(), 150);
    EXPECT_EQ(vec.GetY(), 150);
}

TEST_F(Gr2DVectorTest, RelOffsetInstant)
{
    Gr2DVector vec(100, 100);

    // endTime <= startTime with endTime != 0 → instant offset
    vec.RelOffset(50, 50, 100, 50);

    EXPECT_EQ(vec.GetX(), 150);
    EXPECT_EQ(vec.GetY(), 150);
}

// =============================================================================
// Scale
// =============================================================================

TEST_F(Gr2DVectorTest, ScaleWithoutChain)
{
    Gr2DVector vec(100, 100);

    // Scale by 2x around origin (0,0)
    vec.Scale(2, 1, 2, 1, 0, 0);

    EXPECT_EQ(vec.GetX(), 200);  // 0 + 2*(100-0)/1
    EXPECT_EQ(vec.GetY(), 200);
}

TEST_F(Gr2DVectorTest, ScaleWithCenter)
{
    Gr2DVector vec(100, 100);

    // Scale by 2x around center (50, 50)
    vec.Scale(2, 1, 2, 1, 50, 50);

    EXPECT_EQ(vec.GetX(), 150);  // 50 + 2*(100-50)/1
    EXPECT_EQ(vec.GetY(), 150);
}

// =============================================================================
// Rotation Animation
// =============================================================================

TEST_F(Gr2DVectorTest, FiniteRotation)
{
    Gr2DVector vec(100, 0);

    Gr2DTime::SetCurrentTime(0);
    vec.Rotate(90.0, 1000);  // Rotate 90 degrees, completes at frame 1000

    Gr2DTime::SetCurrentTime(0);
    EXPECT_NEAR(vec.GetA(), 0.0, 0.1);

    Gr2DTime::SetCurrentTime(500);
    EXPECT_NEAR(vec.GetA(), 45.0, 1.0);  // Midpoint

    Gr2DTime::SetCurrentTime(1000);
    EXPECT_NEAR(vec.GetA(), 90.0, 0.1);
}

TEST_F(Gr2DVectorTest, ContinuousRotation)
{
    Gr2DVector vec(0, 0);

    Gr2DTime::SetCurrentTime(0);
    // totalAngle=0 means continuous rotation, period=1000
    vec.Rotate(0.0, 1000);

    Gr2DTime::SetCurrentTime(500);
    EXPECT_NEAR(vec.GetA(), 180.0, 1.0);  // Half rotation

    Gr2DTime::SetCurrentTime(1000);
    EXPECT_NEAR(vec.GetA(), 0.0, 1.0);  // Full rotation back to 0
}

TEST_F(Gr2DVectorTest, RotateWithParent)
{
    Gr2DVector parent(0, 0);
    Gr2DVector child(100, 0);

    child.PutOrigin(&parent);

    Gr2DTime::SetCurrentTime(0);
    parent.Rotate(45.0, 1000);

    Gr2DTime::SetCurrentTime(1000);
    EXPECT_NEAR(parent.GetA(), 45.0, 0.1);
    EXPECT_NEAR(child.GetA(), 45.0, 0.1);  // Child inherits parent rotation
}

// =============================================================================
// Ratio Following (delta-based)
// =============================================================================

TEST_F(Gr2DVectorTest, RatioFollowsMovement)
{
    Gr2DVector target(0, 0);
    Gr2DVector follower(0, 0);

    // Ratio captures baseline at creation time. Follows delta at 50% (1/2).
    follower.Ratio(&target, 2, 2, 1, 1);

    // Target hasn't moved from baseline, so follower stays at 0
    EXPECT_EQ(follower.GetX(), 0);
    EXPECT_EQ(follower.GetY(), 0);

    // Now move target (advance time to invalidate evaluation cache)
    target.Move(200, 400);
    Gr2DTime::SetCurrentTime(1);

    // Follower gets 50% of delta: (200-0)/2, (400-0)/2
    EXPECT_EQ(follower.GetX(), 100);
    EXPECT_EQ(follower.GetY(), 200);
}

TEST_F(Gr2DVectorTest, RatioScale)
{
    Gr2DVector target(0, 0);
    Gr2DVector follower(0, 0);

    follower.Ratio(&target, 1, 1, 2, 3);  // Scale 2x and 3x of delta

    target.Move(100, 100);
    Gr2DTime::SetCurrentTime(1);

    EXPECT_EQ(follower.GetX(), 200);  // 2 * (100-0) / 1
    EXPECT_EQ(follower.GetY(), 300);  // 3 * (100-0) / 1
}

TEST_F(Gr2DVectorTest, RatioTracking)
{
    Gr2DVector target(0, 0);
    Gr2DVector follower(0, 0);

    follower.Ratio(&target, 2, 2, 1, 1);

    target.Move(200, 400);
    Gr2DTime::SetCurrentTime(1);

    EXPECT_EQ(follower.GetX(), 100);  // 200 * 1/2
    EXPECT_EQ(follower.GetY(), 200);  // 400 * 1/2
}

// =============================================================================
// Wrap/Clip Boundaries
// =============================================================================

TEST_F(Gr2DVectorTest, ClampMode)
{
    Gr2DVector bounds(0, 0);
    Gr2DVector vec(150, 150);

    vec.WrapClip(&bounds, 0, 0, 100, 100, true);  // clampMode=true

    EXPECT_EQ(vec.GetX(), 100);  // Clamped to max
    EXPECT_EQ(vec.GetY(), 100);
}

TEST_F(Gr2DVectorTest, WrapMode)
{
    Gr2DVector bounds(0, 0);
    Gr2DVector vec(150, 250);

    vec.WrapClip(&bounds, 0, 0, 100, 100, false);  // clampMode=false (wrap)

    EXPECT_EQ(vec.GetX(), 50);   // 150 % 100 = 50
    EXPECT_EQ(vec.GetY(), 50);   // 250 % 100 = 50
}

TEST_F(Gr2DVectorTest, WrapClipStaticHelpers)
{
    // Test the static helper functions directly
    EXPECT_EQ(WrapClipNode::wrapVal(50, 0, 100), 50);
    EXPECT_EQ(WrapClipNode::wrapVal(150, 0, 100), 50);
    EXPECT_EQ(WrapClipNode::wrapVal(-50, 0, 100), 50);
    EXPECT_EQ(WrapClipNode::wrapVal(0, 0, 100), 0);
    EXPECT_EQ(WrapClipNode::wrapVal(100, 0, 100), 0);
    EXPECT_EQ(WrapClipNode::wrapVal(5, 0, 0), 0);  // zero size returns start

    EXPECT_EQ(WrapClipNode::clampVal(50, 0, 100), 50);
    EXPECT_EQ(WrapClipNode::clampVal(-10, 0, 100), 0);
    EXPECT_EQ(WrapClipNode::clampVal(150, 0, 100), 100);
}

// =============================================================================
// Fly Path Animation
// =============================================================================

TEST_F(Gr2DVectorTest, FlyBasicPath)
{
    // FlyNode is phase 0: it transforms the parent position
    // For a vec with no parent, par_x/par_y start at 0.
    Gr2DVector vec(0, 0);
    Gr2DVector p0(0, 0);
    Gr2DVector p1(100, 100);

    std::vector<FlyKeyframe> keyframes = {
        {&p0, 0.0, 0.0, 0.0, 0.0, 0},
        {&p1, 0.0, 0.0, 0.0, 0.0, 1000}
    };

    vec.Fly(keyframes);

    Gr2DTime::SetCurrentTime(0);
    EXPECT_EQ(vec.GetX(), 0);
    EXPECT_EQ(vec.GetY(), 0);

    Gr2DTime::SetCurrentTime(500);
    EXPECT_NEAR(vec.GetX(), 50, 5);
    EXPECT_NEAR(vec.GetY(), 50, 5);
}

TEST_F(Gr2DVectorTest, FlyWithCompletionTarget)
{
    Gr2DVector vec(0, 0);
    Gr2DVector p0(0, 0);
    Gr2DVector p1(100, 100);
    Gr2DVector completionParent(200, 200);

    std::vector<FlyKeyframe> keyframes = {
        {&p0, 0.0, 0.0, 0.0, 0.0, 0},
        {&p1, 0.0, 0.0, 0.0, 0.0, 1000}
    };

    vec.Fly(keyframes, &completionParent);

    // After completion (past last keyframe), FlyNode returns 1
    // and takeCompletion() returns the completion target as new parent
    Gr2DTime::SetCurrentTime(1001);
    auto x = vec.GetX();
    auto y = vec.GetY();
    // After fly completes: parent becomes completionParent(200,200)
    // world = parent(200) + local(0) = 200
    EXPECT_EQ(x, 200);
    EXPECT_EQ(y, 200);
}

TEST_F(Gr2DVectorTest, FlyMultipleKeyframes)
{
    Gr2DVector vec(0, 0);
    Gr2DVector p0(0, 0);
    Gr2DVector p1(100, 0);
    Gr2DVector p2(100, 100);

    std::vector<FlyKeyframe> keyframes = {
        {&p0, 0.0, 0.0, 0.0, 0.0, 0},
        {&p1, 0.0, 0.0, 0.0, 0.0, 500},
        {&p2, 0.0, 0.0, 0.0, 0.0, 1000}
    };

    vec.Fly(keyframes);

    Gr2DTime::SetCurrentTime(250);
    EXPECT_NEAR(vec.GetX(), 50, 10);
    EXPECT_NEAR(vec.GetY(), 0, 10);

    Gr2DTime::SetCurrentTime(750);
    EXPECT_NEAR(vec.GetX(), 100, 10);
    EXPECT_NEAR(vec.GetY(), 50, 10);
}

// =============================================================================
// Complex Scenarios
// =============================================================================

TEST_F(Gr2DVectorTest, AnimationWithParent)
{
    Gr2DVector parent(100, 100);
    Gr2DVector child(0, 0);

    child.PutOrigin(&parent);
    child.RelMove(50, 50, 0, 1000);

    Gr2DTime::SetCurrentTime(500);

    // Child should animate relative to parent
    EXPECT_NEAR(child.GetX(), 125, 2);  // 100 (parent) + 25 (50% of 50)
    EXPECT_NEAR(child.GetY(), 125, 2);
}

TEST_F(Gr2DVectorTest, MultipleAnimationsSequential)
{
    Gr2DVector vec(0, 0);

    // First animation: move to position 100
    vec.RelMove(100, 0, 0, 1000);

    Gr2DTime::SetCurrentTime(1000);
    EXPECT_EQ(vec.GetX(), 100);

    // Second animation: move from 100 to 200 (target is absolute position)
    vec.RelMove(200, 0, 1000, 2000);

    Gr2DTime::SetCurrentTime(1500);
    EXPECT_NEAR(vec.GetX(), 150, 2);  // 100 + 50% of 100

    Gr2DTime::SetCurrentTime(2000);
    EXPECT_EQ(vec.GetX(), 200);
}

// =============================================================================
// Snapshot
// =============================================================================

TEST_F(Gr2DVectorTest, GetSnapshotNoChain)
{
    Gr2DVector vec(42, 99);

    std::int32_t x, y, rx, ry, ox, oy;
    double a, ra;
    vec.GetSnapshot(&x, &y, &rx, &ry, &ox, &oy, &a, &ra);

    EXPECT_EQ(x, 42);
    EXPECT_EQ(y, 99);
    EXPECT_EQ(rx, 42);
    EXPECT_EQ(ry, 99);
    EXPECT_EQ(ox, 0);
    EXPECT_EQ(oy, 0);
    EXPECT_DOUBLE_EQ(a, 0.0);
    EXPECT_DOUBLE_EQ(ra, 0.0);
}

TEST_F(Gr2DVectorTest, GetSnapshotWithParent)
{
    Gr2DVector parent(100, 200);
    Gr2DVector child(10, 20);
    child.PutOrigin(&parent);

    std::int32_t x, y, rx, ry, ox, oy;
    double a, ra;
    child.GetSnapshot(&x, &y, &rx, &ry, &ox, &oy, &a, &ra);

    EXPECT_EQ(x, 110);   // world
    EXPECT_EQ(y, 220);
    EXPECT_EQ(rx, 10);   // local
    EXPECT_EQ(ry, 20);
    EXPECT_EQ(ox, 100);  // parent
    EXPECT_EQ(oy, 200);
}

TEST_F(Gr2DVectorTest, GetSnapshotNullArgs)
{
    Gr2DVector vec(10, 20);

    // Should not crash with null output pointers
    vec.GetSnapshot(nullptr, nullptr, nullptr, nullptr,
                    nullptr, nullptr, nullptr, nullptr);
}

// =============================================================================
// Serialize
// =============================================================================

TEST_F(Gr2DVectorTest, SerializeParenFormat)
{
    Gr2DVector vec;
    vec.Serialize("(42, 99)");
    EXPECT_EQ(vec.GetX(), 42);
    EXPECT_EQ(vec.GetY(), 99);
}

TEST_F(Gr2DVectorTest, SerializeTabFormat)
{
    Gr2DVector vec;
    vec.Serialize("42\t99");
    EXPECT_EQ(vec.GetX(), 42);
    EXPECT_EQ(vec.GetY(), 99);
}

TEST_F(Gr2DVectorTest, SerializeCommaFormat)
{
    Gr2DVector vec;
    vec.Serialize("42,99");
    EXPECT_EQ(vec.GetX(), 42);
    EXPECT_EQ(vec.GetY(), 99);
}

TEST_F(Gr2DVectorTest, SerializeNegative)
{
    Gr2DVector vec;
    vec.Serialize("(-10, -20)");
    EXPECT_EQ(vec.GetX(), -10);
    EXPECT_EQ(vec.GetY(), -20);
}

TEST_F(Gr2DVectorTest, SerializeNull)
{
    Gr2DVector vec(42, 99);
    vec.Serialize(nullptr);  // Should not crash
    EXPECT_EQ(vec.GetX(), 42);
    EXPECT_EQ(vec.GetY(), 99);
}

// =============================================================================
// LooseLevel
// =============================================================================

TEST_F(Gr2DVectorTest, LooseLevelDefault)
{
    Gr2DVector vec(0, 0);
    EXPECT_EQ(vec.GetLooseLevel(), 0);
}

TEST_F(Gr2DVectorTest, LooseLevelSetGet)
{
    Gr2DVector vec(0, 0);

    // Create an easing node first
    vec.RelMove(100, 0, 0, 10000, true, true);

    vec.PutLooseLevel(5);
    EXPECT_EQ(vec.GetLooseLevel(), 5);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(Gr2DVectorTest, ZeroDurationAnimation)
{
    Gr2DVector vec(0, 0);

    vec.RelMove(100, 0, 0, 0);  // Zero duration

    Gr2DTime::SetCurrentTime(0);
    // Should handle gracefully (no crash)
    (void)vec.GetX();
}

TEST_F(Gr2DVectorTest, NegativeTime)
{
    Gr2DVector vec(0, 0);

    vec.RelMove(100, 0, 100, 1000);

    Gr2DTime::SetCurrentTime(50);  // Before start time
    EXPECT_EQ(vec.GetX(), 0);
}

TEST_F(Gr2DVectorTest, BeyondEndTime)
{
    Gr2DVector vec(0, 0);

    vec.RelMove(100, 0, 0, 1000);

    Gr2DTime::SetCurrentTime(2000);  // After end time
    EXPECT_EQ(vec.GetX(), 100);
}

TEST_F(Gr2DVectorTest, NullParent)
{
    Gr2DVector vec(100, 200);

    vec.PutOrigin(nullptr);

    EXPECT_EQ(vec.GetX(), 100);
    EXPECT_EQ(vec.GetY(), 200);
}

TEST_F(Gr2DVectorTest, EmptyFlyPath)
{
    Gr2DVector vec(50, 50);

    std::vector<FlyKeyframe> keyframes;
    vec.Fly(keyframes);

    // Should handle gracefully
    EXPECT_EQ(vec.GetX(), 50);
    EXPECT_EQ(vec.GetY(), 50);
}

TEST_F(Gr2DVectorTest, MoveResetsChain)
{
    Gr2DVector vec(0, 0);

    vec.RelMove(100, 0, 0, 1000);
    Gr2DTime::SetCurrentTime(500);
    EXPECT_NEAR(vec.GetX(), 50, 1);

    // Move should reset the chain
    vec.Move(200, 200);
    EXPECT_EQ(vec.GetX(), 200);
    EXPECT_EQ(vec.GetY(), 200);
}

TEST_F(Gr2DVectorTest, ScaleDivisionByZero)
{
    Gr2DVector vec(100, 200);

    // Should handle gracefully (no crash, no change)
    vec.Scale(2, 0, 2, 1, 0, 0);
    EXPECT_EQ(vec.GetX(), 100);
    EXPECT_EQ(vec.GetY(), 200);

    vec.Scale(2, 1, 2, 0, 0, 0);
    EXPECT_EQ(vec.GetX(), 100);
    EXPECT_EQ(vec.GetY(), 200);
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(Gr2DVectorTest, Performance1000Vectors)
{
    std::vector<Gr2DVector> vectors;
    vectors.reserve(1000);

    for (int i = 0; i < 1000; ++i) {
        vectors.emplace_back(0, 0);
        vectors.back().RelMove(100, 100, 0, 1000);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < 60; ++frame) {
        Gr2DTime::SetCurrentTime(frame * 16);
        for (auto& vec : vectors) {
            (void)vec.GetX();
            (void)vec.GetY();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    EXPECT_LT(duration.count(), 50000) << "Performance test took " << duration.count() << " microseconds";
}

// =============================================================================
// Global Time Tests
// =============================================================================

TEST_F(Gr2DVectorTest, GlobalTime)
{
    Gr2DTime::SetCurrentTime(0);
    EXPECT_EQ(Gr2DTime::GetCurrentTime(), 0);

    Gr2DTime::SetCurrentTime(1000);
    EXPECT_EQ(Gr2DTime::GetCurrentTime(), 1000);

    Gr2DTime::SetCurrentTime(-100);
    EXPECT_EQ(Gr2DTime::GetCurrentTime(), -100);
}

TEST_F(Gr2DVectorTest, GetCurrentTimeFromVector)
{
    Gr2DVector vec;
    Gr2DTime::SetCurrentTime(42);
    EXPECT_EQ(vec.GetCurrentTime(), 42);
}

// =============================================================================
// FlipX
// =============================================================================

TEST_F(Gr2DVectorTest, FlipXDefault)
{
    Gr2DVector vec(0, 0);
    EXPECT_FALSE(vec.GetFlipX());
}
