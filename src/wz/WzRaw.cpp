#include "WzRaw.h"

namespace ms
{

WzRaw::WzRaw() = default;

WzRaw::~WzRaw() = default;

WzRaw::WzRaw(WzRaw&& other) noexcept
    : m_nType(other.m_nType)
    , m_data(std::move(other.m_data))
{
    other.m_nType = 0;
}

auto WzRaw::operator=(WzRaw&& other) noexcept -> WzRaw&
{
    if (this != &other)
    {
        m_nType = other.m_nType;
        m_data = std::move(other.m_data);
        other.m_nType = 0;
    }
    return *this;
}

} // namespace ms
