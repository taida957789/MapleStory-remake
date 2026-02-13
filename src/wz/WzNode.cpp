#include "WzNode.h"

namespace ms
{

WzNode::WzNode(std::string name) : m_name(std::move(name))
{
}

auto WzNode::GetName() const noexcept -> const std::string&
{
    return m_name;
}

auto WzNode::SetName(std::string name) noexcept -> void
{
    m_name = std::move(name);
}

auto WzNode::GetParent() const noexcept -> std::weak_ptr<WzNode>
{
    return m_parent;
}

auto WzNode::SetParent(std::shared_ptr<WzNode> parent) noexcept -> void
{
    m_parent = std::move(parent);
}

auto WzNode::GetPath() const -> std::string
{
    // If no parent, we're at the root - return just our name
    auto parent = m_parent.lock();
    if (!parent) {
        return m_name;
    }

    // Recursively build the path from root to this node
    std::string parentPath = parent->GetPath();
    if (parentPath.empty()) {
        return m_name;
    }

    return parentPath + "/" + m_name;
}

} // namespace ms
