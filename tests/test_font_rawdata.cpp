/**
 * Tests for WzResMan::LoadFontData using Etc/Font.img/NotoSansTMS-Medium
 */

#include <gtest/gtest.h>
#include <filesystem>

#include "wz/WzResMan.h"
#include "wz/WzProperty.h"
#include "wz/WzRaw.h"

using namespace ms;

static const std::string WZ_TEST_PATH = "../resources/Data";

class WzFontDataTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bHasTestFiles = std::filesystem::exists(WZ_TEST_PATH + "/Etc");

        if (m_bHasTestFiles)
        {
            auto& resMan = WzResMan::GetInstance();
            resMan.SetBasePath(WZ_TEST_PATH);
            resMan.Initialize();
        }
    }

    bool m_bHasTestFiles = false;
};

TEST_F(WzFontDataTest, LoadFontProperty)
{
    if (!m_bHasTestFiles)
        GTEST_SKIP() << "Test WZ files not found at " << WZ_TEST_PATH;

    auto& resMan = WzResMan::GetInstance();
    auto prop = resMan.GetProperty("Etc/Font.img/NotoSansTMS-Medium");

    ASSERT_NE(prop, nullptr) << "Etc/Font.img/NotoSansTMS-Medium not found";

    std::cout << "NotoSansTMS-Medium children:" << std::endl;
    for (const auto& [name, child] : prop->GetChildren())
    {
        std::cout << "  - " << name
                  << " (nodeType=" << static_cast<int>(child->GetNodeType()) << ")"
                  << std::endl;
    }
}

TEST_F(WzFontDataTest, LoadFontDataNonEmpty)
{
    if (!m_bHasTestFiles)
        GTEST_SKIP() << "Test WZ files not found at " << WZ_TEST_PATH;

    auto& resMan = WzResMan::GetInstance();
    auto prop = resMan.GetProperty("Etc/Font.img/NotoSansTMS-Medium");
    ASSERT_NE(prop, nullptr) << "Etc/Font.img/NotoSansTMS-Medium not found";

    auto data = resMan.LoadFontData(prop);
    ASSERT_FALSE(data.empty()) << "LoadFontData returned empty data";

    std::cout << "Font data size: " << data.size() << " bytes" << std::endl;
}

TEST_F(WzFontDataTest, LoadFontDataReasonableSize)
{
    if (!m_bHasTestFiles)
        GTEST_SKIP() << "Test WZ files not found at " << WZ_TEST_PATH;

    auto& resMan = WzResMan::GetInstance();
    auto prop = resMan.GetProperty("Etc/Font.img/NotoSansTMS-Medium");
    ASSERT_NE(prop, nullptr);

    auto data = resMan.LoadFontData(prop);

    // atlasData is a raw font texture atlas, not a TTF/OTF file
    // NotoSansTMS-Medium atlas should be well over 100KB
    EXPECT_GT(data.size(), 100'000u) << "Font atlas data seems too small";
    EXPECT_LT(data.size(), 50'000'000u) << "Font atlas data seems too large";

    std::cout << "Font atlas data size: " << data.size() << " bytes" << std::endl;
    std::cout << "First 8 bytes:";
    for (std::size_t i = 0; i < std::min<std::size_t>(8, data.size()); ++i)
        std::cout << " " << std::hex << static_cast<int>(data[i]);
    std::cout << std::dec << std::endl;
}

TEST_F(WzFontDataTest, AtlasDataChildExists)
{
    if (!m_bHasTestFiles)
        GTEST_SKIP() << "Test WZ files not found at " << WZ_TEST_PATH;

    auto& resMan = WzResMan::GetInstance();
    auto prop = resMan.GetProperty("Etc/Font.img/NotoSansTMS-Medium");
    ASSERT_NE(prop, nullptr);

    auto atlasData = prop->GetChild("atlasData");
    ASSERT_NE(atlasData, nullptr) << "atlasData child not found";

    auto raw = atlasData->GetRaw();
    ASSERT_NE(raw, nullptr) << "atlasData does not contain WzRaw data";

    EXPECT_GT(raw->GetData().size(), 0u) << "WzRaw data is empty";
    std::cout << "atlasData raw type: " << raw->GetType() << std::endl;
    std::cout << "atlasData raw size: " << raw->GetData().size() << " bytes" << std::endl;
}

TEST_F(WzFontDataTest, LoadFontDataNullPropReturnsEmpty)
{
    auto& resMan = WzResMan::GetInstance();
    auto data = resMan.LoadFontData(nullptr);
    EXPECT_TRUE(data.empty());
}

TEST_F(WzFontDataTest, LoadFontDataWrongPropReturnsEmpty)
{
    auto& resMan = WzResMan::GetInstance();

    // Property without atlasData child should return empty
    auto fakeProp = std::make_shared<WzProperty>("fake");
    fakeProp->SetInt(42);

    auto data = resMan.LoadFontData(fakeProp);
    EXPECT_TRUE(data.empty());
}
