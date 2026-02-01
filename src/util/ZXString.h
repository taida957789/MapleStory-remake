#pragma once

#include <string>
#include <string_view>
#include <type_traits>

namespace ms
{

namespace detail
{
    // Helper to get empty string for different character types
    template <typename CharT>
    [[nodiscard]] constexpr auto GetEmptyStr() noexcept -> const CharT*
    {
        if constexpr (std::is_same_v<CharT, char>)
            return "";
        else if constexpr (std::is_same_v<CharT, wchar_t>)
            return L"";
        else if constexpr (std::is_same_v<CharT, char16_t>)
            return u"";
        else if constexpr (std::is_same_v<CharT, char32_t>)
            return U"";
        else
            return "";
    }

    // Helper to get default whitespace for different character types
    template <typename CharT>
    [[nodiscard]] constexpr auto GetDefaultWhitespace() noexcept -> const CharT*
    {
        if constexpr (std::is_same_v<CharT, char>)
            return " \t\r\n";
        else if constexpr (std::is_same_v<CharT, wchar_t>)
            return L" \t\r\n";
        else if constexpr (std::is_same_v<CharT, char16_t>)
            return u" \t\r\n";
        else if constexpr (std::is_same_v<CharT, char32_t>)
            return U" \t\r\n";
        else
            return " \t\r\n";
    }
} // namespace detail

/**
 * @brief ZXString template class
 *
 * Based on ZXString<char> from the original MapleStory client.
 * The original uses reference-counted strings with custom allocators.
 * This implementation wraps std::basic_string for simplicity and safety.
 *
 * @tparam CharT Character type (char, wchar_t, etc.)
 */
template <typename CharT>
class ZXString
{
public:
    using string_type = std::basic_string<CharT>;
    using view_type = std::basic_string_view<CharT>;
    using size_type = typename string_type::size_type;

    // Constructors
    ZXString() = default;

    ZXString(const CharT* str)
        : m_str(str ? str : detail::GetEmptyStr<CharT>())
    {
    }

    ZXString(const CharT* str, int len)
        : m_str(str, len > 0 ? static_cast<size_type>(len) : 0)
    {
    }

    ZXString(const string_type& str) : m_str(str) {}
    ZXString(string_type&& str) noexcept : m_str(std::move(str)) {}

    // Copy/Move (defaulted)
    ZXString(const ZXString&) = default;
    ZXString(ZXString&&) noexcept = default;
    auto operator=(const ZXString&) -> ZXString& = default;
    auto operator=(ZXString&&) noexcept -> ZXString& = default;
    ~ZXString() = default;

    // Conversion
    [[nodiscard]] auto c_str() const noexcept -> const CharT*
    {
        return m_str.c_str();
    }

    [[nodiscard]] operator const CharT*() const noexcept
    {
        return m_str.c_str();
    }

    [[nodiscard]] operator view_type() const noexcept
    {
        return m_str;
    }

    // Assignment
    auto operator=(const CharT* str) -> ZXString&
    {
        m_str = str ? str : detail::GetEmptyStr<CharT>();
        return *this;
    }

    auto operator=(const string_type& str) -> ZXString&
    {
        m_str = str;
        return *this;
    }

    // Comparison
    [[nodiscard]] auto operator==(const CharT* str) const noexcept -> bool
    {
        return m_str == (str ? str : detail::GetEmptyStr<CharT>());
    }

    [[nodiscard]] auto operator==(const ZXString& other) const noexcept -> bool
    {
        return m_str == other.m_str;
    }

    [[nodiscard]] auto operator!=(const CharT* str) const noexcept -> bool
    {
        return !(*this == str);
    }

    [[nodiscard]] auto operator!=(const ZXString& other) const noexcept -> bool
    {
        return !(*this == other);
    }

    // String state
    [[nodiscard]] auto IsEmpty() const noexcept -> bool
    {
        return m_str.empty();
    }

    void Empty() noexcept
    {
        m_str.clear();
    }

    [[nodiscard]] auto GetLength() const noexcept -> int
    {
        return static_cast<int>(m_str.length());
    }

    // Trim operations
    auto TrimLeft(const CharT* chars = detail::GetDefaultWhitespace<CharT>()) -> ZXString&;
    auto TrimRight(const CharT* chars = detail::GetDefaultWhitespace<CharT>()) -> ZXString&;

    // Substring
    [[nodiscard]] auto Mid(int start, int len = -1) const -> ZXString;

    [[nodiscard]] auto Substring(int start, int len = -1) const -> ZXString
    {
        return Mid(start, len);
    }

    // Get underlying string
    [[nodiscard]] auto GetString() const noexcept -> const string_type&
    {
        return m_str;
    }

    [[nodiscard]] auto GetString() noexcept -> string_type&
    {
        return m_str;
    }

private:
    string_type m_str;
};

// Explicit instantiation declarations
extern template class ZXString<char>;
extern template class ZXString<wchar_t>;

} // namespace ms
