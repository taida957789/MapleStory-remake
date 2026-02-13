#pragma once

#include <cstdint>

namespace ms
{

/**
 * @brief Cross-platform FILETIME equivalent
 *
 * Windows FILETIME: 64-bit value representing 100-nanosecond intervals
 * since January 1, 1601.
 *
 * Epoch difference: 1601-01-01 to 1970-01-01 = 11644473600 seconds
 *                   = 116444736000000000 in 100-ns ticks.
 */
struct FileTime
{
    /// 100-ns ticks between Windows epoch (1601) and Unix epoch (1970)
    static constexpr std::uint64_t kEpochDiff = 116444736000000000ULL;

    /// 100-ns ticks per second
    static constexpr std::uint64_t kTicksPerSecond = 10000000ULL;

    /// 100-ns ticks per millisecond
    static constexpr std::uint64_t kTicksPerMillisecond = 10000ULL;

    std::uint32_t dwLowDateTime{};
    std::uint32_t dwHighDateTime{};

    // === Construction ===

    /// Construct from raw low/high parts
    static constexpr auto FromUint64(std::uint64_t val) noexcept -> FileTime
    {
        return {static_cast<std::uint32_t>(val), static_cast<std::uint32_t>(val >> 32)};
    }

    /// Construct from Unix timestamp (seconds since 1970-01-01)
    static constexpr auto FromUnixTime(std::int64_t unixTime) noexcept -> FileTime
    {
        auto ticks = static_cast<std::uint64_t>(unixTime) * kTicksPerSecond + kEpochDiff;
        return FromUint64(ticks);
    }

    /// Construct from Unix timestamp with millisecond precision
    static constexpr auto FromUnixTimeMs(std::int64_t unixTimeMs) noexcept -> FileTime
    {
        auto ticks = static_cast<std::uint64_t>(unixTimeMs) * kTicksPerMillisecond + kEpochDiff;
        return FromUint64(ticks);
    }

    // === Conversion ===

    [[nodiscard]] constexpr auto ToUint64() const noexcept -> std::uint64_t
    {
        return (static_cast<std::uint64_t>(dwHighDateTime) << 32) | dwLowDateTime;
    }

    /// Convert to Unix timestamp (seconds since 1970-01-01).
    /// Returns 0 if the FileTime is before Unix epoch.
    [[nodiscard]] constexpr auto ToUnixTime() const noexcept -> std::int64_t
    {
        auto ticks = ToUint64();
        if (ticks < kEpochDiff)
            return 0;
        return static_cast<std::int64_t>((ticks - kEpochDiff) / kTicksPerSecond);
    }

    /// Convert to Unix timestamp with millisecond precision.
    [[nodiscard]] constexpr auto ToUnixTimeMs() const noexcept -> std::int64_t
    {
        auto ticks = ToUint64();
        if (ticks < kEpochDiff)
            return 0;
        return static_cast<std::int64_t>((ticks - kEpochDiff) / kTicksPerMillisecond);
    }

    /// Convert to SystemTime (broken-down date/time components).
    [[nodiscard]] auto ToSystemTime() const noexcept -> struct SystemTime;

    // === Arithmetic ===

    /// Add seconds to this FileTime
    [[nodiscard]] constexpr auto AddSeconds(std::int64_t seconds) const noexcept -> FileTime
    {
        return FromUint64(ToUint64() + static_cast<std::uint64_t>(seconds) * kTicksPerSecond);
    }

    /// Add milliseconds to this FileTime
    [[nodiscard]] constexpr auto AddMilliseconds(std::int64_t ms) const noexcept -> FileTime
    {
        return FromUint64(ToUint64() + static_cast<std::uint64_t>(ms) * kTicksPerMillisecond);
    }

    /// Difference in seconds (this - rhs). Can be negative.
    [[nodiscard]] constexpr auto DiffSeconds(const FileTime& rhs) const noexcept -> std::int64_t
    {
        return static_cast<std::int64_t>(ToUint64() - rhs.ToUint64())
               / static_cast<std::int64_t>(kTicksPerSecond);
    }

    /// Difference in milliseconds (this - rhs). Can be negative.
    [[nodiscard]] constexpr auto DiffMilliseconds(const FileTime& rhs) const noexcept -> std::int64_t
    {
        return static_cast<std::int64_t>(ToUint64() - rhs.ToUint64())
               / static_cast<std::int64_t>(kTicksPerMillisecond);
    }

    // === Comparison (matches CompareFileTime semantics) ===

    [[nodiscard]] constexpr auto operator<(const FileTime& rhs) const noexcept -> bool
    {
        return ToUint64() < rhs.ToUint64();
    }

    [[nodiscard]] constexpr auto operator>(const FileTime& rhs) const noexcept -> bool
    {
        return rhs < *this;
    }

    [[nodiscard]] constexpr auto operator<=(const FileTime& rhs) const noexcept -> bool
    {
        return !(rhs < *this);
    }

    [[nodiscard]] constexpr auto operator>=(const FileTime& rhs) const noexcept -> bool
    {
        return !(*this < rhs);
    }

    [[nodiscard]] constexpr auto operator==(const FileTime& rhs) const noexcept -> bool
    {
        return ToUint64() == rhs.ToUint64();
    }

    [[nodiscard]] constexpr auto operator!=(const FileTime& rhs) const noexcept -> bool
    {
        return !(*this == rhs);
    }

    /// CompareFileTime-compatible: returns -1, 0, or 1
    [[nodiscard]] constexpr auto Compare(const FileTime& rhs) const noexcept -> std::int32_t
    {
        auto a = ToUint64();
        auto b = rhs.ToUint64();
        if (a < b) return -1;
        if (a > b) return 1;
        return 0;
    }

    // === Queries ===

    [[nodiscard]] constexpr auto IsZero() const noexcept -> bool
    {
        return dwLowDateTime == 0 && dwHighDateTime == 0;
    }
};

/**
 * @brief Cross-platform SYSTEMTIME equivalent
 *
 * Windows SYSTEMTIME: date/time broken down into components.
 */
struct SystemTime
{
    std::uint16_t wYear{};
    std::uint16_t wMonth{};
    std::uint16_t wDayOfWeek{};
    std::uint16_t wDay{};
    std::uint16_t wHour{};
    std::uint16_t wMinute{};
    std::uint16_t wSecond{};
    std::uint16_t wMilliseconds{};
};

// === FileTime::ToSystemTime implementation (needs SystemTime defined) ===

inline auto FileTime::ToSystemTime() const noexcept -> SystemTime
{
    auto unixMs = ToUnixTimeMs();
    auto unixSec = unixMs / 1000;
    auto remainMs = static_cast<std::uint16_t>(unixMs % 1000);

    // Days since Unix epoch
    auto totalDays = static_cast<std::int64_t>(unixSec / 86400);
    auto daySeconds = static_cast<std::int32_t>(unixSec % 86400);
    if (daySeconds < 0)
    {
        daySeconds += 86400;
        --totalDays;
    }

    // Day of week: 1970-01-01 was Thursday (4)
    auto wday = static_cast<std::uint16_t>((totalDays + 4) % 7);

    // Civil date from days since epoch (Euclidean affine algorithm)
    // Shift epoch to 0000-03-01 for cleaner leap year handling
    auto z = totalDays + 719468;
    auto era = (z >= 0 ? z : z - 146096) / 146097;
    auto doe = z - era * 146097;
    auto yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    auto y = yoe + era * 400;
    auto doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    auto mp = (5 * doy + 2) / 153;
    auto d = doy - (153 * mp + 2) / 5 + 1;
    auto m = mp < 10 ? mp + 3 : mp - 9;
    if (m <= 2)
        ++y;

    SystemTime st{};
    st.wYear = static_cast<std::uint16_t>(y);
    st.wMonth = static_cast<std::uint16_t>(m);
    st.wDay = static_cast<std::uint16_t>(d);
    st.wDayOfWeek = wday;
    st.wHour = static_cast<std::uint16_t>(daySeconds / 3600);
    st.wMinute = static_cast<std::uint16_t>((daySeconds % 3600) / 60);
    st.wSecond = static_cast<std::uint16_t>(daySeconds % 60);
    st.wMilliseconds = remainMs;
    return st;
}

// =============================================================================
// Well-known date constants (from original MapleStory client .rdata)
// =============================================================================

/// DB_DATE_19000101 — "permanent" / default timestamp (1900-01-01 00:00:00 UTC)
inline constexpr FileTime kDbDate19000101{0xFDE04000u, 0x014F373Bu};

/// DB_DATE_20790101 — "no expiry" sentinel (2079-01-01 00:00:00 UTC)
inline constexpr FileTime kDbDate20790101{0xBB058000u, 0x0217E646u};

} // namespace ms
