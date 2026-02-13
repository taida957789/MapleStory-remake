#include "user/AvatarData.h"
#include "util/Rand32.h"
#include "util/security/TSecType.h"
#include "util/security/ZtlSecureTear.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <limits>

namespace ms
{

// ---- Rand32 tests ----

TEST(Rand32, SeededDeterminism)
{
    Rand32 a(42);
    Rand32 b(42);

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(a.Random(), b.Random());
    }
}

TEST(Rand32, DifferentSeedsDiverge)
{
    Rand32 a(1);
    Rand32 b(2);

    // Very unlikely all 100 outputs match with different seeds
    bool allSame = true;
    for (int i = 0; i < 100; ++i)
    {
        if (a.Random() != b.Random())
        {
            allSame = false;
            break;
        }
    }
    EXPECT_FALSE(allSame);
}

TEST(Rand32, RollBack)
{
    Rand32 rng(123);

    // Generate a value
    const auto val1 = rng.Random();

    // Roll back and generate again — should get the same value
    rng.RollBack();
    const auto val2 = rng.Random();

    EXPECT_EQ(val1, val2);
}

TEST(Rand32, GetPastRand)
{
    Rand32 rng(456);

    // Generate a value, then verify GetPastRand reproduces it
    const auto val = rng.Random();
    const auto pastVal = rng.GetPastRand();

    EXPECT_EQ(static_cast<uint32_t>(val), pastVal);
}

TEST(Rand32, RandomFloatRange)
{
    Rand32 rng(789);

    for (int i = 0; i < 1000; ++i)
    {
        const auto f = rng.RandomFloat();
        EXPECT_GE(f, 0.0f);
        EXPECT_LT(f, 1.0f);
    }
}

TEST(Rand32, CrtRand)
{
    uint32_t seed = 12345;
    const auto original = seed;

    const auto result = Rand32::CrtRand(&seed);

    EXPECT_EQ(result, original);
    EXPECT_EQ(seed, 214013u * original + 2531011u);
}

TEST(Rand32, SeedEnforcesMinBits)
{
    Rand32 rng;
    rng.Seed(0, 0, 0);

    // After Seed(0,0,0), internal state should have minimum bits set.
    // Verify by checking Random produces non-zero output.
    EXPECT_NE(rng.Random(), 0);
}

// ---- ZtlSecureTear tests ----

TEST(ZtlSecureTear, PutGetInt)
{
    ZtlSecureTear<int> secure;

    secure.Put(42);
    EXPECT_EQ(secure.Get(), 42);

    secure.Put(-1);
    EXPECT_EQ(secure.Get(), -1);

    secure.Put(0);
    EXPECT_EQ(secure.Get(), 0);
}

TEST(ZtlSecureTear, PutGetIntMax)
{
    ZtlSecureTear<int> secure;

    secure.Put(std::numeric_limits<int>::max());
    EXPECT_EQ(secure.Get(), std::numeric_limits<int>::max());

    secure.Put(std::numeric_limits<int>::min());
    EXPECT_EQ(secure.Get(), std::numeric_limits<int>::min());
}

TEST(ZtlSecureTear, PutGetUint32)
{
    ZtlSecureTear<uint32_t> secure;

    secure.Put(0xDEADBEEFu);
    EXPECT_EQ(secure.Get(), 0xDEADBEEFu);

    secure.Put(0u);
    EXPECT_EQ(secure.Get(), 0u);

    secure.Put(0xFFFFFFFFu);
    EXPECT_EQ(secure.Get(), 0xFFFFFFFFu);
}

TEST(ZtlSecureTear, PutGetInt16)
{
    ZtlSecureTear<int16_t> secure;

    secure.Put(static_cast<int16_t>(12345));
    EXPECT_EQ(secure.Get(), static_cast<int16_t>(12345));

    secure.Put(static_cast<int16_t>(-32768));
    EXPECT_EQ(secure.Get(), static_cast<int16_t>(-32768));
}

TEST(ZtlSecureTear, PutGetUint16)
{
    ZtlSecureTear<uint16_t> secure;

    secure.Put(static_cast<uint16_t>(65535));
    EXPECT_EQ(secure.Get(), static_cast<uint16_t>(65535));

    secure.Put(static_cast<uint16_t>(0));
    EXPECT_EQ(secure.Get(), static_cast<uint16_t>(0));
}

TEST(ZtlSecureTear, PutGetUint8)
{
    ZtlSecureTear<uint8_t> secure;

    secure.Put(static_cast<uint8_t>(255));
    EXPECT_EQ(secure.Get(), static_cast<uint8_t>(255));

    secure.Put(static_cast<uint8_t>(0));
    EXPECT_EQ(secure.Get(), static_cast<uint8_t>(0));
}

TEST(ZtlSecureTear, PutGetDouble)
{
    ZtlSecureTear<double> secure;

    secure.Put(3.14159265358979);
    EXPECT_DOUBLE_EQ(secure.Get(), 3.14159265358979);

    secure.Put(-0.0);
    // -0.0 and 0.0 are equal in floating point comparison
    EXPECT_DOUBLE_EQ(secure.Get(), -0.0);

    secure.Put(std::numeric_limits<double>::infinity());
    EXPECT_EQ(secure.Get(), std::numeric_limits<double>::infinity());
}

TEST(ZtlSecureTear, PutGetFloat)
{
    ZtlSecureTear<float> secure;

    secure.Put(1.5f);
    EXPECT_FLOAT_EQ(secure.Get(), 1.5f);

    secure.Put(-999.999f);
    EXPECT_FLOAT_EQ(secure.Get(), -999.999f);
}

TEST(ZtlSecureTear, AssignmentOperator)
{
    ZtlSecureTear<int> secure;
    secure = 100;
    EXPECT_EQ(static_cast<int>(secure), 100);
}

TEST(ZtlSecureTear, ImplicitConversion)
{
    ZtlSecureTear<int> secure(77);

    int val = secure;
    EXPECT_EQ(val, 77);
}

TEST(ZtlSecureTear, ConstructorWithValue)
{
    ZtlSecureTear<int> secure(999);
    EXPECT_EQ(secure.Get(), 999);
}

TEST(ZtlSecureTear, RepeatedPutGet)
{
    ZtlSecureTear<int> secure;

    for (int i = -500; i <= 500; ++i)
    {
        secure.Put(i);
        EXPECT_EQ(secure.Get(), i);
    }
}

TEST(ZtlSecureTear, TamperDetection)
{
    ZtlSecureTear<int> secure;
    secure.Put(42);

    // Corrupt the internal storage by writing directly to memory
    auto* raw = reinterpret_cast<int32_t*>(&secure);
    raw[0] ^= 1; // flip a bit in the random key

    EXPECT_THROW(secure.Get(), std::runtime_error);
}

TEST(ZtlSecureTear, MultipleInstancesIndependent)
{
    ZtlSecureTear<int> a(10);
    ZtlSecureTear<int> b(20);

    EXPECT_EQ(a.Get(), 10);
    EXPECT_EQ(b.Get(), 20);

    a = 30;
    EXPECT_EQ(a.Get(), 30);
    EXPECT_EQ(b.Get(), 20);
}

// ---- Legacy macro tests ----

class LegacySecureTest
{
public:
    ZTL_SECURE_MEMBER(int, zTestValue)
    ZTL_SECURE_MEMBER(double, zTestDouble)
};

TEST(ZtlSecureTear, LegacyMacroPutGet)
{
    LegacySecureTest obj;

    obj._ZtlSecurePut_zTestValue_(42);
    EXPECT_EQ(obj._ZtlSecureGet_zTestValue_(), 42);

    obj._ZtlSecurePut_zTestDouble_(3.14);
    EXPECT_DOUBLE_EQ(obj._ZtlSecureGet_zTestDouble_(), 3.14);
}

TEST(ZtlSecureTear, LegacyMacroPutReturnsValue)
{
    LegacySecureTest obj;

    const auto result = obj._ZtlSecurePut_zTestValue_(123);
    EXPECT_EQ(result, 123);
}

// ---- TSecType tests ----

TEST(TSecType, SetGetUint8)
{
    TSecType<uint8_t> secure;

    secure.SetData(42);
    EXPECT_EQ(secure.GetData(), 42);

    secure.SetData(0);
    EXPECT_EQ(secure.GetData(), 0);

    secure.SetData(255);
    EXPECT_EQ(secure.GetData(), 255);
}

TEST(TSecType, SetGetInt)
{
    TSecType<int32_t> secure;

    secure.SetData(12345);
    EXPECT_EQ(secure.GetData(), 12345);

    secure.SetData(-1);
    EXPECT_EQ(secure.GetData(), -1);

    secure.SetData(0);
    EXPECT_EQ(secure.GetData(), 0);

    secure.SetData(std::numeric_limits<int32_t>::max());
    EXPECT_EQ(secure.GetData(), std::numeric_limits<int32_t>::max());

    secure.SetData(std::numeric_limits<int32_t>::min());
    EXPECT_EQ(secure.GetData(), std::numeric_limits<int32_t>::min());
}

TEST(TSecType, SetGetInt16)
{
    TSecType<int16_t> secure;

    secure.SetData(static_cast<int16_t>(12345));
    EXPECT_EQ(secure.GetData(), static_cast<int16_t>(12345));

    secure.SetData(static_cast<int16_t>(-32768));
    EXPECT_EQ(secure.GetData(), static_cast<int16_t>(-32768));
}

TEST(TSecType, SetGetUint16)
{
    TSecType<uint16_t> secure;

    secure.SetData(static_cast<uint16_t>(65535));
    EXPECT_EQ(secure.GetData(), static_cast<uint16_t>(65535));

    secure.SetData(static_cast<uint16_t>(0));
    EXPECT_EQ(secure.GetData(), static_cast<uint16_t>(0));
}

TEST(TSecType, SetGetUint32)
{
    TSecType<uint32_t> secure;

    secure.SetData(0xDEADBEEFu);
    EXPECT_EQ(secure.GetData(), 0xDEADBEEFu);
}

TEST(TSecType, ConstructorWithValue)
{
    TSecType<int32_t> secure(999);
    EXPECT_EQ(secure.GetData(), 999);
}

TEST(TSecType, DefaultConstructorInitializesToZero)
{
    TSecType<int32_t> secure;
    EXPECT_EQ(secure.GetData(), 0);
}

TEST(TSecType, AssignmentOperator)
{
    TSecType<int32_t> secure;
    secure = 100;
    EXPECT_EQ(static_cast<int32_t>(secure), 100);
}

TEST(TSecType, ImplicitConversion)
{
    TSecType<uint8_t> secure;
    secure.SetData(77);

    uint8_t val = secure;
    EXPECT_EQ(val, 77);
}

TEST(TSecType, RepeatedSetGet)
{
    TSecType<int32_t> secure;

    for (int32_t i = -500; i <= 500; ++i)
    {
        secure.SetData(i);
        EXPECT_EQ(secure.GetData(), i);
    }
}

TEST(TSecType, CopyConstructor)
{
    TSecType<int32_t> a(42);
    TSecType<int32_t> b(a);

    EXPECT_EQ(b.GetData(), 42);

    // Modifying copy doesn't affect original
    b = 100;
    EXPECT_EQ(a.GetData(), 42);
    EXPECT_EQ(b.GetData(), 100);
}

TEST(TSecType, CopyAssignment)
{
    TSecType<int32_t> a(42);
    TSecType<int32_t> b(0);

    b = a;
    EXPECT_EQ(b.GetData(), 42);

    b = 100;
    EXPECT_EQ(a.GetData(), 42);
}

TEST(TSecType, MoveConstructor)
{
    TSecType<int32_t> a(42);
    TSecType<int32_t> b(std::move(a));

    EXPECT_EQ(b.GetData(), 42);
}

TEST(TSecType, MultipleInstancesIndependent)
{
    TSecType<int32_t> a(10);
    TSecType<int32_t> b(20);

    EXPECT_EQ(a.GetData(), 10);
    EXPECT_EQ(b.GetData(), 20);

    a = 30;
    EXPECT_EQ(a.GetData(), 30);
    EXPECT_EQ(b.GetData(), 20);
}

TEST(TSecType, TamperDetectionData)
{
    TSecType<uint8_t> secure;
    secure.SetData(42);

    // Corrupt the heap-allocated data directly
    // TSecType layout: [m_nFakePtr1(4)] [m_nFakePtr2(4)] [m_pSecData(8 on 64-bit)]
    auto** ptrToSecData = reinterpret_cast<TSecData<uint8_t>**>(
        reinterpret_cast<char*>(&secure) + 2 * sizeof(uint32_t));
    (*ptrToSecData)->data ^= 1; // flip a bit in encrypted data

    EXPECT_THROW(secure.GetData(), std::runtime_error);
}

TEST(TSecType, TamperDetectionChecksum)
{
    TSecType<uint8_t> secure;
    secure.SetData(42);

    auto** ptrToSecData = reinterpret_cast<TSecData<uint8_t>**>(
        reinterpret_cast<char*>(&secure) + 2 * sizeof(uint32_t));
    (*ptrToSecData)->wChecksum ^= 1;

    EXPECT_THROW(secure.GetData(), std::runtime_error);
}

TEST(TSecType, TamperDetectionFakePtr)
{
    TSecType<uint8_t> secure;
    secure.SetData(42);

    auto** ptrToSecData = reinterpret_cast<TSecData<uint8_t>**>(
        reinterpret_cast<char*>(&secure) + 2 * sizeof(uint32_t));
    (*ptrToSecData)->nFakePtr1 ^= 1;

    EXPECT_THROW(secure.GetData(), std::runtime_error);
}

TEST(TSecType, ReshuffleOnRepeatedAccess)
{
    TSecType<int32_t> secure(42);

    // Exercise the reshuffle paths — value should remain consistent
    for (int i = 0; i < 300; ++i)
    {
        EXPECT_EQ(secure.GetData(), 42) << "Failed on read iteration " << i;
    }

    for (int i = 0; i < 300; ++i)
    {
        secure.SetData(static_cast<int32_t>(i));
        EXPECT_EQ(secure.GetData(), static_cast<int32_t>(i))
            << "Failed on write/read iteration " << i;
    }
}

} // namespace ms
