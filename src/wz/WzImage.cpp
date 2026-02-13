#include "WzImage.h"
#include "WzProperty.h"

namespace ms
{

WzImage::WzImage(std::string name)
    : WzNode(std::move(name))
{
}

WzImage::~WzImage() = default;

auto WzImage::GetType() const noexcept -> WzNodeType
{
    return WzNodeType::Image;
}

auto WzImage::IsLoaded() const noexcept -> bool
{
    return m_loaded;
}

auto WzImage::MarkLoaded() noexcept -> void
{
    m_loaded = true;
}

auto WzImage::SetOffset(std::size_t offset) noexcept -> void
{
    m_offset = offset;
}

auto WzImage::GetOffset() const noexcept -> std::size_t
{
    return m_offset;
}

auto WzImage::SetSize(std::size_t size) noexcept -> void
{
    m_size = size;
}

auto WzImage::GetSize() const noexcept -> std::size_t
{
    return m_size;
}

auto WzImage::SetChecksum(std::uint32_t checksum) noexcept -> void
{
    m_checksum = checksum;
}

auto WzImage::GetChecksum() const noexcept -> std::uint32_t
{
    return m_checksum;
}

auto WzImage::SetWzFile(std::shared_ptr<WzFile> file) noexcept -> void
{
    m_file = file;
}

auto WzImage::GetWzFile() const noexcept -> std::weak_ptr<WzFile>
{
    return m_file;
}

auto WzImage::AddProperty(std::shared_ptr<WzProperty> property) -> void
{
    if (property)
    {
        try
        {
            auto self = shared_from_this();
            property->SetParent(self);
        }
        catch (const std::bad_weak_ptr&)
        {
            // This object is not managed by shared_ptr
            // Don't set parent in this case
        }
        m_properties[property->GetName()] = std::move(property);
    }
}

auto WzImage::GetProperty(const std::string& name) -> std::shared_ptr<WzProperty>
{
    auto it = m_properties.find(name);
    if (it != m_properties.end())
    {
        return it->second;
    }
    return nullptr;
}

auto WzImage::operator[](const std::string& name) -> std::shared_ptr<WzProperty>
{
    return GetProperty(name);
}

auto WzImage::GetProperties() const noexcept -> const std::map<std::string, std::shared_ptr<WzProperty>>&
{
    return m_properties;
}

auto WzImage::GetPropertyCount() const noexcept -> std::size_t
{
    return m_properties.size();
}

auto WzImage::Clear() -> void
{
    m_properties.clear();
    m_loaded = false;
}

} // namespace ms
