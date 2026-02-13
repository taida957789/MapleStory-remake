#include "WzDirectory.h"

namespace ms
{

WzDirectory::WzDirectory(std::string name)
    : WzNode(std::move(name))
{
}

auto WzDirectory::GetType() const noexcept -> WzNodeType
{
    return WzNodeType::Directory;
}

auto WzDirectory::AddChild(std::shared_ptr<WzNode> child) -> void
{
    if (!child)
    {
        return;
    }

    // Set this directory as the child's parent
    try
    {
        auto self = shared_from_this();
        child->SetParent(self);
    }
    catch (const std::bad_weak_ptr&)
    {
        // This object is not managed by shared_ptr (e.g., stack object)
        // Don't set parent in this case
    }

    // Add to children map (keyed by child's name)
    m_children[child->GetName()] = std::move(child);
}

auto WzDirectory::GetChild(const std::string& name) const -> std::shared_ptr<WzNode>
{
    auto it = m_children.find(name);
    if (it != m_children.end())
    {
        return it->second;
    }
    return nullptr;
}

auto WzDirectory::operator[](const std::string& name) const -> std::shared_ptr<WzNode>
{
    return GetChild(name);
}

auto WzDirectory::RemoveChild(const std::string& name) -> bool
{
    auto it = m_children.find(name);
    if (it != m_children.end())
    {
        m_children.erase(it);
        return true;
    }
    return false;
}

auto WzDirectory::Clear() -> void
{
    m_children.clear();
}

auto WzDirectory::GetChildren() const noexcept -> const std::map<std::string, std::shared_ptr<WzNode>>&
{
    return m_children;
}

auto WzDirectory::GetChildCount() const noexcept -> std::size_t
{
    return m_children.size();
}

} // namespace ms
