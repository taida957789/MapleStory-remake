#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace ms
{

/**
 * @brief Fixed-size bit flag container
 *
 * Based on CFlag<N> from the original MapleStory client.
 * Bits are stored MSB-first: bit 0 is the highest bit of m_data[0].
 * Scalar values (e.g. from setValue(unsigned int)) occupy m_data[Size-1].
 *
 * @tparam N Number of bits
 */
template <int N>
class CFlag
{
    static constexpr int Size = (N + 31) / 32;

public:
    std::array<std::uint32_t, Size> m_data{};

    CFlag() = default;

    explicit CFlag(std::uint32_t value)
    {
        m_data.fill(0);
        m_data[Size - 1] = value;
    }

    auto setBitNumber(unsigned int bit, unsigned int value) -> CFlag&
    {
        const auto mask = 1u << (31 - (bit & 0x1F));
        m_data[bit >> 5] |= mask;
        if (!value)
            m_data[bit >> 5] ^= mask;
        return *this;
    }

    [[nodiscard]] auto getBitNumber(unsigned int bit) const -> unsigned int
    {
        if (bit >= static_cast<unsigned int>(N))
            return 0;
        return (m_data[bit >> 5] >> (31 - (bit & 0x1F))) & 1;
    }

    auto setValue(std::uint32_t value) -> CFlag&
    {
        m_data.fill(0);
        m_data[Size - 1] = value;
        return *this;
    }

    auto setValue(const CFlag& value) -> CFlag&
    {
        m_data = value.m_data;
        return *this;
    }

    void clear()
    {
        m_data.fill(0);
    }

    [[nodiscard]] auto compareTo(const CFlag& other) const -> int
    {
        for (int i = 0; i < Size; ++i)
        {
            if (m_data[i] < other.m_data[i])
                return -1;
            if (m_data[i] > other.m_data[i])
                return 1;
        }
        return 0;
    }

    [[nodiscard]] auto compareTo(std::uint32_t value) const -> int
    {
        for (int i = 0; i < Size - 1; ++i)
        {
            if (m_data[i] != 0)
                return 1;
        }
        if (m_data[Size - 1] > value)
            return 1;
        if (m_data[Size - 1] < value)
            return -1;
        return 0;
    }

    auto operator=(const CFlag& other) -> CFlag&
    {
        m_data = other.m_data;
        return *this;
    }

    auto operator=(std::uint32_t value) -> CFlag&
    {
        m_data.fill(0);
        m_data[Size - 1] = value;
        return *this;
    }

    [[nodiscard]] auto operator|(const CFlag& other) const -> CFlag
    {
        CFlag result;
        for (int i = 0; i < Size; ++i)
            result.m_data[i] = m_data[i] | other.m_data[i];
        return result;
    }

    [[nodiscard]] auto operator&(const CFlag& other) const -> CFlag
    {
        CFlag result;
        for (int i = 0; i < Size; ++i)
            result.m_data[i] = m_data[i] & other.m_data[i];
        return result;
    }

    [[nodiscard]] auto operator~() const -> CFlag
    {
        CFlag result;
        for (int i = 0; i < Size; ++i)
            result.m_data[i] = ~m_data[i];
        return result;
    }

    auto operator|=(const CFlag& other) -> CFlag&
    {
        for (int i = 0; i < Size; ++i)
            m_data[i] |= other.m_data[i];
        return *this;
    }

    auto operator&=(const CFlag& other) -> CFlag&
    {
        for (int i = 0; i < Size; ++i)
            m_data[i] &= other.m_data[i];
        return *this;
    }

    [[nodiscard]] auto operator==(const CFlag& other) const -> bool
    {
        return compareTo(other) == 0;
    }

    [[nodiscard]] auto operator!=(const CFlag& other) const -> bool
    {
        return compareTo(other) != 0;
    }

    [[nodiscard]] auto operator==(std::uint32_t value) const -> bool
    {
        return compareTo(value) == 0;
    }

    [[nodiscard]] explicit operator bool() const
    {
        for (int i = 0; i < Size; ++i)
        {
            if (m_data[i] != 0)
                return true;
        }
        return false;
    }

    [[nodiscard]] auto operator!() const -> bool
    {
        return !static_cast<bool>(*this);
    }
};

} // namespace ms
