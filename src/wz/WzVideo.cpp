#include "WzVideo.h"

namespace ms
{

WzVideo::WzVideo() = default;

WzVideo::~WzVideo() = default;

WzVideo::WzVideo(WzVideo&& other) noexcept
    : m_nType(other.m_nType)
    , m_data(std::move(other.m_data))
{
    other.m_nType = 0;
}

auto WzVideo::operator=(WzVideo&& other) noexcept -> WzVideo&
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
