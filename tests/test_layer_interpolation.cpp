#include <gtest/gtest.h>
#include "graphics/WzGr2DLayer.h"
#include "graphics/WzGr2DCanvas.h"
#include "graphics/Gr2DVector.h"
#include "wz/WzCanvas.h"

using namespace ms;

class LayerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        Gr2DTime::SetCurrentTime(0);
    }

    auto makeCanvas() -> std::shared_ptr<WzGr2DCanvas>
    {
        return std::make_shared<WzGr2DCanvas>(std::make_shared<WzCanvas>());
    }
};

// === Frame management tests ===

TEST_F(LayerTest, InsertAndCountFrames)
{
    WzGr2DLayer layer(0, 0, 100, 100, 0);
    EXPECT_EQ(layer.GetCanvasCount(), 0);
    EXPECT_EQ(layer.get_canvasCount(), 0);

    auto c1 = makeCanvas();
    auto c2 = makeCanvas();

    layer.InsertCanvas(c1, 100, 255, 255);
    EXPECT_EQ(layer.GetCanvasCount(), 1);

    layer.InsertCanvas(c2, 200, 255, 255);
    EXPECT_EQ(layer.GetCanvasCount(), 2);
    EXPECT_EQ(layer.get_canvasCount(), 2);
}

TEST_F(LayerTest, CurrentCanvasAfterInsert)
{
    WzGr2DLayer layer;
    auto c1 = makeCanvas();
    auto c2 = makeCanvas();

    layer.InsertCanvas(c1, 100, 255, 255);
    // First frame becomes current
    EXPECT_EQ(layer.GetCurrentCanvas(), c1);

    layer.InsertCanvas(c2, 100, 255, 255);
    // Current stays on first frame
    EXPECT_EQ(layer.GetCurrentCanvas(), c1);
}

TEST_F(LayerTest, RemoveAllCanvases)
{
    WzGr2DLayer layer;
    layer.InsertCanvas(makeCanvas(), 100, 255, 255);
    layer.InsertCanvas(makeCanvas(), 100, 255, 255);
    EXPECT_EQ(layer.GetCanvasCount(), 2);

    layer.RemoveAllCanvases();
    EXPECT_EQ(layer.GetCanvasCount(), 0);
    EXPECT_EQ(layer.get_canvasCount(), 0);
    EXPECT_EQ(layer.GetCurrentCanvas(), nullptr);
}

TEST_F(LayerTest, SourceStyleInsertCanvas)
{
    WzGr2DLayer layer;
    auto c1 = makeCanvas();

    // Use source-style API with ICanvas*
    int frameId = layer.InsertCanvas(static_cast<ICanvas*>(c1.get()), 500, -1, -1, 0, 0);
    EXPECT_GE(frameId, 0);
    EXPECT_EQ(layer.get_canvasCount(), 1);
    EXPECT_EQ(layer.get_canvas(), static_cast<ICanvas*>(c1.get()));
}

TEST_F(LayerTest, RemoveCanvasByIndex)
{
    WzGr2DLayer layer;
    auto c1 = makeCanvas();
    auto c2 = makeCanvas();
    auto c3 = makeCanvas();

    layer.InsertCanvas(static_cast<ICanvas*>(c1.get()), 100);
    layer.InsertCanvas(static_cast<ICanvas*>(c2.get()), 100);
    layer.InsertCanvas(static_cast<ICanvas*>(c3.get()), 100);
    EXPECT_EQ(layer.get_canvasCount(), 3);

    // Remove middle frame
    layer.RemoveCanvas(1);
    EXPECT_EQ(layer.get_canvasCount(), 2);
}

TEST_F(LayerTest, ShiftCanvas)
{
    WzGr2DLayer layer;
    auto c1 = makeCanvas();
    auto c2 = makeCanvas();
    auto c3 = makeCanvas();

    layer.InsertCanvas(static_cast<ICanvas*>(c1.get()), 100);
    layer.InsertCanvas(static_cast<ICanvas*>(c2.get()), 100);
    layer.InsertCanvas(static_cast<ICanvas*>(c3.get()), 100);

    // Current should be first
    EXPECT_EQ(layer.get_canvas(), static_cast<ICanvas*>(c1.get()));

    // Shift to second
    layer.ShiftCanvas(1);
    EXPECT_EQ(layer.get_canvas(), static_cast<ICanvas*>(c2.get()));

    // Shift to third
    layer.ShiftCanvas(2);
    EXPECT_EQ(layer.get_canvas(), static_cast<ICanvas*>(c3.get()));

    // Negative wrap
    layer.ShiftCanvas(-1);
    EXPECT_EQ(layer.get_canvas(), static_cast<ICanvas*>(c3.get()));
}

TEST_F(LayerTest, InitCanvasOrder)
{
    WzGr2DLayer layer;
    auto c1 = makeCanvas();
    auto c2 = makeCanvas();

    layer.InsertCanvas(static_cast<ICanvas*>(c1.get()), 100);
    layer.InsertCanvas(static_cast<ICanvas*>(c2.get()), 100);

    layer.ShiftCanvas(1);
    EXPECT_EQ(layer.get_canvas(), static_cast<ICanvas*>(c2.get()));

    layer.InitCanvasOrder();
    EXPECT_EQ(layer.get_canvas(), static_cast<ICanvas*>(c1.get()));
}

// === Color tests (3 Gr2DVector channels) ===

TEST_F(LayerTest, ColorViaVectors)
{
    WzGr2DLayer layer;

    // Default color is white (fully opaque)
    EXPECT_EQ(layer.GetColor(), 0xFFFFFFFF);

    // Set color via put_color
    layer.put_color(0x80FF0000); // 50% alpha, red
    EXPECT_EQ(layer.get_color(), 0x80FF0000);

    // Verify individual channels
    EXPECT_NE(layer.get_alpha(), nullptr);
    EXPECT_NE(layer.get_redTone(), nullptr);
    EXPECT_NE(layer.get_greenBlueTone(), nullptr);
}

TEST_F(LayerTest, SetColorBackwardCompat)
{
    WzGr2DLayer layer;

    layer.SetColor(0xAABBCCDD);
    EXPECT_EQ(layer.GetColor(), 0xAABBCCDD);

    // Alpha accessor
    EXPECT_EQ(layer.GetAlpha(), 0xAA);

    layer.SetAlpha(0x55);
    EXPECT_EQ(layer.GetAlpha(), 0x55);
}

// === Animation tests ===

TEST_F(LayerTest, AnimateRequiresMultipleFrames)
{
    WzGr2DLayer layer;
    layer.InsertCanvas(makeCanvas(), 100, 255, 255);

    // Single frame: Animate should fail
    EXPECT_FALSE(layer.Animate(Gr2DAnimationType::Loop));
}

TEST_F(LayerTest, ForwardAnimation)
{
    WzGr2DLayer layer;
    layer.InsertCanvas(makeCanvas(), 100, 255, 255);
    layer.InsertCanvas(makeCanvas(), 100, 255, 255);
    layer.InsertCanvas(makeCanvas(), 100, 255, 255);

    EXPECT_TRUE(layer.Animate(Gr2DAnimationType::First, 1000, 0));
    EXPECT_TRUE(layer.IsAnimating());
    EXPECT_EQ(layer.GetCurrentFrame(), 0);

    // First update initializes time
    layer.Update(0);
    EXPECT_EQ(layer.GetCurrentFrame(), 0);

    // Advance past first frame delay (100ms)
    Gr2DTime::SetCurrentTime(100);
    layer.Update(100);
    EXPECT_EQ(layer.GetCurrentFrame(), 1);

    // Advance past second frame
    Gr2DTime::SetCurrentTime(200);
    layer.Update(200);
    EXPECT_EQ(layer.GetCurrentFrame(), 2);

    // One-shot: should stop after last frame
    Gr2DTime::SetCurrentTime(300);
    layer.Update(300);
    EXPECT_FALSE(layer.IsAnimating());
}

TEST_F(LayerTest, LoopAnimation)
{
    WzGr2DLayer layer;
    layer.InsertCanvas(makeCanvas(), 100, 255, 255);
    layer.InsertCanvas(makeCanvas(), 100, 255, 255);

    EXPECT_TRUE(layer.Animate(Gr2DAnimationType::Loop));
    EXPECT_TRUE(layer.IsAnimating());

    layer.Update(0);
    EXPECT_EQ(layer.GetCurrentFrame(), 0);

    Gr2DTime::SetCurrentTime(100);
    layer.Update(100);
    EXPECT_EQ(layer.GetCurrentFrame(), 1);

    // Should loop back to frame 0
    Gr2DTime::SetCurrentTime(200);
    layer.Update(200);
    EXPECT_EQ(layer.GetCurrentFrame(), 0);

    // Still animating (infinite loop)
    EXPECT_TRUE(layer.IsAnimating());
}

TEST_F(LayerTest, StopAnimation)
{
    WzGr2DLayer layer;
    layer.InsertCanvas(makeCanvas(), 100, 255, 255);
    layer.InsertCanvas(makeCanvas(), 100, 255, 255);

    layer.Animate(Gr2DAnimationType::Loop);
    EXPECT_TRUE(layer.IsAnimating());

    layer.StopAnimation();
    EXPECT_FALSE(layer.IsAnimating());
}

// === Source-style Animate producing RenderCommands ===

TEST_F(LayerTest, AnimateProducesRenderCommands)
{
    WzGr2DLayer layer;
    auto c1 = makeCanvas();
    auto c2 = makeCanvas();

    layer.InsertCanvas(static_cast<ICanvas*>(c1.get()), 500);
    layer.InsertCanvas(static_cast<ICanvas*>(c2.get()), 500);

    int count = layer.Animate(static_cast<std::uint32_t>(0x20), 100);
    EXPECT_EQ(count, 2);

    auto& cmds = layer.getRenderCommands();
    EXPECT_EQ(cmds.size(), 2);
    EXPECT_EQ(cmds[0].frameIndex, 0);
    EXPECT_EQ(cmds[1].frameIndex, 1);
}

// === Properties tests ===

TEST_F(LayerTest, ZOrder)
{
    WzGr2DLayer layer(10, 20, 100, 100, 5);
    EXPECT_EQ(layer.GetZ(), 5);
    EXPECT_EQ(layer.get_z(), 5);

    layer.SetZ(10);
    EXPECT_EQ(layer.GetZ(), 10);

    layer.put_z(15);
    EXPECT_EQ(layer.get_z(), 15);
}

TEST_F(LayerTest, Visibility)
{
    WzGr2DLayer layer;
    EXPECT_TRUE(layer.IsVisible());
    EXPECT_TRUE(layer.get_visible());

    layer.SetVisible(false);
    EXPECT_FALSE(layer.IsVisible());

    layer.put_visible(true);
    EXPECT_TRUE(layer.get_visible());
}

TEST_F(LayerTest, FlipMode)
{
    WzGr2DLayer layer;
    EXPECT_EQ(layer.GetFlip(), LayerFlipState::None);
    EXPECT_EQ(layer.get_flip(), 0);

    layer.SetFlip(LayerFlipState::Horizontal);
    EXPECT_EQ(layer.GetFlip(), LayerFlipState::Horizontal);
    EXPECT_EQ(layer.get_flip(), 1);

    layer.put_flip(3);
    EXPECT_EQ(layer.GetFlip(), LayerFlipState::Both);
}

TEST_F(LayerTest, Dimensions)
{
    WzGr2DLayer layer(10, 20, 300, 400, 0);

    EXPECT_EQ(layer.GetWidth(), 300);
    EXPECT_EQ(layer.GetHeight(), 400);
    EXPECT_EQ(layer.get_width(), 300);
    EXPECT_EQ(layer.get_height(), 400);
    EXPECT_EQ(layer.GetLeft(), 10);
    EXPECT_EQ(layer.GetTop(), 20);

    layer.put_width(500);
    layer.put_height(600);
    EXPECT_EQ(layer.get_width(), 500);
    EXPECT_EQ(layer.get_height(), 600);
}

// === Initialization tests ===

TEST_F(LayerTest, SetVideoMode)
{
    WzGr2DLayer layer;

    layer.SetVideoMode(800, 600, 640, 480);

    // After SetVideoMode, get_lt and get_rb should be valid
    EXPECT_NE(layer.get_lt(), nullptr);
    EXPECT_NE(layer.get_rb(), nullptr);

    // Dimensions should be set
    EXPECT_EQ(layer.get_width(), 640);
    EXPECT_EQ(layer.get_height(), 480);

    // Color should be white
    EXPECT_EQ(layer.get_color(), 0xFFFFFFFF);
}

TEST_F(LayerTest, InitAnimationAndOrigin)
{
    WzGr2DLayer layer;

    layer.InitAnimation(0);
    EXPECT_NE(layer.getAnimOriginVector(), nullptr);

    Gr2DVector origin(100, 200);
    layer.SetAnimOrigin(&origin);
    // Should not crash
}

// === Boundary vectors ===

TEST_F(LayerTest, BoundaryVectors)
{
    WzGr2DLayer layer;

    EXPECT_NE(layer.get_lt(), nullptr);
    EXPECT_NE(layer.get_rb(), nullptr);

    // InterlockedOffset should not crash
    layer.InterlockedOffset(10, 20, 30, 40);
}

// === Tag and flags ===

TEST_F(LayerTest, TagAndFlags)
{
    WzGr2DLayer layer;

    layer.setTag(42);
    EXPECT_EQ(layer.getTag(), 42);

    layer.setFlags(0x0F);
    layer.clearFlags(0x03);
    // Internal state - just ensure no crash
}

// === Particle system ===

TEST_F(LayerTest, ParticleEmitter)
{
    WzGr2DLayer layer;

    auto* emitter = layer.getEmitter();
    EXPECT_NE(emitter, nullptr);

    // Second call returns same emitter
    EXPECT_EQ(layer.getEmitter(), emitter);

    // UpdateParticles should not crash
    layer.UpdateParticles(0.016F);
}
