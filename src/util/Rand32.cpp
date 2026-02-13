#include "Rand32.h"

#include <chrono>
#include <cstring>

namespace ms
{

Rand32::Rand32()
{
    // Original uses ZAPI.timeGetTime() as initial seed
    auto seed = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());

    // Apply MINSTD LCG three times (multiplier=214013, increment=2531011)
    seed = 214013u * seed + 2531011u;
    seed = 214013u * seed + 2531011u;
    seed = 214013u * seed + 2531011u;

    Seed(seed, seed, seed);
}

Rand32::Rand32(uint32_t uSeed)
{
    auto seed = uSeed;
    seed = 214013u * seed + 2531011u;
    seed = 214013u * seed + 2531011u;
    seed = 214013u * seed + 2531011u;

    Seed(seed, seed, seed);
}

void Rand32::Seed(uint32_t s1, uint32_t s2, uint32_t s3)
{
    std::lock_guard lock(m_lock);

    // Ensure minimum bit requirements for each component
    m_s1 = s1 | 0x100000u;
    m_s2 = s2 | 0x1000u;
    m_s3 = s3 | 0x10u;
    m_past_s1 = m_s1;
    m_past_s2 = m_s2;
    m_past_s3 = m_s3;
}

void Rand32::SetSeed(uint32_t s1, uint32_t s2, uint32_t s3)
{
    std::lock_guard lock(m_lock);

    m_s1 = s1;
    m_s2 = s2;
    m_s3 = s3;
    m_past_s1 = s1;
    m_past_s2 = s2;
    m_past_s3 = s3;
}

auto Rand32::Random() -> int32_t
{
    std::lock_guard lock(m_lock);

    // Save current state for rollback / GetPastRand
    m_past_s1 = m_s1;
    m_past_s2 = m_s2;
    m_past_s3 = m_s3;

    // Tausworthe combined generator (taus88)
    // Component 1: (q=13, s=19, mask=0xFFFFFFFE, p=12)
    uint32_t b = ((m_s1 << 13) ^ m_s1) >> 19;
    m_s1 = ((m_s1 & 0xFFFFFFFEu) << 12) ^ b;

    // Component 2: (q=2, s=25, mask=0xFFFFFFF8, p=4)
    b = ((m_s2 << 2) ^ m_s2) >> 25;
    m_s2 = ((m_s2 & 0xFFFFFFF8u) << 4) ^ b;

    // Component 3: (q=3, s=11, mask=0xFFFFFFF0, p=17)
    b = ((m_s3 << 3) ^ m_s3) >> 11;
    m_s3 = ((m_s3 & 0xFFFFFFF0u) << 17) ^ b;

    return static_cast<int32_t>(m_s1 ^ m_s2 ^ m_s3);
}

auto Rand32::RandomFloat() -> float
{
    // Map random bits to IEEE 754 float in [1.0, 2.0), then subtract 1.0
    const auto bits = (static_cast<uint32_t>(Random()) & 0x7FFFFFu) | 0x3F800000u;
    float f{};
    std::memcpy(&f, &bits, sizeof(f));
    return f - 1.0f;
}

auto Rand32::GetPastRand() -> uint32_t
{
    std::lock_guard lock(m_lock);

    // Compute taus88 step on past state (without modifying it)
    uint32_t b = ((m_past_s1 << 13) ^ m_past_s1) >> 19;
    const auto s1 = ((m_past_s1 & 0xFFFFFFFEu) << 12) ^ b;

    b = ((m_past_s2 << 2) ^ m_past_s2) >> 25;
    const auto s2 = ((m_past_s2 & 0xFFFFFFF8u) << 4) ^ b;

    b = ((m_past_s3 << 3) ^ m_past_s3) >> 11;
    const auto s3 = ((m_past_s3 & 0xFFFFFFF0u) << 17) ^ b;

    return s1 ^ s2 ^ s3;
}

void Rand32::RollBack()
{
    std::lock_guard lock(m_lock);

    m_s1 = m_past_s1;
    m_s2 = m_past_s2;
    m_s3 = m_past_s3;
}

auto Rand32::CrtRand(uint32_t* uSeed) -> uint32_t
{
    const auto result = *uSeed;
    *uSeed = 214013u * (*uSeed) + 2531011u;
    return result;
}

} // namespace ms
