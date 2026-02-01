/**
 * Packet encoding/decoding tests using Google Test
 */

#include <gtest/gtest.h>
#include "network/InPacket.h"
#include "network/OutPacket.h"

using namespace ms;

class PacketTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PacketTest, OutPacketBasicEncode)
{
    OutPacket packet(0x0001);

    packet.Encode1(0x12);
    packet.Encode2(0x3456);
    packet.Encode4(0x789ABCDE);

    const auto& data = packet.GetData();

    // Verify opcode (little endian)
    EXPECT_EQ(data[0], 0x01);
    EXPECT_EQ(data[1], 0x00);

    // Verify Encode1
    EXPECT_EQ(data[2], 0x12);

    // Verify Encode2 (little endian)
    EXPECT_EQ(data[3], 0x56);
    EXPECT_EQ(data[4], 0x34);

    // Verify Encode4 (little endian)
    EXPECT_EQ(data[5], 0xDE);
    EXPECT_EQ(data[6], 0xBC);
    EXPECT_EQ(data[7], 0x9A);
    EXPECT_EQ(data[8], 0x78);
}

TEST_F(PacketTest, OutPacketEncodeString)
{
    OutPacket packet(0x0001);
    packet.EncodeStr("Hello");

    const auto& data = packet.GetData();

    // Skip opcode (2 bytes)
    // String length (little endian)
    EXPECT_EQ(data[2], 0x05);
    EXPECT_EQ(data[3], 0x00);

    // String content
    EXPECT_EQ(data[4], 'H');
    EXPECT_EQ(data[5], 'e');
    EXPECT_EQ(data[6], 'l');
    EXPECT_EQ(data[7], 'l');
    EXPECT_EQ(data[8], 'o');
}

TEST_F(PacketTest, OutPacketEncode8)
{
    OutPacket packet(0x0001);
    packet.Encode8(0x123456789ABCDEF0LL);

    const auto& data = packet.GetData();

    // Skip opcode, check 8 bytes (little endian)
    EXPECT_EQ(data[2], 0xF0);
    EXPECT_EQ(data[3], 0xDE);
    EXPECT_EQ(data[4], 0xBC);
    EXPECT_EQ(data[5], 0x9A);
    EXPECT_EQ(data[6], 0x78);
    EXPECT_EQ(data[7], 0x56);
    EXPECT_EQ(data[8], 0x34);
    EXPECT_EQ(data[9], 0x12);
}

TEST_F(PacketTest, InPacketBasicDecode)
{
    uint8_t data[] = {
        0x01, 0x00,                         // Header (opcode)
        0x12,                               // 1 byte
        0x56, 0x34,                         // 2 bytes
        0xDE, 0xBC, 0x9A, 0x78,             // 4 bytes
    };

    InPacket packet(data, sizeof(data));

    EXPECT_EQ(packet.GetHeader(), 0x0001);
    EXPECT_EQ(packet.Decode1(), 0x12);
    EXPECT_EQ(packet.Decode2(), 0x3456);
    EXPECT_EQ(packet.Decode4(), 0x789ABCDE);
    EXPECT_TRUE(packet.IsEnd());
}

TEST_F(PacketTest, InPacketDecodeString)
{
    uint8_t data[] = {
        0x01, 0x00,                         // Header
        0x05, 0x00, 'H', 'e', 'l', 'l', 'o' // String
    };

    InPacket packet(data, sizeof(data));

    EXPECT_EQ(packet.GetHeader(), 0x0001);
    EXPECT_EQ(packet.DecodeStr(), "Hello");
    EXPECT_TRUE(packet.IsEnd());
}

TEST_F(PacketTest, InPacketDecode8)
{
    uint8_t data[] = {
        0x01, 0x00,                                     // Header
        0xF0, 0xDE, 0xBC, 0x9A, 0x78, 0x56, 0x34, 0x12  // 8 bytes
    };

    InPacket packet(data, sizeof(data));

    EXPECT_EQ(packet.GetHeader(), 0x0001);
    EXPECT_EQ(packet.Decode8(), 0x123456789ABCDEF0LL);
    EXPECT_TRUE(packet.IsEnd());
}

TEST_F(PacketTest, PacketRoundTrip)
{
    // Create and encode
    OutPacket out(0x1234);
    out.Encode1(-128);
    out.Encode2(-32768);
    out.Encode4(-2147483648);
    out.Encode8(0x123456789ABCDEF0LL);
    out.EncodeStr("Test String");

    // Decode
    InPacket in(out.GetData());

    EXPECT_EQ(in.GetHeader(), 0x1234);
    EXPECT_EQ(in.Decode1(), -128);
    EXPECT_EQ(in.Decode2(), -32768);
    EXPECT_EQ(in.Decode4(), -2147483648);
    EXPECT_EQ(in.Decode8(), 0x123456789ABCDEF0LL);
    EXPECT_EQ(in.DecodeStr(), "Test String");
    EXPECT_TRUE(in.IsEnd());
}

TEST_F(PacketTest, OutPacketReset)
{
    OutPacket packet(0x0001);
    packet.Encode4(0x12345678);

    packet.Reset(0x1234);

    EXPECT_EQ(packet.GetOpcode(), 0x1234);
    EXPECT_EQ(packet.GetSize(), 2); // Only opcode
}

TEST_F(PacketTest, InPacketRemaining)
{
    uint8_t data[] = {0x01, 0x00, 0x11, 0x22, 0x33, 0x44};

    InPacket packet(data, sizeof(data));

    EXPECT_EQ(packet.GetRemaining(), 4); // After header
    packet.Decode1();
    EXPECT_EQ(packet.GetRemaining(), 3);
    packet.Decode2();
    EXPECT_EQ(packet.GetRemaining(), 1);
}

TEST_F(PacketTest, InPacketDecodeBuffer)
{
    uint8_t data[] = {0x01, 0x00, 0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t buffer[4] = {0};

    InPacket packet(data, sizeof(data));
    packet.DecodeBuffer(buffer, 4);

    EXPECT_EQ(buffer[0], 0xAA);
    EXPECT_EQ(buffer[1], 0xBB);
    EXPECT_EQ(buffer[2], 0xCC);
    EXPECT_EQ(buffer[3], 0xDD);
}

TEST_F(PacketTest, OutPacketEncodeBuffer)
{
    uint8_t buffer[] = {0xAA, 0xBB, 0xCC, 0xDD};

    OutPacket packet(0x0001);
    packet.EncodeBuffer(buffer, 4);

    const auto& data = packet.GetData();

    EXPECT_EQ(data[2], 0xAA);
    EXPECT_EQ(data[3], 0xBB);
    EXPECT_EQ(data[4], 0xCC);
    EXPECT_EQ(data[5], 0xDD);
}
