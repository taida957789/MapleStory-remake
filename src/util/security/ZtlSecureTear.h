#pragma once

#include "util/Rand32.h"

#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace ms
{

namespace detail
{

[[nodiscard]] constexpr auto Ror32(uint32_t value, unsigned int shift) noexcept -> uint32_t
{
    return (value >> shift) | (value << (32u - shift));
}

[[nodiscard]] constexpr auto Rol32(uint32_t value, unsigned int shift) noexcept -> uint32_t
{
    return (value << shift) | (value >> (32u - shift));
}

} // namespace detail

/**
 * @brief Anti-tampering secure variable storage
 *
 * Based on _ZtlSecureTear<T> from the original MapleStory client.
 * Stores values XOR'd with a random key and validates via checksum on read.
 * Detects memory tampering (e.g. from cheat tools) at runtime.
 *
 * Each 32-bit word of the value is stored as:
 *   tear[0] = random key
 *   tear[1] = ROR(value ^ key, 5)
 *   checksum = tear[1] + ROR(key ^ 0xBAADF00D, 5)
 *
 * @tparam T The value type to protect (int, short, double, etc.)
 */
template <typename T>
class ZtlSecureTear
{
    static constexpr uint32_t kMagic = 0xBAADF00Du;
    static constexpr unsigned int kRotation = 5u;
    static constexpr std::size_t kWordCount =
        (sizeof(T) + sizeof(uint32_t) - 1) / sizeof(uint32_t);

    struct SecureWord
    {
        int32_t nRandom{};
        int32_t nEncrypted{};
        int32_t nChecksum{};
    };

    SecureWord m_aWord[kWordCount]{};

public:
    ZtlSecureTear() = default;

    explicit ZtlSecureTear(T value) { Put(value); }

    void Put(T value)
    {
        uint32_t raw[kWordCount]{};
        std::memcpy(raw, &value, sizeof(T));

        auto& rand = detail::get_rand();

        for (std::size_t i = 0; i < kWordCount; ++i)
        {
            const auto r = static_cast<uint32_t>(rand.Random());
            const auto encrypted = detail::Ror32(raw[i] ^ r, kRotation);

            m_aWord[i].nRandom = static_cast<int32_t>(r);
            m_aWord[i].nEncrypted = static_cast<int32_t>(encrypted);
            m_aWord[i].nChecksum = static_cast<int32_t>(
                encrypted + detail::Ror32(r ^ kMagic, kRotation));
        }
    }

    [[nodiscard]] auto Get() const -> T
    {
        uint32_t raw[kWordCount]{};

        for (std::size_t i = 0; i < kWordCount; ++i)
        {
            const auto r = static_cast<uint32_t>(m_aWord[i].nRandom);
            const auto encrypted = static_cast<uint32_t>(m_aWord[i].nEncrypted);
            const auto checksum = static_cast<uint32_t>(m_aWord[i].nChecksum);

            raw[i] = r ^ detail::Rol32(encrypted, kRotation);

            if (encrypted + detail::Ror32(r ^ kMagic, kRotation) != checksum)
            {
                throw std::runtime_error(
                    "ZtlSecureTear: checksum mismatch (memory tampering detected)");
            }
        }

        T value{};
        std::memcpy(&value, raw, sizeof(T));
        return value;
    }

    auto operator=(T value) -> ZtlSecureTear&
    {
        Put(value);
        return *this;
    }

    [[nodiscard]] operator T() const { return Get(); } // NOLINT(google-explicit-constructor)
};

} // namespace ms

// ---------------------------------------------------------------------------
// Legacy macros matching original _ZtlSecureTear_ naming convention
//
// Usage inside a class body:
//   ZTL_SECURE_MEMBER(int, zElementalCharge)
//
// Generates:
//   ZtlSecureTear<int>  _ZtlSecureTear_zElementalCharge_;
//   auto _ZtlSecurePut_zElementalCharge_(int t) -> int;
//   auto _ZtlSecureGet_zElementalCharge_() const -> int;
// ---------------------------------------------------------------------------
#define ZTL_SECURE_MEMBER(type, name)                                       \
    ::ms::ZtlSecureTear<type> _ZtlSecureTear_##name##_;                     \
    auto _ZtlSecurePut_##name##_(type t) -> type                            \
    {                                                                       \
        _ZtlSecureTear_##name##_.Put(t);                                    \
        return t;                                                           \
    }                                                                       \
    [[nodiscard]] auto _ZtlSecureGet_##name##_() const -> type              \
    {                                                                       \
        return _ZtlSecureTear_##name##_.Get();                              \
    }
