/**
 * Point and Rect structure tests using Google Test
 */

#include <gtest/gtest.h>
#include <cmath>
#include "util/Point.h"

using namespace ms;

class PointTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Point2D Tests
TEST_F(PointTest, Point2DDefaultConstructor)
{
    Point2D p;
    EXPECT_EQ(p.x, 0);
    EXPECT_EQ(p.y, 0);
}

TEST_F(PointTest, Point2DParameterizedConstructor)
{
    Point2D p(10, 20);
    EXPECT_EQ(p.x, 10);
    EXPECT_EQ(p.y, 20);
}

TEST_F(PointTest, Point2DAddition)
{
    Point2D p1(10, 20);
    Point2D p2(5, 5);
    Point2D result = p1 + p2;

    EXPECT_EQ(result.x, 15);
    EXPECT_EQ(result.y, 25);
}

TEST_F(PointTest, Point2DSubtraction)
{
    Point2D p1(10, 20);
    Point2D p2(3, 7);
    Point2D result = p1 - p2;

    EXPECT_EQ(result.x, 7);
    EXPECT_EQ(result.y, 13);
}

TEST_F(PointTest, Point2DMultiplication)
{
    Point2D p(10, 20);
    Point2D result = p * 2;

    EXPECT_EQ(result.x, 20);
    EXPECT_EQ(result.y, 40);
}

TEST_F(PointTest, Point2DDivision)
{
    Point2D p(10, 20);
    Point2D result = p / 2;

    EXPECT_EQ(result.x, 5);
    EXPECT_EQ(result.y, 10);
}

TEST_F(PointTest, Point2DEquality)
{
    Point2D p1(10, 20);
    Point2D p2(10, 20);
    Point2D p3(10, 21);

    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 == p3);
    EXPECT_TRUE(p1 != p3);
}

TEST_F(PointTest, Point2DLength)
{
    // 3-4-5 triangle
    Point2D p(3, 4);
    EXPECT_EQ(p.Length(), 5);
}

TEST_F(PointTest, Point2DDistance)
{
    Point2D p1(0, 0);
    Point2D p2(3, 4);
    EXPECT_EQ(p1.DistanceTo(p2), 5);
}

// Point2DF Tests
TEST_F(PointTest, Point2DFOperations)
{
    Point2DF p1(1.5f, 2.5f);
    Point2DF p2(0.5f, 0.5f);

    Point2DF sum = p1 + p2;
    EXPECT_NEAR(sum.x, 2.0f, 0.001f);
    EXPECT_NEAR(sum.y, 3.0f, 0.001f);

    Point2DF product = p1 * 2.0f;
    EXPECT_NEAR(product.x, 3.0f, 0.001f);
    EXPECT_NEAR(product.y, 5.0f, 0.001f);
}

TEST_F(PointTest, Point2DFLength)
{
    Point2DF p(3.0f, 4.0f);
    EXPECT_NEAR(p.Length(), 5.0f, 0.001f);
}

// Rect Tests
class RectTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RectTest, RectDefaultConstructor)
{
    Rect r;
    EXPECT_EQ(r.left, 0);
    EXPECT_EQ(r.top, 0);
    EXPECT_EQ(r.right, 0);
    EXPECT_EQ(r.bottom, 0);
}

TEST_F(RectTest, RectParameterizedConstructor)
{
    Rect r(10, 20, 100, 80);
    EXPECT_EQ(r.left, 10);
    EXPECT_EQ(r.top, 20);
    EXPECT_EQ(r.right, 100);
    EXPECT_EQ(r.bottom, 80);
}

TEST_F(RectTest, RectDimensions)
{
    Rect r(10, 20, 100, 80);
    EXPECT_EQ(r.Width(), 90);
    EXPECT_EQ(r.Height(), 60);
}

TEST_F(RectTest, RectContainsPoint)
{
    Rect r(10, 20, 100, 80);

    // Inside
    EXPECT_TRUE(r.Contains(Point2D(50, 50)));

    // On left edge (inclusive)
    EXPECT_TRUE(r.Contains(Point2D(10, 50)));

    // On top edge (inclusive)
    EXPECT_TRUE(r.Contains(Point2D(50, 20)));

    // Outside left
    EXPECT_FALSE(r.Contains(Point2D(5, 50)));

    // Outside right (exclusive)
    EXPECT_FALSE(r.Contains(Point2D(100, 50)));

    // Outside bottom (exclusive)
    EXPECT_FALSE(r.Contains(Point2D(50, 80)));
}

TEST_F(RectTest, RectIntersects)
{
    Rect r1(10, 20, 100, 80);

    // Overlapping
    Rect r2(50, 40, 150, 120);
    EXPECT_TRUE(r1.Intersects(r2));

    // Non-overlapping
    Rect r3(200, 200, 300, 300);
    EXPECT_FALSE(r1.Intersects(r3));

    // Adjacent (not intersecting)
    Rect r4(100, 20, 150, 80);
    EXPECT_FALSE(r1.Intersects(r4));

    // Contained
    Rect r5(20, 30, 50, 60);
    EXPECT_TRUE(r1.Intersects(r5));
}

TEST_F(RectTest, RectOffset)
{
    Rect r(0, 0, 10, 10);
    r.Offset(5, 5);

    EXPECT_EQ(r.left, 5);
    EXPECT_EQ(r.top, 5);
    EXPECT_EQ(r.right, 15);
    EXPECT_EQ(r.bottom, 15);
}

TEST_F(RectTest, RectNegativeOffset)
{
    Rect r(10, 10, 20, 20);
    r.Offset(-5, -5);

    EXPECT_EQ(r.left, 5);
    EXPECT_EQ(r.top, 5);
    EXPECT_EQ(r.right, 15);
    EXPECT_EQ(r.bottom, 15);
}
