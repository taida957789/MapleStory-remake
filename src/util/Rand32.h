#pragma once

#include <cstdint>
#include <mutex>

namespace ms
{

/**
 * @brief Combined Tausworthe (taus88) pseudo-random number generator
 *
 * Based on CRand32 from the original MapleStory client.
 * Uses a 3-component combined Tausworthe generator with thread-safe state.
 *
 * Reference: P. L'Ecuyer, "Maximally Equidistributed Combined Tausworthe Generators", 1996.
 */
class Rand32
{
public:
    Rand32();
    explicit Rand32(uint32_t uSeed);

    void Seed(uint32_t s1, uint32_t s2, uint32_t s3);
    void SetSeed(uint32_t s1, uint32_t s2, uint32_t s3);

    [[nodiscard]] auto Random() -> int32_t;
    [[nodiscard]] auto RandomFloat() -> float;
    [[nodiscard]] auto GetPastRand() -> uint32_t;
    void RollBack();

    [[nodiscard]] static auto CrtRand(uint32_t* uSeed) -> uint32_t;

private:
    uint32_t m_s1{};
    uint32_t m_s2{};
    uint32_t m_s3{};
    uint32_t m_past_s1{};
    uint32_t m_past_s2{};
    uint32_t m_past_s3{};
    std::mutex m_lock;
};

namespace detail
{

/// Global Rand32 instance used by security wrappers (equivalent to g_rand)
inline auto GetSecureRand() -> Rand32&
{
    static Rand32 instance;
    return instance;
}

} // namespace detail

} // namespace ms
