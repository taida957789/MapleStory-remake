/**
 * WZ Property and Canvas tests using Google Test
 */

#include <gtest/gtest.h>
#include <cmath>
#include "wz/WzProperty.h"
#include "wz/WzCanvas.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"

using namespace ms;

class WzPropertyTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(WzPropertyTest, PropertyName)
{
    auto prop = std::make_shared<WzProperty>("test");
    EXPECT_EQ(prop->GetName(), "test");

    prop->SetName("renamed");
    EXPECT_EQ(prop->GetName(), "renamed");
}

TEST_F(WzPropertyTest, IntValue)
{
    auto prop = std::make_shared<WzProperty>("test");

    prop->SetInt(42);
    EXPECT_EQ(prop->GetInt(), 42);
    EXPECT_EQ(prop->GetInt(100), 42);
}

TEST_F(WzPropertyTest, IntDefaultValue)
{
    auto prop = std::make_shared<WzProperty>("test");
    EXPECT_EQ(prop->GetInt(999), 999);
}

TEST_F(WzPropertyTest, LongValue)
{
    auto prop = std::make_shared<WzProperty>("test");

    prop->SetLong(0x123456789ABCLL);
    EXPECT_EQ(prop->GetLong(), 0x123456789ABCLL);
}

TEST_F(WzPropertyTest, FloatValue)
{
    auto prop = std::make_shared<WzProperty>("test");

    prop->SetFloat(3.14f);
    EXPECT_NEAR(prop->GetFloat(), 3.14f, 0.001f);
}

TEST_F(WzPropertyTest, DoubleValue)
{
    auto prop = std::make_shared<WzProperty>("test");

    prop->SetDouble(3.14159265359);
    EXPECT_NEAR(prop->GetDouble(), 3.14159265359, 0.0000001);
}

TEST_F(WzPropertyTest, StringValue)
{
    auto prop = std::make_shared<WzProperty>("test");

    prop->SetString("Hello World");
    EXPECT_EQ(prop->GetString(), "Hello World");
}

TEST_F(WzPropertyTest, StringDefaultValue)
{
    auto prop = std::make_shared<WzProperty>("test");
    EXPECT_EQ(prop->GetString("default"), "default");
}

TEST_F(WzPropertyTest, IntToLongConversion)
{
    auto prop = std::make_shared<WzProperty>("test");
    prop->SetInt(42);

    // Should be able to read as long
    EXPECT_EQ(prop->GetLong(), 42);
}

TEST_F(WzPropertyTest, FloatToDoubleConversion)
{
    auto prop = std::make_shared<WzProperty>("test");
    prop->SetFloat(3.14f);

    // Should be able to read as double
    EXPECT_NEAR(prop->GetDouble(), 3.14, 0.01);
}

// Property Tree Tests
class WzPropertyTreeTest : public ::testing::Test
{
protected:
    std::shared_ptr<WzProperty> root;

    void SetUp() override
    {
        root = std::make_shared<WzProperty>("root");

        auto child1 = std::make_shared<WzProperty>("child1");
        child1->SetInt(100);

        auto child2 = std::make_shared<WzProperty>("child2");
        child2->SetString("value");

        auto child3 = std::make_shared<WzProperty>("child3");
        child3->SetFloat(1.5f);

        root->AddChild(child1);
        root->AddChild(child2);
        root->AddChild(child3);
    }

    void TearDown() override
    {
        root.reset();
    }
};

TEST_F(WzPropertyTreeTest, GetChild)
{
    auto child = root->GetChild("child1");
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->GetInt(), 100);
}

TEST_F(WzPropertyTreeTest, OperatorBracket)
{
    auto child = (*root)["child2"];
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->GetString(), "value");
}

TEST_F(WzPropertyTreeTest, NonExistentChild)
{
    auto missing = root->GetChild("missing");
    EXPECT_EQ(missing, nullptr);
}

TEST_F(WzPropertyTreeTest, ChildCount)
{
    const auto& children = root->GetChildren();
    EXPECT_EQ(children.size(), 3);
}

TEST_F(WzPropertyTreeTest, IterateChildren)
{
    int count = 0;
    for (const auto& [name, prop] : root->GetChildren())
    {
        EXPECT_FALSE(name.empty());
        EXPECT_NE(prop, nullptr);
        count++;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(WzPropertyTreeTest, NestedChildren)
{
    auto nested = std::make_shared<WzProperty>("nested");
    nested->SetInt(999);

    auto child = root->GetChild("child1");
    child->AddChild(nested);

    auto retrieved = (*child)["nested"];
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->GetInt(), 999);
}

// WzCanvas Tests
class WzCanvasTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(WzCanvasTest, DefaultConstructor)
{
    WzCanvas canvas;
    EXPECT_EQ(canvas.GetWidth(), 0);
    EXPECT_EQ(canvas.GetHeight(), 0);
    // Note: GetZ() moved to WzGr2DCanvas
}

TEST_F(WzCanvasTest, SizedConstructor)
{
    WzCanvas canvas(100, 50);
    EXPECT_EQ(canvas.GetWidth(), 100);
    EXPECT_EQ(canvas.GetHeight(), 50);

    // Should allocate RGBA pixel data
    EXPECT_EQ(canvas.GetPixelData().size(), 100 * 50 * 4);
}

// Note: Origin functionality moved to WzGr2DCanvas
// TEST_F(WzCanvasTest, Origin)
// {
//     Origin functionality moved to WzGr2DCanvas
// }

// Note: Z value functionality moved to WzGr2DCanvas
// TEST_F(WzCanvasTest, ZValue)
// {
//     Z functionality moved to WzGr2DCanvas
// }

TEST_F(WzCanvasTest, SetPixelData)
{
    WzCanvas canvas(10, 10);

    std::vector<uint8_t> testData(100 * 4, 255);
    canvas.SetPixelData(testData);

    EXPECT_EQ(canvas.GetPixelData().size(), testData.size());
    EXPECT_EQ(canvas.GetPixelData()[0], 255);
}

TEST_F(WzCanvasTest, CanvasProperty)
{
    auto prop = std::make_shared<WzProperty>("image");
    auto canvas = std::make_shared<WzCanvas>(64, 64);

    prop->SetCanvas(canvas);

    auto retrieved = prop->GetCanvas();
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->GetWidth(), 64);
    EXPECT_EQ(retrieved->GetHeight(), 64);
}

TEST_F(WzCanvasTest, NullCanvas)
{
    auto prop = std::make_shared<WzProperty>("empty");
    EXPECT_EQ(prop->GetCanvas(), nullptr);
}

// =============================================================================
// WZ File Integration Tests (using real WZ files)
// =============================================================================

#include "wz/WzFile.h"
#include "wz/WzResMan.h"
#include "wz/WzDirectory.h"
#include "wz/WzImage.h"
#include <filesystem>
#include <set>
#include <algorithm>
#include <cctype>

// Path to test WZ files
static const std::string WZ_TEST_PATH = "../resources/new";

class WzFileTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Check if test files exist
        m_bHasTestFiles = std::filesystem::exists(WZ_TEST_PATH + "/Base.wz");
    }

    void TearDown() override {}

    bool m_bHasTestFiles = false;
};

TEST_F(WzFileTest, OpenBaseWz)
{
    if (!m_bHasTestFiles)
    {
        GTEST_SKIP() << "Test WZ files not found at " << WZ_TEST_PATH;
    }

    WzFile wzFile;
    bool result = wzFile.Open(WZ_TEST_PATH + "/Base.wz");

    EXPECT_TRUE(result) << "Failed to open Base.wz";
    EXPECT_TRUE(wzFile.IsOpen());
    EXPECT_GT(wzFile.GetVersion(), 0) << "Version should be positive";

    std::cout << "Base.wz version: " << wzFile.GetVersion() << std::endl;
}

TEST_F(WzFileTest, ParseBaseWzRoot)
{
    if (!m_bHasTestFiles)
    {
        GTEST_SKIP() << "Test WZ files not found";
    }

    WzFile wzFile;
    ASSERT_TRUE(wzFile.Open(WZ_TEST_PATH + "/Base.wz"));

    auto root = wzFile.GetRoot();
    ASSERT_NE(root, nullptr);

    std::cout << "Base.wz root children:" << std::endl;
    for (const auto& [name, child] : root->GetChildren())
    {
        std::cout << "  - " << name << std::endl;
    }

    EXPECT_GT(root->GetChildCount(), 0) << "Base.wz should have children";
}

TEST_F(WzFileTest, OpenStringWz)
{
    if (!m_bHasTestFiles)
    {
        GTEST_SKIP() << "Test WZ files not found";
    }

    WzFile wzFile;
    bool result = wzFile.Open(WZ_TEST_PATH + "/String.wz");

    EXPECT_TRUE(result) << "Failed to open String.wz";
    EXPECT_TRUE(wzFile.IsOpen());

    std::cout << "String.wz version: " << wzFile.GetVersion() << std::endl;
}

TEST_F(WzFileTest, OpenEtcWz)
{
    if (!m_bHasTestFiles)
    {
        GTEST_SKIP() << "Test WZ files not found";
    }

    WzFile wzFile;
    bool result = wzFile.Open(WZ_TEST_PATH + "/Etc.wz");

    EXPECT_TRUE(result) << "Failed to open Etc.wz";

    auto root = wzFile.GetRoot();
    ASSERT_NE(root, nullptr);

    std::cout << "Etc.wz root children:" << std::endl;
    int count = 0;
    for (const auto& [name, child] : root->GetChildren())
    {
        std::cout << "  - " << name << " (type: " << static_cast<int>(child->GetType()) << ")" << std::endl;
        if (++count >= 10)
        {
            std::cout << "  ... and more" << std::endl;
            break;
        }
    }
}

TEST_F(WzFileTest, LazyLoadingImg)
{
    if (!m_bHasTestFiles)
    {
        GTEST_SKIP() << "Test WZ files not found";
    }

    WzFile wzFile;
    ASSERT_TRUE(wzFile.Open(WZ_TEST_PATH + "/String.wz"));

    auto root = wzFile.GetRoot();
    ASSERT_NE(root, nullptr);

    // Find first .img child
    std::shared_ptr<WzProperty> imgNode;
    for (const auto& [name, child] : root->GetChildren())
    {
        if (name.find(".img") != std::string::npos)
        {
            imgNode = std::dynamic_pointer_cast<WzProperty>(child);
            std::cout << "Found img: " << name << std::endl;
            break;
        }
    }

    if (imgNode)
    {
        // Before accessing children, it should need loading
        bool needsLoad = imgNode->NeedsLoad();
        std::cout << "Needs load before access: " << (needsLoad ? "yes" : "no") << std::endl;

        // Access children to trigger lazy loading
        auto& children = imgNode->GetChildren();
        std::cout << "Children count after access: " << children.size() << std::endl;

        // After accessing, it should be loaded
        EXPECT_FALSE(imgNode->NeedsLoad()) << "Should be loaded after accessing children";
    }
}

TEST_F(WzFileTest, OpenUIWz)
{
    if (!m_bHasTestFiles)
    {
        GTEST_SKIP() << "Test WZ files not found";
    }

    WzFile wzFile;
    bool result = wzFile.Open(WZ_TEST_PATH + "/UI.wz");

    EXPECT_TRUE(result) << "Failed to open UI.wz";

    auto root = wzFile.GetRoot();
    ASSERT_NE(root, nullptr);

    std::cout << "UI.wz root children:" << std::endl;
    int count = 0;
    for (const auto& [name, child] : root->GetChildren())
    {
        std::cout << "  - " << name << std::endl;
        if (++count >= 15)
        {
            std::cout << "  ... and " << (root->GetChildCount() - count) << " more" << std::endl;
            break;
        }
    }
}

// Test WzResMan with actual WZ files
class WzResManTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bHasTestFiles = std::filesystem::exists(WZ_TEST_PATH + "/Base.wz");
    }

    void TearDown() override
    {
        // Reset singleton state if needed
    }

    bool m_bHasTestFiles = false;
};

TEST_F(WzResManTest, InitializeWithTestFiles)
{
    if (!m_bHasTestFiles)
    {
        GTEST_SKIP() << "Test WZ files not found";
    }

    auto& resMan = WzResMan::GetInstance();
    resMan.SetBasePath(WZ_TEST_PATH);

    bool result = resMan.Initialize();
    EXPECT_TRUE(result) << "Failed to initialize WzResMan";
}

// NOTE: This test is disabled because MapLogin.img is no longer used.
// Login now uses LoginBack.img instead (simple background format, not map format).
TEST_F(WzResManTest, DISABLED_CheckMapLoginStructure)
{
    GTEST_SKIP() << "MapLogin.img is no longer used - Login now uses LoginBack.img";

    if (!m_bHasTestFiles)
    {
        GTEST_SKIP() << "Test WZ files not found";
    }

    auto& resMan = WzResMan::GetInstance();
    resMan.SetBasePath(WZ_TEST_PATH);
    resMan.Initialize();

    // Check UI/MapLogin.img structure
    auto mapLoginProp = resMan.GetProperty("UI/MapLogin.img");
    if (mapLoginProp)
    {
        std::cout << "UI/MapLogin.img children:" << std::endl;
        for (const auto& [name, child] : mapLoginProp->GetChildren())
        {
            std::cout << "  - " << name << std::endl;

            // If it's "back", print its children too
            if (name == "back")
            {
                std::cout << "    'back' has " << child->GetChildCount() << " children:" << std::endl;
                int count = 0;
                for (const auto& [backName, backChild] : child->GetChildren())
                {
                    std::cout << "      - " << backName << std::endl;

                    // Print first back piece properties
                    if (count == 0)
                    {
                        std::cout << "        Properties:" << std::endl;
                        for (const auto& [propName, propChild] : backChild->GetChildren())
                        {
                            std::cout << "          - " << propName;
                            auto strVal = propChild->GetString("");
                            auto intVal = propChild->GetInt(0);
                            if (!strVal.empty())
                            {
                                std::cout << " = \"" << strVal << "\"";
                            }
                            else if (intVal != 0)
                            {
                                std::cout << " = " << intVal;
                            }
                            std::cout << std::endl;
                        }
                    }
                    if (++count >= 5)
                    {
                        std::cout << "      ... and more" << std::endl;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        std::cout << "UI/MapLogin.img not found" << std::endl;
    }

    // Check Map/Back/login.img structure
    auto loginBackProp = resMan.GetProperty("Map/Back/login.img");
    if (loginBackProp)
    {
        std::cout << "\nMap/Back/login.img:" << std::endl;
        std::cout << "  NeedsLoad: " << (loginBackProp->NeedsLoad() ? "yes" : "no") << std::endl;
        std::cout << "  NodeType: " << static_cast<int>(loginBackProp->GetNodeType()) << std::endl;
        std::cout << "  ChildCount: " << loginBackProp->GetChildCount() << std::endl;

        // Force load by accessing children
        auto& children = loginBackProp->GetChildren();
        std::cout << "  After GetChildren: " << children.size() << " children" << std::endl;

        for (const auto& [name, child] : children)
        {
            std::cout << "  - " << name << " (" << child->GetChildCount() << " children)" << std::endl;

            // Show first few children of each section
            int count = 0;
            for (const auto& [subName, subChild] : child->GetChildren())
            {
                std::cout << "      - " << subName;
                // Check for canvas
                auto canvas = subChild->GetCanvas();
                if (canvas)
                {
                    std::cout << " [canvas " << canvas->GetWidth() << "x" << canvas->GetHeight() << "]";
                }
                std::cout << std::endl;
                if (++count >= 3)
                {
                    std::cout << "      ... and more" << std::endl;
                    break;
                }
            }
        }
    }
    else
    {
        std::cout << "Map/Back/login.img not found" << std::endl;
    }

    // Check what's actually in Map.wz
    auto mapProp = resMan.GetProperty("Map");
    if (mapProp)
    {
        std::cout << "\nMap/ top-level children:" << std::endl;
        int count = 0;
        for (const auto& [name, child] : mapProp->GetChildren())
        {
            std::cout << "  - " << name << std::endl;
            if (++count >= 20) break;
        }

        // Check Map/Map subdirectory
        auto mapMapDir = resMan.GetProperty("Map/Map");
        if (mapMapDir)
        {
            std::cout << "\nMap/Map/ children:" << std::endl;
            count = 0;
            for (const auto& [name, child] : mapMapDir->GetChildren())
            {
                std::cout << "  - " << name << std::endl;
                if (++count >= 15) break;
            }
        }

        // Check for Back directory
        auto backDir = resMan.GetProperty("Map/Back");
        if (backDir)
        {
            std::cout << "\nMap/Back/ children:" << std::endl;
            count = 0;
            for (const auto& [name, child] : backDir->GetChildren())
            {
                std::cout << "  - " << name << std::endl;
                if (++count >= 20) break;
            }
        }
        else
        {
            std::cout << "\nMap/Back not found in Map.wz" << std::endl;
        }
    }

    // Check Map2.wz for Back
    auto map2Prop = resMan.GetProperty("Map2");
    if (map2Prop)
    {
        std::cout << "\nMap2/ top-level children:" << std::endl;
        int count = 0;
        for (const auto& [name, child] : map2Prop->GetChildren())
        {
            std::cout << "  - " << name << std::endl;
            if (++count >= 15) break;
        }

        auto backDir2 = resMan.GetProperty("Map2/Back");
        if (backDir2)
        {
            std::cout << "\nMap2/Back/ all children:" << std::endl;
            count = 0;
            for (const auto& [name, child] : backDir2->GetChildren())
            {
                std::cout << "  - " << name;
                // Highlight login-related
                if (name.find("ogin") != std::string::npos || name.find("Login") != std::string::npos)
                {
                    std::cout << " <-- LOGIN RELATED";
                }
                std::cout << std::endl;
            }

            // Check specifically for login.img
            auto loginImg = resMan.GetProperty("Map2/Back/login.img");
            if (loginImg)
            {
                std::cout << "\nMap2/Back/login.img found!" << std::endl;
                std::cout << "  NeedsLoad: " << (loginImg->NeedsLoad() ? "yes" : "no") << std::endl;
                std::cout << "  NodeType: " << static_cast<int>(loginImg->GetNodeType()) << std::endl;

                // Force children load
                auto& children = loginImg->GetChildren();
                std::cout << "  Children after GetChildren(): " << children.size() << std::endl;
                for (const auto& [name, child] : children)
                {
                    std::cout << "    - " << name << " (" << child->GetChildCount() << " children)" << std::endl;
                }
            }
            else
            {
                std::cout << "\nMap2/Back/login.img not found, searching for login..." << std::endl;
                for (const auto& [name, child] : backDir2->GetChildren())
                {
                    if (name.find("login") != std::string::npos ||
                        name.find("Login") != std::string::npos)
                    {
                        std::cout << "  Found: " << name << std::endl;
                    }
                }
            }
        }
        else
        {
            std::cout << "\nMap2/Back not found" << std::endl;
        }
    }
    else
    {
        std::cout << "\nMap2.wz not found" << std::endl;
    }

    // Check what bS values are used in MapLogin.img/back
    auto mapLoginBackProp = resMan.GetProperty("UI/MapLogin.img/back");
    if (mapLoginBackProp)
    {
        std::cout << "\nAnalyzing UI/MapLogin.img/back pieces..." << std::endl;
        std::set<std::string> bsValues;
        int count = 0;
        for (const auto& [name, child] : mapLoginBackProp->GetChildren())
        {
            auto bsProp = child->GetChild("bS");
            if (bsProp)
            {
                bsValues.insert(bsProp->GetString(""));
            }
            ++count;
        }
        std::cout << "Total back pieces: " << count << std::endl;
        std::cout << "Unique bS values used:" << std::endl;
        for (const auto& bs : bsValues)
        {
            std::cout << "  - \"" << bs << "\"" << std::endl;
        }

        // Check if any of these exist in Map/Back or Map2/Back
        std::cout << "\nChecking if bS values exist as .img files:" << std::endl;
        for (const auto& bs : bsValues)
        {
            if (bs.empty()) continue;

            auto mapPath = resMan.GetProperty("Map/Back/" + bs + ".img");
            auto map2Path = resMan.GetProperty("Map2/Back/" + bs + ".img");

            std::cout << "  " << bs << ".img: ";
            if (mapPath && mapPath->GetChildCount() > 0)
            {
                std::cout << "Map/Back/" << bs << ".img (" << mapPath->GetChildCount() << " children)";
            }
            else if (map2Path && map2Path->GetChildCount() > 0)
            {
                std::cout << "Map2/Back/" << bs << ".img (" << map2Path->GetChildCount() << " children)";
            }
            else
            {
                std::cout << "NOT FOUND";
            }
            std::cout << std::endl;
        }
    }

    // Check UI/MapLogin.img numbered children (0-7) for embedded backgrounds
    std::cout << "\nChecking UI/MapLogin.img numbered children for canvases..." << std::endl;
    for (int i = 0; i <= 7; ++i)
    {
        auto numProp = resMan.GetProperty("UI/MapLogin.img/" + std::to_string(i));
        if (numProp)
        {
            std::cout << "  " << i << ": " << numProp->GetChildCount() << " children";
            auto canvas = numProp->GetCanvas();
            if (canvas)
            {
                std::cout << ", direct canvas " << canvas->GetWidth() << "x" << canvas->GetHeight();
            }
            std::cout << std::endl;

            // Check for canvases in children
            int canvasCount = 0;
            for (const auto& [name, child] : numProp->GetChildren())
            {
                auto childCanvas = child->GetCanvas();
                if (childCanvas)
                {
                    ++canvasCount;
                    if (canvasCount <= 3)
                    {
                        std::cout << "    - " << name << " [canvas " << childCanvas->GetWidth()
                                  << "x" << childCanvas->GetHeight() << "]" << std::endl;
                    }
                }
            }
            if (canvasCount > 3)
            {
                std::cout << "    ... and " << (canvasCount - 3) << " more canvases" << std::endl;
            }
        }
    }

    // Also check UI/Login.img/Title_new for directly usable content
    std::cout << "\nChecking UI/Login.img/Title_new children..." << std::endl;
    auto titleNewProp = resMan.GetProperty("UI/Login.img/Title_new");
    if (titleNewProp)
    {
        for (const auto& [name, child] : titleNewProp->GetChildren())
        {
            std::cout << "  - " << name;
            auto canvas = child->GetCanvas();
            if (canvas)
            {
                std::cout << " [canvas " << canvas->GetWidth() << "x" << canvas->GetHeight() << "]";
            }
            else
            {
                std::cout << " (" << child->GetChildCount() << " children)";
            }
            std::cout << std::endl;
        }
    }

    // Check Map001.wz for login backgrounds
    std::cout << "\nChecking Map001.wz for login backgrounds..." << std::endl;
    auto map001Prop = resMan.GetProperty("Map001");
    if (map001Prop)
    {
        std::cout << "Map001/ top-level children:" << std::endl;
        int count = 0;
        for (const auto& [name, child] : map001Prop->GetChildren())
        {
            std::cout << "  - " << name << std::endl;
            if (++count >= 15) break;
        }

        // Check Map001/Back for login.img
        auto map001Back = resMan.GetProperty("Map001/Back");
        if (map001Back)
        {
            std::cout << "\nMap001/Back/ children:" << std::endl;
            for (const auto& [name, child] : map001Back->GetChildren())
            {
                std::cout << "  - " << name;
                if (name.find("login") != std::string::npos || name.find("Login") != std::string::npos)
                {
                    std::cout << " <-- LOGIN";
                }
                std::cout << std::endl;
            }

            // Try to load login.img from Map001/Back
            auto loginImg = resMan.GetProperty("Map001/Back/login.img");
            if (loginImg)
            {
                auto& children = loginImg->GetChildren();
                std::cout << "\nMap001/Back/login.img has " << children.size() << " children:" << std::endl;
                for (const auto& [name, child] : children)
                {
                    std::cout << "  - " << name << " (" << child->GetChildCount() << " sub-children)" << std::endl;
                }
            }
        }
        else
        {
            std::cout << "Map001/Back not found" << std::endl;
        }
    }
    else
    {
        std::cout << "Map001.wz not found or not loaded" << std::endl;
    }

    // Also check Map1.wz
    std::cout << "\nChecking Map1.wz for login backgrounds..." << std::endl;
    auto map1Prop = resMan.GetProperty("Map1");
    if (map1Prop)
    {
        std::cout << "Map1/ top-level children (first 10):" << std::endl;
        int count = 0;
        for (const auto& [name, child] : map1Prop->GetChildren())
        {
            std::cout << "  - " << name << std::endl;
            if (++count >= 10) break;
        }

        // Check Map1/Back - search for login-related entries
        auto map1Back = resMan.GetProperty("Map1/Back");
        if (map1Back)
        {
            std::cout << "\nSearching Map1/Back for login-related:" << std::endl;
            for (const auto& [name, child] : map1Back->GetChildren())
            {
                // Case-insensitive search for login/cristal/CF
                std::string lowerName = name;
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                if (lowerName.find("login") != std::string::npos ||
                    lowerName.find("cristal") != std::string::npos ||
                    lowerName.find("logincf") != std::string::npos)
                {
                    std::cout << "  FOUND: " << name << std::endl;
                }
            }

            // Try login.img from Map1/Back
            auto loginImg1 = resMan.GetProperty("Map1/Back/login.img");
            if (loginImg1 && loginImg1->GetChildCount() > 0)
            {
                auto& children = loginImg1->GetChildren();
                std::cout << "\nMap1/Back/login.img has " << children.size() << " children!" << std::endl;
            }
        }

        // Also check combined "Map/Back" path which may resolve through Base.wz
        std::cout << "\nSearching through all Map*/Back for login.img..." << std::endl;
        std::vector<std::string> mapPrefixes = {"Map", "Map1", "Map2"};
        for (const auto& prefix : mapPrefixes)
        {
            auto backProp = resMan.GetProperty(prefix + "/Back");
            if (backProp)
            {
                for (const auto& [name, child] : backProp->GetChildren())
                {
                    std::string lowerName = name;
                    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                    if (lowerName.find("login") != std::string::npos ||
                        lowerName.find("cristal") != std::string::npos)
                    {
                        std::cout << "  " << prefix << "/Back/" << name << std::endl;

                        // Check if it has actual content - MUST call GetChildren() to trigger lazy loading
                        auto prop = resMan.GetProperty(prefix + "/Back/" + name);
                        if (prop)
                        {
                            // Trigger lazy loading by calling GetChildren()
                            auto& propChildren = prop->GetChildren();
                            std::cout << "    -> " << propChildren.size() << " children" << std::endl;

                            // Show what's inside
                            int count = 0;
                            for (const auto& [pName, pChild] : propChildren)
                            {
                                std::cout << "       - " << pName;
                                auto& subChildren = pChild->GetChildren();
                                std::cout << " (" << subChildren.size() << " children)" << std::endl;

                                // Show sub-children for "back" section
                                if (pName == "back" && !subChildren.empty())
                                {
                                    int subCount = 0;
                                    for (const auto& [sName, sChild] : subChildren)
                                    {
                                        auto canvas = sChild->GetCanvas();
                                        if (canvas)
                                        {
                                            std::cout << "         - " << sName << " [canvas "
                                                      << canvas->GetWidth() << "x" << canvas->GetHeight() << "]" << std::endl;
                                        }
                                        if (++subCount >= 3) break;
                                    }
                                }
                                if (++count >= 5) break;
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        std::cout << "Map1.wz not found or not loaded" << std::endl;
    }
}

// Test for _inlink resolution
TEST(WzCanvasLinkTest, InlinkResolution)
{
    // Create a target canvas with actual data
    auto targetCanvas = std::make_shared<WzCanvas>(456, 285);
    std::vector<std::uint8_t> pixelData(456 * 285 * 4, 0xFF); // White pixels
    targetCanvas->SetPixelData(std::move(pixelData));

    // Create target property containing the canvas
    auto targetProp = std::make_shared<WzProperty>("24");
    targetProp->SetCanvas(targetCanvas);

    // Create WzImage (Logo.img equivalent)
    auto logoImg = std::make_shared<WzImage>("Logo.img");

    // Create Wizet property under Logo.img
    auto wizet = std::make_shared<WzProperty>("Wizet");
    wizet->AddChild(targetProp);

    // Create a property with _inlink
    auto linkedProp = std::make_shared<WzProperty>("25");

    // Add _inlink child
    auto inlinkChild = std::make_shared<WzProperty>("_inlink");
    inlinkChild->SetString("Wizet/24");
    linkedProp->AddChild(inlinkChild);

    // Add placeholder canvas (1x1) to linkedProp
    auto placeholderCanvas = std::make_shared<WzCanvas>(1, 1);
    linkedProp->SetCanvas(placeholderCanvas);

    wizet->AddChild(linkedProp);

    // Add Wizet to Logo.img
    logoImg->AddProperty(wizet);

    // Create a mock WzFile for path resolution
    auto wzFile = std::make_shared<WzFile>();

    // CRITICAL: Set WzFile pointer on the linked property so it can resolve the path
    linkedProp->SetWzFile(wzFile.get());

    std::cout << "\n=== Testing _inlink resolution ===" << std::endl;
    std::cout << "Target canvas (Wizet/24): " << targetCanvas->GetWidth() << "x" << targetCanvas->GetHeight() << std::endl;
    std::cout << "Linked property (Wizet/25) has _inlink child: "
              << (linkedProp->GetChild("_inlink") != nullptr) << std::endl;
    std::cout << "Linked property _inlink value: "
              << (linkedProp->GetChild("_inlink") ? linkedProp->GetChild("_inlink")->GetString() : "NULL") << std::endl;

    // Try to get canvas from linked property
    auto resolvedCanvas = linkedProp->GetCanvas();

    std::cout << "Resolved canvas: " << (resolvedCanvas != nullptr ? "NOT NULL" : "NULL") << std::endl;
    if (resolvedCanvas)
    {
        std::cout << "Resolved canvas size: " << resolvedCanvas->GetWidth() << "x" << resolvedCanvas->GetHeight() << std::endl;
    }

    // Verify that the resolved canvas is the target canvas (456x285), not the placeholder (1x1)
    ASSERT_NE(resolvedCanvas, nullptr) << "_inlink resolution returned nullptr";
    EXPECT_EQ(resolvedCanvas->GetWidth(), 456) << "_inlink resolution failed - got placeholder canvas";
    EXPECT_EQ(resolvedCanvas->GetHeight(), 285) << "_inlink resolution failed - got placeholder canvas";
}
