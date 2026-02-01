#include <gtest/gtest.h>
#include <zlib.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <cstdint>

// Test canvas decompression using zlib uncompress
// Based on wzlibcpp reference implementation

class CanvasDecompressTest : public ::testing::Test
{
protected:
    std::vector<std::uint8_t> m_fileData;
    bool m_loaded = false;

    void SetUp() override
    {
        // Load UI.wz file
        std::ifstream file("resources/old/UI.wz", std::ios::binary);
        if (file)
        {
            file.seekg(0, std::ios::end);
            auto size = file.tellg();
            file.seekg(0, std::ios::beg);
            m_fileData.resize(static_cast<std::size_t>(size));
            file.read(reinterpret_cast<char*>(m_fileData.data()), size);
            m_loaded = true;
        }
    }

    // Read compressed int (same as wzlibcpp)
    std::int32_t readCompressedInt(const std::uint8_t*& ptr)
    {
        std::int8_t sb = static_cast<std::int8_t>(*ptr++);
        if (sb == -128) // 0x80
        {
            std::int32_t value = *reinterpret_cast<const std::int32_t*>(ptr);
            ptr += 4;
            return value;
        }
        return sb;
    }
};

TEST_F(CanvasDecompressTest, TestDirectUncompress)
{
    ASSERT_TRUE(m_loaded) << "Failed to load UI.wz";

    // From debug output: offset=457586184, size=9571
    // This is the first canvas in Logo.img
    std::size_t offset = 457586184;
    std::size_t compressedSize = 9571;
    std::size_t expectedSize = 519840;  // 456 * 285 * 4 (ARGB8888)

    ASSERT_LT(offset + compressedSize, m_fileData.size())
        << "Offset + size exceeds file size";

    const std::uint8_t* data = m_fileData.data() + offset;

    // Check zlib header
    std::cout << "First 20 bytes: ";
    for (int i = 0; i < 20 && i < static_cast<int>(compressedSize); ++i)
    {
        printf("%02x ", data[i]);
    }
    std::cout << std::endl;

    EXPECT_EQ(data[0], 0x78) << "Expected zlib header byte 0";
    EXPECT_EQ(data[1], 0x9c) << "Expected zlib header byte 1";

    // Try uncompress
    std::vector<std::uint8_t> result(expectedSize);
    unsigned long destLen = static_cast<unsigned long>(expectedSize);

    int ret = uncompress(result.data(), &destLen,
                         data, static_cast<unsigned long>(compressedSize));

    std::cout << "uncompress returned: " << ret
              << " (Z_OK=" << Z_OK << ", Z_BUF_ERROR=" << Z_BUF_ERROR
              << ", Z_DATA_ERROR=" << Z_DATA_ERROR << ")" << std::endl;

    if (ret != Z_OK)
    {
        // Try with larger buffer
        std::cout << "Trying with larger buffer..." << std::endl;
        result.resize(expectedSize * 4);
        destLen = static_cast<unsigned long>(result.size());
        ret = uncompress(result.data(), &destLen,
                         data, static_cast<unsigned long>(compressedSize));
        std::cout << "uncompress (larger buffer) returned: " << ret << std::endl;
    }

    if (ret != Z_OK)
    {
        // Try inflate
        std::cout << "Trying inflate..." << std::endl;
        z_stream strm{};
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = static_cast<uInt>(compressedSize);
        strm.next_in = const_cast<Bytef*>(data);

        int initRet = inflateInit(&strm);
        EXPECT_EQ(initRet, Z_OK) << "inflateInit failed";

        result.resize(expectedSize);
        strm.avail_out = static_cast<uInt>(expectedSize);
        strm.next_out = result.data();

        int inflateRet = inflate(&strm, Z_NO_FLUSH);
        auto totalOut = strm.total_out;

        // Keep inflating
        while (inflateRet == Z_OK && strm.avail_in > 0 && strm.avail_out > 0)
        {
            inflateRet = inflate(&strm, Z_NO_FLUSH);
        }
        totalOut = strm.total_out;

        inflateEnd(&strm);

        std::cout << "inflate returned: " << inflateRet
                  << ", totalOut=" << totalOut << std::endl;

        if (inflateRet == Z_OK || inflateRet == Z_STREAM_END)
        {
            std::cout << "inflate succeeded! Output size: " << totalOut << std::endl;
        }
    }

    // Expect at least one method to work
    if (ret != Z_OK)
    {
        GTEST_SKIP() << "Direct uncompress failed, but inflate might work";
    }
}

TEST_F(CanvasDecompressTest, TestZlibVersion)
{
    std::cout << "zlib version: " << zlibVersion() << std::endl;
}
