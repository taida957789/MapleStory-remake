#pragma once

#include "util/Rand32.h"

#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace ms
{

namespace detail
{

/// Shared counter for periodic TSecType heap reshuffling
inline uint32_t g_nShuffleCounter{0};

} // namespace detail

/**
 * @brief Heap-allocated secure data block
 *
 * Based on TSecData<T> from the original MapleStory client.
 * Stored on the heap and periodically reallocated to prevent memory scanners
 * from tracking a fixed address.
 *
 * @tparam T The integral value type
 */
template <typename T>
struct TSecData
{
    static_assert(std::is_integral_v<T>, "TSecData requires an integral type");

    T data{};
    T bKey{};
    uint8_t nFakePtr1{};
    uint8_t nFakePtr2{};
    uint16_t wChecksum{};
};

/**
 * @brief Heap-based anti-tampering secure variable with integrity checks
 *
 * Based on TSecType<T> from the original MapleStory client.
 * More aggressive than ZtlSecureTear — stores data on the heap and periodically
 * reallocates to a new address (every 111 writes / 55 reads) to defeat memory
 * scanners. Validates XOR key, checksum, and fake-pointer integrity on every read.
 *
 * Storage layout (TSecData on heap):
 *   data      = value ^ bKey
 *   bKey      = random XOR key (0 → fallback to 42)
 *   FakePtr1/2 = low bytes of parent's address-derived tags
 *   wChecksum = 0xD328 | (uint8_t(data + 42 + bKey) + 4)
 *
 * @tparam T An integral type (unsigned char, short, int, etc.)
 */
template <typename T>
class TSecType
{
    static_assert(std::is_integral_v<T>, "TSecType requires an integral type");

    static constexpr uint16_t kChecksumInit = 0x9A65u;  // -26011 as uint16
    static constexpr uint16_t kChecksumBase =
        static_cast<uint16_t>(kChecksumInit * 8u);      // 0xD328
    static constexpr uint16_t kChecksumCarry =
        static_cast<uint16_t>(kChecksumInit >> 13u);     // 4
    static constexpr T kFallbackKey = static_cast<T>(42);
    static constexpr uint32_t kSetShuffleInterval = 111u;
    static constexpr uint32_t kGetShuffleInterval = 55u;

    uint32_t m_nFakePtr1{};
    uint32_t m_nFakePtr2{};
    mutable TSecData<T>* m_pSecData{nullptr};

    /// Compute the expected checksum for the current TSecData contents
    [[nodiscard]] auto ComputeChecksum() const -> uint16_t
    {
        auto bKey = m_pSecData->bKey;
        if (!bKey)
            bKey = kFallbackKey;

        const auto mix = static_cast<uint8_t>(m_pSecData->data + 42 + bKey);
        return static_cast<uint16_t>(kChecksumBase | (mix + kChecksumCarry));
    }

    /// Internal SetData that works in const context (m_pSecData is mutable)
    void SetDataImpl(T data) const
    {
        ++detail::g_nShuffleCounter;
        if (detail::g_nShuffleCounter % kSetShuffleInterval == 0)
        {
            // Periodic heap reshuffling — reallocate to defeat memory scanners
            auto* pNew = new TSecData<T>(*m_pSecData);
            delete m_pSecData;
            m_pSecData = pNew;
        }

        auto& rand = detail::GetSecureRand();
        m_pSecData->bKey = static_cast<T>(rand.Random());

        auto bKey = m_pSecData->bKey;
        m_pSecData->wChecksum = kChecksumInit;

        if (!bKey)
            bKey = kFallbackKey;

        m_pSecData->data = static_cast<T>(data ^ bKey);
        m_pSecData->wChecksum = ComputeChecksum();
    }

public:
    TSecType()
    {
        m_pSecData = new TSecData<T>();

        auto& rand = detail::GetSecureRand();
        const auto addr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this));
        m_nFakePtr1 = addr + static_cast<uint32_t>(rand.Random());
        m_nFakePtr2 = addr + static_cast<uint32_t>(rand.Random());

        m_pSecData->nFakePtr1 = static_cast<uint8_t>(m_nFakePtr1);
        m_pSecData->nFakePtr2 = static_cast<uint8_t>(m_nFakePtr2);

        SetDataImpl(T{});
    }

    explicit TSecType(T value)
    {
        m_pSecData = new TSecData<T>();

        auto& rand = detail::GetSecureRand();
        const auto addr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this));
        m_nFakePtr1 = addr + static_cast<uint32_t>(rand.Random());
        m_nFakePtr2 = addr + static_cast<uint32_t>(rand.Random());

        m_pSecData->nFakePtr1 = static_cast<uint8_t>(m_nFakePtr1);
        m_pSecData->nFakePtr2 = static_cast<uint8_t>(m_nFakePtr2);

        SetDataImpl(value);
    }

    ~TSecType() { delete m_pSecData; }

    TSecType(const TSecType& other) : TSecType(other.GetData()) {}

    auto operator=(const TSecType& other) -> TSecType&
    {
        if (this != &other)
            SetData(other.GetData());
        return *this;
    }

    TSecType(TSecType&& other) noexcept
        : m_nFakePtr1(other.m_nFakePtr1)
        , m_nFakePtr2(other.m_nFakePtr2)
        , m_pSecData(other.m_pSecData)
    {
        other.m_pSecData = nullptr;
    }

    auto operator=(TSecType&& other) noexcept -> TSecType&
    {
        if (this != &other)
        {
            delete m_pSecData;
            m_nFakePtr1 = other.m_nFakePtr1;
            m_nFakePtr2 = other.m_nFakePtr2;
            m_pSecData = other.m_pSecData;
            other.m_pSecData = nullptr;
        }
        return *this;
    }

    void SetData(T data) { SetDataImpl(data); }

    [[nodiscard]] auto GetData() const -> T
    {
        auto bKey = m_pSecData->bKey;
        if (!bKey)
            bKey = kFallbackKey;

        const auto value = static_cast<T>(bKey ^ m_pSecData->data);

        const auto expectedChecksum = ComputeChecksum();

        if (expectedChecksum != m_pSecData->wChecksum
            || static_cast<uint8_t>(m_nFakePtr1) != m_pSecData->nFakePtr1
            || static_cast<uint8_t>(m_nFakePtr2) != m_pSecData->nFakePtr2)
        {
            throw std::runtime_error(
                "TSecType: integrity check failed (memory tampering detected)");
        }

        ++detail::g_nShuffleCounter;
        if (detail::g_nShuffleCounter % kGetShuffleInterval == 0)
        {
            // Periodic re-encryption with fresh key
            SetDataImpl(value);
        }

        return value;
    }

    auto operator=(T value) -> TSecType&
    {
        SetData(value);
        return *this;
    }

    [[nodiscard]] operator T() const { return GetData(); } // NOLINT(google-explicit-constructor)
};

} // namespace ms
