#include "ZXString.h"

namespace ms
{

template <typename CharT>
auto ZXString<CharT>::TrimLeft(const CharT* chars) -> ZXString&
{
    if (!chars || m_str.empty())
        return *this;

    const auto pos = m_str.find_first_not_of(chars);
    if (pos != string_type::npos)
        m_str.erase(0, pos);
    else
        m_str.clear();

    return *this;
}

template <typename CharT>
auto ZXString<CharT>::TrimRight(const CharT* chars) -> ZXString&
{
    if (!chars || m_str.empty())
        return *this;

    const auto pos = m_str.find_last_not_of(chars);
    if (pos != string_type::npos)
        m_str.erase(pos + 1);
    else
        m_str.clear();

    return *this;
}

template <typename CharT>
auto ZXString<CharT>::Mid(int start, int len) const -> ZXString
{
    if (start < 0)
        start = 0;

    const auto ustart = static_cast<size_type>(start);
    if (ustart >= m_str.length())
        return ZXString{};

    if (len < 0)
        return ZXString{m_str.substr(ustart)};

    return ZXString{m_str.substr(ustart, static_cast<size_type>(len))};
}

// Explicit instantiations
template class ZXString<char>;
template class ZXString<wchar_t>;

} // namespace ms
